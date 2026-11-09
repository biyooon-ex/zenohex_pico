#!/bin/bash
set -e

case "$(pwd -P)" in
    */AtomVM/src/platforms/esp32/build)
        ;;
    *)
        echo "Error: Run this script from a directory ending in AtomVM/src/platforms/esp32/build." >&2
        exit 1
        ;;
esac

ZENOH_PICO_SOURCE_DIR="$(sed -n 's/^# Install script for directory: //p' esp-idf/zenoh_pico_component/zenoh-pico/cmake_install.cmake)"
ZENOH_PICO_VERSION="$(git -C "$ZENOH_PICO_SOURCE_DIR" describe --tags --always --dirty)"
ZENOH_PICO_COMMIT="$(git -C "$ZENOH_PICO_SOURCE_DIR" rev-parse --short=8 HEAD)"

ARTIFACT_DIR=zenohex_pico_atomvm_artifact_zenoh-pico-${ZENOH_PICO_VERSION}-${ZENOH_PICO_COMMIT}
ZIP_FILE="${ARTIFACT_DIR}.zip"

test -d "$ARTIFACT_DIR" && rm -r -- "$ARTIFACT_DIR"
mkdir -p "$ARTIFACT_DIR"

cp flash_args "$ARTIFACT_DIR/"
sed \
    -e 's|bootloader/bootloader.bin|bootloader.bin|' \
    -e 's|partition_table/partition-table.bin|partition-table.bin|' \
    -e 's|../../../../build/libs/esp32boot/esp32boot.avm|esp32boot.avm|' \
    "$ARTIFACT_DIR/flash_args" > "$ARTIFACT_DIR/flash_args.tmp"
mv "$ARTIFACT_DIR/flash_args.tmp" "$ARTIFACT_DIR/flash_args"

cp bootloader/bootloader.bin "$ARTIFACT_DIR/"
cp atomvm-esp32.bin "$ARTIFACT_DIR/"
cp partition_table/partition-table.bin "$ARTIFACT_DIR/"
cp ../../../../build/libs/esp32boot/esp32boot.avm "$ARTIFACT_DIR/"

cat > "$ARTIFACT_DIR/README.md" <<EOF
# How to flash

\`\`\`bash
cd "$ARTIFACT_DIR"
source "\$HOME/.espressif/tools/activate_idf_v5.5.5.sh"
python -m esptool --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash "@flash_args"
\`\`\`
EOF

rm -f -- "$ZIP_FILE"
zip -r "$ZIP_FILE" "$ARTIFACT_DIR"
rm -r -- "$ARTIFACT_DIR"
