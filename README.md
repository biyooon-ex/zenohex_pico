# ZenohexPico

A thin Elixir wrapper around zenoh-pico, implemented using C.

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

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at <https://hexdocs.pm/zenohex_pico>.

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
