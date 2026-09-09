import { createWasi } from './wasi.js';
let instance, revision = -1, interval, lastTick;
function frame() {
  const api = instance.exports;
  const next = api.preview_revision();
  if (next === revision) return;
  revision = next;
  const pixels = new Uint16Array(api.memory.buffer, api.preview_framebuffer(), 240 * 320);
  const rgba = new Uint8ClampedArray(240 * 320 * 4);
  for (let i = 0; i < pixels.length; ++i) {
    const p = pixels[i];
    rgba[i * 4] = ((p >> 11) & 31) * 255 / 31;
    rgba[i * 4 + 1] = ((p >> 5) & 63) * 255 / 63;
    rgba[i * 4 + 2] = (p & 31) * 255 / 31;
    rgba[i * 4 + 3] = 255;
  }
  postMessage({ type: 'frame', rgba, revision }, [rgba.buffer]);
}
function failure(error) {
  clearInterval(interval);
  postMessage({ type: 'error', message: error.message });
}
onmessage = async ({ data }) => {
  try {
    if (data.type === 'load') {
      const wasi = createWasi(() => instance.exports.memory, text => postMessage({ type: 'log', text }));
      ({ instance } = await WebAssembly.instantiate(data.wasm, { wasi_snapshot_preview1: wasi }));
      instance.exports.preview_init();
      frame();
      lastTick = performance.now();
      interval = setInterval(() => {
        try {
          const now = performance.now();
          instance.exports.preview_tick(Math.min(100, Math.max(1, Math.round(now - lastTick))));
          lastTick = now; frame();
          postMessage({ type: 'alive' });
        } catch (error) { failure(error); }
      }, 16);
    } else if (data.type === 'button' && instance) {
      instance.exports.preview_button(data.button); frame();
    }
  } catch (error) { failure(error); }
};
