if Mix.target() == :avm_esp32 do
  defmodule ZenohexPico.AvmEsp32.Wifi do
    @moduledoc false

    @compile {:no_warn_undefined, [:network]}

    def connect(ssid, psk, pid \\ self()) do
      {:ok, _} =
        :network.start_link(
          sta: [
            ssid: ssid,
            psk: psk,
            connected: fn -> IO.puts("WiFi connected!") end,
            got_ip: fn _info -> send(pid, :got_ip) end,
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
