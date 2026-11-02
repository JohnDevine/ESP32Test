# AGENTS.md

Bare ESP-IDF (C) firmware project. Not Arduino. Project/binary name is `ESP32Test`.

## Environment

OpenCode runs inside a Docker container. The native project at
`/home/johndevine/Projects/ESP32Projects/ESP32Test` is mapped to `/workspace` in the
container. Building happens in the container, but **flashing must be done from the
native environment** (not the container), because USB ports are not available inside
the container.

## Toolchain setup (required in every shell)

ESP-IDF v6.1 is installed at `/root/.espressif/v6.1/esp-idf`. The environment ships
with `IDF_PATH=/root/.espressif/v6.1` (the parent directory, which is wrong). Unset it
before sourcing, or `idf.py` and CMake will not find `tools/cmake/project.cmake`:

```sh
unset IDF_PATH
. /root/.espressif/v6.1/esp-idf/export.sh
```

## Commands

```sh
idf.py build                  # default target is esp32; outputs to build/
idf.py set-target esp32s3     # only when intentionally changing chip
idf.py -p /dev/ttyUSB0 flash monitor
idf.py menuconfig             # edits sdkconfig
idf.py fullclean              # remove build/ + sdkconfig thoroughly
```

Important: Always set the correct chip target using `idf.py set-target <chip_name>` before building. Refer to `hardware/esp32` for specific chip requirements.

Verified: `idf.py build` succeeds on the default `esp32` target.

## External flash & monitor

Flashing is done in the **native environment**, not the container.

```sh
# make sure permissions are ok on espressif idf
sudo chown -R $USER:$USER /home/johndevine/.espressif

# install
/home/johndevine/.espressif/v6.1/esp-idf/install.sh

# export environment (use at each new login)
cd /home/johndevine/Projects/ESP32Projects/ESP32Test
. /home/johndevine/.espressif/v6.1/esp-idf/export.sh

# remove the build directory (permission problems)
sudo rm -rf build

# ESP-IDF re-configures the project for the current path
# (/home/johndevine/Projects/ESP32Projects/ESP32Test), compiles cleanly as your
# user, and is then ready to flash.

# find the USB port the ESP32 is on
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null

# set permissions on the port
sudo chmod 666 /dev/ttyUSB0

# flash the chip
idf.py -p /dev/ttyUSB0 flash

# monitor
idf.py -p /dev/ttyUSB0 monitor
```

### Monitor keyboard shortcuts

Press `Ctrl+]` to exit the monitor directly. Press `Ctrl+T` for the menu, then:

- `Ctrl+T`: send the menu character itself to the remote.
- `Ctrl+]`: send the exit character itself to the remote.
- `Ctrl+R`: reset the target board via RTS.
- `Ctrl+F`: build & flash project (fast reflash, ESP-IDF 6.1+).
- `Ctrl+A` (or `A`): build & flash app only.
- `Ctrl+E` (or `E`): build & full flash project.
- `Ctrl+Y`: toggle output display.
- `Ctrl+L`: toggle saving output to a file.
- `Ctrl+I` (or `I`): toggle printing timestamps.
- `Ctrl+P`: reset the target into the bootloader via DTR/RTS.
- `Ctrl+X` (or `X`): exit program.

## Structure / conventions

- `main/ESP32Test.c` — application entrypoint `app_main()`.
- `main/CMakeLists.txt` — component registered with explicit `SRCS`; **add every new
  `.c` file to `SRCS`**, sources are not globbed.
- Root `CMakeLists.txt` — its boilerplate lines (`include($ENV{IDF_PATH}/tools/cmake/project.cmake)`
  before `project(...)`) must stay in order or CMake config breaks.
- `build/`, `sdkconfig`, `sdkconfig.old`, and `managed_components/` are generated; do not
  hand-edit them.

## Testing / lint

No test, lint, or formatter config exists. Verify changes with `idf.py build`.
