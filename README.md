# esp32-chess-lib

Standalone **C ABI** chess engine for ESP32 (and host), extracted from
[`esp32-chess`](../esp32-chess). Search/rules descend from Sergey Urusov’s
engine (via [hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine));
this tree adds `chess_api`, host tests, and fixed-depth benches.

**License:** GPL-3.0 — see [`LICENSE`](LICENSE).

## Quick start (host)

```bash
make test                 # API + depth goldens + fast WAC must-pass (required)
make test-wac-regression  # full WAC must-pass @ depth 5 (~2 min; exit 1 on regress)
make bench                # print depth/nodes/nps (no asserts)
make bench-wac-smoke      # first 5 WAC @ 30k nodes
make bench-wac NODES=1200000 LIMIT=300
make bench-wac DEPTH=5 LIMIT=300
```

WAC positions are the public **Win At Chess** suite (same FENs as
[Arasan `wacnew.epd`](https://github.com/jdart1/arasan-chess/blob/master/tests/wacnew.epd)),
kept under [`host/epd/`](host/epd/). `make test` fails if any **must-pass** ID
misses — update `host/epd/wac_must_pass_*.txt` only when deliberately accepting
a strength tradeoff. Prefer **node** or **depth** budgets over wall-clock.
Esp32 reference ~272/300 @ 1 min/pos ≈ **1.2M nodes** at ~20 kN/s.

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

Register this directory as a component (`EXTRA_COMPONENT_DIRS` or as a
firmware `components/chess` submodule). Firmware should call only `chess_api.h`.

## Status

Consumed by [`esp32-chess`](https://github.com/valleyco/esp32-chess) as git submodule
`components/chess`. Host tests/benches live here; optional WAC is opt-in
(`make bench-wac`).