# ZenohexPico

**TODO: Add description**

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `zenohex_pico` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:zenohex_pico, "~> 0.1.0"}
  ]
end
```

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at <https://hexdocs.pm/zenohex_pico>.

## Development with M5STACK CORE S3

### clone AtomVM

```
git clone https://github.com/atomvm/AtomVM.git
cd AtomVM/
git checkout 0220c78ee9e7cf6c763a278b44d81ce309fcf1ab
```

### build AtomVM for unix

This step is necessary to prepare `AtomVM/build/libs`, which is used in the following step.

```
mkdir build
cd build
mise use erlang@27.3.4.17
mise use elixir@1.18.5-otp-27
mix local.hex
mix local.rebar
export PATH="$MIX_HOME/elixir/1-18:$PATH"
cmake ..
make -j
cd ..
```

### build for the device

#### set target

```
source "~/.espressif/tools/activate_idf_v5.5.5.sh"
cd src/platforms/esp32/
idf.py set-target esp32s3
```

#### menuconfig

```
idf.py menuconfig
```

- Enable LWIP_IPV6
  - Zenoh Pico requires IPv6.
- Change ESP_MAIN_TASK_STACK_SIZE from 3584 to 8192
  - ZenohexPico on the ESP32-S3 requires a larger main task stack than the default 3584 bytes.

#### build

```
idf.py -DEXTRA_COMPONENT_DIRS="/path/to/zenohex_pico/zxp_avm_esp32/zenoh_pico_component;/path/to/zenohex_pico/zxp_avm_esp32/zenohex_pico_component" build
```

### flash AtomVM to the device

```
idf.py -p /dev/ttyACM0 flash
```

### prepare avm_deps

```
cd zenohex_pico
mkdir -p avm_deps
cp /path/to/AtomVM/build/libs/atomvmlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/eavmlib/src/eavmlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/estdlib/src/estdlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/exavmlib/lib/exavmlib.avm avm_deps/
```

### flash Elixir app to the device

```
export MIX_TARGET=avm_esp32
mix deps.get
# source is needed for `mix atomvm.esp32.flash`
source "~/.espressif/tools/activate_idf_v5.5.5.sh"
# `mix flash` is an alias of `mix atomvm.packbeam` and `mix atomvm.esp32.flash --port /dev/ttyACM0 --baud 921600`
mix flash
```
