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

  - `key_expr` : The key expression to publish to.
  - `payload` : The binary payload to publish.
  - `opts` : Additional options. Use `:config` to specify the configuration;
    otherwise, the default configuration is used. See `t:ZenohexPico.Session.put_opts/0`
    for the remaining options.
  """
  @spec put(
          String.t(),
          binary(),
          [{:config, ZenohexPico.Config.t()} | ZenohexPico.Session.put_opt()]
        ) ::
          :ok | {:error, reason :: term()}
  def put(key_expr, payload, opts \\ []) do
    config_result =
      case Keyword.fetch(opts, :config) do
        {:ok, config} -> {:ok, config}
        :error -> ZenohexPico.Config.default()
      end

    opts = Keyword.delete(opts, :config)

    with {:ok, config} <- config_result,
         {:ok, session} <- ZenohexPico.Session.open(config) do
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

  - `selector` : The selector to query.
  - `timeout` : Timeout in milliseconds to wait for query replies.
  - `opts` : Additional options. Use `:config` to specify the configuration;
    otherwise, the default configuration is used. See `t:ZenohexPico.Session.get_opts/0`
    for the remaining options.
  """
  @spec get(
          String.t(),
          pos_integer(),
          [{:config, ZenohexPico.Config.t()} | ZenohexPico.Session.get_opt()]
        ) ::
          {:ok, [ZenohexPico.Sample.t() | ZenohexPico.Query.ReplyError.t()]}
          | {:error, :timeout}
          | {:error, reason :: term()}
  def get(selector, timeout, opts \\ []) do
    config_result =
      case Keyword.fetch(opts, :config) do
        {:ok, config} -> {:ok, config}
        :error -> ZenohexPico.Config.default()
      end

    opts = Keyword.delete(opts, :config)

    with {:ok, config} <- config_result,
         {:ok, session} <- ZenohexPico.Session.open(config) do
      try do
        ZenohexPico.Session.get(session, selector, timeout, opts)
      after
        ZenohexPico.Session.close(session)
      end
    end
  end
end
