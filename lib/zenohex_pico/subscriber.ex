defmodule ZenohexPico.Subscriber do
  @moduledoc """
  Interface for managing Zenoh subscribers.

  This module provides functions to undeclare subscribers, which stops
  message receiving and releases associated native resources.

  Subscribers are created with `ZenohexPico.Session.declare_subscriber/4`.
  """

  @typedoc """
  An opaque native Zenoh Pico subscriber.
  """
  @type t :: reference()

  @doc """
  Undeclares a subscriber and stops message delivery.

  Stops receiving messages and releases resources associated with the subscriber.
  """
  @spec undeclare(t()) :: :ok | {:error, reason :: term()}
  defdelegate undeclare(subscriber), to: ZenohexPico.Nif, as: :subscriber_undeclare
end
