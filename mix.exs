defmodule ZenohexPico.MixProject do
  use Mix.Project

  def project do
    [
      app: :zenohex_pico,
      version: "0.1.0",
      elixir: "~> 1.18",
      start_permanent: Mix.env() == :prod,
      deps: deps(),
      aliases: aliases(),
      atomvm: [
        start: ZenohexPico,
        flash_offset: 0x250000
      ]
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      extra_applications: [:logger]
    ]
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      {:exatomvm,
       git: "https://github.com/atomvm/exatomvm.git",
       ref: "ff7daf7e83a4e86fbf078730b6c49045a99de9f8"}
    ]
  end

  defp aliases do
    [
      flash: [
        "atomvm.packbeam",
        "atomvm.esp32.flash --port /dev/ttyACM0 --baud 921600"
      ]
    ]
  end
end
