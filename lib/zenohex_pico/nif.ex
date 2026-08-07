defmodule ZenohexPico.Nif do
  defp err, do: :erlang.nif_error(:nif_not_loaded)

  @spec config_default() :: {:ok, reference()}
  def config_default, do: err()

  @spec session_open(reference()) :: {:ok, reference()}
  def session_open(_config), do: err()
end
