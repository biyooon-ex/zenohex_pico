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
    if Mix.shell().yes?("zenoh-pico submodule is missing. Initialize it now?", default: :no) do
      case System.cmd("git", ["submodule", "update", "--init", "--recursive"],
             stderr_to_stdout: true
           ) do
        {output, 0} ->
          IO.write(output)

        {output, _status} ->
          Mix.raise("Failed to initialize zenoh-pico submodule:\n#{output}")
      end
    else
      Mix.raise("""
      zenoh-pico submodule is missing. Run at the repository root:
        git submodule update --init --recursive
      """)
    end
  end
end
