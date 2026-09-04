defmodule ZenohexPico.SubscriberTest do
  use ExUnit.Case

  alias ZenohexPico.{Config, Session, Subscriber}

  test "receives samples and undeclares subscribers" do
    assert {:ok, listen_session} = Session.open(peer_config(:listen, 7447))
    on_exit(fn -> Session.close(listen_session) end)

    assert {:ok, subscriber} =
             Session.declare_subscriber(listen_session, "zenohex_pico/subscriber")

    on_exit(fn -> Subscriber.undeclare(subscriber) end)

    assert {:ok, connect_session} = Session.open(peer_config(:connect, 7447))
    on_exit(fn -> Session.close(connect_session) end)

    assert :ok = Session.put(connect_session, "zenohex_pico/subscriber", "payload")

    assert_receive %ZenohexPico.Sample{
      attachment: "",
      congestion_control: :drop,
      encoding: "zenoh/bytes",
      express: false,
      key_expr: "zenohex_pico/subscriber",
      kind: :put,
      payload: "payload",
      priority: :data,
      timestamp: nil
    }

    assert :ok = Subscriber.undeclare(subscriber)
    assert {:error, :subscriber_undeclared} = Subscriber.undeclare(subscriber)
  end

  defp peer_config(endpoint_kind, port) do
    {:ok, config} = Config.default()
    {:ok, config} = Config.insert(config, :mode, "peer")
    {:ok, config} = Config.insert(config, endpoint_kind, "tcp/127.0.0.1:#{port}")
    config
  end
end
