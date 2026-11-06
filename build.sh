#!/usr/bin/env bash
# ==============================================================================
# Gyro Mouse Base: ESP32-S3 Firmware Build & Flash Tool
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Target Board Configuration
FQBN="esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=default"
ESP32_INDEX_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"

# Build & Toolchain Directory Paths (Contained inside BUILD_DIR)
CLI_BIN_DIR="$BUILD_DIR/bin"
CLI_BIN="$CLI_BIN_DIR/arduino-cli"
CONFIG_FILE="$BUILD_DIR/arduino-cli.yaml"
DATA_DIR="$BUILD_DIR/data"
USER_DIR="$BUILD_DIR/sketchbook"
DOWNLOADS_DIR="$BUILD_DIR/staging"
SKETCH_BUILD_DIR="$BUILD_DIR/sketch"
OUTPUT_DIR="$BUILD_DIR/output"
CACHE_DIR="$BUILD_DIR/cache"

# UI Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# ------------------------------------------------------------------------------
# 1. Environment Setup & Toolchain Installation
# ------------------------------------------------------------------------------
setup_toolchain() {
    mkdir -p "$BUILD_DIR" "$CLI_BIN_DIR" "$DATA_DIR" "$USER_DIR" "$DOWNLOADS_DIR" "$SKETCH_BUILD_DIR" "$OUTPUT_DIR" "$CACHE_DIR"

    # Locate or install arduino-cli
    if [ -x "$CLI_BIN" ]; then
        : # Local CLI already present
    elif command -v arduino-cli >/dev/null 2>&1; then
        CLI_BIN="$(command -v arduino-cli)"
    else
        echo -e "${YELLOW}[SETUP] Installing arduino-cli to $CLI_BIN_DIR...${NC}"
        curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$CLI_BIN_DIR" sh
        CLI_BIN="$CLI_BIN_DIR/arduino-cli"
    fi

    # Ensure configuration file points strictly to BUILD_DIR always
    echo -e "${CYAN}[SETUP] Configuring arduino-cli environment...${NC}"
    "$CLI_BIN" config init --dest-file "$CONFIG_FILE" --overwrite >/dev/null 2>&1 || true
    "$CLI_BIN" config set directories.data "$DATA_DIR" --config-file "$CONFIG_FILE"
    "$CLI_BIN" config set directories.user "$USER_DIR" --config-file "$CONFIG_FILE"
    "$CLI_BIN" config set directories.downloads "$DOWNLOADS_DIR" --config-file "$CONFIG_FILE"
    "$CLI_BIN" config set board_manager.additional_urls "$ESP32_INDEX_URL" --config-file "$CONFIG_FILE"

    # Ensure ESP32 Core is installed
    if ! "$CLI_BIN" core list --config-file "$CONFIG_FILE" 2>/dev/null | grep -q "esp32:esp32"; then
        echo -e "${YELLOW}[SETUP] Updating board manager index...${NC}"
        "$CLI_BIN" core update-index --config-file "$CONFIG_FILE"
        echo -e "${YELLOW}[SETUP] Installing ESP32 board package (esp32:esp32)...${NC}"
        "$CLI_BIN" core install esp32:esp32 --config-file "$CONFIG_FILE"
    fi

    # Ensure required MPU6050_light library is installed
    if ! "$CLI_BIN" lib list --config-file "$CONFIG_FILE" 2>/dev/null | grep -q "MPU6050_light"; then
        echo -e "${YELLOW}[SETUP] Installing MPU6050_light library...${NC}"
        "$CLI_BIN" lib install "MPU6050_light" --config-file "$CONFIG_FILE"
    fi
}

# ------------------------------------------------------------------------------
# 2. Serial Port Auto-Detection
# ------------------------------------------------------------------------------
find_port() {
    if [ -n "$CUSTOM_PORT" ]; then
        echo "$CUSTOM_PORT"
        return
    fi

    local PORT=""
    # 1. Check serial-by-id for Espressif device (works for both runtime CDC & ROM JTAG bootloader)
    PORT=$(ls -1 /dev/serial/by-id/usb-Espressif* 2>/dev/null | head -n 1 || true)
    if [ -n "$PORT" ] && [ -e "$PORT" ]; then
        readlink -f "$PORT"
        return
    fi

    # 2. Query udev for Espressif USB Vendor ID (0x303a)
    for DEV in /dev/ttyACM* /dev/ttyUSB*; do
        [ -e "$DEV" ] || continue
        if udevadm info -q property -n "$DEV" 2>/dev/null | grep -q "ID_VENDOR_ID=303a"; then
            echo "$DEV"
            return
        fi
    done

    # 3. Fallback to first available serial port
    for DEV in /dev/ttyACM* /dev/ttyUSB*; do
        [ -e "$DEV" ] || continue
        echo "$DEV"
        return
    done
}

