<p align="right">
  <a href="CI-validation.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Pull Request Validation

The firmware and host workflows validate pull requests: `.github/workflows/static-checks.yml` and
`.github/workflows/firmware-checks.yml`. Both run for pull requests, pushes to
`main`, and manual dispatch; local development and CI share `tools/validate.sh`.

## Workflows

- **Static checks** (`static-checks.yml`): validates English-default bilingual
  Markdown, relative links, full-SHA Actions, issue forms, the dependency lock,
  conflict markers, and likely sensitive data; then runs `actionlint` and host
  tests. It runs on every pull request.
- **Firmware checks** (`firmware-checks.yml`): runs
  `./tools/validate.sh --firmware` for ESP32-C3 in a fresh isolated
  build/configuration directory, verifies the build and the merged `0x0` image
  contents/offsets, and retains the artifact for seven days.

Both jobs use macOS 15 and the shared `.github/actions/setup-embedded` action:
Swift 6.3.1 and OpenSwiftUI revision
`e35b91a31789c3c277e9a784b9f9b86e5eb708f5` are installed explicitly. The
OpenSwiftUI checkout lives under ignored `build/ci/`, with no sibling workspace
or OpenAttributeGraph checkout required. Firmware jobs additionally install
ESP-IDF 5.5.3, its ESP32-C3 tools, and Python 3.12, then activate them through
`tools/with-env.sh`. The tag/manual `build-firmware.yml` uses the same setup;
an ESP-IDF-only Docker image does not include the required Swift compiler.

## Path filtering

`firmware-checks.yml` ignores paths that cannot affect the firmware build:
`docs/**`, `plays/**`, `skills/**`, and `*.md`. A documentation-only pull request
therefore runs the static checks but skips the firmware build. `assets/**` is
intentionally **not** ignored: `assets/` holds reusable fonts, images, music, and
sound effects that may be embedded into the firmware, so any asset change must
trigger the firmware build.

## Path filtering vs required checks

A path-filtered workflow that is skipped produces no check run for that job. If
the firmware job is ever configured as a **required** status check, a
documentation-only pull request would have no firmware check to satisfy and could
block merging. Keep this in mind before marking the firmware job required; if it
is required, the path filter must be revisited (for example, split the job into a
documentation-only pass/fail stub, or use a single unconditional workflow).

Both workflows have only `contents: read` and use no repository secrets, so they
can validate fork pull requests. Every GitHub Action is pinned to a full commit
SHA.

## Reproduce locally

```bash
./tools/validate.sh --static
source <path-to-esp-idf-v5.5.3>/export.sh
./tools/validate.sh --firmware
```

Follow the [environment bootstrap](../engineering/environment-setup.md) if ESP-IDF 5.5.3 is
not installed. Reproduce a CI failure with the same mode locally. Do not
maintain duplicate validation commands inside the workflow.

## Playground Pages

`playground-pages.yml` builds the OpenSwiftUI browser preview, validates the local
compiler API and WASM artifact, and packages the standalone setup skill. Matching
PRs validate without deploying; main pushes in `OpenSwiftUIProject/ai-passport`
deploy through GitHub Pages. See [Playground deployment](../../../tools/playground/DEPLOYMENT.md).
The fork disables the Upstream Sync workflow because its main branch contains
maintained OpenSwiftUI changes; upstream updates are reviewed manually.
