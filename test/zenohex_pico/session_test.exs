defmodule ZenohexPico.SessionTest do
  use ExUnit.Case

  alias ZenohexPico.{Config, Session}

  @z_config_mode_key 0x40
  @z_config_mode_peer "peer"
  @z_config_connect_key 0x41
  @z_config_listen_key 0x42

  test "opens, publishes, queries, and closes" do
    listen_config = peer_config(@z_config_listen_key, "tcp/0.0.0.0:7448")
    connect_config = peer_config(@z_config_connect_key, "tcp/127.0.0.1:7448")

    assert {:ok, listen_session} = Session.open(listen_config)
    on_exit(fn -> Session.close(listen_session) end)

    assert {:ok, session} = Session.open(connect_config)
    assert :ok = Session.put(session, "zenohex_pico/session", "payload")

    assert {:error, :timeout} =
             Session.get(session, "zenohex_pico/no_responder", 100, query_timeout: 10)

    assert :ok = Session.close(session)
    assert {:error, :session_closed} = Session.close(session)
  end

  defp peer_config(endpoint_key, endpoint) do
    {:ok, config} = Config.default()
    {:ok, config} = Config.insert(config, @z_config_mode_key, @z_config_mode_peer)
    {:ok, config} = Config.insert(config, endpoint_key, endpoint)
    config
  end
end