# ------------------------------------------------------------------------------
# 3. Compilation
# ------------------------------------------------------------------------------
do_compile() {
    setup_toolchain

    echo -e "${BLUE}[BUILD] Preparing sketch sources...${NC}"
    rm -rf "$SKETCH_BUILD_DIR/firmware"
    mkdir -p "$SKETCH_BUILD_DIR/firmware/src"
    cp "$SCRIPT_DIR/firmware.ino" "$SKETCH_BUILD_DIR/firmware/firmware.ino"
    cp -r "$SCRIPT_DIR/src/"* "$SKETCH_BUILD_DIR/firmware/src/"
    mkdir -p "$OUTPUT_DIR" "$CACHE_DIR"

    echo -e "${BLUE}[BUILD] Compiling Gyro Mouse Base for ESP32-S3 (TinyUSB + CDC)...${NC}"
    "$CLI_BIN" compile \
        --config-file "$CONFIG_FILE" \
        --build-path "$CACHE_DIR" \
        --output-dir "$OUTPUT_DIR" \
        -b "$FQBN" \
        "$SKETCH_BUILD_DIR/firmware"

    # Ensure boot_app0.bin is preserved in output directory
    if [ -f "$CACHE_DIR/boot_app0.bin" ]; then
        cp "$CACHE_DIR/boot_app0.bin" "$OUTPUT_DIR/boot_app0.bin"
    fi

    echo -e "${GREEN}[BUILD] Compilation successful! Binaries saved to: $OUTPUT_DIR${NC}"
}

# ------------------------------------------------------------------------------
# 4. Flashing Firmware
# ------------------------------------------------------------------------------
do_flash() {
    do_compile

    local TARGET_PORT
    TARGET_PORT="$(find_port)"

    if [ -z "$TARGET_PORT" ]; then
        echo -e "${RED}[ERROR] No ESP32-S3 serial port detected!${NC}"
        echo -e "${YELLOW}Troubleshooting:${NC}"
        echo "  1. Connect your ESP32-S3 via USB."
        echo "  2. Hold BOOT, press & release RESET, then release BOOT to enter download mode."
        echo "  3. Re-run './build.sh flash' or specify port with '-p /dev/ttyACM...'"
        exit 1
    fi

    local ESPTOOL_BIN
    ESPTOOL_BIN=$(find "$DATA_DIR" -type f -name esptool 2>/dev/null | head -n 1 || true)

    local BOOT_APP0_BIN=""
    if [ -f "$OUTPUT_DIR/boot_app0.bin" ]; then
        BOOT_APP0_BIN="$OUTPUT_DIR/boot_app0.bin"
    elif [ -f "$CACHE_DIR/boot_app0.bin" ]; then
        BOOT_APP0_BIN="$CACHE_DIR/boot_app0.bin"
    elif [ -f "$OUTPUT_DIR/boot_app0_flashed.bin" ]; then
        BOOT_APP0_BIN="$OUTPUT_DIR/boot_app0_flashed.bin"
    else
        BOOT_APP0_BIN=$(find "$DATA_DIR" -name "boot_app0.bin" 2>/dev/null | head -n 1 || true)
    fi

    local BOOTLOADER_BIN="$OUTPUT_DIR/firmware.ino.bootloader.bin"
    [ -f "$BOOTLOADER_BIN" ] || BOOTLOADER_BIN="$CACHE_DIR/firmware.ino.bootloader.bin"

    local PARTITIONS_BIN="$OUTPUT_DIR/firmware.ino.partitions.bin"
    [ -f "$PARTITIONS_BIN" ] || PARTITIONS_BIN="$CACHE_DIR/firmware.ino.partitions.bin"

    local FIRMWARE_BIN="$OUTPUT_DIR/firmware.ino.bin"
    [ -f "$FIRMWARE_BIN" ] || FIRMWARE_BIN="$CACHE_DIR/firmware.ino.bin"

    echo -e "${BLUE}[FLASH] Uploading firmware to ${TARGET_PORT}...${NC}"
    local FLASH_SUCCESS=false

    if [ -n "$ESPTOOL_BIN" ] && [ -x "$ESPTOOL_BIN" ] && [ -n "$BOOT_APP0_BIN" ] && [ -f "$BOOT_APP0_BIN" ]; then
        if "$ESPTOOL_BIN" --chip esp32s3 --port "$TARGET_PORT" --before default-reset --after hard-reset --no-stub write-flash -z \
            --flash-mode dio --flash-freq 80m --flash-size 4MB \
            0x0 "$BOOTLOADER_BIN" \
            0x8000 "$PARTITIONS_BIN" \
            0xe000 "$BOOT_APP0_BIN" \
            0x10000 "$FIRMWARE_BIN"; then
            FLASH_SUCCESS=true
        fi
    fi

    if [ "$FLASH_SUCCESS" != "true" ]; then
        if ! "$CLI_BIN" upload --config-file "$CONFIG_FILE" -b "$FQBN" -p "$TARGET_PORT" --input-dir "$OUTPUT_DIR"; then
            echo -e "\n${YELLOW}[HINT] If upload timed out, put the board into bootloader mode:${NC}"
            echo "       1. Hold the BOOT button"
            echo "       2. Press and release RESET"
            echo "       3. Release BOOT and run './build.sh flash' again."
            exit 1
        fi
    fi

    echo -e "${GREEN}[FLASH] Upload complete! If the device does not reboot automatically, press RESET once.${NC}"
}

