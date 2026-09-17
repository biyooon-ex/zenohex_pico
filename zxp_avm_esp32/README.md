# Development with M5STACK CORE S3

## prepare zenoh-pico submodule

```
cd /path/to/zenohex_pico
export MIX_TARGET=avm_esp32
mix deps.get
mix compile # this step prepares zenoh-pico submodule
```

## prepare AtomVM

### clone AtomVM

```
cd /path/to/repos
git clone https://github.com/atomvm/AtomVM.git
cd AtomVM/
# main branch head commit as of 2026-09-17
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

### build AtomVM for the device

#### set target

```
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"
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

## prepare avm_deps

```
cd /path/to/zenohex_pico
mkdir -p avm_deps
cp /path/to/AtomVM/build/libs/atomvmlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/eavmlib/src/eavmlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/estdlib/src/estdlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/exavmlib/lib/exavmlib.avm avm_deps/
```

## flash Elixir app to the device

```
export MIX_TARGET=avm_esp32
mix deps.get
# source is needed for `mix atomvm.esp32.flash`
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"
# `mix flash` is an alias of `mix atomvm.packbeam` and `mix atomvm.esp32.flash --port /dev/ttyACM0 --baud 921600`
mix flash
```

## How to test

Verify that the ESP32 publishes ten samples to a subscriber running on a Unix host.

1. Store Wi-Fi credentials once from an application running on the device. The test firmware reads them from AtomVM NVS.

```elixir
:esp.nvs_put_binary(:atomvm, :sta_ssid, "SSID")
:esp.nvs_put_binary(:atomvm, :sta_psk, "Password")
```

2. From the repository root, start the subscriber in one terminal. This call waits until it receives ten samples.

```elixir
iex -S mix
iex> ZenohexPico.AvmEsp32.Test.start_subscriber_on_unix()
```

3. In another terminal, from the repository root, flash the test firmware.

```sh
export MIX_TARGET=avm_esp32
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"
MIX_ENV=test mix flash
```

4. Verify that the Unix terminal prints ten samples with payload suffixes from `0` through `9`. The subscriber and session close automatically after the tenth sample.

```elixir
%ZenohexPico.Sample{attachment: "", congestion_control: :drop, encoding: "zenoh/bytes", express: false, key_expr: "key/expr", kind: :put, payload: "from M5STACK CORE S3, 0", priority: :data, timestamp: nil}
%ZenohexPico.Sample{attachment: "", congestion_control: :drop, encoding: "zenoh/bytes", express: false, key_expr: "key/expr", kind: :put, payload: "from M5STACK CORE S3, 9", priority: :data, timestamp: nil}
```
