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
OBJS := host/chess_api.o host/chess_engine.o host/arduino_shim_host.o

.PHONY: all test bench clean

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

test: $(TEST_BIN) $(BENCH_BIN)
	@echo "== test_chess_api =="
	./$(TEST_BIN)
	@echo "== bench_chess =="
	./$(BENCH_BIN)

bench: $(BENCH_BIN)
	./$(BENCH_BIN) --print-only

clean:
	$(RM) $(TEST_BIN) $(BENCH_BIN) $(OBJS)
