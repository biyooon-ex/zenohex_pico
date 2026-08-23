defmodule ZenohexPico.NifTest do
  use ExUnit.Case

  alias ZenohexPico.Nif

  @z_config_mode_key 0x40
  @z_config_mode_client "client"
  @z_config_mode_peer "peer"
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
      assert {:ok, ^config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      assert {:ok, @z_config_mode_peer} = Nif.config_get(config, @z_config_mode_key)
    end
  end

  describe "session functions" do
    test "session_open/1" do
      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_listen_key, "tcp/0.0.0.0:7447")

      assert {:ok, session} = Nif.session_open(config)
      assert is_reference(session)
    end

    test "session_close/1" do
      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_listen_key, "tcp/0.0.0.0:7447")
      {:ok, session} = Nif.session_open(config)

      assert Nif.session_close(session) == :ok
      # Ensure closing an already-closed session is safe.
      assert Nif.session_close(session) == :ok
    end

    test "session_put/4 with options" do
      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_listen_key, "tcp/0.0.0.0:7447")
      {:ok, session} = Nif.session_open(config)

      assert :ok =
               Nif.session_put(session, "zenohex_pico/test", "payload",
                 attachment: "metadata",
                 congestion_control: :block,
                 encoding: "text/plain",
                 express: true,
                 priority: :data_high,
                 timestamp: "2025-07-16T01:34:56.871273403Z/208a2ec783ec4527a39cc1d5559c70e9"
               )

      assert :ok = Nif.session_close(session)
    end

    test "session_put/4 rejects malformed timestamp options" do
      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_listen_key, "tcp/0.0.0.0:7447")
      {:ok, session} = Nif.session_open(config)

      assert_raise ArgumentError, fn ->
        Nif.session_put(session, "zenohex_pico/test", "payload",
          timestamp: "2025-07-16T01:34:56Z/not-a-zenoh-id"
        )
      end

      assert :ok = Nif.session_close(session)
    end

    test "session_get/4 returns timeout when there are no queryables" do
      {:ok, config} = Nif.config_default()
      {:ok, config} = Nif.config_insert(config, @z_config_mode_key, @z_config_mode_peer)
      {:ok, config} = Nif.config_insert(config, @z_config_listen_key, "tcp/0.0.0.0:7447")
      {:ok, session} = Nif.session_open(config)

      assert Nif.session_get(session, "zenohex_pico/no_responder", 100, query_timeout: 10) ==
               {:error, :timeout}

      assert :ok = Nif.session_close(session)
    end
  end
end
