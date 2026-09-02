defmodule ZenohexPico.NifTest do
  use ExUnit.Case

  alias ZenohexPico.Nif

  @z_config_mode_key 0x40
  @z_config_mode_client "client"
  @z_config_mode_peer "peer"
  @z_config_connect_key 0x41
  @z_config_listen_key 0x42

  test "test_raise/0" do
    assert_raise ErlangError, ~r/raise.+/, fn -> Nif.test_raise() end
  end

  describe "config functions" do
    test "config_default/0" do
      assert {:ok, config} = Nif.config_default()
      assert is_reference(config)
    end

    test "config_get/2" do
      {:ok, config} = Nif.config_default()
      assert Nif.config_get(config, @z_config_mode_key) == {:ok, "client"}
    end

    test "config_insert/3" do
      {:ok, config} = Nif.config_default()
      {:ok, @z_config_mode_client} = Nif.config_get(config, @z_config_mode_key)

      assert {:ok, updated_config} =
               Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)

      refute updated_config == config
      assert {:ok, @z_config_mode_client} = Nif.config_get(config, @z_config_mode_key)
      assert {:ok, @z_config_mode_peer} = Nif.config_get(updated_config, @z_config_mode_key)
    end
  end

  describe "session open/close" do
    setup do
      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_listen_key, "tcp/0.0.0.0:7447")

      %{config: config}
    end

    test "session_open/1", %{config: config} do
      assert {:ok, session} = Nif.session_open(config)
      assert is_reference(session)
      assert Nif.session_close(session) == :ok
    end

    test "session_close/1", %{config: config} do
      {:ok, session} = Nif.session_open(config)

      assert Nif.session_close(session) == :ok
      assert Nif.session_close(session) == {:error, :session_closed}

      assert {:ok, reopened_session} = Nif.session_open(config)
      assert Nif.session_close(reopened_session) == :ok
    end
  end

  describe "functions that take a session" do
    setup do
      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_listen_key, "tcp/0.0.0.0:7447")
      {:ok, listen_session} = Nif.session_open(config)
      on_exit(fn -> Nif.session_close(listen_session) end)

      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_connect_key, "tcp/127.0.0.1:7447")
      {:ok, connect_session} = Nif.session_open(config)
      on_exit(fn -> Nif.session_close(connect_session) end)

      %{listen_session: listen_session, connect_session: connect_session}
    end

    test "session operations reject a closed session", %{
      listen_session: listen_session,
      connect_session: connect_session
    } do
      assert :ok = Nif.session_close(connect_session)
      assert :ok = Nif.session_close(listen_session)

      assert {:error, :session_closed} =
               Nif.session_put(connect_session, "zenohex_pico/test", "payload")

      assert {:error, :session_closed} =
               Nif.session_get(connect_session, "zenohex_pico/test", 100)

      assert {:error, :session_closed} =
               Nif.session_declare_subscriber(listen_session, "zenohex_pico/test", self())
    end

    test "session_put/4 with options", %{
      listen_session: _listen_session,
      connect_session: connect_session
    } do
      assert :ok =
               Nif.session_put(connect_session, "zenohex_pico/test", "payload",
                 attachment: "metadata",
                 congestion_control: :block,
                 encoding: "text/plain",
                 express: true,
                 priority: :data_high,
                 timestamp: "2025-07-16T01:34:56.871273403Z/208a2ec783ec4527a39cc1d5559c70e9"
               )
    end

    test "session_put/4 rejects malformed timestamp options", %{
      listen_session: _listen_session,
      connect_session: connect_session
    } do
      malformed_timestamps = [
        "2025-07-16T01:34:56Z/208a2ec783ec4527a39cc1d5559c70e9",
        "2025-07-16T01:34:56.1Z/208a2ec783ec4527a39cc1d5559c70e9",
        "2025-07-16T01:34:56.871273403Z/208A2EC783EC4527A39CC1D5559C70E9",
        "2025-07-16T01:34:56Z/not-a-zenoh-id"
      ]

      for timestamp <- malformed_timestamps do
        assert_raise ArgumentError, fn ->
          Nif.session_put(connect_session, "zenohex_pico/test", "payload", timestamp: timestamp)
        end
      end
    end

    test "session_get/4 returns timeout when there are no queryables", %{
      listen_session: _listen_session,
      connect_session: connect_session
    } do
      assert Nif.session_get(connect_session, "zenohex_pico/no_responder", 100, query_timeout: 10) ==
               {:error, :timeout}
    end

    test "session_get/4 accepts supported options", %{
      listen_session: _listen_session,
      connect_session: connect_session
    } do
      assert Nif.session_get(connect_session, "zenohex_pico/no_responder", 100,
               accept_replies: :any,
               attachment: "metadata",
               congestion_control: :block,
               consolidation: :none,
               encoding: "text/plain",
               express: true,
               payload: "query payload",
               priority: :data_high,
               target: :all,
               query_timeout: 10
             ) == {:error, :timeout}
    end

    test "session_get/4 rejects invalid options", %{
      listen_session: _listen_session,
      connect_session: connect_session
    } do
      assert_raise ArgumentError, fn ->
        Nif.session_get(connect_session, "zenohex_pico/no_responder", 100,
          consolidation: :invalid
        )
      end

      assert_raise ArgumentError, fn ->
        Nif.session_get(connect_session, "zenohex_pico/no_responder", 100, payload: :invalid)
      end

      assert_raise ArgumentError, fn ->
        Nif.session_get(connect_session, "zenohex_pico/no_responder", 100, attachment: nil)
      end

      assert_raise ArgumentError, fn ->
        Nif.session_get(connect_session, "zenohex_pico/no_responder", 100, unknown: :option)
      end
    end

    test "session_declare_subscriber/4 and subscriber_undeclare/1", %{
      listen_session: listen_session,
      connect_session: connect_session
    } do
      assert {:ok, subscriber} =
               Nif.session_declare_subscriber(listen_session, "zenohex_pico/test", self(), [])

      assert is_reference(subscriber)

      timestamp = "2025-07-16T01:34:56.871273403Z/208a2ec783ec4527a39cc1d5559c70e9"

      :ok = Nif.session_put(connect_session, "zenohex_pico/test", "payload", timestamp: timestamp)

      assert_receive %ZenohexPico.Sample{
        attachment: "",
        congestion_control: :drop,
        encoding: "zenoh/bytes",
        express: false,
        key_expr: "zenohex_pico/test",
        kind: :put,
        payload: "payload",
        priority: :data,
        timestamp: ^timestamp
      }

      assert :ok = Nif.subscriber_undeclare(subscriber)
      assert {:error, :subscriber_undeclared} = Nif.subscriber_undeclare(subscriber)
    end

    test "session_declare_subscriber/4 rejects unsupported options", %{
      listen_session: listen_session,
      connect_session: _connect_session
    } do
      assert_raise ArgumentError, fn ->
        Nif.session_declare_subscriber(
          listen_session,
          "zenohex_pico/test",
          self(),
          allowed_origin: :any
        )
      end
    end
  end
end
