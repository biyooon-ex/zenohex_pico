defmodule ZenohexPico.SubscriberTest do
  use ExUnit.Case

  alias ZenohexPico.{Config, Session, Subscriber}

  test "undeclares subscribers declared with the calling process by default" do
    config = peer_config()
    assert {:ok, session} = Session.open(config)
    on_exit(fn -> Session.close(session) end)

    assert {:ok, subscriber} = Session.declare_subscriber(session, "zenohex_pico/subscriber")
    assert :ok = Subscriber.undeclare(subscriber)
    assert {:error, :subscriber_undeclared} = Subscriber.undeclare(subscriber)
  end

  defp peer_config do
    {:ok, config} = Config.default()
    {:ok, config} = Config.insert(config, :mode, "peer")
    {:ok, config} = Config.insert(config, :listen, "tcp/0.0.0.0:7449")
    config
  end
end
