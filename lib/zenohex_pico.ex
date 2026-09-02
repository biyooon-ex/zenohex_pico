defmodule ZenohexPico do
  @moduledoc """
  Convenience APIs for one-shot Zenoh Pico operations.

  A Pico configuration must be supplied explicitly. For reusable connections,
  use `ZenohexPico.Session` directly.
  """

  @doc """
  Publishes a payload using a session opened from `config`.

  The temporary session is closed after the publish attempt, including when the
  publish returns an error.
  """
  @spec put(ZenohexPico.Config.t(), String.t(), binary(), ZenohexPico.Session.put_opts()) ::
          :ok | {:error, reason :: term()}
  def put(config, key_expr, payload, opts \\ []) do
    with {:ok, session} <- ZenohexPico.Session.open(config) do
      try do
        ZenohexPico.Session.put(session, key_expr, payload, opts)
      after
        ZenohexPico.Session.close(session)
      end
    end
  end

  @doc """
  Queries a selector using a session opened from `config`.

  The temporary session is closed after the query attempt, including when the
  query returns an error.
  """
  @spec get(
          ZenohexPico.Config.t(),
          String.t(),
          non_neg_integer(),
          ZenohexPico.Session.get_opts()
        ) ::
          {:ok, [ZenohexPico.Sample.t() | ZenohexPico.Query.ReplyError.t()]}
          | {:error, :timeout}
          | {:error, reason :: term()}
  def get(config, selector, timeout, opts \\ []) do
    with {:ok, session} <- ZenohexPico.Session.open(config) do
      try do
        ZenohexPico.Session.get(session, selector, timeout, opts)
      after
        ZenohexPico.Session.close(session)
      end
    end
  end

  if Mix.target() == :avm_esp32 do
    def start do
      {:ok, config} = ZenohexPico.Nif.config_default()
      IO.puts("ZenohexPico! #{inspect(ZenohexPico.Nif.session_open(config))}")
      :ok
    end
  end
end
