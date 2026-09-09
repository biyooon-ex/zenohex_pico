defmodule ZenohexPicoTest do
  use ExUnit.Case

  alias ZenohexPico.{Config, Session}

  test "top-level put and get use an explicit configuration" do
    listen_config = peer_config(:listen, "tcp/localhost:7447")
    connect_config = peer_config(:connect, "tcp/localhost:7447")

    assert {:ok, listen_session} = Session.open(listen_config)
    on_exit(fn -> Session.close(listen_session) end)

    assert :ok = ZenohexPico.put(connect_config, "zenohex_pico/public_api", "payload")

    assert {:error, :timeout} =
             ZenohexPico.get(connect_config, "zenohex_pico/no_responder", 100, query_timeout: 10)
  end

  defp peer_config(endpoint_key, endpoint) do
    {:ok, config} = Config.default()
    {:ok, config} = Config.insert(config, :mode, "peer")
    {:ok, config} = Config.insert(config, endpoint_key, endpoint)
    config
  end
end
