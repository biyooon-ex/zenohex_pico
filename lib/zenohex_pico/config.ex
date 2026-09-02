defmodule ZenohexPico.Config do
  @moduledoc """
  Functions for creating and updating Zenoh Pico configurations.

  Configurations are opaque native references. Unlike `Zenohex.Config`, Pico
  configurations use a fixed set of Zenoh Pico configuration keys rather than
  JSON paths.
  """

  @keys %{
    mode: 0x40,
    connect: 0x41,
    listen: 0x42,
    user: 0x43,
    password: 0x44,
    multicast_scouting: 0x45,
    multicast_locator: 0x46,
    scouting_timeout: 0x47,
    scouting_what: 0x48,
    session_zid: 0x49,
    add_timestamp: 0x4A,
    tls_root_ca_certificate: 0x4B,
    tls_root_ca_certificate_base64: 0x4C,
    tls_listen_private_key: 0x4D,
    tls_listen_private_key_base64: 0x4E,
    tls_listen_certificate: 0x4F,
    tls_listen_certificate_base64: 0x50,
    tls_enable_mtls: 0x51,
    tls_connect_private_key: 0x52,
    tls_connect_private_key_base64: 0x53,
    tls_connect_certificate: 0x54,
    tls_connect_certificate_base64: 0x55,
    tls_verify_name_on_connect: 0x56
  }

  @typedoc """
  An opaque native Zenoh Pico configuration.
  """
  @type t :: reference()

  @typedoc """
  A supported Zenoh Pico runtime configuration key.

  Connection and listen timeout keys are excluded because Zenoh Pico exposes
  them only when built with its unstable API feature.
  """
  @type key ::
          :mode
          | :connect
          | :listen
          | :user
          | :password
          | :multicast_scouting
          | :multicast_locator
          | :scouting_timeout
          | :scouting_what
          | :session_zid
          | :add_timestamp
          | :tls_root_ca_certificate
          | :tls_root_ca_certificate_base64
          | :tls_listen_private_key
          | :tls_listen_private_key_base64
          | :tls_listen_certificate
          | :tls_listen_certificate_base64
          | :tls_enable_mtls
          | :tls_connect_private_key
          | :tls_connect_private_key_base64
          | :tls_connect_certificate
          | :tls_connect_certificate_base64
          | :tls_verify_name_on_connect

  @doc """
  Creates the default Zenoh Pico configuration.
  """
  @spec default() :: {:ok, t()} | {:error, reason :: term()}
  defdelegate default(), to: ZenohexPico.Nif, as: :config_default

  @doc """
  Gets the value for a Zenoh Pico configuration key.

  Raises `ArgumentError` when `key` is not supported by this build-independent
  API.
  """
  @spec get(t(), key()) :: {:ok, String.t()} | {:error, reason :: term()}
  def get(config, key), do: ZenohexPico.Nif.config_get(config, key_id!(key))

  @doc """
  Returns a new configuration with `value` assigned to a Zenoh Pico configuration key.

  The original configuration remains unchanged.

  Raises `ArgumentError` when `key` is not supported by this build-independent
  API.
  """
  @spec insert(t(), key(), String.t()) :: {:ok, t()} | {:error, reason :: term()}
  def insert(config, key, value), do: ZenohexPico.Nif.config_insert(config, key_id!(key), value)

  defp key_id!(key) do
    case @keys do
      %{^key => key_id} -> key_id
      _ -> raise ArgumentError, "unsupported Zenoh Pico configuration key: #{inspect(key)}"
    end
  end
end
