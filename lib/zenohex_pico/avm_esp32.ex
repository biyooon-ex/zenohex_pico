if Mix.target() == :avm_esp32 do
  defmodule ZenohexPico.AvmEsp32 do
    @moduledoc false

    def start do
      {:ok, config} = ZenohexPico.Nif.config_default()
      IO.puts("ZenohexPico.AvmEsp32! #{inspect(ZenohexPico.Nif.session_open(config))}")
      :ok
    end
  end
end
