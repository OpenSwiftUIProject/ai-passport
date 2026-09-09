import { compilerURL, compilerFetchOptions } from './compiler-config.js';

export async function sha256(bytes) {
  return [...new Uint8Array(await crypto.subtle.digest('SHA-256', bytes))]
    .map(byte => byte.toString(16).padStart(2, '0')).join('');
}

export function firmwareControls(currentSource) {
  const build = document.querySelector('#build-firmware');
  const download = document.querySelector('#download-firmware');
  const send = document.querySelector('#send-firmware');
  const status = document.querySelector('#firmware-status');
  const log = document.querySelector('#firmware-log');
  const simulator = document.querySelector('#simulator-url');
  const open = document.querySelector('#open-simulator');
  let endpoint, available = false, previewSource, ready, building = false, transferring = false;
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
    const source = previewSource, target = endpoint;
    building = true; ready = null; open.hidden = true; log.textContent = ''; update();
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
        if (Date.now() > deadline) throw new Error('Firmware build timed out');
        await new Promise(resolve => setTimeout(resolve, 1500));
        job = await (await request(jobURL)).json();
        log.textContent = job.log || '';
        log.scrollTop = log.scrollHeight;
      }
      if (job.status !== 'ready') throw new Error(job.error || 'Firmware build failed');
      if (job.sourceSha256 !== expected || !/^[a-f0-9]{64}$/.test(job.sha256)) throw new Error('Firmware metadata mismatch');
      if (endpoint?.href === target.href) ready = { ...job, source, url: new URL(`${jobURL.href}/download`) };
      building = false; update();
    } catch (error) {
      building = false; update(); status.textContent = error.message;
    }
  };
  async function artifact(job) {
    const bytes = await (await request(job.url)).arrayBuffer();
    if (bytes.byteLength !== job.bytes || await sha256(bytes) !== job.sha256) throw new Error('Firmware download checksum mismatch');
    return bytes;
  }
  async function transfer(action) {
    const job = ready;
    if (!job || job.source !== currentSource()) return;
    transferring = true; update();
    try { await action(job); }
    catch (error) { status.textContent = error.message; }
    finally {
      transferring = false;
      download.disabled = send.disabled = !endpoint || previewSource !== currentSource() || job.source !== currentSource();
    }
  }
  download.onclick = () => transfer(async job => {
    status.textContent = 'Downloading and checking firmware…';
    const url = URL.createObjectURL(new Blob([await artifact(job)], { type: 'application/octet-stream' }));
    const link = document.createElement('a'); link.href = url; link.download = 'ContentView-full.bin'; link.click();
    setTimeout(() => URL.revokeObjectURL(url), 60000);
    status.textContent = 'Full firmware downloaded.';
  });
  send.onclick = () => transfer(async job => {
    open.hidden = true;
    // Reuse the compiler URL validation: HTTPS or explicit HTTP loopback only.
    const target = compilerURL(simulator.value, location.href);
    const api = new URL('/api/playground-firmware', target);
    status.textContent = 'Sending firmware to Simulator…';
    const bytes = await artifact(job);
    const response = await request(api, { method: 'POST', headers: {
      'Content-Type': 'application/octet-stream', 'X-Firmware-SHA256': job.sha256,
    }, body: bytes });
    const result = await response.json();
    if (!/^[a-f0-9]{48}$/.test(result.id) || result.sha256 !== job.sha256) throw new Error('Simulator handoff mismatch');
    open.href = new URL(`/?playground=${result.id}`, target).href;
    open.hidden = false;
    status.textContent = 'Firmware sent. Open Simulator to run it (link lasts 10 minutes).';
    open.focus();
  });
  document.querySelector('#simulator-command').textContent = `git clone https://github.com/OpenSwiftUIProject/FoloToy-Passport-Simulator.git\ncd FoloToy-Passport-Simulator\nnpm ci\nnpm start -- --playground-origin ${location.origin}`;
  return {
    connected(value, canBuild) { endpoint = value; available = !!canBuild; ready = null; update(); },
    preview(source) { previewSource = source; update(); },
    edited() { open.hidden = true; update(); },
  };
}
