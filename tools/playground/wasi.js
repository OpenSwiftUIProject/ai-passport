// Only the WASI functions imported by this LVGL build. No file or network access.
export function createWasi(getMemory, log = console.log) {
  const memory = () => new DataView(getMemory().buffer);
  return {
    clock_time_get(clock, precision, pointer) {
      if (clock !== 0 && clock !== 1) return 28;
      const milliseconds = clock === 0 ? Date.now() : performance.now();
      memory().setBigUint64(pointer, BigInt(Math.floor(milliseconds * 1000000)), true);
      return 0;
    },
    fd_fdstat_get(fd, pointer) {
      if (fd !== 1 && fd !== 2) return 8;
      new Uint8Array(getMemory().buffer, pointer, 24).fill(0);
      memory().setUint8(pointer, 2); // Character device.
      memory().setBigUint64(pointer + 8, 64n, true); // FD_WRITE only.
      return 0;
    },
    random_get(pointer, length) {
      const bytes = new Uint8Array(getMemory().buffer, pointer, length);
      for (let i = 0; i < length; i += 65536) crypto.getRandomValues(bytes.subarray(i, i + 65536));
      return 0;
    },
    fd_write(fd, vectors, count, written) {
      if (fd !== 1 && fd !== 2) return 8;
      const view = memory(); let total = 0; let text = '';
      for (let i = 0; i < count; ++i) {
        const pointer = view.getUint32(vectors + i * 8, true);
        const length = view.getUint32(vectors + i * 8 + 4, true);
        text += new TextDecoder().decode(new Uint8Array(getMemory().buffer, pointer, length));
        total += length;
      }
      view.setUint32(written, total, true);
      log(text);
      return 0;
    },
    fd_close() { return 8; },
    fd_seek() { return 70; },
    proc_exit(code) { throw new Error(`WASM exited (${code})`); },
  };
}
