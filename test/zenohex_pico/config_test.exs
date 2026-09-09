defmodule ZenohexPico.ConfigTest do
  use ExUnit.Case
  doctest ZenohexPico.Config

  alias ZenohexPico.Config

  test "returns documented default configuration values" do
    assert {:ok, config} = Config.default()
    assert {:ok, "client"} = Config.get(config, :mode)
    assert {:ok, "true"} = Config.get(config, :multicast_scouting)
    assert {:ok, "udp/224.0.0.224:7446"} = Config.get(config, :multicast_locator)
    assert {:ok, "1000"} = Config.get(config, :scouting_timeout)
    assert {:error, :not_found} = Config.get(config, :scouting_what)
  end

  test "delegates configuration operations" do
    assert {:ok, config} = Config.default()
    assert {:ok, "client"} = Config.get(config, :mode)

    assert {:ok, updated_config} = Config.insert(config, :mode, "peer")
    assert {:ok, "client"} = Config.get(config, :mode)
    assert {:ok, "peer"} = Config.get(updated_config, :mode)
  end

  test "returns the most recently configured connect locator" do
    assert {:ok, config} = Config.default()
    assert {:ok, config} = Config.insert(config, :connect, "tcp/192.168.1.10:7447")
    assert {:ok, config} = Config.insert(config, :connect, "tcp/192.168.1.20:7447")

    assert {:ok, "tcp/192.168.1.20:7447"} = Config.get(config, :connect)
  end

  test "replaces the configured listen locator" do
    assert {:ok, config} = Config.default()
    assert {:ok, config} = Config.insert(config, :listen, "tcp/0.0.0.0:7447")
    assert {:ok, config} = Config.insert(config, :listen, "tcp/0.0.0.0:7448")

    assert {:ok, "tcp/0.0.0.0:7448"} = Config.get(config, :listen)
  end

  test "rejects unsupported configuration keys" do
    {:ok, config} = Config.default()

    assert_raise ArgumentError, ~r/unsupported Zenoh configuration key/, fn ->
      Config.get(config, :unknown)
    end
  end
end
