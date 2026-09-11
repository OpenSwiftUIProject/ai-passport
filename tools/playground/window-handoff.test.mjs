import assert from 'node:assert/strict';
import test from 'node:test';
import { sha256 } from './browser-handoff.js';
import { prepareWindowFirmware, messageLink, messageRequest, openFirmwareWindow, receiveWindowFirmware } from './window-handoff.js';

const origin = 'http://127.0.0.1:4192';
const destination = 'https://openswiftuiproject.github.io/FoloToy-Passport-Simulator/';
const remoteOrigin = new URL(destination).origin;
const image = new Uint8Array(0x10040).fill(0xe9);
image[0x8000] = 0xaa; image[0x8001] = 0x50;
const record = await prepareWindowFirmware(image.buffer, await sha256(image), await sha256(new TextEncoder().encode('ContentView')), destination);

function event(data, source, origin) {
  return Object.assign(new Event('message'), { data, source, origin });
}
function windows() {
  const host = new EventTarget(), childHost = new EventTarget(), messages = [];
  host.location = { origin };
  const sender = { postMessage(data, target) {
    assert.equal(target, origin);
    queueMicrotask(() => host.dispatchEvent(event(structuredClone(data), child, remoteOrigin)));
  } };
  const child = { opener: host, location: {}, closed: false, postMessage(data, target) {
    assert.equal(target, remoteOrigin);
    messages.push(data);
    queueMicrotask(() => childHost.dispatchEvent(event(structuredClone(data), sender, origin)));
  } };
  host.open = (url, target) => { assert.equal(url, 'about:blank'); assert.equal(target, '_blank'); return child; };
  return { host, childHost, child, sender, messages };
}

test('local Playground sends checked bytes to an online Simulator without an opener', async () => {
  const w = windows();
  const link = messageLink(record, origin);
  const receiver = receiveWindowFirmware(messageRequest(link), destination, { host: w.childHost });
  await openFirmwareWindow(record, { host: w.host });
  assert.deepEqual(await receiver, image.buffer);
  assert.equal(w.child.opener, null);
  assert.equal(w.child.location.href, link.href);
  assert.equal(new URL(w.child.location.href).pathname, '/FoloToy-Passport-Simulator/');
  assert.equal(record.bytes.byteLength, image.byteLength, 'retains the original for another explicit Open click');
});

test('wrong origin or window cannot elicit firmware; cancelling stops the transfer', async () => {
  const w = windows(), controller = new AbortController();
  const transfer = openFirmwareWindow(record, { host: w.host, signal: controller.signal });
  const ready = { channel: 'openswiftui-passport-firmware-v1', id: record.id, type: 'ready' };
  w.host.dispatchEvent(event(ready, w.child, 'https://unrelated.example'));
  w.host.dispatchEvent(event(ready, {}, remoteOrigin));
  assert.equal(w.messages.filter(message => message.type === 'firmware').length, 0);
  controller.abort();
  await assert.rejects(transfer, /cancelled/);
});

test('receiver rejects a corrupted payload from the expected peer', async () => {
  const host = new EventTarget(), replies = [];
  const sender = { postMessage(data, target) { assert.equal(target, origin); replies.push(data); } };
  const receive = receiveWindowFirmware({ id: record.id, origin }, destination, { host });
  const base = { channel: 'openswiftui-passport-firmware-v1', id: record.id };
  host.dispatchEvent(event({ ...base, type: 'hello' }, sender, origin));
  host.dispatchEvent(event({ ...base, type: 'firmware', record: { ...record, sha256: '0'.repeat(64) } }, sender, origin));
  await assert.rejects(receive, /checksum/);
  assert.equal(replies.at(-1).type, 'error');
});

test('bad links, blocked popups and absent receivers fail explicitly', async () => {
  assert.equal(messageRequest(destination), null);
  assert.throws(() => messageRequest(`${destination}?playground-message=bad&playground-origin=${origin}`));
  assert.throws(() => messageRequest(`${destination}?playground-message=${record.id}&playground-origin=http://example.org`));
  await assert.rejects(openFirmwareWindow(record, { host: { open: () => null } }), /popups/);
  await assert.rejects(openFirmwareWindow(record, { host: windows().host, timeoutMs: 5 }), /did not accept/);
});
