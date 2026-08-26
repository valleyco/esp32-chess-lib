# EPD test suites

- `wac.epd` — Win At Chess (Reinfeld), same FENs as Arasan
  [`tests/wacnew.epd`](https://github.com/jdart1/arasan-chess/blob/master/tests/wacnew.epd)
  (Arasan engine is MIT; positions widely used for engine testing). Also mirrored
  in `host/wac_positions.h` for the embedded default.
- `wac_must_pass_d5.txt` — honest depth-5 floor (`make test-wac-regression`; no EPD early-exit).
- `wac_must_pass_fast.txt` — cheaper honest subset for `make test`.

Must-pass benches default to `--no-early-exit` so a lucky low-depth `bm` hit
cannot short-circuit the search. Update lists only when deliberately accepting
a strength tradeoff. Do not invent positions; add further public EPD suites here.
