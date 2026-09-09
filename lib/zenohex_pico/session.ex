defmodule ZenohexPico.Session do
  @moduledoc """
  Interface for managing Zenoh sessions.

  This module provides functions to open and close Zenoh sessions, publish
  and retrieve data, and declare subscribers.

  Typical usage starts with `open/1` to create a session,
  followed by operations such as `put/4`, `get/4`, or `declare_subscriber/4`.

  ## Examples

      iex> {:ok, config} = ZenohexPico.Config.default()
      iex> {:ok, session} = ZenohexPico.Session.open(config)
      iex> ZenohexPico.Session.put(session, "key/expr", "payload")
      iex> ZenohexPico.Session.close(session)

  This module serves as the main entry point for using Zenoh in Elixir.
  """

  @typedoc """
  An opaque native Zenoh session.
  """
  @type t :: reference()

  @typedoc """
  A Zenoh timestamp formatted as `YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ/<32 lowercase hexadecimal digits>`.
  """
  @type zenoh_timestamp_string :: String.t()

  @typedoc """
  The delivery behavior used when congestion occurs.
  """
  @type congestion_control :: :drop | :block

  @typedoc """
  The priority assigned to a Zenoh message.
  """
  @type priority ::
          :real_time
          | :interactive_high
          | :interactive_low
          | :data_high
          | :data
          | :data_low
          | :background

  @typedoc """
  The set of queryable entities that should receive a query.
  """
  @type query_target :: :best_matching | :all | :all_complete

  @typedoc """
  The reply consolidation policy for a query.
  """
  @type query_consolidation :: :auto | :none | :monotonic | :latest

  @typedoc """
  The key expression used for reply acceptance.
  """
  @type reply_key_expr :: :matching_query | :any

  @typedoc """
  Options accepted by `put/4`.
  """
  @type put_opts :: [
          encoding: String.t(),
          attachment: binary(),
          congestion_control: congestion_control(),
          priority: priority(),
          express: boolean(),
          timestamp: zenoh_timestamp_string()
        ]

  @typedoc """
  Options accepted by `get/4`.
  """
  @type get_opts :: [
          accept_replies: reply_key_expr(),
          attachment: binary(),
          congestion_control: congestion_control(),
          consolidation: query_consolidation(),
          encoding: String.t(),
          express: boolean(),
          payload: binary(),
          priority: priority(),
          target: query_target(),
          query_timeout: non_neg_integer()
        ]

  @typedoc """
  Options accepted by `declare_subscriber/4`.
  """
  @type subscriber_opts :: []

  @doc """
  Opens a session with the given configuration.

  ## Parameters

  - `config` : The configuration used to open the session.

  > ### Important {: .info}
  >
  > The returned `session` must be held for as long as the session is in use.
  > If it is not held and gets garbage-collected by the BEAM,
  > the underlying session in C will be automatically dropped and closed.
  """
  @spec open(ZenohexPico.Config.t()) :: {:ok, t()} | {:error, reason :: term()}
  defdelegate open(config), to: ZenohexPico.Nif, as: :session_open

  @doc """
  Closes a session.

  After calling this function, the `session` must not be used again.

  ## Parameters

  - `session` : The session returned by `open/1`.
  """
  @spec close(t()) :: :ok | {:error, :session_closed} | {:error, reason :: term()}
  defdelegate close(session), to: ZenohexPico.Nif, as: :session_close

  @doc """
  Publishes a binary payload to the given `key_expr`

  This function sends a value (as a binary) to the specified key expression.

  ## Parameters

  - `session` : The session identifier returned by or `open/1`.
  - `key_expr` : The key expression to publish to.
  - `payload` : The value to publish, as a binary.
  - `opts` : Options for the publish operation.

  ## Examples

      iex> {:ok, config} = ZenohexPico.Config.default()
      iex> {:ok, session} = ZenohexPico.Session.open(config)
      iex> ZenohexPico.Session.put(session, "key/expr", "payload")
      :ok
  """
  @spec put(t(), String.t(), binary(), put_opts()) :: :ok | {:error, reason :: term()}
  defdelegate put(session, key_expr, payload, opts \\ []),
    to: ZenohexPico.Nif,
    as: :session_put

  @doc """
  Query data with the given `selector`.

  ## Parameters

  - `session` : The session identifier returned by `open/1`.
  - `selector` : The selector to query.
  - `timeout` : Timeout in milliseconds to wait for query replies.
  - `opts` : Options for the get operation.

  ## Examples

      iex> {:ok, config} = ZenohexPico.Config.default()
      iex> {:ok, session} = ZenohexPico.Session.open(config)
      iex> ZenohexPico.Session.get(session, "key/expr", 100)
      {:ok, [%ZenohexPico.Sample{}]}
  """
  @spec get(t(), String.t(), non_neg_integer(), get_opts()) ::
          {:ok, [ZenohexPico.Sample.t() | ZenohexPico.Query.ReplyError.t()]}
          | {:error, :timeout}
          | {:error, reason :: term()}
  defdelegate get(session, selector, timeout, opts \\ []),
    to: ZenohexPico.Nif,
    as: :session_get

  @doc """
  Declares a subscriber for the specified `key_expr`.

  ## Parameters

    - `session`: Identifier of the session returned by `open/1`.
    - `key_expr`: Key expression to subscribe to.
    - `pid`: Process to receive subscription messages. Defaults to the calling process.
      - Messages are delivered as `ZenohexPico.Sample`.
    - `opts`: Options for configuring the subscriber.

  > ### Important {: .info}
  >
  > The returned `subscriber` must be held for as long as the subscriber is in use.
  > If it is not held and gets garbage-collected by the BEAM,
  > the underlying subscriber will be automatically dropped.
  """
  @spec declare_subscriber(t(), String.t(), pid(), subscriber_opts()) ::
          {:ok, ZenohexPico.Subscriber.t()} | {:error, reason :: term()}
  defdelegate declare_subscriber(session, key_expr, pid \\ self(), opts \\ []),
    to: ZenohexPico.Nif,
    as: :session_declare_subscriber
end
