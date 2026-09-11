import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { createHash } from 'node:crypto';
import { resolve, join } from 'node:path';
import { pathToFileURL, fileURLToPath } from 'node:url';

const root = resolve(process.argv[2] ?? '../../build/pages/ai-passport');
const local = fileURLToPath(new URL('.', import.meta.url));
const config = JSON.parse(await readFile(join(root, 'config.json'), 'utf8'));
for (const line of (await readFile(join(root, 'SHA256SUMS'), 'utf8')).trim().split('\n')) {
  const [sha, name] = line.split('  ');
  assert.equal(createHash('sha256').update(await readFile(join(root, name))).digest('hex'), sha, name);
}
assert.equal(config.mode, 'static');
assert.equal(config.compileEndpoint, null);
const { DEFAULT_SIMULATOR, simulatorURL, simulatorTransport } = await import(pathToFileURL(join(root, 'simulator-config.js')));
const simulator = simulatorURL(DEFAULT_SIMULATOR, 'https://openswiftuiproject.github.io/ai-passport/');
assert.equal(simulatorTransport(simulator, 'https://openswiftuiproject.github.io/ai-passport/'), 'discover');
assert.equal(simulator.pathname, '/FoloToy-Passport-Simulator/');
const html = await readFile(join(root, 'index.html'), 'utf8');
const metadata = Object.fromEntries([...html.matchAll(/<meta (?:name|property)="([^"]+)" content="([^"]*)"/g)]
  .map(([, name, value]) => [name, value]));
assert.equal(metadata['twitter:card'], 'summary_large_image');
const canonical = new URL(metadata['og:url']);
const coverURL = new URL(metadata['og:image']);
assert.equal(canonical.protocol, 'https:');
assert.equal(coverURL.href, new URL('social-card.png', canonical).href);
assert.equal(metadata['twitter:image'], coverURL.href);
assert.ok(metadata['og:title'] && metadata['og:description'] && metadata['og:image:alt']);
const cover = await readFile(join(root, 'social-card.png'));
assert.equal(cover.subarray(0, 8).toString('hex'), '89504e470d0a1a0a', 'share image must be a real PNG');
assert.equal(cover.readUInt32BE(16), Number(metadata['og:image:width']));
assert.equal(cover.readUInt32BE(20), Number(metadata['og:image:height']));
const { createWasi } = await import(pathToFileURL(join(root, 'wasi.js')));
let instance;
const logs = [];
({ instance } = await WebAssembly.instantiate(await readFile(join(root, 'preview.wasm')), {
  wasi_snapshot_preview1: createWasi(() => instance.exports.memory, text => logs.push(text)),
}));
const api = instance.exports;
const pixels = () => Buffer.from(new Uint8Array(api.memory.buffer, api.preview_framebuffer(), 153600));
api.preview_init();
assert.deepEqual(pixels(), await readFile(join(local, '../../build/playground/build/initial.rgb565')));
api.preview_button(1);
assert.deepEqual(pixels(), await readFile(join(local, '../../build/playground/build/down.rgb565')));
api.preview_button(0);
api.preview_button(2);
assert.deepEqual(pixels(), await readFile(join(local, '../../build/playground/build/hidden.rgb565')));
assert.ok(!logs.some(text => text.includes('render=FAIL')));
console.log('PASS: exported checksums, share card and PNG dimensions, no compiler endpoint, three native-matching WASM frames');
