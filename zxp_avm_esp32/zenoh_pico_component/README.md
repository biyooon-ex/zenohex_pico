# zenoh-pico-component

This repository is a wrapper that makes `zenoh-pico` usable from native ESP-IDF (`idf.py`).

It provides only the minimum integration layer required to build `zenoh-pico` as an ESP-IDF component.
The `zenoh-pico` source is kept as a git submodule at the Mix project root.

## Scope

- In scope: a minimal, native ESP-IDF (`idf.py`) component integration
- Out of scope: modifications to upstream `zenoh-pico`, wrapper-specific feature tuning, automated on-device testing

Automated on-device testing means CI that flashes firmware to a physical ESP32
and checks its startup, serial output, Wi-Fi connection, or Zenoh traffic. This
component does not provide test firmware, connected hardware, or CI automation
for those checks.

## Repository Layout

- `CMakeLists.txt`: ESP-IDF component entry point
- `../../zenoh-pico`: upstream source tree expected by this component

## Usage

### 1) Add the parent repository to your workspace

If you are reading this on GitHub and do not have a local copy yet:

```bash
git clone <zenohex_pico-repository-url>
cd zenohex_pico
```

This component is not a standalone repository: it requires the `zenoh-pico`
submodule at `../../zenoh-pico`.

Then, from the repository root, initialize/update submodules:

```bash
git submodule update --init --recursive
```

Ensure the `zenoh-pico` submodule is present at the Mix project root (relative
to this component: `../../zenoh-pico`).

### 2) Reference it with `EXTRA_COMPONENT_DIRS`

For a native ESP-IDF project, add this component directory to
`EXTRA_COMPONENT_DIRS`:

```bash
idf.py -DEXTRA_COMPONENT_DIRS="/absolute/path/to/zenohex_pico/zxp_avm_esp32/zenoh_pico_component" build
```

For AtomVM, include both components:

```bash
idf.py -DEXTRA_COMPONENT_DIRS="/absolute/path/to/zenohex_pico/zxp_avm_esp32/zenoh_pico_component;/absolute/path/to/zenohex_pico/zxp_avm_esp32/zenohex_pico_component" build
```

Run `idf.py reconfigure` after changing `EXTRA_COMPONENT_DIRS` in an existing
build directory.

### 3) ESP-IDF IPv6 requirement

With upstream default feature settings, Zenoh-Pico multicast code is enabled and
expects IPv6 socket types/constants from lwIP.

Enable IPv6 in your ESP-IDF project configuration:

- `CONFIG_LWIP_IPV6=y`

For AtomVM applications, also set `CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192`.
Zenoh-Pico initialization and network setup can exceed the default main-task
stack; insufficient stack may cause a stack overflow, reset, or failure during
startup.

### 4) Control the Zenoh log level with `idf.py -D`

When `ZENOH_LOG` is unset, Zenoh-Pico logging is disabled. To enable it for an
AtomVM build, pass one of `TRACE`, `DEBUG`, `INFO`, `WARN`, or `ERROR`:

```bash
idf.py -DZENOH_LOG=TRACE -DEXTRA_COMPONENT_DIRS="/absolute/path/to/zenohex_pico/zxp_avm_esp32/zenoh_pico_component;/absolute/path/to/zenohex_pico/zxp_avm_esp32/zenohex_pico_component" build
```

## Source Selection Policy

This wrapper adds zenoh-pico to the ESP-IDF CMake tree with
`ZP_PLATFORM=espidf`. zenoh-pico selects the ESP-IDF sources and excludes
other platform backends.

## Notes

- `zenoh-pico/config.h` is generated from
	`zenoh-pico/include/zenoh-pico/config.h.in` in zenoh-pico's directory within
	the ESP-IDF build tree.
- Feature definitions use upstream CMake defaults. The upstream cache variables
	remain available for applications that need them, for example
	`-DZ_FEATURE_QUERY=0`.
