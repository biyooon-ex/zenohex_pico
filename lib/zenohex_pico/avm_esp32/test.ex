defmodule ZenohexPico.AvmEsp32.Test do
  @moduledoc false

  if Mix.target() == :avm_esp32 and Mix.env() == :test do
    @compile {:no_warn_undefined, [:esp]}
    @host_ipv4_address (
                         ipv4_address = ~S/\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/
                         {route, 0} = System.cmd("ip", ["-4", "route", "get", "1.1.1.1"])
                         [_, host] = Regex.run(~r/\bsrc\s+(#{ipv4_address})\b/, route)
                         host
                       )

    def start do
      IO.puts("#{inspect(__MODULE__)}: start")
      test_config_default_and_insert()
      test_config_rejects_nul_value()
      test_session_put()
      IO.puts("#{inspect(__MODULE__)}: ok")
      :ok
    end

    defp test_config_default_and_insert do
      {:ok, config} = ZenohexPico.Config.default()
      {:ok, "client"} = ZenohexPico.Config.get(config, :mode)
      {:error, :not_found} = ZenohexPico.Config.get(config, :connect)

      {:ok, updated_config} = ZenohexPico.Config.insert(config, :connect, "tcp/127.0.0.1:7447")
      {:ok, "tcp/127.0.0.1:7447"} = ZenohexPico.Config.get(updated_config, :connect)
      {:error, :not_found} = ZenohexPico.Config.get(config, :connect)
    end

    defp test_config_rejects_nul_value do
      {:ok, config} = ZenohexPico.Config.default()

      try do
        ZenohexPico.Config.insert(config, :connect, <<"tcp/127.0.0.1", 0, ":7447">>)
        raise "config accepted an embedded NUL byte"
      rescue
        ArgumentError -> :ok
      end
    end

    defp test_session_put do
      # Call these once to configure your Wi-Fi station.
      # :esp.nvs_put_binary(:atomvm, :sta_ssid, "SSID")
      # :esp.nvs_put_binary(:atomvm, :sta_psk, "Password")

      {:ok, ssid} = :esp.nvs_fetch_binary(:atomvm, :sta_ssid)
      {:ok, psk} = :esp.nvs_fetch_binary(:atomvm, :sta_psk)

      ZenohexPico.AvmEsp32.Wifi.connect(ssid, psk)

      {:ok, config} = ZenohexPico.Config.default()
      {:ok, config} = ZenohexPico.Config.insert(config, :mode, "peer")

      {:ok, config} =
        ZenohexPico.Config.insert(config, :connect, "tcp/#{@host_ipv4_address}:7447")

      for i <- 1..10 do
        :ok = ZenohexPico.put(config, "key/expr", "from M5STACK CORE S3, #{i}")
      end
    end
  else
    def start_subscriber_on_unix() do
      {:ok, config} = ZenohexPico.Config.default()
      {:ok, config} = ZenohexPico.Config.insert(config, :mode, "peer")
      {:ok, config} = ZenohexPico.Config.insert(config, :listen, "tcp/0.0.0.0:7447")
      {:ok, session} = ZenohexPico.Session.open(config)
      {:ok, subscriber} = ZenohexPico.Session.declare_subscriber(session, "key/expr")

      for _ <- 1..10 do
        receive do
          %ZenohexPico.Sample{} = sample ->
            IO.puts("#{inspect(sample)}")
        end
      end

      :ok = ZenohexPico.Subscriber.undeclare(subscriber)
      :ok = ZenohexPico.Session.close(session)
    end
  end
end
