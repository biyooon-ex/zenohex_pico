defmodule ZenohexPico do
  @moduledoc """
  ZenohexPico is a thin Elixir wrapper around Zenoh-Pico, implemented using C.

  - Zenoh:
    - https://zenoh.io/
    - https://github.com/eclipse-zenoh/zenoh-pico

  For reusable connections, use `ZenohexPico.Session` directly.
  """

  @doc """
  Publishes a `payload` to the specified `key_expr`.

  Internally opens a session, performs the publish, and ensures the session is closed.

  ## Parameters

  - `config` : The configuration used to open the session.
  - `key_expr` : The key expression to publish to.
  - `payload` : The binary payload to publish.
  - `opts` : Additional options. See `ZenohexPico.Session.put/4` for details.
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
  Query data with the given `selector`.

  Internally opens a session, performs the query, and ensures the session is closed.

  ## Parameters

  - `config` : The configuration used to open the session.
  - `selector` : The selector to query.
  - `timeout` : Timeout in milliseconds to wait for query replies.
  - `opts` : Additional options. See `ZenohexPico.Session.get/4` for details.
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
