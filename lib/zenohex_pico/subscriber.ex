defmodule ZenohexPico.Subscriber do
  @moduledoc """
  Interface for releasing Zenoh Pico subscribers.

  Subscribers are created with `ZenohexPico.Session.declare_subscriber/4`.
  """

  @typedoc """
  An opaque native Zenoh Pico subscriber.
  """
  @type id :: reference()

  @doc """
  Undeclares a subscriber and stops message delivery.
  """
  @spec undeclare(id()) :: :ok | {:error, reason :: term()}
  defdelegate undeclare(subscriber), to: ZenohexPico.Nif, as: :subscriber_undeclare
end
