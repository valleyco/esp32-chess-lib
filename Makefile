# Default host build is -O2 (speed). Use DEBUG=1 for -O0 (easier gdb).
ifeq ($(DEBUG),1)
CFLAGS ?= -O0 -g -Wall -Wextra -Werror
else
CFLAGS ?= -O2 -g -Wall -Wextra -Werror
endif
CXXFLAGS ?= $(CFLAGS) -std=c++17
ENGINE_CXXFLAGS := $(CXXFLAGS) -Wno-parentheses -Wno-sign-compare -Wno-unused-parameter \
	-Wno-unused-variable -Wno-type-limits -Wno-dangling-else -Wno-return-type -Wno-format

ROOT := $(abspath .)
CPPFLAGS := -I$(ROOT)/include -I$(ROOT)/host -DCHESS_HOST -DCHESS_ENGINE_LIBRARY

TEST_BIN := host/test_chess_api
BENCH_BIN := host/bench_chess
WAC_BIN := host/bench_wac
OBJS := host/chess_api.o host/chess_engine.o host/arduino_shim_host.o

# Optional WAC: node budget preferred (CPU-independent). ~20 kN/s on ESP32 →
# 1 min ≈ NODES=1200000. Or use DEPTH=5. TIME_MS is Hackster replay only.
NODES ?= 50000
DEPTH ?=
TIME_MS ?=
LIMIT ?= 300
FROM ?= 0

.PHONY: all test bench bench-wac bench-wac-smoke test-wac-fast test-wac-regression clean

all: test

host/chess_api.o: src/chess_api.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

host/chess_engine.o: src/chess_engine.cpp
	$(CXX) $(ENGINE_CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

host/arduino_shim_host.o: src/arduino_shim_host.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TEST_BIN): host/test_chess_api.c $(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ host/test_chess_api.c $(OBJS) -pthread

$(BENCH_BIN): host/bench_chess.c $(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ host/bench_chess.c $(OBJS) -pthread

$(WAC_BIN): host/bench_wac.c host/wac_positions.h $(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ host/bench_wac.c $(OBJS) -pthread

# API + depth goldens + honest fast WAC must-pass (no EPD early-exit).
# Full floor: make test-wac-regression (~2–4 min).
test: $(TEST_BIN) $(BENCH_BIN) $(WAC_BIN)
	@echo "== test_chess_api =="
	./$(TEST_BIN)
	@echo "== bench_chess =="
	./$(BENCH_BIN)
	@echo "== wac must-pass fast (depth 5, no early-exit) =="
	./$(WAC_BIN) --depth 5 --must-pass host/epd/wac_must_pass_fast.txt --epd host/epd/wac.epd --quiet

bench: $(BENCH_BIN)
	./$(BENCH_BIN) --print-only

# Optional strength suite — full 300 or custom budget.
bench-wac: $(WAC_BIN)
	@args="--limit $(LIMIT) --from $(FROM) --epd host/epd/wac.epd"; \
	if [ -n "$(TIME_MS)" ]; then args="$$args --time $(TIME_MS)"; \
	elif [ -n "$(DEPTH)" ]; then args="$$args --depth $(DEPTH)"; \
	else args="$$args --nodes $(NODES)"; fi; \
	./$(WAC_BIN) $$args

bench-wac-smoke: $(WAC_BIN)
	./$(WAC_BIN) --nodes 30000 --limit 5 --from 0 --epd host/epd/wac.epd

test-wac-fast: $(WAC_BIN)
	./$(WAC_BIN) --depth 5 --must-pass host/epd/wac_must_pass_fast.txt --epd host/epd/wac.epd

# Full honest regression floor (no EPD early-exit).
test-wac-regression: $(WAC_BIN)
	./$(WAC_BIN) --depth 5 --must-pass host/epd/wac_must_pass_d5.txt --epd host/epd/wac.epd --quiet

clean:
	$(RM) $(TEST_BIN) $(BENCH_BIN) $(WAC_BIN) $(OBJS)
