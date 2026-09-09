[简体中文](DEPLOYMENT.zh_CN.md) · English

# GitHub Pages deployment

The destination repository is [OpenSwiftUIProject/ai-passport](https://github.com/OpenSwiftUIProject/ai-passport),
with intended entry https://openswiftuiproject.github.io/ai-passport/.
The Pages workflow builds the static frontend, example WASM and standalone setup
skill. Visitors can play immediately; arbitrary Swift editing and firmware
builds use their own local compiler. Simulator runs separately with its Node/QEMU
runtime. None of these require the creator's computer to stay online.

## Build and review locally

From a clone of the full URL above on **main**, with the prerequisites in
[local compiler setup](LOCAL_COMPILER.md):

```sh
./tools/playground/start-compiler.sh --export-pages build/pages/ai-passport
cd tools/playground
node smoke.mjs
node verify-static.mjs ../../build/pages/ai-passport
cd ../..
python3 -m http.server 4192 --bind 127.0.0.1 --directory build/pages
```

Open `http://127.0.0.1:4192/ai-passport/`. The setup command derives its allowed
origin from this actual URL. All browser assets use relative URLs, including
workers and the downloadable `skills/ai-passport-local-compiler.zip`.
The adjacent `build/pages/ai-passport.zip` bundles the static site.
Export uses an empty output directory and includes matching ContentView/WASM,
local editor dependencies, licenses, recursive SHA256SUMS and `.nojekyll`.
The initial source and WASM hashes are verified before the example starts.

## Publish the reviewed change

The repository workflow `.github/workflows/playground-pages.yml` builds and tests
on matching PRs and main pushes. It deploys only main on
`OpenSwiftUIProject/ai-passport`, with a separate Pages deployment job.
In repository Settings → Pages choose **GitHub Actions**. Merge the reviewed PR,
then use the deployment job's URL as the actual public entry. The workflow also
supports manual dispatch. Local preparation does not enable Pages or publish it.

The build uses macOS, Swift 6.3.1, Node 22, CMake/Ninja and the pinned dependency
bootstrap. It does not install ESP-IDF or package a device firmware; firmware is
built on demand on the visitor's machine. Public Pages deployment and public
HTTPS-to-loopback permissions have not yet been exercised by this local change.

## Compiler and Simulator contracts

The website's compiler URL defaults to `http://127.0.0.1:4191/compile` and only
connects after the user clicks Connect. `?compiler=...` and saved URLs prefill it.
HTTPS services or HTTP loopback addresses are accepted; public HTTP addresses,
URL credentials, queries and fragments are rejected. The local service is for
trusted development, not an Internet-facing public compilation service.

- `GET /health` identifies `openswiftui-passport-compiler`, protocol 1, and reports
  `firmwareAvailable` for the optional ESP-IDF installation.
- `POST /compile`, JSON `{ "source": "..." }`, returns preview WASM or JSON error.
- `POST /firmware`, the same JSON, returns a build job ID and source SHA-256.
- `GET /firmware/<id>` returns building/ready/failed status and a bounded log tail.
- `GET /firmware/<id>/download` returns verified full firmware when ready.
- Simulator `POST /api/playground-firmware` accepts that binary with
  `X-Firmware-SHA256`, returning an expiring `/?playground=<id>` run link.
  It requires local upload enabled plus an explicit allowed Playground origin.

See [local setup](LOCAL_COMPILER.md) for standalone clone commands, ESP-IDF and
Simulator setup, browser permissions and request/resource limits.
