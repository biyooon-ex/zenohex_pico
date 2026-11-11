# ZenohexPico for UNIX

ZenohexPico is primarily designed for use with AtomVM running on ESP32.

So we highly recommend using [Zenohex](https://github.com/biyooon-ex/zenohex) for Unix use cases because it supports more of the Zenoh API than ZenohexPico.

see. [Zenohex API compatibility](../ZENOHEX_API_COMPATIBILITY.md)


## Usage

ZenohexPico has a policy of providing APIs that wrap the basic functionality of Zenoh like other API libraries.

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
