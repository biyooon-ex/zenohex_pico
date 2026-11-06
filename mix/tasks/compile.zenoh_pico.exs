defmodule Mix.Tasks.Compile.ZenohPico do
  @moduledoc "Ensures zenoh-pico is available before native compilation."

  @zenoh_pico_git_url "https://github.com/eclipse-zenoh/zenoh-pico.git"
  @zenoh_pico_version "1.10.2"

  use Mix.Task.Compiler

  def zenoh_pico_version, do: @zenoh_pico_version

  @impl Mix.Task.Compiler
  def run(_args) do
    if File.exists?("zenoh-pico/CMakeLists.txt") do
      {:noop, []}
    else
      git_clone()
      {:ok, []}
    end
  end

  defp git_clone do
    cond do
      auto_git_clone?() ->
        Mix.shell().info("Automatic git clone enabled.")
        git_clone_impl()

      Mix.shell().yes?("zenoh-pico is missing. Cloning it now?", default: :no) ->
        git_clone_impl()

      true ->
        Mix.raise("""
        zenoh-pico is missing. Run at the repository root:
          git clone --branch #{@zenoh_pico_version} #{@zenoh_pico_git_url}
        """)
    end
  end

  defp git_clone_impl do
    case System.cmd("git", ["clone", "--branch", @zenoh_pico_version, @zenoh_pico_git_url],
           stderr_to_stdout: true
         ) do
      {output, 0} ->
        IO.write(output)

      {output, _status} ->
        Mix.raise("Failed to git clone zenoh-pico:\n#{output}")
    end
  end

  defp auto_git_clone? do
    case System.get_env("ZENOHEX_PICO_AUTO_GIT_CLONE") do
      nil -> false
      value -> String.downcase(value) not in ["", "false", "0"]
    end
  end
end
