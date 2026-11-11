# ZenohexPico

[![Hex version](https://img.shields.io/hexpm/v/zenohex_pico.svg "Hex version")](https://hex.pm/packages/zenohex_pico)
[![API docs](https://img.shields.io/hexpm/v/zenohex_pico.svg?label=hexdocs "API docs")](https://hexdocs.pm/zenohex_pico/)
[![License](https://img.shields.io/hexpm/l/zenohex_pico.svg)](https://github.com/biyooon-ex/zenohex_pico/blob/main/LICENSE)
[![CI](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml/badge.svg)](https://github.com/biyooon-ex/zenohex_pico/actions/workflows/ci.yaml)

A thin Elixir wrapper around zenoh-pico, implemented using C.

ZenohexPico is primarily designed for use with AtomVM running on ESP32.

**Currently, ZenohexPico uses version 1.10.2 of Zenoh.**

We recommend you use the same version to communicate with other Zenoh clients or
routers since version compatibility is somewhat important for Zenoh.

## 使い方

ZenohexPico を ESP32 上の AtomVM で使うには、次の 2 つの手順が必要です。

1. zenoh-pico を組み込んだ AtomVM をターゲット機に書き込む。
2. Elixir AtomVM プロジェクトに ZenohexPico を追加し、アプリケーションをターゲット機に書き込む。

以下で順に説明します。

### zenoh-pico を組み込んだ AtomVM をターゲット機に書き込む

[GitHub Releases](https://github.com/biyooon-ex/zenohex_pico/releases) で、
zenoh-pico を組み込み済みの AtomVM を ZIP ファイルとして配布しています。
プロジェクトで使用する ZenohexPico と同じバージョンのリリースから、
ターゲット機に対応する ZIP ファイルをダウンロードしてください。
ZIP ファイルを解凍し、そのディレクトリ内の README に従って
AtomVM をターゲット機に書き込んでください。

### Elixir AtomVM プロジェクトに ZenohexPico を追加する

[ExAtomVM](https://github.com/atomvm/exatomvm) を利用している
Elixir AtomVM プロジェクトの `mix.exs` の依存関係に、以下を追加してください。

```elixir
{:zenohex_pico, "~> 0.1.0"},
```

プロジェクトのルートで、環境変数 `MIX_TARGET` を `avm_esp32` に設定してから、
依存関係を取得してください。

```sh
export MIX_TARGET=avm_esp32
mix deps.get
```

以降の Mix コマンドも同じ設定で実行してください。別のシェルを使う場合は、
そのシェルでも `MIX_TARGET` を設定してください。

`mix.lock` で取得された ZenohexPico のバージョンを確認し、
先ほど使用した ZIP ファイルのリリースバージョンと一致していることを確認してください。

The Hex package does not include the upstream `zenoh-pico` source or compiled
binaries. When Mix compiles the package, its compiler task clones `zenoh-pico`.
This requires Git and network access. The task asks for
confirmation before cloning; for non-interactive builds, set
`ZENOHEX_PICO_AUTO_GIT_CLONE=true`.

Building from Hex also requires CMake, Make, a C compiler, and Erlang
development headers. This wrapper is MIT-licensed. The upstream `zenoh-pico`
source is licensed under Apache-2.0 OR EPL-2.0; see `zenoh-pico/LICENSE` and
`zenoh-pico/NOTICE.md` after it has been cloned.

次に、解凍したディレクトリ内の `avm_deps` をプロジェクトのルートにコピーしてください。
この `avm_deps` は、ZIP ファイルに含まれる AtomVM と互換性があります。
プロジェクトに既に `avm_deps` がある場合は、ZIP ファイル内のものに置き換えてください。

その後、次のコマンドを順に実行して、アプリケーションをパックし、
ターゲット機に書き込みます。

```sh
mix atomvm.packbeam
mix atomvm.esp32.flash --port /dev/your.usbserial
```

これらの Mix タスクの詳細については、
[ExAtomVM](https://github.com/atomvm/exatomvm) を参照してください。

以上で準備完了です。ZenohexPico をお楽しみください！
