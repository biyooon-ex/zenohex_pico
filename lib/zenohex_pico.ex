defmodule ZenohexPico do
  def start do
    {:ok, config} = ZenohexPico.Nif.config_default()
    IO.puts("ZenohexPico! #{inspect(ZenohexPico.Nif.session_open(config))}")
    :ok
  end
end
