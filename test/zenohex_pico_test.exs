defmodule ZenohexPicoTest do
  use ExUnit.Case

  test "top-level put and get use an explicit configuration" do
    listen_config = peer_config(:listen, "tcp/localhost:7447")
    connect_config = peer_config(:connect, "tcp/localhost:7447")

    assert {:ok, listen_session} = ZenohexPico.Session.open(listen_config)
    on_exit(fn -> ZenohexPico.Session.close(listen_session) end)

    assert :ok = ZenohexPico.put("zenohex_pico/public_api", "payload", config: connect_config)

    assert {:error, :timeout} =
             ZenohexPico.get("zenohex_pico/no_responder", 100, config: connect_config)
  end

  defp peer_config(endpoint_key, endpoint) do
    {:ok, config} = ZenohexPico.Config.default()
    {:ok, config} = ZenohexPico.Config.insert(config, :mode, "peer")
    {:ok, config} = ZenohexPico.Config.insert(config, endpoint_key, endpoint)
    config
  end
end
