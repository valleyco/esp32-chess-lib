CC ?= gcc
CXX ?= g++
CFLAGS ?= -O0 -g -Wall -Wextra -Werror
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

.PHONY: all test bench bench-wac bench-wac-smoke clean

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

# Mandatory gate: unit tests + fixed-depth goldens only (no WAC).
test: $(TEST_BIN) $(BENCH_BIN)
	@echo "== test_chess_api =="
	./$(TEST_BIN)
	@echo "== bench_chess =="
	./$(BENCH_BIN)

bench: $(BENCH_BIN)
	./$(BENCH_BIN) --print-only

# Optional strength suite — never required by `make test`.
bench-wac: $(WAC_BIN)
	@args="--limit $(LIMIT) --from $(FROM)"; \
	if [ -n "$(TIME_MS)" ]; then args="$$args --time $(TIME_MS)"; \
	elif [ -n "$(DEPTH)" ]; then args="$$args --depth $(DEPTH)"; \
	else args="$$args --nodes $(NODES)"; fi; \
	./$(WAC_BIN) $$args

bench-wac-smoke: $(WAC_BIN)
	./$(WAC_BIN) --nodes 30000 --limit 5 --from 0

clean:
	$(RM) $(TEST_BIN) $(BENCH_BIN) $(WAC_BIN) $(OBJS)
