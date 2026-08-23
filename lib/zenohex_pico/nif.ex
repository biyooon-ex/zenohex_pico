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

  @type session_put_option ::
          {:encoding, String.t()}
          | {:attachment, binary()}
          | {:congestion_control, :block | :drop}
          | {:priority,
             :real_time
             | :interactive_high
             | :interactive_low
             | :data_high
             | :data
             | :data_low
             | :background}
          | {:express, boolean()}
          | {:timestamp, String.t()}

  @doc """
  Publishes a payload with optional Zenoh metadata.

  `:timestamp` must use Zenohex's UTC timestamp form:
  `YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ/<32 lowercase hexadecimal digits>`.
  """
  @spec session_put(reference(), String.t(), binary(), [session_put_option()]) ::
          :ok | {:error, reason :: term()}
  def session_put(_session, _keyexpr, _payload, _opts \\ []), do: err()

  @spec session_get(reference(), String.t(), non_neg_integer(), keyword()) ::
          {:ok, [ZenohexPico.Sample.t() | ZenohexPico.Query.ReplyError.t()]}
          | {:error, :timeout}
          | {:error, reason :: term()}
  def session_get(_session, _selector, _timeout, _opts \\ []), do: err()
end
