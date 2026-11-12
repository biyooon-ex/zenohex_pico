# For developers

## ESP32 development

The following instructions have been tested using an M5Stack Core S3.

### Build and flash AtomVM

1. Prepare the zenoh-pico sources.

```
cd /path/to/repos
git clone https://github.com/biyooon-ex/zenohex_pico.git
cd zenohex_pico
git checkout tag # FIXME
export MIX_TARGET=avm_esp32
mix deps.get
mix compile.zenoh_pico # this step prepares zenoh-pico sources
mix compile
```

2. Clone AtomVM and build its Unix artifacts. The build produces `AtomVM/build/libs`, which is required in the next step.

```
cd /path/to/repos
git clone https://github.com/atomvm/AtomVM.git
cd AtomVM/
# main branch head commit as of 2026-10-27
git checkout 3a9ea46b8cf28066caf126c46119dc58f950de5f
mkdir build
cd build
mise use erlang@29.1.2
mise use elixir@1.20.5-otp-29
```

Confirm that `MIX_HOME` has changed.

```
echo "$MIX_HOME"
```

Then build. The `export` command adds `rebar3` to `PATH`.

```
mix local.hex --force
mix local.rebar --force
export PATH="$MIX_HOME/elixir/1-20-otp-29:$PATH"
cmake ..
make -j
cd ..
```

3. Build and flash AtomVM for the device.

```
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"
cd src/platforms/esp32/
idf.py set-target esp32s3
idf.py menuconfig
```

Other ESP32 targets may also work by changing `set-target`, but we have not tested them.
If you try another target, please share your results in an issue.

In `idf.py menuconfig`, enable `LWIP_IPV6` and set `ESP_MAIN_TASK_STACK_SIZE` to `8192`.

Then let's build,

```
idf.py -DEXTRA_COMPONENT_DIRS="/path/to/zenohex_pico/zxp_avm_esp32/zenoh_pico_component;/path/to/zenohex_pico/zxp_avm_esp32/zenohex_pico_component" build
...
idf.py -p /dev/ttyACM0 flash
```

On macOS, use your board's serial device path (for example, `/dev/cu.usbmodem...` or `/dev/cu.usbserial...`) instead of `/dev/ttyACM0`.

If build failed with following messages, you need to follow https://github.com/biyooon-ex/zenohex_pico/issues/15.

> Error: app partition is too small for binary atomvm-esp32.bin size 0x1c0920:
>  - Part 'factory' 0/0 @ 0x10000 size 0x1c0000 (overflow 0x920)

### Flash ZenohexPico Application

1. Prepare `avm_deps` and flash ZenohexPico application.

```
cd /path/to/zenohex_pico
mkdir -p avm_deps
cp /path/to/AtomVM/build/libs/atomvmlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/eavmlib/src/eavmlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/estdlib/src/estdlib.avm avm_deps/
cp /path/to/AtomVM/build/libs/exavmlib/lib/exavmlib.avm avm_deps/
export MIX_TARGET=avm_esp32
mix deps.get
# source is needed for `mix atomvm.esp32.flash`
source "$HOME/.espressif/tools/activate_idf_v5.5.5.sh"
# `mix flash` is an alias of `mix atomvm.packbeam` and `mix atomvm.esp32.flash --port /dev/ttyACM0 --baud 921600`
mix flash
```

### Test

Verify that the ESP32 publishes ten samples to a subscriber running on a Unix host.

1. Store Wi-Fi credentials once from an application running on the device. The test firmware reads them from AtomVM NVS.

```elixir
:esp.nvs_put_binary(:atomvm, :sta_ssid, "SSID")
:esp.nvs_put_binary(:atomvm, :sta_psk, "Password")
```

2. From the repository root, start IEx with the host target in one terminal:

```sh
MIX_TARGET=host iex -S mix
```

In IEx, start the subscriber. This call waits until it receives ten samples.

```elixir
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

## Release

### Publish to Hex

```sh
mix hex.publish
```

### Make release artifact zip

1. Create artifact zip.

```
cd /path/to/AtomVM/src/platforms/esp32/build
cp /path/to/zenohex_pico/zxp_avm_esp32/zenohex_pico_atomvm_artifact.sh .
# this script creates zip file under the same directory
./zenohex_pico_atomvm_artifact.sh
```

2. Upload the ZIP file to the [GitHub release](https://github.com/biyooon-ex/zenohex_pico/releases) corresponding to the ZenohexPico version.