# ------------------------------------------------------------------------------
# 5. Serial Monitor
# ------------------------------------------------------------------------------
do_monitor() {
    local TARGET_PORT
    TARGET_PORT="$(find_port)"

    if [ -z "$TARGET_PORT" ]; then
        echo -e "${YELLOW}[MONITOR] Waiting for ESP32-S3 serial device to appear...${NC}"
        while [ -z "$TARGET_PORT" ]; do
            sleep 0.5
            TARGET_PORT="$(find_port)"
        done
    fi

    if python3 -c "import serial" >/dev/null 2>&1; then
        python3 "$SCRIPT_DIR/monitor.py" "$TARGET_PORT"
    else
        "$CLI_BIN" monitor --config-file "$CONFIG_FILE" -p "$TARGET_PORT" -c baudrate=115200
    fi
}

# ------------------------------------------------------------------------------
# 6. Cleaning Build Artifacts
# ------------------------------------------------------------------------------
do_clean() {
    echo -e "${BLUE}[CLEAN] Cleaning build artifacts and cache...${NC}"
    rm -rf "$CACHE_DIR" "$SKETCH_BUILD_DIR" "$OUTPUT_DIR"
    echo -e "${GREEN}[CLEAN] Clean complete. (Toolchains and core packages preserved)${NC}"
}

do_distclean() {
    echo -e "${BLUE}[DISTCLEAN] Removing all build data and toolchain packages...${NC}"
    rm -rf "${BUILD_DIR:?}"/*
    echo -e "${GREEN}[DISTCLEAN] Entire build directory reset.${NC}"
}

# ------------------------------------------------------------------------------
# 7. Help Menu
# ------------------------------------------------------------------------------
show_help() {
    echo "=============================================================="
    echo "           Gyro Mouse Base - Build & Flash Helper             "
    echo "=============================================================="
    echo ""
    echo "Usage: ./build.sh [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  build       Compile firmware (default)"
    echo "  flash       Compile and flash firmware to ESP32-S3"
    echo "  monitor     Open interactive serial monitor with auto-reconnect"
    echo "  diagnose    Interactive tilt range & symmetry diagnostic tool"
    echo "  all         Compile, flash, and immediately open the monitor"
    echo "  setup       Install toolchain, ESP32 core, and required libraries"
    echo "  clean       Remove temporary build cache and binary outputs"
    echo "  distclean   Full wipe of build directory (cores, tools, binaries)"
    echo "  help        Display this help menu"
    echo ""
    echo "Options:"
    echo "  -p, --port <device>   Manually specify serial port (e.g. /dev/ttyACM0)"
    echo ""
}

# ------------------------------------------------------------------------------
# 8. Command-Line Argument Parsing
# ------------------------------------------------------------------------------
COMMAND="build"
CUSTOM_PORT=""

while [[ $# -gt 0 ]]; do
    case $1 in
        build|compile)
            COMMAND="build"
            shift
            ;;
        flash|upload)
            COMMAND="flash"
            shift
            ;;
        monitor)
            COMMAND="monitor"
            shift
            ;;
        diagnose|calib|test)
            COMMAND="diagnose"
            shift
            ;;
        all|run)
            COMMAND="all"
            shift
            ;;
        setup)
            COMMAND="setup"
            shift
            ;;
        clean)
            COMMAND="clean"
            shift
            ;;
        distclean)
            COMMAND="distclean"
            shift
            ;;
        -p|--port)
            CUSTOM_PORT="$2"
            shift 2
            ;;
        -h|--help|help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown argument: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

case "$COMMAND" in
    setup)
        setup_toolchain
        echo -e "${GREEN}[SETUP] Toolchain and dependencies ready!${NC}"
        ;;
    build)
        do_compile
        ;;
    flash)
        do_flash
        ;;
    monitor)
        do_monitor
        ;;
    diagnose)
        TARGET_PORT="$(find_port)"
        python3 "$SCRIPT_DIR/diagnose.py" "$TARGET_PORT"
        ;;
    all)
        do_flash
        echo ""
        do_monitor
        ;;
    clean)
        do_clean
        ;;
    distclean)
        do_distclean
        ;;
esac
