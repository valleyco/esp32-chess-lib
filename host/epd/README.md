# EPD test suites

- `wac.epd` — Win At Chess (Reinfeld), same FENs as Arasan
  [`tests/wacnew.epd`](https://github.com/jdart1/arasan-chess/blob/master/tests/wacnew.epd)
  (Arasan engine is MIT; positions widely used for engine testing). Also mirrored
  in `host/wac_positions.h` for the embedded default.
- `wac_must_pass_d5.txt` — IDs that must stay solved at depth 5 (`make test-wac-regression`).
- `wac_must_pass_fast.txt` — cheaper subset wired into `make test`.

Do not invent positions; add further public EPD suites here when needed.
Update must-pass lists only when deliberately accepting a strength tradeoff.
