"""One bounded firmware worker, with downloadable results for this server session."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import threading
import uuid
import build


class FirmwareJobs:
    def __init__(self):
        self.lock = threading.Lock()
        self.jobs = {}
        self.process_lock = threading.Lock()
        self.process = None
        self.closed = False
        self.root = build.REPOSITORY / 'build/playground/firmware-jobs' / uuid.uuid4().hex

    @staticmethod
    def available():
        idf = Path(os.environ.get('IDF_PATH', build.WORKSPACE / 'toolchains/esp-idf-v5.5.3'))
        return (idf / 'export.sh').is_file()

    def start(self, source):
        if self.closed: raise RuntimeError('Compiler is stopping')
        if not self.available(): raise RuntimeError('Install ESP-IDF 5.5.3 first; see the local compiler setup instructions')
        if not self.lock.acquire(blocking=False): raise BlockingIOError('A firmware build is already running')
        try:
            if len(self.jobs) >= 8:
                raise RuntimeError('This session has reached 8 firmware builds; restart the compiler to start a new session')
            job_id = uuid.uuid4().hex
            work = self.root / job_id
            work.mkdir(parents=True)
            (work / 'ContentView.swift').write_text(source, encoding='utf-8')
            job = {'id': job_id, 'status': 'building', 'sourceSha256': hashlib.sha256(source.encode()).hexdigest()}
            self.jobs[job_id] = job
            threading.Thread(target=self.build, args=(job, work), daemon=True).start()
            return dict(job)
        except Exception:
            self.lock.release()
            raise

    def build(self, job, work):
        try:
            env = dict(os.environ, SWIFT_TOOLCHAIN=str(build.SWIFTC.parents[2]))
            command = [str(build.REPOSITORY / 'tools/with-env.sh'), 'python3',
                       str(build.ROOT / 'build_firmware.py'), '--source', str(work / 'ContentView.swift'),
                       '--output', str(work / 'ContentView-full.bin')]
            with (work / 'build.log').open('w') as log:
                with self.process_lock:
                    if self.closed: raise RuntimeError('Compiler stopped')
                    process = subprocess.Popen(command, cwd=build.REPOSITORY, env=env,
                                               stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                    self.process = process
                try:
                    process.wait(timeout=1200)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
                    raise RuntimeError('Firmware compilation exceeded 20 minutes')
            if process.returncode: raise RuntimeError('Firmware compilation failed; see the build log')
            result = json.loads((work / 'build.log').read_text().splitlines()[-1])
            # Publish all result fields before exposing the ready state.
            job.update(result, filename='ContentView-full.bin')
            job['status'] = 'ready'
        except Exception as error:
            job.update(status='failed', error=str(error))
        finally:
            with self.process_lock: self.process = None
            self.lock.release()

    def close(self):
        with self.process_lock:
            self.closed = True
            if self.process and self.process.poll() is None:
                try: os.killpg(self.process.pid, signal.SIGKILL)
                except ProcessLookupError: pass

    def get(self, job_id):
        job = self.jobs.get(job_id)
        if job is None: return None
        result = dict(job)
        path = self.root / job_id / 'build.log'
        if path.exists():
            with path.open('rb') as log:
                log.seek(max(0, path.stat().st_size - 12000))
                result['log'] = log.read().decode('utf-8', errors='replace')
        return result

    def artifact(self, job_id):
        job = self.jobs.get(job_id)
        if not job or job['status'] != 'ready': return None
        return self.root / job_id / 'ContentView-full.bin'
