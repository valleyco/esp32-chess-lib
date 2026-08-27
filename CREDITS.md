# Credits and attribution

## Upstream engine

Search and rules descend from **Sergey Urusov**’s ESP32 chess engine
([Hackster](https://www.hackster.io/Sergey_Urusov/esp32-chess-engine-c29dd9),
**GPL-3+**).

Packaged for PlatformIO by **hpsaturn**:
[hpsaturn/esp32-chess-engine](https://github.com/hpsaturn/esp32-chess-engine).

Keep the credit comment in `src/chess_engine.cpp` when redistributing.

## This library

`chess_api`, host tests/benches, Arduino shims, and ESP-IDF component wiring are
part of this **GPL-3.0** work — see [`LICENSE`](LICENSE).

Consumed by [`valleyco/esp32-chess`](https://github.com/valleyco/esp32-chess).

## Test suites

Win At Chess (WAC) FENs under `host/epd/` are a public tactical suite used for
strength regression (same family of positions as Arasan `wacnew.epd`).
