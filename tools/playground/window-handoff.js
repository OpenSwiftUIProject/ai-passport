// Browser-only cross-origin handoff. Shared with ai-passport/tools/playground.
import { HANDOFF_TTL, validateHandoff } from './browser-handoff.js';

const CHANNEL = 'openswiftui-passport-firmware-v1';
const ID = /^[a-f0-9]{48}$/;

export function messageLink(record, origin) {
  const url = new URL(record.destination);
  url.searchParams.set('playground-message', record.id);
  url.searchParams.set('playground-origin', origin);
  return url;
}

export function messageRequest(url) {
  const parameters = new URL(url).searchParams;
  if (!parameters.has('playground-message')) return null;
  const id = parameters.get('playground-message');
  const origin = parameters.get('playground-origin');
  const source = new URL(origin);
  if (!ID.test(id) || source.origin !== origin || source.username || source.password
      || (source.protocol !== 'https:' && !(source.protocol === 'http:'
        && ['localhost', '127.0.0.1'].includes(source.hostname)))) {
    throw new Error('Invalid Playground window link.');
  }
  return { id, origin };
}

export async function prepareWindowFirmware(bytes, sha256, sourceSha256, destination) {
  const id = [...crypto.getRandomValues(new Uint8Array(24))]
    .map(byte => byte.toString(16).padStart(2, '0')).join('');
  const record = { version: 1, id, bytes, sha256, sourceSha256, destination,
    expiresAt: Date.now() + HANDOFF_TTL };
  await validateHandoff(record, id, destination);
  return record;
}

function matches(event, request) {
  return event.origin === request.origin && event.data?.channel === CHANNEL
    && event.data.id === request.id;
}

// Call synchronously from the Open Simulator click so popup permission is retained.
export function openFirmwareWindow(record, { host = window, signal, timeoutMs = 30000, intervalMs = 300 } = {}) {
  if (signal?.aborted) return Promise.reject(new Error('Firmware transfer cancelled.'));
  if (record.expiresAt <= Date.now()) return Promise.reject(new Error('Firmware link expired. Send it again.'));
  const target = new URL(record.destination);
  const child = host.open('about:blank', '_blank');
  if (!child) return Promise.reject(new Error('Allow popups, then click Open Simulator again.'));
  // Retain our WindowProxy for messaging without giving the destination an opener.
  child.opener = null;
  child.location.href = messageLink(record, host.location.origin).href;
  return new Promise((resolve, reject) => {
    let sent = false;
    const finish = error => {
      clearInterval(interval); clearTimeout(deadline);
      host.removeEventListener('message', receive);
      signal?.removeEventListener('abort', abort);
      error ? reject(error) : resolve();
    };
    const abort = () => finish(new Error('Firmware transfer cancelled.'));
    const receive = event => {
      if (event.source !== child || !matches(event, { id: record.id, origin: target.origin })) return;
      if (event.data.type === 'ready' && !sent) {
        sent = true;
        const copy = { ...record, bytes: record.bytes.slice(0) };
        child.postMessage({ channel: CHANNEL, id: record.id, type: 'firmware', record: copy }, target.origin, [copy.bytes]);
      } else if (event.data.type === 'received' && sent) {
        finish();
      } else if (event.data.type === 'error') {
        finish(new Error(String(event.data.message).slice(0, 300)));
      }
    };
    const hello = () => {
      if (child.closed) return finish(new Error('Simulator window closed or isolated. Download full.bin and select it in Simulator.'));
      if (!sent) child.postMessage({ channel: CHANNEL, id: record.id, type: 'hello' }, target.origin);
    };
    host.addEventListener('message', receive);
    signal?.addEventListener('abort', abort, { once: true });
    const interval = setInterval(hello, intervalMs);
    const deadline = setTimeout(() => finish(new Error('Simulator did not accept the firmware. Update Simulator or download full.bin.')), timeoutMs);
    hello();
  });
}

export function receiveWindowFirmware(request, destination, { host = window, timeoutMs = 35000 } = {}) {
  return new Promise((resolve, reject) => {
    let sender, receiving = false, done = false;
    const finish = (error, bytes) => {
      if (done) return;
      done = true;
      clearTimeout(deadline); host.removeEventListener('message', receive);
      error ? reject(error) : resolve(bytes);
    };
    const receive = async event => {
      if (done || !event.source || !matches(event, request) || (sender && event.source !== sender)) return;
      const reply = message => event.source.postMessage({ channel: CHANNEL, id: request.id, ...message }, request.origin);
      if (event.data.type === 'hello') {
        sender = event.source;
        reply({ type: 'ready' });
      } else if (event.data.type === 'firmware' && sender === event.source && !receiving) {
        receiving = true;
        try {
          const bytes = await validateHandoff(event.data.record, request.id, destination);
          if (done) return;
          reply({ type: 'received' });
          finish(null, bytes);
        } catch (error) {
          reply({ type: 'error', message: error.message });
          finish(error);
        }
      }
    };
    host.addEventListener('message', receive);
    const deadline = setTimeout(() => finish(new Error('Return to Playground and click Open Simulator again.')), timeoutMs);
  });
}
