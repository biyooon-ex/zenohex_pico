# ZenohexPico

[![Hex version](https://img.shields.io/hexpm/v/zenohex_pico.svg "Hex version")](https://hex.pm/packages/zenohex_pico)
[![API docs](https://img.shields.io/hexpm/v/zenohex_pico.svg?label=hexdocs "API docs")](https://hexdocs.pm/zenohex_pico/)
[![License](https://img.shields.io/hexpm/l/zenohex_pico.svg)](https://github.com/biyooon-ex/zenohex_pico/blob/main/LICENSE)
[![CI](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml/badge.svg)](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml)

A thin Elixir wrapper around zenoh-pico, implemented using C.

ZenohexPico is primarily designed for use with AtomVM running on ESP32.

**Currently, ZenohexPico uses version 1.10.2 of Zenoh.**

We recommend you use the same version to communicate with other Zenoh clients or
routers since version compatibility is somewhat important for Zenoh.

## Limitations

ZenohexPico supports fewer APIs than [Zenohex](https://github.com/biyooon-ex/zenohex).
For a comparison, see [Zenohex API compatibility](./ZENOHEX_API_COMPATIBILITY.md).

For Unix use cases, we recommend [Zenohex](https://github.com/biyooon-ex/zenohex),
which supports more of the Zenoh API. To use ZenohexPico on Unix, see the
[Unix README](https://github.com/biyooon-ex/zenohex_pico/blob/main/zxp_unix/README.md).

## Usage

<!-- TODO: Confirm and document prerequisites, including supported operating systems, Elixir and Erlang/OTP versions, required tools, and dependency build requirements. -->

To use ZenohexPico with AtomVM on ESP32, follow these two steps:

1. Flash AtomVM with zenoh-pico built in to the target device.
2. Add ZenohexPico to your Elixir AtomVM project and flash the application to the target device.

These steps are described below.

### Flash AtomVM with zenoh-pico built in

[GitHub Releases](https://github.com/biyooon-ex/zenohex_pico/releases) provides
ZIP files containing AtomVM with zenoh-pico built in. Download the ZIP file for
your target device from the release matching the ZenohexPico version used in your project.
The currently published ZIP file is for ESP32-S3. Available targets may vary by
release, so check the release for your version.
Extract the ZIP file and follow the README in the extracted directory to flash
AtomVM to your target device.

To try an ESP32 target other than ESP32-S3, refer to the AtomVM build
instructions in [FOR_DEVELOPERS.md](./FOR_DEVELOPERS.md).

### Add ZenohexPico to your Elixir AtomVM project

Add the following dependency to `mix.exs` in your Elixir AtomVM project using
[ExAtomVM](https://github.com/atomvm/exatomvm):

```elixir
{:zenohex_pico, "~> 0.1.0"},
```

From the project root, set `MIX_TARGET` to `avm_esp32` and fetch the dependencies:

```sh
export MIX_TARGET=avm_esp32
mix deps.get
```

Run subsequent Mix commands with the same setting. If you use another shell,
set `MIX_TARGET` there as well.

Check the ZenohexPico version in `mix.lock` and make sure it matches the release
version of the ZIP file you used.

The Hex package does not include the upstream `zenoh-pico` source or compiled
binaries. When Mix compiles the package, its compiler task clones `zenoh-pico`.
This requires Git and network access. The task asks for
confirmation before cloning; for non-interactive builds, set
`ZENOHEX_PICO_AUTO_GIT_CLONE=true`.

This wrapper is MIT-licensed. The upstream `zenoh-pico` source is licensed under
Apache-2.0 OR EPL-2.0; see `zenoh-pico/LICENSE` and
`zenoh-pico/NOTICE.md` after it has been cloned.

Next, copy `avm_deps` from the extracted directory to the project root. This
`avm_deps` is compatible with the AtomVM included in the ZIP file.
If your project already has an `avm_deps` directory, replace it with the one
from the ZIP file. If it contains project-specific files, back them up before
replacing it and check the compatibility of any files you need.

Then run the following commands in order to pack the application and flash it
to the target device:

```sh
mix atomvm.packbeam
mix atomvm.esp32.flash --port /dev/your.usbserial
```

For details on these Mix tasks, see [ExAtomVM](https://github.com/atomvm/exatomvm).

You are now ready to use ZenohexPico. Enjoy!

For examples of using ZenohexPico, see the `start/0` function and the functions
it calls in
[ZenohexPico.AvmEsp32.Test](https://github.com/biyooon-ex/zenohex_pico/blob/main/lib/zenohex_pico/avm_esp32/test.ex).
