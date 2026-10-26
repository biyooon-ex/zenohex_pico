defmodule Mix.Tasks.Compile.ZenohPico do
  use Mix.Task.Compiler

  @moduledoc "Ensures the zenoh-pico submodule is available before native compilation."

  @impl Mix.Task.Compiler
  def run(_args) do
    if File.exists?("zenoh-pico/CMakeLists.txt") do
      {:noop, []}
    else
      initialize_submodule()
      {:ok, []}
    end
  end

  defp initialize_submodule do
    cond do
      ci?() ->
        Mix.shell().info("CI detected; initializing zenoh-pico submodule.")
        initialize_submodule_impl()

      Mix.shell().yes?("zenoh-pico submodule is missing. Initialize it now?", default: :no) ->
        initialize_submodule_impl()

      true ->
        Mix.raise("""
        zenoh-pico submodule is missing. Run at the repository root:
          git submodule update --init --recursive
        """)
    end
  end

  defp initialize_submodule_impl do
    case System.cmd("git", ["submodule", "update", "--init", "--recursive"],
           stderr_to_stdout: true
         ) do
      {output, 0} ->
        IO.write(output)

      {output, _status} ->
        Mix.raise("Failed to initialize zenoh-pico submodule:\n#{output}")
    end
  end

  defp ci? do
    case System.get_env("ZENOHEX_PICO_CI") do
      nil -> false
      value -> String.downcase(value) not in ["", "false", "0"]
    end
  end
end
