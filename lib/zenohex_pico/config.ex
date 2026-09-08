defmodule ZenohexPico.Config do
  @moduledoc """
  Utility functions for working with Zenoh session configurations.

  This module provides helpers to obtain default configuration
  and retrieve or update individual config keys.
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
      iex> {:ok, config} = ZenohexPico.Config.insert(config, :connect, "tcp/127.0.0.1:7447")
      iex> {:ok, _value} = ZenohexPico.Config.get(config, :connect)
      {:ok, "tcp/127.0.0.1:7447"}
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
