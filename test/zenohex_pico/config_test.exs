defmodule ZenohexPico.ConfigTest do
  use ExUnit.Case

  alias ZenohexPico.Config

  @z_config_mode_key 0x40
  @z_config_mode_peer "peer"

  test "delegates configuration operations" do
    assert {:ok, config} = Config.default()
    assert {:ok, "client"} = Config.get(config, @z_config_mode_key)

    assert {:ok, updated_config} = Config.insert(config, @z_config_mode_key, @z_config_mode_peer)
    assert {:ok, "client"} = Config.get(config, @z_config_mode_key)
    assert {:ok, "peer"} = Config.get(updated_config, @z_config_mode_key)
  end
end
