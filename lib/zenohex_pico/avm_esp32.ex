if Mix.target() == :avm_esp32 do
  defmodule ZenohexPico.AvmEsp32 do
    @moduledoc false

    def start do
      # Call these once to configure your Wi-Fi station.
      # :esp.nvs_put_binary(:atomvm, :sta_ssid, "SSID")
      # :esp.nvs_put_binary(:atomvm, :sta_psk, "Password")

      {:ok, ssid} = :esp.nvs_fetch_binary(:atomvm, :sta_ssid)
      {:ok, psk} = :esp.nvs_fetch_binary(:atomvm, :sta_psk)

      connect_wifi(ssid, psk)

      {:ok, config} = ZenohexPico.Nif.config_default()
      IO.puts("ZenohexPico.AvmEsp32! #{inspect(ZenohexPico.Nif.session_open(config))}")
      :ok
    end

    defp connect_wifi(ssid, psk) do
      me = self()

      {:ok, _} =
        :network.start_link(
          sta: [
            ssid: ssid,
            psk: psk,
            connected: fn -> IO.puts("WiFi connected!") end,
            got_ip: fn _info -> send(me, :got_ip) end,
            disconnected: fn -> IO.puts("WiFi disconnected") end
          ]
        )

      IO.puts("Waiting for WiFi...")

      receive do
        :got_ip -> :ok
      end
    end
  end
end
