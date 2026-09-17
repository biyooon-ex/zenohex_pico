# zenoh-pico-component

This repository is a wrapper that makes `zenoh-pico` usable from native ESP-IDF (`idf.py`).

It provides only the minimum integration layer required to build `zenoh-pico` as an ESP-IDF component.
The `zenoh-pico` source is kept as a git submodule at the Mix project root.

## Scope

- In scope: a minimal, native ESP-IDF (`idf.py`) component integration
- Out of scope: modifications to upstream `zenoh-pico`, feature tuning, automated hardware validation

## Repository Layout

- `CMakeLists.txt`: ESP-IDF component entry point
- `../../zenoh-pico`: upstream source tree expected by this component

## Usage

### 1) Add this repository to your workspace

If you are reading this on GitHub and do not have a local copy yet:

```bash
git clone <this-repo-url>
cd <this-repo-dir>
```

If you are already inside the local repository, skip `git clone` and `cd`.

Then, from the repository root, initialize/update submodules:

```bash
git submodule update --init --recursive
```

Ensure the `zenoh-pico` submodule is present at the Mix project root (relative
to this component: `../../zenoh-pico`).

### 2) Reference it with `-DEXTRA_COMPONENT_DIRS`

Pass this repository path from your ESP-IDF project when running `idf.py`.

```bash
idf.py -DEXTRA_COMPONENT_DIRS="/absolute/path/to/zenoh_pico_component" reconfigure
idf.py -DEXTRA_COMPONENT_DIRS="/absolute/path/to/zenoh_pico_component" build
```

You can also set the same path once in your project `CMakeLists.txt`:

```cmake
set(EXTRA_COMPONENT_DIRS "/absolute/path/to/zenoh_pico_component")
```

### 3) Depend on this component from another component

If another component in your ESP-IDF project needs this wrapper, add it to
`PRIV_REQUIRES` in that component's `idf_component_register` call.

Use the actual component name resolved by ESP-IDF (normally the directory name
that contains this `CMakeLists.txt`).

```cmake
idf_component_register(
	SRCS "my_component.c"
	INCLUDE_DIRS "include"
	PRIV_REQUIRES "<this-component-name>"
)
```

Use `PRIV_REQUIRES` when the dependency is only needed inside your component
implementation files and should not be exposed to components that depend on
your component.

If your public headers include Zenoh-Pico headers, use `REQUIRES` instead.

### 4) ESP-IDF IPv6 requirement

With upstream default feature settings, Zenoh-Pico multicast code is enabled and
expects IPv6 socket types/constants from lwIP.

Enable IPv6 in your ESP-IDF project configuration:

- `CONFIG_LWIP_IPV6=y`

### 5) Control the Zenoh log level with `idf.py -D`

Pass zenoh-pico's `ZENOH_LOG` CMake variable directly:

```bash
# Valid values: TRACE, DEBUG, INFO, WARN, ERROR
idf.py -DZENOH_LOG=TRACE build
```

## Source Selection Policy

This wrapper adds zenoh-pico to the ESP-IDF CMake tree with
`ZP_PLATFORM=espidf`. zenoh-pico selects the ESP-IDF sources and excludes
other platform backends.

## Notes

- `zenoh-pico/config.h` is generated from
	`zenoh-pico/include/zenoh-pico/config.h.in` in zenoh-pico's directory within
	the ESP-IDF build tree; no `zxp_unix` build is required.
- Feature definitions use the upstream CMake defaults and can be overridden with
	ESP-IDF CMake cache variables such as `-DZ_FEATURE_QUERY=0`.
