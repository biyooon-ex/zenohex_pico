defmodule ZenohexPico do
  def start do
    IO.puts("ZenohexPico! #{inspect(ZenohexPico.Nif.config_default())}")
    :ok
  end
end
