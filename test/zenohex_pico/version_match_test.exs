defmodule ZenohexPico.VersionMatchTest do
  use ExUnit.Case

  @mise_path "mise.toml"
  @ci_path ".github/workflows/ci.yaml"

  test "mise and CI use identical Elixir and Erlang versions" do
    mise = File.read!(@mise_path)
    ci = File.read!(@ci_path)

    assert version(mise, ~r/^elixir = "([^-]+)-otp-\d+"$/m) ==
             version(ci, ~r/^\s*elixir_version:\s*([\d.]+)$/m)

    assert version(mise, ~r/^erlang = "([\d.]+)"$/m) ==
             version(ci, ~r/^\s*-?\s*otp_version:\s*([\d.]+)$/m)
  end

  defp version(contents, pattern) do
    [_, version] = Regex.run(pattern, contents)
    version
  end
end
