# AGENTS.md

Bare ESP-IDF (C) firmware project. Not Arduino. Project/binary name is `ESP32Test`.

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

Verified: `idf.py build` succeeds on the default `esp32` target.

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
