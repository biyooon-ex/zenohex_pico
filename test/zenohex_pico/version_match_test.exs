defmodule ZenohexPico.VersionMatchTest do
  use ExUnit.Case

  @mise_path "mise.toml"
  @ci_path ".github/workflows/ci.yaml"
  @readme_path "README.md"
  @esp_idf_cmake_path "zxp_avm_esp32/zenoh_pico_component/CMakeLists.txt"
  @esp_idf_readme_path "zxp_avm_esp32/zenoh_pico_component/README.md"

  test "mise and CI use identical Elixir and Erlang versions" do
    mise = File.read!(@mise_path)
    ci = File.read!(@ci_path)

    assert version(mise, ~r/^elixir = "([^-]+)-otp-\d+"$/m) ==
             version(ci, ~r/^\s*elixir_version:\s*([\d.]+)$/m)

    assert version(mise, ~r/^erlang = "([\d.]+)"$/m) ==
             version(ci, ~r/^\s*-?\s*otp_version:\s*([\d.]+)$/m)
  end

  test "zenoh-pico versions in documentation and clone instructions match the Mix task" do
    zenoh_pico_version = Mix.Tasks.Compile.ZenohPico.zenoh_pico_version()
    ci = File.read!(@ci_path)
    readme = File.read!(@readme_path)
    esp_idf_cmake = File.read!(@esp_idf_cmake_path)
    esp_idf_readme = File.read!(@esp_idf_readme_path)
    clone_version_pattern = ~r/git clone --branch\s+([^\s]+)/

    assert zenoh_pico_version == version(ci, clone_version_pattern)
    assert zenoh_pico_version == version(readme, ~r/uses version ([^\s]+) of Zenoh/)
    assert zenoh_pico_version == version(esp_idf_cmake, clone_version_pattern)
    assert zenoh_pico_version == version(esp_idf_readme, clone_version_pattern)
  end

  defp version(contents, pattern) do
    [_, version] = Regex.run(pattern, contents)
    version
  end
end
