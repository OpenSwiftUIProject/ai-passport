import assert from 'node:assert/strict';
import test from 'node:test';
import { compilerURL, compilerFetchOptions, DEFAULT_COMPILER } from './compiler-config.js';
const page = 'https://example.github.io/simulator/';
test('loopback, project-relative, and HTTPS compiler URLs', () => {
  assert.equal(compilerURL(DEFAULT_COMPILER, page).href, DEFAULT_COMPILER);
  assert.equal(compilerURL('http://localhost:4201', page).href, 'http://localhost:4201/compile');
  assert.equal(compilerURL('./compile', page).href, 'https://example.github.io/simulator/compile');
  assert.equal(compilerURL('https://compiler.example/api/compile', page).pathname, '/api/compile');
  assert.equal(compilerFetchOptions(compilerURL(DEFAULT_COMPILER, page)).targetAddressSpace, 'loopback');
});
test('reject insecure remote URLs, credentials, fragments and queries', () => {
  for (const value of ['', 'http://192.168.1.2:4191/compile', 'http://localhost.evil/compile',
    'file:///compile', 'javascript:alert(1)', 'https://user:secret@compiler.example/compile',
    'https://compiler.example/compile?token=secret', 'http://127.0.0.1:4191/compile#secret']) {
    assert.throws(() => compilerURL(value, page), undefined, value);
  }
});
