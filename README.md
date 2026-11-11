# ZenohexPico

[![Hex version](https://img.shields.io/hexpm/v/zenohex_pico.svg "Hex version")](https://hex.pm/packages/zenohex_pico)
[![API docs](https://img.shields.io/hexpm/v/zenohex_pico.svg?label=hexdocs "API docs")](https://hexdocs.pm/zenohex_pico/)
[![License](https://img.shields.io/hexpm/l/zenohex_pico.svg)](https://github.com/biyooon-ex/zenohex_pico/blob/main/LICENSE)
[![CI](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml/badge.svg)](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml)

A thin Elixir wrapper around zenoh-pico, implemented using C.

**Currently, ZenohexPico uses version 1.10.2 of Zenoh.**

We recommend you use the same version to communicate with other Zenoh clients or routers since version compatibility is somewhat important for Zenoh.
Please also check the description on [Releases](https://github.com/biyooon-ex/zenohex_pico/releases) about the corresponding Zenoh version.

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `zenohex_pico` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:zenohex_pico, "~> 0.1.0"}
  ]
end
```

## Publishing

Build and publish the Hex package:

```sh
mix hex.build
mix hex.publish
```

The Hex package does not include the upstream `zenoh-pico` source or compiled
binaries. When Mix compiles the package, its compiler task clones `zenoh-pico`
version `1.10.2`. This requires Git and network access. The task asks for
confirmation before cloning; for non-interactive builds, set
`ZENOHEX_PICO_AUTO_GIT_CLONE=true`.

Building from Hex also requires CMake, Make, a C compiler, and Erlang
development headers. This wrapper is MIT-licensed. The upstream `zenoh-pico`
source is licensed under Apache-2.0 OR EPL-2.0; see `zenoh-pico/LICENSE` and
`zenoh-pico/NOTICE.md` after it has been cloned.
