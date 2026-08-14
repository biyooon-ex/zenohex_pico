defmodule ZenohexPico.NifTest do
  use ExUnit.Case

  alias ZenohexPico.Nif

  @z_config_mode_key 0x40
  @z_config_mode_client "client"
  @z_config_mode_peer "peer"

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
      assert {:error, reason} = Nif.session_open(config)
      assert reason =~ "_z_err_scout_no_results"
    end
  end
end
