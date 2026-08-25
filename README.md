# esp32-chess-lib

Standalone **C ABI** chess engine for ESP32 (and host), extracted from
[`esp32-chess`](../esp32-chess). Search/rules descend from Sergey Urusov’s
engine (via [hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine));
this tree adds `chess_api`, host tests, and fixed-depth benches.

**License:** GPL-3.0 — see [`LICENSE`](LICENSE).

## Quick start (host)

```bash
make test    # API unit tests + golden fixed-depth bench
make bench   # print depth/nodes/nps (no asserts)
```

Public header: [`include/chess_api.h`](include/chess_api.h).

## Layout

```text
include/chess_api.h          # public C API
include/arduino_shim.h       # private (host/IDF portability)
include/chess_engine_internal.h
src/chess_api.cpp
src/chess_engine.cpp
src/arduino_shim_{host,idf}.cpp
host/                        # gcc tests + bench
CMakeLists.txt               # optional ESP-IDF component
```

## ESP-IDF

Register this directory as a component (`EXTRA_COMPONENT_DIRS` or
`components/chess` symlink). Firmware should call only `chess_api.h`.

## Status

Initial copy from `esp32-chess` (in-tree `components/chess` + `host/chess`).
The CYD app still vendors its own copy until we switch it to this repo.
