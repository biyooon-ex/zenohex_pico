if Mix.target() == :avm_esp32 do
  defmodule ZenohexPico.AvmEsp32 do
    @moduledoc false

    @compile {:no_warn_undefined, [:esp]}

    def start do
      # Call these once to configure your Wi-Fi station.
      # :esp.nvs_put_binary(:atomvm, :sta_ssid, "SSID")
      # :esp.nvs_put_binary(:atomvm, :sta_psk, "Password")

      {:ok, ssid} = :esp.nvs_fetch_binary(:atomvm, :sta_ssid)
      {:ok, psk} = :esp.nvs_fetch_binary(:atomvm, :sta_psk)

      ZenohexPico.AvmEsp32.Wifi.connect(ssid, psk)

      :ok
    end
  end
end
