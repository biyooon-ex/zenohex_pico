defmodule ZenohexPico.Nif do
  if Mix.target() != :avm_esp32 do
    @on_load :load_nif
    @nif_name ~c"zenohex_pico"
    def load_nif do
      priv_dir = :code.priv_dir(:zenohex_pico)
      nif_path = :filename.join(priv_dir, @nif_name)
      :erlang.load_nif(nif_path, 0)
    end
  end

  defp err, do: :erlang.nif_error(:nif_not_loaded)

  @spec config_default() :: {:ok, reference()}
  def config_default, do: err()

  @spec session_open(reference()) :: {:ok, reference()}
  def session_open(_config), do: err()
end
