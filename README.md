# esp32-chess-lib

[![Host tests](https://github.com/valleyco/esp32-chess-lib/actions/workflows/host-tests.yml/badge.svg)](https://github.com/valleyco/esp32-chess-lib/actions/workflows/host-tests.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

Standalone **C ABI** chess engine for ESP32 (and host gcc). Extracted for reuse
from [`esp32-chess`](https://github.com/valleyco/esp32-chess).

Search/rules descend from **Sergey Urusov**’s engine (via
[hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine)).
This tree adds `chess_api`, host tests, benches, and ESP-IDF component wiring.

**License:** GPL-3.0 — [`LICENSE`](LICENSE), [`CREDITS.md`](CREDITS.md).

## Quick start (host)

```bash
git clone https://github.com/valleyco/esp32-chess-lib.git
cd esp32-chess-lib
make test                 # API + depth goldens + honest fast WAC must-pass
make DEBUG=1 test         # -O0
make test-wac-regression  # full honest WAC must-pass @ depth 5 (~2–4 min)
make test-wac-regression-d6  # d5-miss extras that clear @ depth 6 (~20–40s)
make bench                # depth/nodes/nps
make bench-wac-smoke      # first 5 WAC @ 30k nodes
```

Default host build is **`-O2`**. Search uses a small transposition table (see
`chess_tt_budget_bytes()`). Override size with `-DCHESS_TT_ENTRIES=<power-of-2>`.

WAC FENs under [`host/epd/`](host/epd/) are the public Win At Chess suite.
Prefer **node** or **depth** budgets over wall-clock.

Public header: [`include/chess_api.h`](include/chess_api.h).

## Layout

```text
include/chess_api.h                 # public C API
include/arduino_shim.h              # private portability
include/chess_engine_internal.h
src/chess_api.cpp
src/chess_engine.cpp
src/arduino_shim_{host,idf}.cpp
host/                               # gcc tests + bench
CMakeLists.txt                      # ESP-IDF component
```

## ESP-IDF

Use as a component (`EXTRA_COMPONENT_DIRS` or firmware submodule
`components/chess`). Call only `chess_api.h` from application code.

## Consumed by

[`valleyco/esp32-chess`](https://github.com/valleyco/esp32-chess) — touch UI on
ESP32 + SPI TFT (CYD-class bring-up; more boards planned).

See [`CONTRIBUTING.md`](CONTRIBUTING.md).
