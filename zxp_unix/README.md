# ZenohexPico

[![Hex version](https://img.shields.io/hexpm/v/zenohex_pico.svg "Hex version")](https://hex.pm/packages/zenohex_pico)
[![API docs](https://img.shields.io/hexpm/v/zenohex_pico.svg?label=hexdocs "API docs")](https://hexdocs.pm/zenohex_pico/)
[![License](https://img.shields.io/hexpm/l/zenohex_pico.svg)](https://github.com/biyooon-ex/zenohex_pico/blob/main/LICENSE)
[![CI](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml/badge.svg)](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml)

## Usage

**Currently, ZenohexPico uses version 1.10.0 of Zenoh.**

We recommend you use the same version to communicate with other Zenoh clients or routers since version compatibility is somewhat important for Zenoh.
Please also check the description on [Releases](https://github.com/biyooon-ex/zenohex_pico/releases) about the corresponding Zenoh version.

FYI, the development team currently uses the following versions.

- Elixir 1.18.5-otp-27
- Erlang/OTP 27.3.4.17

### Installation

`zenohex_pico` is [available in Hex](https://hex.pm/packages/zenohex_pico).

You can install this package into your project by adding `zenohex_pico` to your list of dependencies in `mix.exs`:

```elixir
  defp deps do
    [
      ...
      {:zenohex_pico, "~> 0.1.0"},
      ...
    ]
  end
```

Documentation is also [available in HexDocs](https://hexdocs.pm/zenohex_pico).

### Getting Started

Zenohex has a policy of providing APIs that wrap the basic functionality of Zenoh like other API libraries.

Here is the first step to building an Elixir application and using this library.

```sh
$ mix deps.get
$ mix compile
$ iex -S mix
```

```elixir
iex()> {:ok, config} = ZenohexPico.Config.default()
{:ok, #Reference<>}
iex()> {:ok, connect_config} = ZenohexPico.Config.insert(config, :connect, "tcp/localhost:7447")
{:ok, #Reference<>}
iex()> {:ok, listen_config} = ZenohexPico.Config.insert(config, :listen, "tcp/localhost:7447")
{:ok, #Reference<>}
iex()> {:ok, listen_session} = ZenohexPico.Session.open(listen_config)
{:ok, #Reference<>}
iex()> {:ok, connect_session} = ZenohexPico.Session.open(connect_config)
{:ok, #Reference<>}
iex()> {:ok, subscriber} = ZenohexPico.Session.declare_subscriber(listen_session, "key/expr", self())
{:ok, #Reference<>}
iex()> :ok = ZenohexPico.Session.put(connect_session, "key/expr", "payload")
:ok
iex(10)> flush
%ZenohexPico.Sample{
  attachment: "",
  congestion_control: :drop,
  encoding: "zenoh/bytes",
  express: false,
  key_expr: "key/expr",
  kind: :put,
  payload: "payload",
  priority: :data,
  timestamp: nil
}
:ok
```
