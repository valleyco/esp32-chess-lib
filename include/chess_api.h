#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Starting position FEN (space-separated fields as accepted by the engine). */
#define CHESS_START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"

/** Piece type magnitudes on the board (±) and for promotion choice. */
typedef enum {
    CHESS_PIECE_NONE = 0,
    CHESS_PIECE_PAWN = 1,
    CHESS_PIECE_KNIGHT = 2,
    CHESS_PIECE_BISHOP = 3,
    CHESS_PIECE_ROOK = 4,
    CHESS_PIECE_QUEEN = 5,
    CHESS_PIECE_KING = 6,
} chess_piece_t;

/**
 * Promotion argument for chess_try_move / legal moves.
 * 0 means queen (default). Otherwise pass CHESS_PIECE_KNIGHT..QUEEN.
 */
typedef enum {
    CHESS_PROMO_QUEEN_DEFAULT = 0,
    CHESS_PROMO_KNIGHT = CHESS_PIECE_KNIGHT,
    CHESS_PROMO_BISHOP = CHESS_PIECE_BISHOP,
    CHESS_PROMO_ROOK = CHESS_PIECE_ROOK,
    CHESS_PROMO_QUEEN = CHESS_PIECE_QUEEN,
} chess_promo_t;

typedef enum {
    CHESS_STATUS_OK = 0,
    CHESS_STATUS_CHECKMATE,
    CHESS_STATUS_STALEMATE,
} chess_status_t;

typedef struct {
    int c1;
    int c2;
    /** 0 if not a promotion; else CHESS_PIECE_KNIGHT..QUEEN. */
    int promo;
} chess_move_t;

typedef struct {
    int c1;
    int c2;
    int promo;
    int depth;
    unsigned long nodes;
    int score;
} chess_search_result_t;

/**
 * Engine API ownership:
 * Single-threaded by default. On ESP-IDF, entry points take an internal mutex;
 * the app should still treat the think worker as the sole caller of chess_think*
 * while the UI task only calls other chess_* when think is idle.
 * Standalone lib consumers: one owner thread, or provide your own locking.
 */

void chess_new_game(void);

/** Load FEN; resets ply/history. Returns false on parse failure. */
bool chess_set_fen(const char *fen);

/**
 * Write current FEN into buf (NUL-terminated).
 * Returns bytes that would be written excluding NUL (like snprintf), or -1.
 */
int chess_get_fen(char *buf, size_t buflen);

/**
 * Apply a legal move for the side to move.
 * promo: CHESS_PROMO_QUEEN_DEFAULT (0) or CHESS_PIECE_KNIGHT..QUEEN.
 */
bool chess_try_move(int c1, int c2, int promo);

/** True if c1→c2 is a legal promotion for the side to move. */
bool chess_is_promotion_move(int c1, int c2);

/**
 * Fill out[] with legal moves for the side to move (including promo variants).
 * Returns number of moves written (capped at max_out).
 */
int chess_legal_moves(chess_move_t *out, int max_out);

/** Search up to timeout_ms, apply best move. out may be NULL. */
bool chess_think_time(unsigned timeout_ms, chess_search_result_t *out);

/**
 * Iterative deepen until ~max_nodes (engine node counter), ignore wall clock.
 * Applies best move. out may be NULL. Prefer this over think_time for
 * host/CI strength checks (CPU-independent effort budget).
 */
bool chess_think_nodes(unsigned long max_nodes, chess_search_result_t *out);

/**
 * Iterative deepen up to max_depth (engine levels), ignore wall clock.
 * Applies best move. out may be NULL. max_depth clamped to 2..20.
 */
bool chess_think_depth(int max_depth, chess_search_result_t *out);

/** Same as chess_think_time(timeout_ms, NULL). */
bool chess_think(unsigned timeout_ms);

/**
 * Parse "bm ..." from the string last passed to chess_set_fen (full EPD line).
 * Fills match candidates for chess_matches_epd_bm. Also arms search early-exit
 * hints unless chess_epd_clear_solve_hints() is called afterward.
 * Returns number of accepted best-move candidates (0 if none / parse failed).
 */
int chess_epd_load_bm(void);

/**
 * Clear engine early-exit hints (bm still usable via chess_matches_epd_bm).
 * Use for honest regression benches so search cannot stop early on a lucky match.
 */
void chess_epd_clear_solve_hints(void);

/** True if (c1,c2,promo) matches a move loaded by chess_epd_load_bm. */
bool chess_matches_epd_bm(int c1, int c2, int promo);

/** Undo one half-move (one ply). False if ply==0. */
bool chess_undo_ply(void);

/** Undo last human+engine pair when possible (game_ply > 1). Uses undo_ply×2. */
bool chess_undo(void);

/** Board square value at index 0..63 (a8=0 … h1=63); ±1 pawn … ±6 king. */
int chess_get_square(int i);

/** 1 if White to move, 0 if Black. */
int chess_side_to_move(void);

/** Half-moves played in the current game line. */
int chess_ply(void);

/**
 * Last applied half-move (game_steps[ply-1]).
 * Returns false if no moves yet; otherwise writes c1/c2 (a8=0 … h1=63).
 */
bool chess_last_move(int *c1, int *c2);

/** Mate / stalemate for the side to move. */
chess_status_t chess_status(void);

/**
 * Transposition-table RAM: bytes used, soft budget, and entry count.
 * Host tests assert chess_tt_bytes() <= chess_tt_budget_bytes() (default 32 KiB
 * table vs 48 KiB budget). Override with -DCHESS_TT_ENTRIES=N (power of 2).
 */
size_t chess_tt_bytes(void);
size_t chess_tt_budget_bytes(void);
unsigned chess_tt_entries(void);

#ifdef __cplusplus
}
#endif
