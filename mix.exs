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
      atomvm: [
        start:
          case Mix.env() do
            :test -> ZenohexPico.AvmEsp32.Test
            _ -> ZenohexPico.AvmEsp32
          end,
        flash_offset: 0x250000
      ],
      aliases: [
        flash: [
          "atomvm.packbeam",
          "atomvm.esp32.flash --port /dev/ttyACM0 --baud 921600"
        ],
        format: [
          fn _ ->
            if not is_nil(System.find_executable("clang-format")) do
              files = Path.wildcard("zxp_avm_esp32/**/*.{c,h}")
              System.cmd("clang-format", ["-i" | files])
            end
          end,
          "format"
        ]
      ]
    ]
  end

  defp project(_) do
    [
      deps: [
        {:ex_doc, "~> 0.34", only: :dev, runtime: false, warn_if_outdated: true},
        {:elixir_make, "~> 0.4", runtime: false},
        {:mix_test_watch, "~> 1.2", only: [:dev, :test], runtime: false},
        {:credo, "~> 1.7", only: [:dev, :test], runtime: false},
        {:dialyxir, "~> 1.4", only: [:dev, :test], runtime: false}
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
      ],
      dialyzer: [
        plt_file: {:no_warn, "priv/plts/project.plt"},
        plt_core_path: "priv/plts/core.plt"
      ],
      docs: [
        extras: ["README.md", "LICENSE"],
        main: "readme"
      ]
    ]
  end
end
