defmodule ZenohexPico.MixProject do
  use Mix.Project

  def project do
    [
      app: :zenohex_pico,
      version: "0.1.0",
      elixir: "~> 1.18",
      start_permanent: Mix.env() == :prod
    ] ++ project(Mix.target())
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp project(:avm_esp32) do
    [
      deps: [
        {:exatomvm,
         git: "https://github.com/atomvm/exatomvm.git",
         ref: "ff7daf7e83a4e86fbf078730b6c49045a99de9f8"}
      ],
      atomvm: [start: ZenohexPico, flash_offset: 0x250000],
      aliases: [
        flash: [
          "atomvm.packbeam",
          "atomvm.esp32.flash --port /dev/ttyACM0 --baud 921600"
        ]
      ]
    ]
  end

  defp project(_) do
    [
      deps: [
        {:elixir_make, "~> 0.4", runtime: false}
      ],
      compilers: [:elixir_make] ++ Mix.compilers(),
      make_cwd: "zxp_unix",
      make_clean: ["clean"],
      aliases: [
        format: [
          fn _ ->
            if not is_nil(System.find_executable("clang-format")) do
              files = Path.wildcard("zxp_unix/**/*.{c,h}")
              System.cmd("clang-format", ["-i" | files])
            end
          end,
          "format"
        ]
      ]
    ]
  end
end
