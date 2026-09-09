import { createSwiftEditor } from './editor.js';
import { compilerURL, compilerFetchOptions, DEFAULT_COMPILER } from './compiler-config.js';
import { firmwareControls } from './firmware.js';
let editor, configuration, compileEndpoint;
const firmware = firmwareControls(() => editor?.value);
const status = document.querySelector('#status');
const diagnostics = document.querySelector('#diagnostics');
const auto = document.querySelector('#auto');
const endpointInput = document.querySelector('#compiler-url');
const connectionStatus = document.querySelector('#connection-status');
const connectButton = document.querySelector('#connect');
const disconnectButton = document.querySelector('#disconnect');
const storageKey = `openswiftui-compiler:${location.pathname}`;
const canvas = document.querySelector('#screen');
const context = canvas.getContext('2d', { alpha: false });
let example, timer, generation = 0, busy = false, queued = false, worker, watchdog, lastAlive, precompiled;
let requestController, connectionController, connectionGeneration = 0;
function report(message, error = false) {
  status.textContent = message;
  status.classList.toggle('error', error);
}
function connectionReport(message, error = false) {
  connectionStatus.textContent = message;
  connectionStatus.classList.toggle('error', error);
}
function startPreview(wasm, message, ticket, source) {
  firmware.preview(null);
  worker?.terminate(); clearInterval(watchdog);
  worker = new Worker(new URL('./worker.js', import.meta.url), { type: 'module' });
  const active = worker;
  let firstFrame = true;
  lastAlive = performance.now();
  watchdog = setInterval(() => {
    if (performance.now() - lastAlive > 4000) {
      active.terminate(); clearInterval(watchdog);
      firmware.preview(null);
      report('Preview stopped: no response for 4 seconds. Reconnect or rebuild to recover.', true);
    }
  }, 1000);
  active.onmessage = ({ data }) => {
    if (active !== worker) return;
    lastAlive = performance.now();
    if (data.type === 'frame') {
      context.putImageData(new ImageData(data.rgba, 240, 320), 0, 0);
      if (firstFrame && ticket === generation) { report(message); firmware.preview(source); }
      firstFrame = false;
    } else if (data.type === 'error') {
      firmware.preview(null);
      report(`Preview error: ${data.message}`, true);
      active.terminate(); clearInterval(watchdog);
    }
  };
  active.onerror = event => {
    firmware.preview(null); report(event.message, true); active.terminate(); clearInterval(watchdog);
  };
  active.postMessage({ type: 'load', wasm }, [wasm]);
}
async function compile() {
  if (!compileEndpoint) return;
  clearTimeout(timer);
  if (busy) { queued = true; return; }
  busy = true; queued = false;
  const ticket = ++generation;
  const source = editor.value;
  const endpoint = compileEndpoint;
  const start = performance.now();
  requestController = new AbortController();
  const controller = requestController;
  const deadline = setTimeout(() => controller.abort(), 35000);
  report('Compiling ContentView…'); diagnostics.textContent = '';
  try {
    const response = await fetch(endpoint, {
      ...compilerFetchOptions(endpoint), signal: controller.signal,
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ source }),
    });
    if (!response.ok) {
      const body = await response.text();
      let error;
      try { error = JSON.parse(body).error; } catch { error = body; }
      if (ticket === generation) {
        report('Build failed — previous preview retained', true);
        diagnostics.textContent = error || `Compiler returned HTTP ${response.status}`;
      }
      return;
    }
    const wasm = await response.arrayBuffer();
    if (ticket !== generation) return;
    const seconds = ((performance.now() - start) / 1000).toFixed(2);
    startPreview(wasm, `Live · ${seconds}s build round trip · ${(wasm.byteLength / 1024).toFixed(0)} KiB WASM`, ticket, source);
  } catch (error) {
    if (ticket === generation) {
      report('Compiler unavailable — previous preview retained', true);
      diagnostics.textContent = `Check that the compiler terminal is running and the website origin is allowed. ${error.message}`;
    }
  } finally {
    clearTimeout(deadline);
    if (requestController === controller) requestController = null;
    busy = false;
    if (compileEndpoint && (queued || (ticket !== generation && auto.checked))) compile();
  }
}
function edited() {
  firmware.edited();
  ++generation; clearTimeout(timer);
  report(auto.checked ? 'Waiting for edits…' : 'Edited — press Build & Run');
  if (auto.checked) timer = setTimeout(compile, 650);
}
function setConnected(endpoint, firmwareAvailable = false) {
  firmware.connected(endpoint, firmwareAvailable);
  compileEndpoint = endpoint;
  editor.setReadOnly(!endpoint);
  document.querySelector('#run').disabled = !endpoint;
  auto.disabled = !endpoint;
  disconnectButton.disabled = !endpoint;
  document.querySelector('#reset').textContent = endpoint ? 'Reset example' : 'Restart example';
  const note = document.querySelector('#mode-note');
  note.hidden = false;
  note.textContent = endpoint
    ? `Swift source is sent to ${endpoint.href}. The compiled preview runs in this browser.`
    : 'Try the example with UP / DOWN / OK, or connect your compiler to edit Swift.';
}
function cancelCompilation() {
  clearTimeout(timer); ++generation; queued = false;
  requestController?.abort();
  setConnected(null);
}
async function connect() {
  const ticket = ++connectionGeneration;
  connectionController?.abort();
  cancelCompilation();
  connectButton.disabled = true;
  try {
    const endpoint = compilerURL(endpointInput.value, location.href);
    connectionReport(`Connecting to ${endpoint.host}…`);
    connectionController = new AbortController();
    const controller = connectionController;
    const deadline = setTimeout(() => controller.abort(), 15000);
    let response;
    try {
      response = await fetch(new URL('./health', endpoint), {
        ...compilerFetchOptions(endpoint), signal: controller.signal,
      });
    } finally { clearTimeout(deadline); }
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const health = await response.json();
    if (health.service !== 'openswiftui-passport-compiler' || health.protocolVersion !== 1) {
      throw new Error('This URL does not provide a compatible OpenSwiftUI compiler.');
    }
    if (ticket !== connectionGeneration) return;
    endpointInput.value = endpoint.href;
    try { localStorage.setItem(storageKey, endpoint.href); } catch { /* Storage may be unavailable. */ }
    setConnected(endpoint, health.firmwareAvailable);
    connectionReport(`Connected · ${endpoint.host}`);
    compile();
  } catch (error) {
    if (ticket !== connectionGeneration) return;
    connectionReport(`Cannot connect. Start the compiler with --allow-origin ${location.origin}, and allow local network access in your browser. ${error.message}`, true);
  } finally {
    if (ticket === connectionGeneration) connectButton.disabled = false;
  }
}
document.querySelector('#compiler-form').onsubmit = event => { event.preventDefault(); connect(); };
disconnectButton.onclick = () => {
  ++connectionGeneration; connectionController?.abort(); cancelCompilation();
  connectButton.disabled = false;
  connectionReport('Disconnected. The current preview remains interactive.');
  report('Compiler disconnected');
};
document.querySelector('#run').onclick = compile;
auto.onchange = () => { clearTimeout(timer); if (auto.checked) compile(); };
document.querySelector('#reset').onclick = () => {
  editor.value = example; ++generation; diagnostics.textContent = '';
  if (compileEndpoint) compile();
  else if (precompiled) startPreview(precompiled.slice(0), 'Live · precompiled example · runs entirely in your browser', generation, example);
  else report('Connect a compiler to rebuild the example.');
};
document.querySelectorAll('[data-button]').forEach(button => button.onclick = () => worker?.postMessage({ type: 'button', button: Number(button.dataset.button) }));
async function initialize() {
  configuration = await (await fetch(new URL('./config.json', import.meta.url))).json();
  example = await (await fetch(new URL('./ContentView.swift', import.meta.url))).text();
  editor = createSwiftEditor(document.querySelector('#source'), { doc: example, readOnly: true, onChange: edited });
  setConnected(null);
  const selected = new URLSearchParams(location.search).get('compiler');
  let remembered;
  try { remembered = localStorage.getItem(storageKey); } catch { /* Optional preference only. */ }
  endpointInput.value = selected ?? remembered ?? (configuration.compileEndpoint
    ? new URL(configuration.compileEndpoint, location.href).href : DEFAULT_COMPILER);
  document.querySelector('#skill-prompt').textContent = `Install the AI Passport local compiler skill from ${new URL('./skills/ai-passport-local-compiler.zip', location.href).href}. Use it to clone https://github.com/OpenSwiftUIProject/ai-passport.git on main, prepare the Swift compiler on my computer, and connect this playground: ${location.origin}${location.pathname}. Allow the website origin ${location.origin}.`;
  document.querySelector('#start-command').textContent = `git clone --branch main --single-branch https://github.com/OpenSwiftUIProject/ai-passport.git\ncd ai-passport\n./tools/playground/start-compiler.sh --allow-origin ${location.origin}`;
  if (configuration.precompiled) {
    const response = await fetch(new URL(configuration.precompiled.file, location.href));
    if (!response.ok) throw new Error('Unable to load the precompiled example');
    precompiled = await response.arrayBuffer();
    const hex = bytes => [...new Uint8Array(bytes)].map(b => b.toString(16).padStart(2, '0')).join('');
    const digest = hex(await crypto.subtle.digest('SHA-256', precompiled));
    const sourceDigest = hex(await crypto.subtle.digest('SHA-256', new TextEncoder().encode(example)));
    if (digest !== configuration.precompiled.sha256 || sourceDigest !== configuration.precompiled.sourceSha256) {
      throw new Error('Example source or WASM checksum mismatch');
    }
    document.querySelector('#licenses-link').hidden = false;
    startPreview(precompiled.slice(0), 'Live · precompiled example · runs entirely in your browser', generation, example);
  }
  // Only the local same-origin editor connects automatically. A link or saved
  // endpoint merely fills the field; a click opts into sending source elsewhere.
  if (selected === null && configuration.mode === 'local'
      && compilerURL(endpointInput.value, location.href).origin === location.origin) await connect();
  else if (!precompiled) report('Connect a compiler to start the preview.');
}
initialize().catch(error => report(error.message, true));
