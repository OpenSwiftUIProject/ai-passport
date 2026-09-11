import assert from 'node:assert/strict';
import test from 'node:test';
import { firmwareControls, sha256 } from './firmware.js';

class Control extends EventTarget {
  value = ''; textContent = ''; hidden = true; disabled = false;
  removeAttribute(name) { delete this[name]; }
  focus() {}
}

async function fixture(t) {
  const controls = new Map();
  const element = id => {
    if (!controls.has(id)) controls.set(id, new Control());
    return controls.get(id);
  };
  const previous = { document: globalThis.document, location: globalThis.location, fetch: globalThis.fetch };
  t.after(() => Object.assign(globalThis, previous));
  globalThis.document = { querySelector: element };
  globalThis.location = new URL('https://example.org/ai-passport/');
  let source = 'initial source', release, sent;
  const pending = new Promise(resolve => { release = resolve; });
  const started = new Promise(resolve => { sent = resolve; });
  const bytes = new Uint8Array(64).fill(0xe9);
  const job = { id: 'a'.repeat(32), status: 'ready', sourceSha256: await sha256(new TextEncoder().encode(source)),
    sha256: await sha256(bytes), bytes: bytes.length };
  globalThis.fetch = async url => {
    if (url.pathname.endsWith('/download')) return new Response(bytes);
    if (url.pathname === '/api/playground-firmware') {
      sent(); await pending;
      return Response.json({ id: 'b'.repeat(48), sha256: job.sha256 });
    }
    return Response.json(job);
  };
  const firmware = firmwareControls(() => source);
  element('#simulator-url').value = 'http://127.0.0.1:4190/';
  firmware.connected(new URL('http://127.0.0.1:4191/compile'), true);
  firmware.preview(source);
  await element('#build-firmware').onclick();
  assert.equal(element('#send-firmware').disabled, false);
  return { firmware, element, release, started, edit() { source = 'edited source'; firmware.edited(); } };
}

test('completed local handoff keeps its target and exposes an explicit run link', async t => {
  const f = await fixture(t);
  const transfer = f.element('#send-firmware').onclick();
  await f.started; f.release(); await transfer;
  assert.equal(f.element('#open-simulator').hidden, false);
  assert.equal(f.element('#open-simulator').href, `http://127.0.0.1:4190/?playground=${'b'.repeat(48)}`);
  f.firmware.edited(); // Reset example must invalidate even if it restores identical text.
  assert.equal(f.element('#open-simulator').hidden, true);
  assert.equal(f.element('#open-simulator').href, undefined);
});

for (const change of ['edit', 'disconnect', 'preview failure', 'target change']) {
  test(`late handoff cannot restore a stale link after ${change}`, async t => {
    const f = await fixture(t);
    const transfer = f.element('#send-firmware').onclick();
    await f.started;
    if (change === 'edit') f.edit();
    if (change === 'disconnect') f.firmware.connected(null, false);
    if (change === 'preview failure') f.firmware.preview(null);
    if (change === 'target change') f.element('#simulator-url').dispatchEvent(new Event('input'));
    f.release(); await transfer;
    assert.equal(f.element('#open-simulator').hidden, true);
    assert.equal(f.element('#open-simulator').href, undefined);
    if (change !== 'target change') assert.equal(f.element('#send-firmware').disabled, true);
  });
}
