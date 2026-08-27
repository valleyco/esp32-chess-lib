# Contributing

`esp32-chess-lib` is **GPL-3.0** (see [`LICENSE`](LICENSE), [`CREDITS.md`](CREDITS.md)).

```bash
git clone https://github.com/valleyco/esp32-chess-lib.git
cd esp32-chess-lib
make test                 # API + benches + fast WAC must-pass
```

- Engine/API changes belong here; the CYD/app UI lives in
  [`esp32-chess`](https://github.com/valleyco/esp32-chess).
- Preserve Sergey Urusov credit in `src/chess_engine.cpp`.
- Prefer node/depth budgets over wall-clock in tests.
- Full strength floor: `make test-wac-regression` (slow; not required for every PR).
- Optional depth-6 extras: `make test-wac-regression-d6`.
