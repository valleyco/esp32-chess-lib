/**
 * Optional WAC strength bench (Urusov suite, CPU-independent budget).
 *
 * Not part of `make test`. Effort is measured in engine nodes (or fixed depth),
 * not wall-clock — so host results are comparable. Map ESP32 ~20 kN/s:
 *   1 min  ≈ 1_200_000 nodes
 *   10 min ≈ 12_000_000 nodes
 *
 *   make bench-wac-smoke
 *   make bench-wac NODES=1200000          # ~1 min ESP32 effort
 *   make bench-wac DEPTH=5 LIMIT=300     # fixed depth instead
 *
 * Options (pick one budget; nodes is default):
 *   --nodes N     node budget per position (default 50000)
 *   --depth D     fixed search depth (overrides --nodes)
 *   --time MS     wall-clock (Hackster replay only; CPU-dependent)
 *   --limit N     only first N positions
 *   --from I      start at 0-based index I
 *   --quiet       less per-position noise
 */
#include "chess_api.h"
#include "wac_positions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    BUDGET_NODES = 0,
    BUDGET_DEPTH,
    BUDGET_TIME,
} budget_kind_t;

static void sq_name(int sq, char out[3])
{
    if (sq < 0 || sq > 63) {
        out[0] = '?';
        out[1] = '?';
        out[2] = '\0';
        return;
    }
    out[0] = (char)('a' + (sq % 8));
    out[1] = (char)('0' + (8 - sq / 8));
    out[2] = '\0';
}

static const char *wac_id(const char *epd, char *buf, size_t n)
{
    const char *p = strstr(epd, "WAC.");
    if (!p) {
        snprintf(buf, n, "?");
        return buf;
    }
    size_t i = 0;
    while (p[i] && p[i] != ';' && p[i] != ' ' && i + 1 < n) {
        buf[i] = p[i];
        i++;
    }
    buf[i] = '\0';
    return buf;
}

int main(int argc, char **argv)
{
    budget_kind_t kind = BUDGET_NODES;
    unsigned long nodes = 50000UL;
    int depth = 5;
    unsigned time_ms = 1000;
    int limit = WAC_POSITION_COUNT;
    int from = 0;
    int quiet = 0;
    int kind_set = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--nodes") && i + 1 < argc) {
            kind = BUDGET_NODES;
            nodes = strtoul(argv[++i], NULL, 10);
            kind_set = 1;
        } else if (!strcmp(argv[i], "--depth") && i + 1 < argc) {
            kind = BUDGET_DEPTH;
            depth = atoi(argv[++i]);
            kind_set = 1;
        } else if (!strcmp(argv[i], "--time") && i + 1 < argc) {
            kind = BUDGET_TIME;
            time_ms = (unsigned)strtoul(argv[++i], NULL, 10);
            kind_set = 1;
        } else if (!strcmp(argv[i], "--limit") && i + 1 < argc) {
            limit = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--from") && i + 1 < argc) {
            from = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--quiet")) {
            quiet = 1;
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            fprintf(stderr,
                    "Usage: %s [--nodes N | --depth D | --time MS] [--limit N] [--from I] [--quiet]\n"
                    "Optional WAC bench; not required for make test.\n"
                    "Prefer --nodes (or --depth); --time is CPU-dependent.\n",
                    argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown arg: %s\n", argv[i]);
            return 2;
        }
    }
    (void)kind_set;

    if (from < 0) {
        from = 0;
    }
    if (from >= WAC_POSITION_COUNT) {
        fprintf(stderr, "from=%d out of range\n", from);
        return 2;
    }
    if (limit < 0) {
        limit = 0;
    }
    if (from + limit > WAC_POSITION_COUNT) {
        limit = WAC_POSITION_COUNT - from;
    }

    int solved = 0;
    int tried = 0;
    unsigned long long nodes_sum = 0;
    const clock_t wall0 = clock();

    if (kind == BUDGET_NODES) {
        printf("WAC optional bench: from=%d limit=%d nodes=%lu\n", from, limit, nodes);
    } else if (kind == BUDGET_DEPTH) {
        printf("WAC optional bench: from=%d limit=%d depth=%d\n", from, limit, depth);
    } else {
        printf("WAC optional bench: from=%d limit=%d time_ms=%u (CPU-dependent)\n", from, limit,
               time_ms);
    }

    for (int i = 0; i < limit; i++) {
        const int idx = from + i;
        const char *epd = WAC_POSITIONS[idx];
        char id[16];
        wac_id(epd, id, sizeof(id));

        if (!chess_set_fen(epd)) {
            fprintf(stderr, "FAIL %s: set_fen\n", id);
            return 1;
        }
        const int bm_n = chess_epd_load_bm();
        if (bm_n <= 0) {
            fprintf(stderr, "FAIL %s: no bm parsed\n", id);
            return 1;
        }

        chess_search_result_t r;
        memset(&r, 0, sizeof(r));
        const clock_t t0 = clock();
        bool ok_think = false;
        if (kind == BUDGET_NODES) {
            ok_think = chess_think_nodes(nodes, &r);
        } else if (kind == BUDGET_DEPTH) {
            ok_think = chess_think_depth(depth, &r);
        } else {
            ok_think = chess_think_time(time_ms, &r);
        }
        if (!ok_think) {
            fprintf(stderr, "FAIL %s: think\n", id);
            return 1;
        }
        const double sec = (double)(clock() - t0) / (double)CLOCKS_PER_SEC;
        const int ok = chess_matches_epd_bm(r.c1, r.c2, r.promo) ? 1 : 0;
        if (ok) {
            solved++;
        }
        tried++;
        nodes_sum += r.nodes;

        char a[3], b[3];
        sq_name(r.c1, a);
        sq_name(r.c2, b);

        if (!quiet || (i + 1) % 10 == 0 || i + 1 == limit) {
            printf("%s %s %s%s depth=%d nodes=%lu %.2fs  %d/%d\n", id, ok ? "OK" : "MISS", a, b,
                   r.depth, (unsigned long)r.nodes, sec, solved, tried);
        }
    }

    const double wall = (double)(clock() - wall0) / (double)CLOCKS_PER_SEC;
    printf("\nSummary: %d/%d solved (%.1f%%)  nodes=%llu  wall=%.1fs\n", solved, tried,
           tried ? (100.0 * solved) / tried : 0.0, nodes_sum, wall);
    if (kind == BUDGET_NODES) {
        printf("Budget: %lu nodes/pos (~%.1f s ESP32 @ 20 kN/s)\n", nodes,
               nodes / 20000.0);
    } else if (kind == BUDGET_DEPTH) {
        printf("Budget: depth %d/pos\n", depth);
    } else {
        printf("Budget: %u ms/pos (wall-clock)\n", time_ms);
    }
    printf("(Reference: Urusov ~272/300 @ 1 min/pos on ESP32 ≈ 1.2M nodes @ 20 kN/s.)\n");
    return 0;
}
