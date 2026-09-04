if Mix.target() == :avm_esp32 do
  defmodule ZenohexPico.AvmEsp32Test do
    @moduledoc false

    def start do
      IO.puts("ZenohexPico.AvmEsp32Test: start")
      test_config_default_and_insert()
      test_config_rejects_nul_value()
      IO.puts("ZenohexPico.AvmEsp32Test: ok")
      :ok
    end

    defp test_config_default_and_insert do
      {:ok, config} = ZenohexPico.Config.default()
      {:ok, "client"} = ZenohexPico.Config.get(config, :mode)
      {:error, :not_found} = ZenohexPico.Config.get(config, :connect)

      {:ok, updated_config} = ZenohexPico.Config.insert(config, :connect, "tcp/127.0.0.1:7447")
      {:ok, "tcp/127.0.0.1:7447"} = ZenohexPico.Config.get(updated_config, :connect)
      {:error, :not_found} = ZenohexPico.Config.get(config, :connect)
    end

    defp test_config_rejects_nul_value do
      {:ok, config} = ZenohexPico.Config.default()

      try do
        ZenohexPico.Config.insert(config, :connect, <<"tcp/127.0.0.1", 0, ":7447">>)
        raise "config accepted an embedded NUL byte"
      rescue
        ArgumentError -> :ok
      end
    end
  end
end
