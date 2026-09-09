import { build } from 'esbuild';
import { fileURLToPath } from 'node:url';
const root = fileURLToPath(new URL('.', import.meta.url));
await build({
  absWorkingDir: root,
  entryPoints: ['editor-entry.js'],
  outfile: 'build/editor.js',
  bundle: true,
  format: 'esm',
  target: 'es2022',
  minify: true,
  legalComments: 'eof',
});
