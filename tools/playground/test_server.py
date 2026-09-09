"""Exercise browser-to-loopback access without invoking the native compiler."""
import http.client
import json
import threading
import unittest
from unittest.mock import patch
from server import CompilerServer


class LocalCompilerAccessTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.origin = 'https://playground.example'
        cls.server = CompilerServer(0, [cls.origin])
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.server.server_close()
        cls.thread.join()

    def request(self, method='GET', path='/health', body=None, headers=None):
        connection = http.client.HTTPConnection('127.0.0.1', self.server.server_port, timeout=5)
        connection.request(method, path, body, headers or {})
        response = connection.getresponse()
        result = response.status, dict(response.getheaders()), response.read()
        connection.close()
        return result

    def test_allowed_site_health_and_private_network_preflight(self):
        code, headers, body = self.request(headers={'Origin': self.origin})
        self.assertEqual(code, 200)
        self.assertEqual(headers['Access-Control-Allow-Origin'], self.origin)
        self.assertEqual(json.loads(body)['protocolVersion'], 1)
        code, headers, _ = self.request('OPTIONS', '/compile', headers={
            'Origin': self.origin, 'Access-Control-Request-Method': 'POST',
            'Access-Control-Request-Headers': 'Content-Type',
            'Access-Control-Request-Private-Network': 'true'})
        self.assertEqual(code, 204)
        self.assertEqual(headers['Access-Control-Allow-Private-Network'], 'true')
        self.assertNotIn('Access-Control-Allow-Credentials', headers)

    def test_unapproved_origin_cannot_read_or_compile(self):
        for origin in ('null', 'https://playground.example.attacker.invalid', 'http://127.0.0.1:4192'):
            for method, path in [('GET', '/health'), ('OPTIONS', '/compile'), ('POST', '/compile')]:
                with self.subTest(origin=origin, method=method):
                    code, headers, _ = self.request(method, path, headers={'Origin': origin})
                    self.assertEqual(code, 403)
                    self.assertNotIn('Access-Control-Allow-Origin', headers)

    def test_rebinding_host_is_rejected(self):
        code, _, _ = self.request(headers={'Host': f'attacker.invalid:{self.server.server_port}'})
        self.assertEqual(code, 403)

    def test_only_declared_preflight_method_and_headers(self):
        for method, headers in [('DELETE', 'content-type'), ('POST', 'authorization')]:
            code, _, _ = self.request('OPTIONS', '/compile', headers={
                'Origin': self.origin, 'Access-Control-Request-Method': method,
                'Access-Control-Request-Headers': headers})
            self.assertEqual(code, 403)

    def test_bad_payloads_never_reach_compiler(self):
        for source in ('[]', '{}', '{"source":1}', 'null', '{broken'):
            code, _, _ = self.request('POST', '/compile', source, {'Content-Type': 'application/json'})
            self.assertEqual(code, 400, source)
        code, _, _ = self.request('POST', '/compile', '{"source":"x"}', {'Content-Type': 'text/plain'})
        self.assertEqual(code, 415)
        code, _, _ = self.request('POST', '/compile', ' ' * 65537, {'Content-Type': 'application/json'})
        self.assertEqual(code, 400)

    def test_firmware_jobs_share_access_policy(self):
        payload = '{"source":"import OpenSwiftUI"}'
        code, _, _ = self.request('POST', '/firmware', payload, {
            'Origin': 'https://other.example', 'Content-Type': 'application/json'})
        self.assertEqual(code, 403)
        with patch.object(self.server.firmware, 'start', return_value={
                'id': 'a' * 32, 'status': 'building', 'sourceSha256': 'b' * 64}) as start:
            code, headers, body = self.request('POST', '/firmware', payload, {
                'Origin': self.origin, 'Content-Type': 'application/json'})
            self.assertEqual(code, 202)
            self.assertEqual(json.loads(body)['status'], 'building')
            start.assert_called_once_with('import OpenSwiftUI')
        with patch.object(self.server.firmware, 'start', side_effect=BlockingIOError('busy')):
            self.assertEqual(self.request('POST', '/firmware', payload, {'Content-Type': 'application/json'})[0], 429)
        self.assertEqual(self.request(path='/firmware/unknown')[0], 404)
        with patch.object(self.server.firmware, 'get', return_value={'status': 'building'}):
            self.assertEqual(self.request(path='/firmware/known/download')[0], 409)
        code, _, _ = self.request('OPTIONS', '/firmware/job/download', headers={
            'Origin': self.origin, 'Access-Control-Request-Method': 'GET'})
        self.assertEqual(code, 204)

    def test_busy_response_keeps_cors_headers(self):
        self.server.compile_lock.acquire()
        try:
            code, headers, body = self.request('POST', '/compile', '{"source":"x"}', {
                'Origin': self.origin, 'Content-Type': 'application/json'})
            self.assertEqual(code, 429)
            self.assertEqual(headers['Access-Control-Allow-Origin'], self.origin)
            self.assertIn('busy', json.loads(body)['error'])
        finally:
            self.server.compile_lock.release()


if __name__ == '__main__': unittest.main()
