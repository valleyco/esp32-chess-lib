/**
 * WAC strength / regression bench (Reinfeld Win At Chess — same FENs as
 * Arasan tests/wacnew.epd). Prefer node or depth budgets over wall-clock.
 *
 *   make bench-wac-smoke
 *   make bench-wac DEPTH=5 LIMIT=300
 *   make test-wac-fast          # must-pass subset @ depth 5 (in make test)
 *   make test-wac-regression   # full must-pass @ depth 5
 *
 * Options:
 *   --nodes N | --depth D | --time MS
 *   --limit N  --from I  --quiet
 *   --epd FILE       EPD suite (default: embedded WAC = host/epd/wac.epd)
 *   --must-pass FILE only these ids; exit 1 if any miss (regression gate)
 */
#include "chess_api.h"
#include "wac_positions.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { MAX_EPD = 512, MAX_MUST = 512 };

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

/* Parse WAC.NNN / "WAC.NNN" / NNN → 1-based id, or 0 if none. */
static int epd_wac_num(const char *epd)
{
    const char *p = strstr(epd, "WAC.");
    if (!p) {
        return 0;
    }
    p += 4;
    while (*p && !isdigit((unsigned char)*p)) {
        p++;
    }
    if (!*p) {
        return 0;
    }
    return atoi(p);
}

static const char *wac_id(const char *epd, char *buf, size_t n)
{
    int num = epd_wac_num(epd);
    if (num > 0) {
        snprintf(buf, n, "WAC.%03d", num);
        return buf;
    }
    snprintf(buf, n, "?");
    return buf;
}

static int load_epd_file(const char *path, const char **out, int max_out)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "cannot open epd: %s\n", path);
        return -1;
    }
    char line[512];
    int n = 0;
    while (fgets(line, sizeof(line), f)) {
        char *s = line;
        while (*s == ' ' || *s == '\t') {
            s++;
        }
        if (*s == '#' || *s == '\0' || *s == '\n') {
            continue;
        }
        if (!strstr(s, "bm ")) {
            continue;
        }
        size_t len = strlen(s);
        while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
            s[--len] = '\0';
        }
        if (n >= max_out) {
            fprintf(stderr, "epd too large (max %d)\n", max_out);
            fclose(f);
            return -1;
        }
        out[n] = strdup(s);
        if (!out[n]) {
            fclose(f);
            return -1;
        }
        n++;
    }
    fclose(f);
    return n;
}

static int load_must_pass(const char *path, int *ids, int max_ids)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "cannot open must-pass: %s\n", path);
        return -1;
    }
    char line[128];
    int n = 0;
    while (fgets(line, sizeof(line), f)) {
        char *s = line;
        while (*s == ' ' || *s == '\t') {
            s++;
        }
        if (*s == '#' || *s == '\0' || *s == '\n') {
            continue;
        }
        if (!strncmp(s, "WAC.", 4)) {
            s += 4;
        }
        int id = atoi(s);
        if (id <= 0) {
            continue;
        }
        if (n >= max_ids) {
            fprintf(stderr, "must-pass too large\n");
            fclose(f);
            return -1;
        }
        ids[n++] = id;
    }
    fclose(f);
    return n;
}

