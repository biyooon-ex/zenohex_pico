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

  def test_raise, do: err()

  @spec config_default() :: {:ok, reference()}
  def config_default, do: err()

  @spec config_get(reference(), integer()) ::
          {:ok, value :: String.t()} | {:error, reason :: term()}
  def config_get(_config, _key), do: err()

  @spec config_insert(reference(), integer(), String.t()) ::
          {:ok, reference()} | {:error, reason :: term()}
  def config_insert(_config, _key, _value), do: err()

  @spec session_open(reference()) :: {:ok, reference()} | {:error, reason :: term()}
  def session_open(_config), do: err()

  @spec session_close(reference()) :: :ok | {:error, reason :: term()}
  def session_close(_session), do: err()
end
