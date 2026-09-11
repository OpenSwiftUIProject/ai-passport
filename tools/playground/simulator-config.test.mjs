import assert from 'node:assert/strict';
import test from 'node:test';
import { simulatorURL, simulatorTransport, DEFAULT_SIMULATOR } from './simulator-config.js';

const page = 'https://openswiftuiproject.github.io/ai-passport/';
test('Simulator keeps its project path and discovers same-origin Pages', () => {
  const target = simulatorURL(DEFAULT_SIMULATOR, page);
  assert.equal(simulatorTransport(target, page), 'discover');
  assert.equal(new URL('?playground=abc', target).pathname, '/FoloToy-Passport-Simulator/');
  assert.equal(simulatorURL('http://127.0.0.1:4190', page).href, 'http://127.0.0.1:4190/');
  assert.equal(simulatorTransport(simulatorURL('http://127.0.0.1:4190', page), page), 'http');
});
test('local preview can hand off to the online Simulator through a browser window', () => {
  assert.equal(simulatorTransport(new URL(DEFAULT_SIMULATOR), 'http://127.0.0.1:4191/'), 'window');
  for (const url of ['', 'http://example.org/', 'https://user:password@example.org', 'https://example.org/?x=1', 'javascript:alert(1)']) {
    assert.throws(() => simulatorURL(url, page));
  }
});
