#ifndef RRA_PARALLEL_CUTTING_DECODER_H
#define RRA_PARALLEL_CUTTING_DECODER_H

/*
 * Random-key decoder for the 2D guillotine cutting problem.
 *
 * A raven position X has 3*n entries in [0,1] for n pieces:
 *   X[i]       order key     -> pieces are placed in ascending key order
 *   X[n+i]     rotation key  -> >= 0.5 rotates a rotatable piece 90 degrees
 *   X[2n+i]    stock key     -> floor(key*m) picks the stock type when a new board is opened
 *
 * Two decoders are available (cutting_set_decoder):
 *   CUT_DECODER_SHELF   first-fit shelf heuristic, two-stage guillotine cuts, 3 keys per piece
 *   CUT_DECODER_KDTREE  guillotine free-rectangle packing; every placement splits its free rectangle
 *                       into two children (a KD-tree of cuts). One extra key per piece:
 *   X[3n+i]    split key     -> < 0.5 splits the leftover horizontally, otherwise vertically
 *   CUT_DECODER_DIRECT  positional encoding, 4 keys per piece: x, y, board, rotated. The board key folds
 *                       (stock row, copy) into one value: flat = floor(key * rows * copies). Pieces are
 *                       inserted on their board in bottom-left order of the desired position into the free
 *                       rectangle closest to it; a piece that does not fit spills to the next copy.
 * All are deterministic, so the fitness is a pure function of X.
 * tools/cutting_decoder.py is the reference implementation of all three.
 */

#define CUT_DECODER_SHELF 0
#define CUT_DECODER_KDTREE 1
#define CUT_DECODER_DIRECT 2

#define CUT_MAX_PIECES 256
#define CUT_MAX_STOCK 16
#define CUT_MAX_BOARDS 64
#define CUT_MAX_SHELVES 32
#define CUT_MAX_FREE 64
#define CUT_MAX_LINES (2 * CUT_MAX_PIECES)
#define CUT_NAME_LEN 48

typedef struct {
    char name[CUT_NAME_LEN];
    double length;      // along the stock length when not rotated
    double width;
    int rotatable;
} cut_piece_t;

typedef struct {
    char name[CUT_NAME_LEN];
    double length;
    double width;
    double price;       // 0 = unknown, objective uses area
} cut_stock_t;

typedef struct {
    int piece;
    int board;
    int shelf;
    double x, y;        // lower-left corner on the board, x along length
    double a, b;        // placed size: a along length, b across width
    int rotated;
    int split;          // kdtree: 1 = horizontal, 2 = vertical; shelf: 0
} cut_placement_t;

typedef struct {
    double x, y, w, h;  // free rectangle (kdtree decoder)
} cut_rect_t;

typedef struct {
    double y, h, used;
} cut_shelf_t;

/* One straight saw line on a board. axis 0: horizontal line y = c spanning x in [s0, s1];
 * axis 1: vertical line x = c spanning y in [s0, s1]. Collinear cut segments that no piece
 * crosses are merged into one line, so "3 pieces of the same height" need one rip. */
typedef struct {
    int board;
    int axis;
    double c, s0, s1;
} cut_line_t;

typedef struct {
    int stock;
    int n_shelves;
    cut_shelf_t shelves[CUT_MAX_SHELVES];
    double next_y;
    int n_free;
    cut_rect_t free_rects[CUT_MAX_FREE];
} cut_board_t;

typedef struct {
    int n_boards;
    cut_board_t boards[CUT_MAX_BOARDS];
    int n_placed;
    cut_placement_t placements[CUT_MAX_PIECES];
    int n_unplaced;
    int n_cuts;         /* number of merged saw lines */
    int n_lines;
    cut_line_t lines[CUT_MAX_LINES];
    double stock_area;
    double piece_area;
    double waste_area;
    double cost;
    double fitness;
} cut_layout_t;

/* Load piece and stock tables (plain fopen, every rank calls it). Returns 0 on success.
 * Fitness = w_cut * cuts + w_board * boards + w_waste * waste_m2 (+ 1e6 per unplaced piece),
 * where cuts is the number of straight saw lines. Defaults 0.6 / 0.3 / 0.1. */
int cutting_load(const char *pieces_path, const char *stock_path, double kerf,
                 double w_cut, double w_board, double w_waste);
/* Select the decoder (CUT_DECODER_SHELF, CUT_DECODER_KDTREE or CUT_DECODER_DIRECT). Default is KDTREE. */
void cutting_set_decoder(int mode);
int cutting_get_decoder(void);
int cutting_num_pieces(void);
int cutting_num_features(void);      /* 3 * pieces (shelf) or 4 * pieces (kdtree, direct) */
const cut_piece_t *cutting_piece(int i);
const cut_stock_t *cutting_stock(int s);

/* Full decode of a position into a layout. */
void cutting_decode(const double *X, cut_layout_t *out);

/* Fitness only (what the ravens minimise). */
double cutting_fitness(const double *X);

/* Write <output_dir>/cutting_plan_<tag>.csv and .svg for the given position; returns 0 on success. */
int cutting_write_plan(const double *X, const char *output_dir, const char *tag);

#endif //RRA_PARALLEL_CUTTING_DECODER_H
