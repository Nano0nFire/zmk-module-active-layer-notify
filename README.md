# zmk-module-active-layer-notify

![ZMK Version](https://img.shields.io/badge/ZMK-master-blue)
[![Test](https://github.com/Nano0nFire/zmk-module-active-layer-notify/actions/workflows/zmk-module.yml/badge.svg?branch=main)](https://github.com/Nano0nFire/zmk-module-active-layer-notify/actions/workflows/zmk-module.yml) [![Devcontainer](https://github.com/Nano0nFire/zmk-module-active-layer-notify/actions/workflows/devcontainer.yml/badge.svg?branch=main)](https://github.com/Nano0nFire/zmk-module-active-layer-notify/actions/workflows/devcontainer.yml)

A custom ZMK Studio RPC subsystem that **notifies connected clients whenever the
keyboard's active (highest) layer changes**, and answers a `get_active_layer`
request. zoo-studio's live keymap overlay and per-app layer switching consume it
via the `useActiveLayer.ts` hook. Built on the **unofficial** custom ZMK Studio
RPC protocol (subsystem id `nano0nfire__active_layer_notify`).

## Summary

This module includes:

- **Firmware**: Custom Studio RPC handler + `zmk_layer_state_changed` listener (`src/studio/active_layer_notify_handler.c`, `active_layer_notify_listener.c`)
- **Protocol**: Protobuf definition (`proto/nano0nfire/active_layer_notify/active_layer_notify.proto`)
- **Web UI**: consumed by the zoo-studio hook `useActiveLayer.ts`
- **Tests**: Firmware unit tests (`tests/studio/`) and build tests (`tests/zmk-config/`)

Read through the [ZMK Module Creation](https://zmk.dev/docs/development/module-creation) page for details on how to configure this module.

## More Info

For more info on modules, you can read through through the [Zephyr modules page](https://docs.zephyrproject.org/3.5.0/develop/modules.html) and [ZMK's page on using modules](https://zmk.dev/docs/features/modules). [Zephyr's west manifest page](https://docs.zephyrproject.org/3.5.0/develop/west/manifest.html#west-manifests) may also be of use.

## Module User Guide

1. Add dependency to your `config/west.yml`. Note: this module requires a patched ZMK with custom Studio RPC support.

   ```yml
   manifest:
       remotes:
           ...
           - name: cormoran
           url-base: https://github.com/cormoran
       projects:
           ...
           - name: zmk-module-template
           remote: cormoran
           revision: main+custom-studio-protocol # or latest commit hash
           import: true
           ...
           # Required: patched ZMK with custom Studio RPC support
           - name: zmk
           remote: cormoran
           revision: main+custom-studio-protocol
           import:
               file: app/west.yml
   ```

2. Enable flags in your `config/<shield>.conf`

   ```conf
   CONFIG_ZMK_ACTIVE_LAYER_NOTIFY=y

   # Optionally enable custom Studio RPC
   CONFIG_ZMK_STUDIO=y
   CONFIG_ZMK_ACTIVE_LAYER_NOTIFY_STUDIO_RPC=y
   CONFIG_ZMK_CUSTOM_SETTINGS=y
   CONFIG_ZMK_CUSTOM_SETTINGS_STUDIO_RPC=y
   CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE=128
   CONFIG_ZMK_LOW_PRIORITY_THREAD_STACK_SIZE=2048
   ```

3. Implement your custom protocol by editing:
   - `proto/nano0nfire/active_layer_notify/active_layer_notify.proto` — message types
   - `src/studio/active_layer_notify_handler.c` — firmware RPC handler
   - `web/src/App.tsx` — web UI

### Web UI

See [web/README.md](./web/README.md) for web UI development instructions.

### Publishing Web UI

**GitHub Pages**: Merge a pull request into `main+custom-studio-protocol` to deploy to `https://<account>.github.io/<repo>/`.

**Cloudflare Workers (PR previews)**: Configure `CLOUDFLARE_API_TOKEN` and `CLOUDFLARE_ACCOUNT_ID` secrets.

## Module Development Guide

### Setup for running test

#### Option0: Dev container (recommended)

Open this repository in VS Code with the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers). The container automatically initializes the west workspace using the isolated layout.

#### Option1: west workspace directory layout

Set west topdir as parent of repository root and download dependencies under `../`.
This layout is useful to reduce disk usage by sharing dependencies with other zephyr modules.
The build result is located in `../build`.

```bash
mkdir west-workspace
cd west-workspace # this directory becomes west workspace root (topdir)
git clone <this repository>
# rm -r .west # if exists to reset workspace
west init -l . --mf west/west-test-workspace.yml
west update --narrow
west zephyr-export
```

#### Option2: isolated directory layout

Set west topdir as repository root and download dependencies under `./dependencies`.
This layout is useful if you don't want to share dependencies to other zephyr modules.
Dev container and github actions uses this layout.
The build result is located in `./build`.

```bash
git clone <this repository>
cd <cloned directory>
west init -l west --mf west-test-isolated.yml
west update --narrow
west zephyr-export
```

### Pre-commit

Every commit need to pass pre-commit verification. The verification contains formatting code and running tests.

```
pip install pre-commit
pre-commit install

# Run pre-commit manually
pre-commit run --all-files
# Run for git staged files
pre-commit run
```

### Running Test

```bash
# Run unit test + build test and verify the results
python3 -m unittest
# Run build test directly
west zmk-build tests/zmk-config
# Run unit test directly
west zmk-test tests -m .
# Run web tests
cd web && npm test
```

### Sync changes from template

Run `Actions > Sync Changes in Template > Run workflow` to get the latest template changes as a pull request.

If the template contains changes in `.github/workflows/*`, register a GitHub personal access token as `GH_TOKEN` repository secret (`repo` + `workflow` scopes).

### Coding agent on actions

Actions for github copilot and claude are available.

- Mention `@copilot`
- Setup `ANTHROPIC_API_KEY` secret and mention `@claude`
  - Or fix [claude.yml](./github/workflows/claude.yml) to use `CLAUDE_CODE_OAUTH_TOKEN`
