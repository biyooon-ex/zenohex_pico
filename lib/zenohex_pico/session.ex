defmodule ZenohexPico.Session do
  @moduledoc """
  Interface for managing Zenoh Pico sessions.

  Sessions are opened from a `ZenohexPico.Config` reference and support publishing,
  querying, and declaring subscribers. APIs unavailable in the Pico native layer are
  intentionally not provided.
  """

  @typedoc """
  An opaque native Zenoh Pico session.
  """
  @type id :: reference()

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
  Opens a Zenoh Pico session from an opaque Pico configuration.
  """
  @spec open(ZenohexPico.Config.t()) :: {:ok, id()} | {:error, reason :: term()}
  defdelegate open(config), to: ZenohexPico.Nif, as: :session_open

  @doc """
  Closes a Zenoh Pico session.
  """
  @spec close(id()) :: :ok | {:error, :session_closed} | {:error, reason :: term()}
  defdelegate close(session), to: ZenohexPico.Nif, as: :session_close

  @doc """
  Publishes a binary payload to a key expression.
  """
  @spec put(id(), String.t(), binary(), put_opts()) :: :ok | {:error, reason :: term()}
  defdelegate put(session, key_expr, payload, opts \\ []),
    to: ZenohexPico.Nif,
    as: :session_put

  @doc """
  Queries a selector and collects replies until the supplied timeout.
  """
  @spec get(id(), String.t(), non_neg_integer(), get_opts()) ::
          {:ok, [ZenohexPico.Sample.t() | ZenohexPico.Query.ReplyError.t()]}
          | {:error, :timeout}
          | {:error, reason :: term()}
  defdelegate get(session, selector, timeout, opts \\ []),
    to: ZenohexPico.Nif,
    as: :session_get

  @doc """
  Declares a subscriber that sends received samples to `pid`.

  When omitted, `pid` defaults to the calling process. Zenoh Pico currently accepts
  no subscriber options, so `opts` must be an empty list.
  """
  @spec declare_subscriber(id(), String.t(), pid(), subscriber_opts()) ::
          {:ok, ZenohexPico.Subscriber.id()} | {:error, reason :: term()}
  defdelegate declare_subscriber(session, key_expr, pid \\ self(), opts \\ []),
    to: ZenohexPico.Nif,
    as: :session_declare_subscriber
end
