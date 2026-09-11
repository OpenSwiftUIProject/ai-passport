import { compilerFetchOptions } from './compiler-config.js';
import { simulatorURL, simulatorTransport, DEFAULT_SIMULATOR } from './simulator-config.js';
import { sha256, stageFirmware, removeFirmware } from './browser-handoff.js';
import { prepareWindowFirmware, messageLink, openFirmwareWindow } from './window-handoff.js';

export { sha256 };

export function firmwareControls(currentSource) {
  const build = document.querySelector('#build-firmware');
  const download = document.querySelector('#download-firmware');
  const send = document.querySelector('#send-firmware');
  const status = document.querySelector('#firmware-status');
  const log = document.querySelector('#firmware-log');
  const simulator = document.querySelector('#simulator-url');
  const open = document.querySelector('#open-simulator');
  let endpoint, available = false, previewSource, ready, building = false, transferring = false;
  let generation = 0;
  let windowFirmware, windowController;
  function invalidate() {
    ++generation; open.hidden = true; open.removeAttribute('href');
    windowFirmware = null; windowController?.abort();
  }
  simulator.value = new URLSearchParams(location.search).get('simulator') || DEFAULT_SIMULATOR;
  simulator.addEventListener('input', invalidate);
  function update() {
    const current = endpoint && previewSource === currentSource();
    build.disabled = !current || !available || building;
    download.disabled = send.disabled = !current || !ready || ready.source !== currentSource() || transferring;
    if (!building && !transferring) status.textContent = !endpoint ? 'Connect your compiler to build firmware.'
      : !available ? 'Firmware builds need ESP-IDF 5.5.3. See setup instructions below.'
      : !current ? 'Build & Run your latest edits before creating firmware.'
      : ready?.source === currentSource() ? `Ready · ${(ready.bytes / 1048576).toFixed(2)} MiB · full firmware`
      : 'Preview ready. Build firmware to download or run in Simulator.';
  }
  async function request(url, options = {}) {
    const response = await fetch(url, { ...compilerFetchOptions(url), signal: AbortSignal.timeout(30000), ...options });
    if (!response.ok) {
      const body = await response.json().catch(() => ({}));
      throw new Error(body.error || `HTTP ${response.status}`);
    }
    return response;
  }
  build.onclick = async () => {
    if (build.disabled) return;
    const source = previewSource, target = endpoint;
    invalidate();
    const ticket = generation;
    building = true; ready = null; log.textContent = ''; update();
    status.textContent = 'Building full firmware… First build can take several minutes.';
    try {
      const response = await request(new URL('./firmware', target), {
        method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ source }),
      });
      let job = await response.json();
      const expected = await sha256(new TextEncoder().encode(source));
      if (!/^[a-f0-9]{32}$/.test(job.id) || job.sourceSha256 !== expected) throw new Error('Firmware source mismatch');
      const jobURL = new URL(`./firmware/${job.id}`, target);
      const deadline = Date.now() + 1250000;
      while (job.status === 'building') {
        if (ticket !== generation) { building = false; update(); return; }
        if (Date.now() > deadline) throw new Error('Firmware build timed out');
        await new Promise(resolve => setTimeout(resolve, 1500));
        job = await (await request(jobURL)).json();
        log.textContent = job.log || '';
        log.scrollTop = log.scrollHeight;
      }
      if (job.status !== 'ready') throw new Error(job.error || 'Firmware build failed');
      if (job.sourceSha256 !== expected || !/^[a-f0-9]{64}$/.test(job.sha256)) throw new Error('Firmware metadata mismatch');
      if (ticket === generation && endpoint?.href === target.href && source === currentSource()) {
        ready = { ...job, source, url: new URL(`${jobURL.href}/download`) };
      }
      building = false; update();
    } catch (error) {
      building = false; update(); if (ticket === generation) status.textContent = error.message;
    }
  };
  async function artifact(job) {
    const bytes = await (await request(job.url)).arrayBuffer();
    if (bytes.byteLength !== job.bytes || await sha256(bytes) !== job.sha256) throw new Error('Firmware download checksum mismatch');
    return bytes;
  }
  async function transfer(action) {
    const job = ready;
    if (transferring || !endpoint || previewSource !== currentSource() || !job || job.source !== currentSource()) return;
    const ticket = generation;
    const current = () => ticket === generation && ready === job && job.source === currentSource();
    transferring = true; update();
    try { await action(job, current); }
    catch (error) { if (current()) status.textContent = error.message; }
    finally {
      transferring = false;
      const message = status.textContent;
      update();
      if (current()) status.textContent = message;
    }
  }
  download.onclick = () => transfer(async (job, current) => {
    status.textContent = 'Downloading and checking firmware…';
    const bytes = await artifact(job);
    if (!current()) return;
    const url = URL.createObjectURL(new Blob([bytes], { type: 'application/octet-stream' }));
    const link = document.createElement('a'); link.href = url; link.download = 'ContentView-full.bin'; link.click();
    setTimeout(() => URL.revokeObjectURL(url), 60000);
    status.textContent = 'Full firmware downloaded.';
  });
  send.onclick = () => transfer(async (job, current) => {
    open.hidden = true;
    const target = simulatorURL(simulator.value, location.href);
    let transport = simulatorTransport(target, location.href);
    if (transport === 'discover' || transport === 'window') {
      const config = await (await request(new URL('playground-config.json', target))).json();
      if (config.service !== 'openswiftui-passport-simulator' || config.protocolVersion !== 1
          || !['indexeddb', 'http'].includes(config.transport)) {
        throw new Error('This Simulator does not support Playground firmware import.');
      }
      if (transport === 'window') {
        if (config.windowHandoff !== true) throw new Error('This Simulator needs the browser window handoff update.');
      } else transport = config.transport;
    }
    if (!current()) return;
    status.textContent = 'Preparing firmware for Simulator…';
    const bytes = await artifact(job);
    if (!current()) return;
    let id;
    windowFirmware = null;
    if (transport === 'window') {
      const record = await prepareWindowFirmware(bytes, job.sha256, job.sourceSha256, target.href);
      if (!current()) return;
      windowFirmware = record;
      open.href = messageLink(record, location.origin).href;
    } else if (transport === 'indexeddb') {
      id = await stageFirmware(bytes, job.sha256, job.sourceSha256, target.href);
      if (!current()) { await removeFirmware(id); return; }
    } else {
      const response = await request(new URL('api/playground-firmware', target), { method: 'POST', headers: {
        'Content-Type': 'application/octet-stream', 'X-Firmware-SHA256': job.sha256,
      }, body: bytes });
      const result = await response.json();
      if (!/^[a-f0-9]{48}$/.test(result.id) || result.sha256 !== job.sha256) throw new Error('Simulator handoff mismatch');
      id = result.id;
      if (!current()) return;
    }
    if (id) open.href = new URL(`?playground=${id}`, target).href;
    open.hidden = false;
    status.textContent = transport === 'window'
      ? 'Ready. Click Open Simulator to transfer directly to the online Simulator.'
      : transport === 'indexeddb'
      ? 'Ready in this browser. Open Simulator to run it (10 minutes; no upload).'
      : 'Firmware sent. Open Simulator to run it (link lasts 10 minutes).';
    open.focus();
  });
  open.onclick = event => {
    if (!windowFirmware) return;
    event.preventDefault();
    const record = windowFirmware, ticket = generation;
    windowController?.abort();
    windowController = new AbortController();
    status.textContent = 'Opening online Simulator and transferring firmware…';
    openFirmwareWindow(record, { signal: windowController.signal }).then(() => {
      if (ticket === generation) status.textContent = 'Firmware delivered to Simulator. No server upload.';
    }).catch(error => {
      if (ticket === generation) status.textContent = error.message;
    });
  };
  document.querySelector('#simulator-command').textContent = `git clone https://github.com/OpenSwiftUIProject/FoloToy-Passport-Simulator.git\ncd FoloToy-Passport-Simulator\nnpm ci\nnpm start -- --playground-origin ${location.origin}`;
  return {
    connected(value, canBuild) { invalidate(); endpoint = value; available = !!canBuild; ready = null; update(); },
    preview(source) { if (source !== previewSource) invalidate(); previewSource = source; update(); },
    edited() { invalidate(); update(); },
  };
}
