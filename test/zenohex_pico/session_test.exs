defmodule ZenohexPico.SessionTest do
  use ExUnit.Case

  alias ZenohexPico.{Config, Session}

  test "opens, publishes, queries, and closes" do
    listen_config = peer_config(:listen, "tcp/localhost:7447")
    connect_config = peer_config(:connect, "tcp/localhost:7447")

    assert {:ok, listen_session} = Session.open(listen_config)
    on_exit(fn -> Session.close(listen_session) end)

    assert {:ok, connect_session} = Session.open(connect_config)
    assert :ok = Session.put(connect_session, "zenohex_pico/session", "payload")

    assert {:error, :timeout} = Session.get(connect_session, "zenohex_pico/no_responder", 100)

    assert :ok = Session.close(connect_session)
    assert {:error, :session_closed} = Session.close(connect_session)
  end

  defp peer_config(endpoint_key, endpoint) do
    {:ok, config} = Config.default()
    {:ok, config} = Config.insert(config, :mode, "peer")
    {:ok, config} = Config.insert(config, endpoint_key, endpoint)
    config
  end
end
