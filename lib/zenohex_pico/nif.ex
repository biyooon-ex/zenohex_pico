defmodule ZenohexPico.Nif do
  defp err, do: :erlang.nif_error(:nif_not_loaded)
  def config_default, do: err()
end
