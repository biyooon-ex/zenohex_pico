defmodule ZenohexPico.Config do
  @moduledoc """
  Functions for creating and updating Zenoh Pico configurations.

  Configurations are opaque native references. Unlike `Zenohex.Config`, Pico
  configurations use integer Zenoh Pico configuration keys rather than JSON paths.
  """

  @typedoc """
  An opaque native Zenoh Pico configuration.
  """
  @type t :: reference()

  @doc """
  Creates the default Zenoh Pico configuration.
  """
  @spec default() :: {:ok, t()} | {:error, reason :: term()}
  defdelegate default(), to: ZenohexPico.Nif, as: :config_default

  @doc """
  Gets the value for an integer Zenoh Pico configuration key.
  """
  @spec get(t(), integer()) :: {:ok, String.t()} | {:error, reason :: term()}
  defdelegate get(config, key), to: ZenohexPico.Nif, as: :config_get

  @doc """
  Returns a new configuration with `value` assigned to an integer configuration key.

  The original configuration remains unchanged.
  """
  @spec insert(t(), integer(), String.t()) :: {:ok, t()} | {:error, reason :: term()}
  defdelegate insert(config, key, value), to: ZenohexPico.Nif, as: :config_insert
end
