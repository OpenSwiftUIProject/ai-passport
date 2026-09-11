#!/usr/bin/env python3
"""Trusted local Swift compiler; explicit website origins, loopback binding only."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import hashlib
import json
import os
import signal
import subprocess
import sys
import tempfile
import threading
import urllib.parse
import build
from firmware_service import FirmwareJobs

ROOT = Path(__file__).resolve().parent
FILES = {'/': ('index.html', 'text/html'), '/app.js': ('app.js', 'text/javascript'),
         '/compiler-config.js': ('compiler-config.js', 'text/javascript'),
         '/firmware.js': ('firmware.js', 'text/javascript'),
         '/simulator-config.js': ('simulator-config.js', 'text/javascript'),
         '/browser-handoff.js': ('browser-handoff.js', 'text/javascript'),
         '/worker.js': ('worker.js', 'text/javascript'), '/wasi.js': ('wasi.js', 'text/javascript'),
         '/editor.js': ('build/editor.js', 'text/javascript'),
         '/styles.css': ('styles.css', 'text/css'),
         '/social-card.png': ('../../assets/images/openswiftui-playground-social.png', 'image/png'),
         '/LOCAL_COMPILER.md': ('LOCAL_COMPILER.md', 'text/plain; charset=utf-8'),
         '/LOCAL_COMPILER.zh_CN.md': ('LOCAL_COMPILER.zh_CN.md', 'text/plain; charset=utf-8'),
         '/DEPLOYMENT.md': ('DEPLOYMENT.md', 'text/plain; charset=utf-8'),
         '/DEPLOYMENT.zh_CN.md': ('DEPLOYMENT.zh_CN.md', 'text/plain; charset=utf-8'),
         '/README.md': ('README.md', 'text/plain; charset=utf-8'),
         '/README.zh_CN.md': ('README.zh_CN.md', 'text/plain; charset=utf-8'),
         '/ContentView.swift': ('ContentView.swift', 'text/plain'),
         '/skills/ai-passport-local-compiler.zip': ('build/ai-passport-local-compiler.zip', 'application/zip')}


def website_origin(value):
    """Only exact HTTPS origins or HTTP loopback origins may opt in."""
    try:
        url = urllib.parse.urlsplit(value)
        _ = url.port
        if (url.scheme not in ('http', 'https') or not url.hostname or url.username
                or url.password or url.query or url.fragment or url.path not in ('', '/')
                or (url.scheme == 'http' and url.hostname not in ('localhost', '127.0.0.1'))):
            raise ValueError()
        return f'{url.scheme}://{url.netloc}'.rstrip('/')
    except ValueError:
        raise argparse.ArgumentTypeError('Use an exact HTTPS origin (no path), or HTTP localhost/127.0.0.1 origin')


class CompilerServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, port=4191, origins=()):
        super().__init__(('127.0.0.1', port), Handler)
        self.allowed_origins = set(origins) | {
            f'http://127.0.0.1:{self.server_port}', f'http://localhost:{self.server_port}'}
        self.compile_lock = threading.Lock()
        self.firmware = FirmwareJobs()

    def server_close(self):
        self.firmware.close()
        super().server_close()


class Handler(BaseHTTPRequestHandler):
    def setup(self):
        super().setup()
        self.connection.settimeout(10)

    def reply(self, status, body, content_type='application/json', headers=None):
        self.send_response(status)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'no-store')
        self.send_header('X-Content-Type-Options', 'nosniff')
        self.send_header('Vary', 'Origin')
        origin = self.headers.get('Origin')
        if origin in self.server.allowed_origins:
            self.send_header('Access-Control-Allow-Origin', origin)
            self.send_header('Access-Control-Expose-Headers', 'X-Build-Seconds, X-Source-SHA256')
        self.send_header('Content-Security-Policy', "default-src 'self'; script-src 'self' 'wasm-unsafe-eval'; style-src 'self' 'unsafe-inline'; worker-src 'self'; connect-src 'self' https: http://127.0.0.1:* http://localhost:*; frame-ancestors 'none'")
        for key, value in (headers or {}).items():
            self.send_header(key, value)
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass  # A disconnected editor does not terminate the compiler server.

    def error(self, status, message):
        self.reply(status, json.dumps({'error': message}).encode())

    def authorized(self):
        port = self.server.server_port
        if self.headers.get('Host') not in (f'127.0.0.1:{port}', f'localhost:{port}'):
            self.error(403, 'Invalid host')
            return False
        origin = self.headers.get('Origin')
        if origin is not None and origin not in self.server.allowed_origins:
            self.error(403, 'Website origin not allowed; restart with --allow-origin for this website')
            return False
        return True

    def do_OPTIONS(self):
        if not self.authorized(): return
        if self.path not in ('/compile', '/health', '/firmware') and not self.path.startswith('/firmware/'):
            return self.error(404, 'Not found')
        method = self.headers.get('Access-Control-Request-Method')
        requested_headers = {h.strip().lower() for h in self.headers.get('Access-Control-Request-Headers', '').split(',') if h.strip()}
        if (not self.headers.get('Origin') or method not in ('GET', 'POST')
                or not requested_headers <= {'content-type'}):
            return self.error(403, 'Preflight not allowed')
        headers = {'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
                   'Access-Control-Allow-Headers': 'Content-Type', 'Access-Control-Max-Age': '600'}
        # Older Chromium Private Network Access preflights still use this header.
        if self.headers.get('Access-Control-Request-Private-Network') == 'true':
            headers['Access-Control-Allow-Private-Network'] = 'true'
        self.reply(204, b'', headers=headers)

    def do_GET(self):
        if not self.authorized(): return
        path = urllib.parse.urlsplit(self.path).path
        if path == '/health':
            return self.reply(200, json.dumps({'service': 'openswiftui-passport-compiler',
                'protocolVersion': 1, 'firmwareAvailable': self.server.firmware.available()}).encode())
        if path.startswith('/firmware/'):
            parts = path.strip('/').split('/')
            job = self.server.firmware.get(parts[1])
            if not job: return self.error(404, 'Unknown firmware build')
            if len(parts) == 2:
                return self.reply(200, json.dumps(job).encode())
            if len(parts) == 3 and parts[2] == 'download':
                artifact = self.server.firmware.artifact(parts[1])
                if not artifact: return self.error(409, 'Firmware is not ready')
                return self.reply(200, artifact.read_bytes(), 'application/octet-stream', {
                    'Content-Disposition': 'attachment; filename="ContentView-full.bin"',
                    'X-Source-SHA256': job['sourceSha256']})
            return self.error(404, 'Not found')
        if path == '/config.json':
            return self.reply(200, (ROOT / 'config.json').read_bytes())
        if path not in FILES: return self.error(404, 'Not found')
        name, mime = FILES[path]
        try:
            self.reply(200, (ROOT / name).read_bytes(), mime)
        except FileNotFoundError:
            self.error(503, 'Assets missing; run tools/playground/start-compiler.sh first')

    def do_POST(self):
        if not self.authorized(): return
        if self.path not in ('/compile', '/firmware'): return self.error(404, 'Not found')
        if self.headers.get_content_type() != 'application/json': return self.error(415, 'JSON required')
        try:
            size = int(self.headers.get('Content-Length', '0'))
            if not 0 < size <= 65536: raise ValueError('Source must fit in 64 KiB')
            payload = json.loads(self.rfile.read(size))
            if not isinstance(payload, dict) or not isinstance(payload.get('source'), str):
                raise ValueError('source must be text')
            source = payload['source']
        except (ValueError, UnicodeDecodeError) as error:
            return self.error(400, str(error))
        if self.path == '/firmware':
            try:
                job = self.server.firmware.start(source)
                return self.reply(202, json.dumps(job).encode())
            except BlockingIOError as error:
                return self.error(429, str(error))
            except RuntimeError as error:
                return self.error(503, str(error))
        if not self.server.compile_lock.acquire(blocking=False):
            return self.error(429, 'Compiler busy; try again')
        try:
            with tempfile.TemporaryDirectory(prefix='edit-', dir=build.BUILD) as folder:
                work = Path(folder)
                (work / 'ContentView.swift').write_text(source, encoding='utf-8')
                process = subprocess.Popen([sys.executable, str(ROOT / 'build.py'), '--source', str(work / 'ContentView.swift'),
                    '--output', str(work / 'preview.wasm')], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    text=True, start_new_session=True)
                try:
                    stdout, stderr = process.communicate(timeout=30)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.communicate()
                    raise
                if process.returncode:
                    return self.error(422, stderr[-24000:].replace(str(work) + '/', ''))
                wasm = (work / 'preview.wasm').read_bytes()
                metrics = json.loads(stdout.splitlines()[-1])
                self.reply(200, wasm, 'application/wasm', {
                    'X-Build-Seconds': str(metrics['seconds']),
                    'X-Source-SHA256': hashlib.sha256(source.encode()).hexdigest()})
        except subprocess.TimeoutExpired:
            self.error(408, 'Compilation exceeded 30 seconds')
        except (OSError, ValueError) as error:
            self.error(500, f'Compiler service failed: {error}')
        finally:
            self.server.compile_lock.release()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=4191)
    parser.add_argument('--allow-origin', type=website_origin, action='append', default=[])
    args = parser.parse_args()
    if not 1 <= args.port <= 65535: parser.error('port must be between 1 and 65535')
    build.BUILD.mkdir(parents=True, exist_ok=True)
    with CompilerServer(args.port, args.allow_origin) as server:
        print(f'Local editor: http://127.0.0.1:{args.port}/', flush=True)
        print(f'Compiler URL: http://127.0.0.1:{args.port}/compile', flush=True)
        print('Allowed website origins: ' + ', '.join(sorted(server.allowed_origins)), flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print('\nCompiler stopped.', flush=True)


if __name__ == '__main__': main()
