defmodule ZenohexPico.ConfigTest do
  use ExUnit.Case

  alias ZenohexPico.Config

  test "delegates configuration operations" do
    assert {:ok, config} = Config.default()
    assert {:ok, "client"} = Config.get(config, :mode)

    assert {:ok, updated_config} = Config.insert(config, :mode, "peer")
    assert {:ok, "client"} = Config.get(config, :mode)
    assert {:ok, "peer"} = Config.get(updated_config, :mode)
  end

  test "rejects unsupported configuration keys" do
    {:ok, config} = Config.default()

    assert_raise ArgumentError, ~r/unsupported Zenoh Pico configuration key/, fn ->
      Config.get(config, :unknown)
    end
  end
end
