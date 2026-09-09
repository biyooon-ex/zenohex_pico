defmodule ZenohexPico.Config do
  @moduledoc """
  Utility functions for working with Zenoh session configurations.

  This module provides helpers to obtain default configuration
  and retrieve or update individual config keys.

  ## Supported values

  Configuration values are strings. The following keys are supported:

  | Key | Accepted values | Config default | Examples |
  | --- | --- | --- | --- |
  | `:mode` | `"client"`, `"peer"` | `"client"` | `"peer"` |
  | `:connect` | One or more locators | None | `"tcp/192.168.1.10:7447"` |
  | `:listen` | A single locator; subsequent inserts replace it | None | `"tcp/0.0.0.0:7447"` |
  | `:multicast_scouting` | `"true"`, `"false"` | `"true"` | `"false"` |
  | `:multicast_locator` | A multicast UDP locator | `"udp/224.0.0.224:7446"` | `"udp/224.0.0.224:7446"` |
  | `:scouting_timeout` | Integer milliseconds | `"1000"` | `"5000"` |
  | `:scouting_what` | Bitmask from `0` to `7` | Not set (runtime: `"3"`) | `"7"` |
  | `:session_zid` | A 128-bit UUID | None | `"01234567-89ab-cdef-0123-456789abcdef"` |

  Multiple `:connect` locators are supported. Add each locator with a separate
  call to `insert/3`:

      {:ok, config} = ZenohexPico.Config.default()
      {:ok, config} = ZenohexPico.Config.insert(config, :connect, "tcp/192.168.1.10:7447")
      {:ok, config} = ZenohexPico.Config.insert(config, :connect, "tcp/192.168.1.20:7447")

  `get/2` returns only the most recently configured `:connect` locator.
  When opening a session, Zenoh Pico reads all configured `:connect` locators.
  In client mode, it tries them from most recently configured to least recently
  configured until one succeeds. In peer mode, it establishes a primary
  transport, then adds the remaining locators as peers.

  **For developers:** Only :connect supports multiple values. Zenoh Pico uses
  the internal _z_config_get_all function to retrieve all configured locators,
  but does not expose a public equivalent. Therefore, ZenohexPico does not provide
  an API for retrieving all configured :connect locators; get/2 returns only one locator.

  The `:scouting_what` bitmask uses `1` for routers, `2` for peers, and
  `4` for clients. Combine values to discover multiple entity types; for
  example, `"7"` discovers routers, peers, and clients.

  `:scouting_timeout` applies only in client mode while scouting for a router.
  `:session_zid` is optional; when it is unset, Zenoh Pico generates the
  session ID.

  Locator protocols depend on the Zenoh Pico features enabled at build time.
  Common examples are `tcp/<address>:<port>` and `udp/<address>:<port>`.
  """

  @keys %{
    mode: 0x40,
    connect: 0x41,
    listen: 0x42,
    # Currently unused by Zenoh Pico.
    # user: 0x43,
    # password: 0x44,
    multicast_scouting: 0x45,
    multicast_locator: 0x46,
    scouting_timeout: 0x47,
    scouting_what: 0x48,
    session_zid: 0x49
    # Currently unused by Zenoh Pico.
    # add_timestamp: 0x4A
    # TLS configuration requires Z_FEATURE_LINK_TLS=1.
    # tls_root_ca_certificate: 0x4B,
    # tls_root_ca_certificate_base64: 0x4C,
    # tls_listen_private_key: 0x4D,
    # tls_listen_private_key_base64: 0x4E,
    # tls_listen_certificate: 0x4F,
    # tls_listen_certificate_base64: 0x50,
    # tls_enable_mtls: 0x51,
    # tls_connect_private_key: 0x52,
    # tls_connect_private_key_base64: 0x53,
    # tls_connect_certificate: 0x54,
    # tls_connect_certificate_base64: 0x55,
    # tls_verify_name_on_connect: 0x56
  }

  @typedoc """
  An opaque native Zenoh configuration.
  """
  @type t :: reference()

  @typedoc """
  A supported Zenoh runtime configuration key.
  """
  @type key ::
          :mode
          | :connect
          | :listen
          # Currently unused by Zenoh Pico.
          # | :user
          # | :password
          | :multicast_scouting
          | :multicast_locator
          | :scouting_timeout
          | :scouting_what
          | :session_zid
  # Currently unused by Zenoh Pico.
  # | :add_timestamp
  # TLS configuration requires Z_FEATURE_LINK_TLS=1.
  # | :tls_root_ca_certificate
  # | :tls_root_ca_certificate_base64
  # | :tls_listen_private_key
  # | :tls_listen_private_key_base64
  # | :tls_listen_certificate
  # | :tls_listen_certificate_base64
  # | :tls_enable_mtls
  # | :tls_connect_private_key
  # | :tls_connect_private_key_base64
  # | :tls_connect_certificate
  # | :tls_connect_certificate_base64
  # | :tls_verify_name_on_connect

  @doc """
  Returns the default Zenoh configuration.
  """
  @spec default() :: {:ok, t()} | {:error, reason :: term()}
  defdelegate default(), to: ZenohexPico.Nif, as: :config_default

  @doc """
  Returns the value of the configuration at `key`.

  For `:connect`, which supports multiple values, returns only the most
  recently configured locator.

  Raises `ArgumentError` when `key` is not supported.

  ## Examples

      iex> {:ok, config} = ZenohexPico.Config.default()
      iex> {:ok, _value} = ZenohexPico.Config.get(config, :mode)
      {:ok, "client"}
      iex> {:error, _value} = ZenohexPico.Config.get(config, :connect)
      {:error, :not_found}
  """
  @spec get(t(), key()) :: {:ok, String.t()} | {:error, reason :: term()}
  def get(config, key), do: ZenohexPico.Nif.config_get(config, key_id!(key))

  @doc """
  Inserts a configuration value at key, returning the updated config.

  Raises `ArgumentError` when `key` is not supported.

  ## Examples

      iex> {:ok, config} = ZenohexPico.Config.default()
      iex> {:error, _value} = ZenohexPico.Config.get(config, :connect)
      {:error, :not_found}
      iex> {:ok, config} = ZenohexPico.Config.insert(config, :connect, "tcp/localhost:7447")
      iex> {:ok, _value} = ZenohexPico.Config.get(config, :connect)
      {:ok, "tcp/localhost:7447"}
  """
  @spec insert(t(), key(), String.t()) :: {:ok, t()} | {:error, reason :: term()}
  def insert(config, key, value), do: ZenohexPico.Nif.config_insert(config, key_id!(key), value)

  defp key_id!(key) do
    case @keys do
      %{^key => key_id} -> key_id
      _ -> raise ArgumentError, "unsupported Zenoh configuration key: #{inspect(key)}"
    end
  end
end
