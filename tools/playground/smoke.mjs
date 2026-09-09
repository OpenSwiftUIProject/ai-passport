import assert from 'node:assert/strict';
import { readFile, writeFile } from 'node:fs/promises';
import { createWasi } from './wasi.js';

const directory = process.argv[2] ?? '../../build/playground/build';
let instance;
const logs = [];
({ instance } = await WebAssembly.instantiate(await readFile(`${directory}/preview.wasm`), {
  wasi_snapshot_preview1: createWasi(() => instance.exports.memory, text => logs.push(text)),
}));
const api = instance.exports;
const frame = () => new Uint8Array(api.memory.buffer, api.preview_framebuffer(), 240 * 320 * 2).slice();
api.preview_init();
const initial = frame();
assert.ok(api.preview_revision() > 0);
await writeFile(`${directory}/initial.rgb565`, initial);
api.preview_button(1);
assert.notDeepEqual(frame(), initial, 'DOWN must update State and the rendered palette');
await writeFile(`${directory}/down.rgb565`, frame());
api.preview_button(0); // Match the native fixture: restore palette before OK.
api.preview_button(2);
await writeFile(`${directory}/hidden.rgb565`, frame());
api.preview_button(2); // Restore image.
assert.deepEqual(frame(), initial, 'Inverse actions must reproduce the original pixels');
const memoryBytes = api.memory.buffer.byteLength;
for (let i = 0; i < 1000; ++i) {
  api.preview_button(i % 3);
  api.preview_tick(16);
}
assert.equal(api.memory.buffer.byteLength, memoryBytes, 'Memory should remain stable after repeated renders');
assert.ok(!logs.some(line => line.includes('render=FAIL')));
const result = { stateAndInput: 'PASS', iterations: 1000, memoryBytes, wasmBytes: (await readFile(`${directory}/preview.wasm`)).length };
await writeFile(`${directory}/smoke-result.json`, JSON.stringify(result, null, 2) + '\n');
console.log(result);