int main(int argc, char **argv)
{
    budget_kind_t kind = BUDGET_NODES;
    unsigned long nodes = 50000UL;
    int depth = 5;
    unsigned time_ms = 1000;
    int limit = -1; /* -1 = all loaded */
    int from = 0;
    int quiet = 0;
    int kind_set = 0;
    const char *epd_path = NULL;
    const char *must_path = NULL;

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
        } else if (!strcmp(argv[i], "--epd") && i + 1 < argc) {
            epd_path = argv[++i];
        } else if (!strcmp(argv[i], "--must-pass") && i + 1 < argc) {
            must_path = argv[++i];
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            fprintf(stderr,
                    "Usage: %s [--nodes N | --depth D | --time MS]\n"
                    "          [--limit N] [--from I] [--quiet]\n"
                    "          [--epd FILE] [--must-pass FILE]\n"
                    "WAC suite (Arasan wacnew.epd FENs). --must-pass → exit 1 on miss.\n",
                    argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown arg: %s\n", argv[i]);
            return 2;
        }
    }
    (void)kind_set;

    const char *owned[MAX_EPD];
    int owned_n = 0;
    const char *const *suite = WAC_POSITIONS;
    int suite_n = WAC_POSITION_COUNT;

    if (epd_path) {
        owned_n = load_epd_file(epd_path, owned, MAX_EPD);
        if (owned_n < 0) {
            return 2;
        }
        suite = owned;
        suite_n = owned_n;
    }

    int must_ids[MAX_MUST];
    int must_n = 0;
    if (must_path) {
        must_n = load_must_pass(must_path, must_ids, MAX_MUST);
        if (must_n < 0) {
            return 2;
        }
        if (must_n == 0) {
            fprintf(stderr, "must-pass file empty: %s\n", must_path);
            return 2;
        }
    }

    if (from < 0) {
        from = 0;
    }
    if (from >= suite_n) {
        fprintf(stderr, "from=%d out of range (suite=%d)\n", from, suite_n);
        return 2;
    }
    int end = suite_n;
    if (limit >= 0 && from + limit < end) {
        end = from + limit;
    }

    /* Map WAC id → suite index for must-pass iteration. */
    int id_to_idx[MAX_EPD + 1];
    for (int i = 0; i <= MAX_EPD; i++) {
        id_to_idx[i] = -1;
    }
    for (int i = 0; i < suite_n; i++) {
        int num = epd_wac_num(suite[i]);
        if (num > 0 && num <= MAX_EPD) {
            id_to_idx[num] = i;
        }
    }

    int solved = 0;
    int tried = 0;
    int must_miss = 0;
    int must_tried = 0;
    unsigned long long nodes_sum = 0;
    const clock_t wall0 = clock();

    if (kind == BUDGET_NODES) {
        printf("WAC bench: nodes=%lu", nodes);
    } else if (kind == BUDGET_DEPTH) {
        printf("WAC bench: depth=%d", depth);
    } else {
        printf("WAC bench: time_ms=%u", time_ms);
    }
    if (must_path) {
        printf(" must-pass=%d", must_n);
    } else {
        printf(" from=%d end=%d", from, end);
    }
    if (epd_path) {
        printf(" epd=%s", epd_path);
    }
    printf("\n");

    /* Build run list: must-pass ids, or sequential suite slice. */
    int run_idx[MAX_EPD];
    int run_n = 0;
    if (must_n > 0) {
        int m_from = from;
        int m_end = must_n;
        if (limit >= 0 && m_from + limit < m_end) {
            m_end = m_from + limit;
        }
        if (m_from > must_n) {
            m_from = must_n;
        }
        for (int mi = m_from; mi < m_end; mi++) {
            int id = must_ids[mi];
            if (id < 0 || id > MAX_EPD || id_to_idx[id] < 0) {
                fprintf(stderr, "must-pass id %d not in suite\n", id);
                return 2;
            }
            run_idx[run_n++] = id_to_idx[id];
        }
    } else {
        for (int idx = from; idx < end; idx++) {
            run_idx[run_n++] = idx;
        }
    }

    for (int ri = 0; ri < run_n; ri++) {
        const int idx = run_idx[ri];
        const char *epd = suite[idx];

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
        } else if (must_n > 0) {
            must_miss++;
        }
        tried++;
        if (must_n > 0) {
            must_tried++;
        }
        nodes_sum += r.nodes;

        char a[3], b[3];
        sq_name(r.c1, a);
        sq_name(r.c2, b);

        if (!ok && must_n > 0) {
            printf("%s REGRESS played %s%s depth=%d nodes=%lu\n", id, a, b, r.depth,
                   (unsigned long)r.nodes);
        } else if (!quiet || ((ri + 1) % 10 == 0) || ri + 1 == run_n) {
            printf("%s %s %s%s depth=%d nodes=%lu %.2fs  %d/%d\n", id, ok ? "OK" : "MISS", a, b,
                   r.depth, (unsigned long)r.nodes, sec, solved, tried);
        }
    }

    const double wall = (double)(clock() - wall0) / (double)CLOCKS_PER_SEC;
    printf("\nSummary: %d/%d solved (%.1f%%)  nodes=%llu  wall=%.1fs\n", solved, tried,
           tried ? (100.0 * solved) / tried : 0.0, nodes_sum, wall);
    if (kind == BUDGET_NODES) {
        printf("Budget: %lu nodes/pos (~%.1f s ESP32 @ 20 kN/s)\n", nodes, nodes / 20000.0);
    } else if (kind == BUDGET_DEPTH) {
        printf("Budget: depth %d/pos\n", depth);
    } else {
        printf("Budget: %u ms/pos (wall-clock)\n", time_ms);
    }
    if (must_n > 0) {
        printf("Must-pass: %d/%d ok, %d regress\n", must_tried - must_miss, must_tried, must_miss);
    }
    printf("(Reference: Urusov ~272/300 @ 1 min/pos on ESP32 ≈ 1.2M nodes @ 20 kN/s.)\n");

    for (int i = 0; i < owned_n; i++) {
        free((void *)owned[i]);
    }

    if (must_n > 0 && must_miss > 0) {
        return 1;
    }
    return 0;
}
