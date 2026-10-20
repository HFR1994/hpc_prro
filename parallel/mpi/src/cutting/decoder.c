#include "cutting/decoder.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/logger.h"

#define EPS 1e-9
#define UNPLACED_PENALTY 1.0e6

static cut_piece_t g_pieces[CUT_MAX_PIECES];
static cut_stock_t g_stock[CUT_MAX_STOCK];
static int g_n = 0;
static int g_m = 0;
static double g_kerf = 3.0;
static double g_w_cut = 0.6;        /* per saw line */
static double g_w_board = 0.3;      /* per board */
static double g_w_waste = 0.1;      /* per m2 of waste */
static int g_mode = CUT_DECODER_KDTREE;

/* ------------------------------------------------------------------------- */
/* Loading                                                                   */
/* ------------------------------------------------------------------------- */

static int split_csv(char *line, char **fields, const int max_fields) {
    int count = 0;
    char *p = line;
    while (p && count < max_fields) {
        fields[count++] = p;
        char *comma = strchr(p, ',');
        if (!comma) break;
        *comma = '\0';
        p = comma + 1;
    }
    /* trim whitespace / newline */
    for (int i = 0; i < count; i++) {
        char *s = fields[i];
        while (*s == ' ' || *s == '\t') s++;
        fields[i] = s;
        char *e = s + strlen(s);
        while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' || e[-1] == '\r')) *--e = '\0';
    }
    return count;
}

static int is_skippable(const char *line) {
    while (*line == ' ' || *line == '\t') line++;
    return *line == '\0' || *line == '\n' || *line == '\r' || *line == '#';
}

int cutting_load(const char *pieces_path, const char *stock_path, const double kerf,
                 const double w_cut, const double w_board, const double w_waste) {
    char line[512];
    char *f[8];

    g_n = 0;
    g_m = 0;
    g_kerf = kerf;
    g_w_cut = w_cut;
    g_w_board = w_board;
    g_w_waste = w_waste;

    FILE *fp = fopen(pieces_path, "r");
    if (!fp) {
        log_err("cutting: cannot open pieces file %s", pieces_path);
        return -1;
    }
    while (fgets(line, sizeof line, fp)) {
        if (is_skippable(line)) continue;
        const int nf = split_csv(line, f, 8);
        if (nf < 3) continue;
        const double length = strtod(f[1], NULL);
        const double width = strtod(f[2], NULL);
        const int qty = nf > 3 ? atoi(f[3]) : 1;
        const int rot = nf > 4 ? atoi(f[4]) : 1;
        if (length <= 0 || width <= 0 || qty <= 0) {
            log_err("cutting: bad piece line '%s'", f[0]);
            fclose(fp);
            return -1;
        }
        for (int k = 0; k < qty; k++) {
            if (g_n >= CUT_MAX_PIECES) {
                log_err("cutting: more than %d pieces", CUT_MAX_PIECES);
                fclose(fp);
                return -1;
            }
            cut_piece_t *p = &g_pieces[g_n++];
            if (qty == 1) snprintf(p->name, CUT_NAME_LEN, "%s", f[0]);
            else snprintf(p->name, CUT_NAME_LEN, "%s_%d", f[0], k + 1);
            p->length = length;
            p->width = width;
            p->rotatable = rot != 0;
        }
    }
    fclose(fp);

    fp = fopen(stock_path, "r");
    if (!fp) {
        log_err("cutting: cannot open stock file %s", stock_path);
        return -1;
    }
    while (fgets(line, sizeof line, fp)) {
        if (is_skippable(line)) continue;
        const int nf = split_csv(line, f, 8);
        if (nf < 3) continue;
        if (g_m >= CUT_MAX_STOCK) {
            log_err("cutting: more than %d stock types", CUT_MAX_STOCK);
            fclose(fp);
            return -1;
        }
        cut_stock_t *s = &g_stock[g_m++];
        snprintf(s->name, CUT_NAME_LEN, "%s", f[0]);
        s->length = strtod(f[1], NULL);
        s->width = strtod(f[2], NULL);
        s->price = nf > 3 ? strtod(f[3], NULL) : 0.0;
        if (s->length <= 0 || s->width <= 0) {
            log_err("cutting: bad stock line '%s'", f[0]);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);

    if (g_n == 0 || g_m == 0) {
        log_err("cutting: empty pieces (%d) or stock (%d) table", g_n, g_m);
        return -1;
    }
    return 0;
}

void cutting_set_decoder(const int mode) {
    g_mode = (mode == CUT_DECODER_SHELF || mode == CUT_DECODER_DIRECT) ? mode : CUT_DECODER_KDTREE;
}
int cutting_get_decoder(void) { return g_mode; }
int cutting_num_pieces(void) { return g_n; }
int cutting_num_features(void) { return (g_mode == CUT_DECODER_SHELF ? 3 : 4) * g_n; }
const cut_piece_t *cutting_piece(const int i) { return &g_pieces[i]; }
const cut_stock_t *cutting_stock(const int s) { return &g_stock[s]; }

/* ------------------------------------------------------------------------- */
/* Decoding                                                                  */
/* ------------------------------------------------------------------------- */

typedef struct { double a, b; int rot; } orient_t;

/* Map a stock key to a row index; NaN or out-of-range keys fall back to row 0 / m-1. */
static int stock_pref(const double key, const int m) {
    if (!(key > 0.0)) return 0;              /* also catches NaN */
    int s = (int)(key * m);
    return s >= m ? m - 1 : s;
}

static int orientations(const cut_piece_t *p, const double rot_key, orient_t *o) {
    const int prefer_rot = p->rotatable && rot_key >= 0.5;
    int k = 0;
    if (prefer_rot) { o[k].a = p->width; o[k].b = p->length; o[k].rot = 1; k++; }
    else            { o[k].a = p->length; o[k].b = p->width; o[k].rot = 0; k++; }
    if (p->rotatable) {
        if (prefer_rot) { o[k].a = p->length; o[k].b = p->width; o[k].rot = 0; k++; }
        else            { o[k].a = p->width; o[k].b = p->length; o[k].rot = 1; k++; }
    }
    return k;
}

static void put(cut_layout_t *L, const int bi, const int si, const orient_t *o, const int piece) {
    cut_shelf_t *sh = &L->boards[bi].shelves[si];
    const double gap = sh->used > 0 ? g_kerf : 0.0;
    cut_placement_t *pl = &L->placements[L->n_placed++];
    pl->piece = piece;
    pl->board = bi;
    pl->shelf = si;
    pl->x = sh->used + gap;
    pl->y = sh->y;
    pl->a = o->a;
    pl->b = o->b;
    pl->rotated = o->rot;
    sh->used = pl->x + o->a;
}

static void sort_order(const double *X, const int n, int *order) {
    for (int i = 0; i < n; i++) {
        int j = i;
        while (j > 0 && X[order[j - 1]] > X[i]) { order[j] = order[j - 1]; j--; }
        order[j] = i;
    }
}

/* Cut segments collected while decoding, merged into saw lines afterwards. */
typedef struct { int board, axis; double c, s0, s1; } cut_seg_t;

static void add_seg(cut_seg_t *segs, int *n, const int board, const int axis, const double c, const double s0, const double s1) {
    if (*n >= CUT_MAX_LINES) return;
    segs[*n].board = board; segs[*n].axis = axis; segs[*n].c = c; segs[*n].s0 = s0; segs[*n].s1 = s1;
    (*n)++;
}

static int seg_cmp(const void *pa, const void *pb) {
    const cut_seg_t *a = pa, *b = pb;
    if (a->board != b->board) return a->board < b->board ? -1 : 1;
    if (a->axis != b->axis) return a->axis < b->axis ? -1 : 1;
    if (a->c != b->c) return a->c < b->c ? -1 : 1;
    if (a->s0 != b->s0) return a->s0 < b->s0 ? -1 : 1;
    if (a->s1 != b->s1) return a->s1 < b->s1 ? -1 : 1;
    return 0;
}

/* Is there a placed piece on `board` straddling the line (axis, c) somewhere inside the gap (g0, g1)? */
static int gap_blocked(const cut_layout_t *L, const int board, const int axis, const double c, const double g0, const double g1) {
    for (int k = 0; k < L->n_placed; k++) {
        const cut_placement_t *pl = &L->placements[k];
        if (pl->board != board) continue;
        if (axis == 0) {   /* horizontal line y = c; piece must straddle c and overlap the gap in x */
            if (pl->y < c - EPS && pl->y + pl->b > c + EPS && pl->x < g1 - EPS && pl->x + pl->a > g0 + EPS) return 1;
        } else {           /* vertical line x = c */
            if (pl->x < c - EPS && pl->x + pl->a > c + EPS && pl->y < g1 - EPS && pl->y + pl->b > g0 + EPS) return 1;
        }
    }
    return 0;
}

/* Merge collinear segments into saw lines: same board, axis and coordinate, and no piece crossing
 * the line between consecutive segments. */
static void merge_lines(cut_layout_t *L, cut_seg_t *segs, const int nseg) {
    qsort(segs, nseg, sizeof *segs, seg_cmp);
    L->n_lines = 0;
    for (int i = 0; i < nseg; i++) {
        cut_line_t *cur = L->n_lines ? &L->lines[L->n_lines - 1] : NULL;
        const cut_seg_t *sg = &segs[i];
        if (cur && cur->board == sg->board && cur->axis == sg->axis && fabs(cur->c - sg->c) <= EPS
            && (sg->s0 <= cur->s1 + EPS || !gap_blocked(L, sg->board, sg->axis, sg->c, cur->s1, sg->s0))) {
            if (sg->s1 > cur->s1) cur->s1 = sg->s1;
            continue;
        }
        if (L->n_lines >= CUT_MAX_LINES) break;
        cut_line_t *ln = &L->lines[L->n_lines++];
        ln->board = sg->board; ln->axis = sg->axis; ln->c = sg->c; ln->s0 = sg->s0; ln->s1 = sg->s1;
    }
}

static void finish_layout(cut_layout_t *L, cut_seg_t *segs, const int nseg) {
    merge_lines(L, segs, nseg);
    const int cuts = L->n_lines;
    double stock_area = 0.0, cost = 0.0, piece_area = 0.0;
    for (int bi = 0; bi < L->n_boards; bi++) {
        const cut_stock_t *st = &g_stock[L->boards[bi].stock];
        stock_area += st->length * st->width;
        cost += st->price;
    }
    for (int k = 0; k < L->n_placed; k++) piece_area += L->placements[k].a * L->placements[k].b;
    L->n_cuts = cuts;
    L->stock_area = stock_area;
    L->piece_area = piece_area;
    L->waste_area = stock_area - piece_area;
    L->cost = cost;
    L->fitness = g_w_cut * cuts + g_w_board * L->n_boards + g_w_waste * L->waste_area / 1e6 + UNPLACED_PENALTY * L->n_unplaced;
}

/* ---- KD-tree / guillotine free-rectangle decoder ------------------------ */

static void decode_kdtree(const double *X, cut_layout_t *L) {
    const int n = g_n;
    const int m = g_m;
    int order[CUT_MAX_PIECES];
    cut_seg_t segs[CUT_MAX_LINES];
    int nseg = 0;

    memset(L, 0, sizeof *L);
    sort_order(X, n, order);

    for (int oi = 0; oi < n; oi++) {
        const int i = order[oi];
        const cut_piece_t *p = &g_pieces[i];
        orient_t ors[2];
        const int no = orientations(p, X[n + i], ors);

        /* best short-side fit over every free rectangle of every open board */
        int found = 0, b_bi = -1, b_ri = -1, b_k = -1;
        double b_s1 = 0, b_s2 = 0;
        for (int bi = 0; bi < L->n_boards; bi++) {
            const cut_board_t *bd = &L->boards[bi];
            for (int ri = 0; ri < bd->n_free; ri++) {
                const cut_rect_t *r = &bd->free_rects[ri];
                for (int k = 0; k < no; k++) {
                    if (ors[k].a <= r->w + EPS && ors[k].b <= r->h + EPS) {
                        const double d1 = r->w - ors[k].a, d2 = r->h - ors[k].b;
                        const double s1 = d1 < d2 ? d1 : d2, s2 = d1 < d2 ? d2 : d1;
                        if (!found || s1 < b_s1 - EPS || (fabs(s1 - b_s1) <= EPS && s2 < b_s2 - EPS)) {
                            found = 1; b_s1 = s1; b_s2 = s2; b_bi = bi; b_ri = ri; b_k = k;
                        }
                    }
                }
            }
        }
        if (!found && L->n_boards < CUT_MAX_BOARDS) {
            const int s_pref = stock_pref(X[2 * n + i], m);
            for (int t = -1; t < m && !found; t++) {
                const int s = t < 0 ? s_pref : t;
                if (t >= 0 && s == s_pref) continue;
                const cut_stock_t *st = &g_stock[s];
                for (int k = 0; k < no; k++) {
                    if (ors[k].a <= st->length + EPS && ors[k].b <= st->width + EPS) {
                        cut_board_t *bd = &L->boards[L->n_boards];
                        bd->stock = s;
                        bd->n_shelves = 0;
                        bd->n_free = 1;
                        bd->free_rects[0].x = 0; bd->free_rects[0].y = 0;
                        bd->free_rects[0].w = st->length; bd->free_rects[0].h = st->width;
                        found = 1; b_bi = L->n_boards; b_ri = 0; b_k = k;
                        L->n_boards++;
                        break;
                    }
                }
            }
        }
        if (!found) { L->n_unplaced++; continue; }

        cut_board_t *bd = &L->boards[b_bi];
        const cut_rect_t r = bd->free_rects[b_ri];
        const double a = ors[b_k].a, b = ors[b_k].b;
        /* remove the rectangle, keeping list order (matches the reference implementation) */
        for (int q = b_ri; q < bd->n_free - 1; q++) bd->free_rects[q] = bd->free_rects[q + 1];
        bd->n_free--;

        const int split_h = X[3 * n + i] < 0.5;
        cut_rect_t right, top;
        if (split_h) {
            right.x = r.x + a + g_kerf; right.y = r.y; right.w = r.w - a - g_kerf; right.h = b;
            top.x = r.x; top.y = r.y + b + g_kerf; top.w = r.w; top.h = r.h - b - g_kerf;
        } else {
            right.x = r.x + a + g_kerf; right.y = r.y; right.w = r.w - a - g_kerf; right.h = r.h;
            top.x = r.x; top.y = r.y + b + g_kerf; top.w = a; top.h = r.h - b - g_kerf;
        }
        if (split_h) {
            if (r.h - b > EPS) add_seg(segs, &nseg, b_bi, 0, r.y + b, r.x, r.x + r.w);   /* rip across the rectangle */
            if (r.w - a > EPS) add_seg(segs, &nseg, b_bi, 1, r.x + a, r.y, r.y + b);    /* crosscut inside the row */
        } else {
            if (r.w - a > EPS) add_seg(segs, &nseg, b_bi, 1, r.x + a, r.y, r.y + r.h);  /* crosscut across the rectangle */
            if (r.h - b > EPS) add_seg(segs, &nseg, b_bi, 0, r.y + b, r.x, r.x + a);    /* rip inside the column */
        }
        if (right.w > EPS && right.h > EPS && bd->n_free < CUT_MAX_FREE) bd->free_rects[bd->n_free++] = right;
        if (top.w > EPS && top.h > EPS && bd->n_free < CUT_MAX_FREE) bd->free_rects[bd->n_free++] = top;

        cut_placement_t *pl = &L->placements[L->n_placed++];
        pl->piece = i; pl->board = b_bi; pl->shelf = 0;
        pl->x = r.x; pl->y = r.y; pl->a = a; pl->b = b;
        pl->rotated = ors[b_k].rot;
        pl->split = split_h ? 1 : 2;
    }
    finish_layout(L, segs, nseg);
}

/* ---- direct positional decoder ------------------------------------------ */

static double clamp01(const double v) { return !(v > 0.0) ? 0.0 : (v > 0.999999 ? 0.999999 : v); }   /* NaN -> 0 */

typedef struct { int flat; double dy, dx; int i; } direct_item_t;

static int direct_item_cmp(const void *pa, const void *pb) {
    const direct_item_t *a = pa, *b = pb;
    if (a->flat != b->flat) return a->flat < b->flat ? -1 : 1;
    if (a->dy != b->dy) return a->dy < b->dy ? -1 : 1;
    if (a->dx != b->dx) return a->dx < b->dx ? -1 : 1;
    return a->i < b->i ? -1 : (a->i > b->i ? 1 : 0);
}

static void direct_dims(const double *X, const int n, const int i, double *a, double *b, int *rot) {
    const cut_piece_t *p = &g_pieces[i];
    if (p->rotatable && X[3 * n + i] >= 0.5) { *a = p->width; *b = p->length; *rot = 1; }
    else { *a = p->length; *b = p->width; *rot = 0; }
}

static void direct_desired(const double *X, const int n, const int i, const int row, const double a, const double b,
                           double *dx, double *dy) {
    const cut_stock_t *st = &g_stock[row];
    *dx = clamp01(X[i]) * fmax(0.0, st->length - a);
    *dy = clamp01(X[n + i]) * fmax(0.0, st->width - b);
}

/* Board `flat` (materialised as L->boards[slot[flat]] or not yet): returns 1 if the piece can go there,
 * with *ri the chosen free rectangle (-1 = board not open yet). */
static int direct_try_board(const cut_layout_t *L, const int *slot, const int copies, const double *X, const int n,
                            const int flat, const int i, const double a, const double b, int *ri) {
    const int row = flat / copies;
    const cut_stock_t *st = &g_stock[row];
    if (slot[flat] < 0) {
        if (a <= st->length + EPS && b <= st->width + EPS) { *ri = -1; return 1; }
        return 0;
    }
    double dx, dy;
    direct_desired(X, n, i, row, a, b, &dx, &dy);
    const cut_board_t *bd = &L->boards[slot[flat]];
    int best = -1;
    double best_d = 0;
    for (int r = 0; r < bd->n_free; r++) {
        const cut_rect_t *fr = &bd->free_rects[r];
        if (a <= fr->w + EPS && b <= fr->h + EPS) {
            const double d = (fr->x - dx) * (fr->x - dx) + (fr->y - dy) * (fr->y - dy);
            if (best < 0 || d < best_d - EPS) { best = r; best_d = d; }
        }
    }
    if (best < 0) return 0;
    *ri = best;
    return 1;
}

static void decode_direct(const double *X, cut_layout_t *L) {
    const int n = g_n;
    const int m = g_m;
    const int copies = n;
    const int nbins = m * copies;
    direct_item_t items[CUT_MAX_PIECES];
    int slot[CUT_MAX_STOCK * CUT_MAX_PIECES];   /* flat board id -> index in L->boards, -1 if not open */
    cut_seg_t segs[CUT_MAX_LINES];
    int nseg = 0;

    memset(L, 0, sizeof *L);
    for (int f = 0; f < nbins; f++) slot[f] = -1;

    for (int i = 0; i < n; i++) {
        int flat = (int)(clamp01(X[2 * n + i]) * nbins);
        if (flat >= nbins) flat = nbins - 1;
        double a, b; int rot;
        direct_dims(X, n, i, &a, &b, &rot);
        items[i].flat = flat;
        items[i].i = i;
        direct_desired(X, n, i, flat / copies, a, b, &items[i].dx, &items[i].dy);
    }
    qsort(items, n, sizeof *items, direct_item_cmp);

    for (int k = 0; k < n; k++) {
        const int i = items[k].i;
        const int flat = items[k].flat;
        const int row = flat / copies;
        double a, b; int rot;
        direct_dims(X, n, i, &a, &b, &rot);

        int target = -1, ri = -1;
        for (int copy = flat % copies; copy < copies && target < 0; copy++) {
            const int f = row * copies + copy;
            if (direct_try_board(L, slot, copies, X, n, f, i, a, b, &ri)) target = f;
        }
        for (int r = 0; r < m && target < 0; r++) {
            if (r == row) continue;
            for (int copy = 0; copy < copies && target < 0; copy++) {
                const int f = r * copies + copy;
                if (direct_try_board(L, slot, copies, X, n, f, i, a, b, &ri)) target = f;
            }
        }
        if (target < 0) { L->n_unplaced++; continue; }

        const int trow = target / copies;
        const cut_stock_t *st = &g_stock[trow];
        if (slot[target] < 0) {
            if (L->n_boards >= CUT_MAX_BOARDS) { L->n_unplaced++; continue; }
            slot[target] = L->n_boards;
            cut_board_t *nb = &L->boards[L->n_boards++];
            nb->stock = trow;
            nb->n_shelves = 0;
            nb->n_free = 1;
            nb->free_rects[0].x = 0; nb->free_rects[0].y = 0;
            nb->free_rects[0].w = st->length; nb->free_rects[0].h = st->width;
            ri = 0;
        }
        const int bi = slot[target];
        cut_board_t *bd = &L->boards[bi];
        const cut_rect_t r = bd->free_rects[ri];
        for (int q = ri; q < bd->n_free - 1; q++) bd->free_rects[q] = bd->free_rects[q + 1];
        bd->n_free--;

        const int split_h = (r.w - a) < (r.h - b);     /* shorter leftover axis */
        cut_rect_t right, top;
        if (split_h) {
            right.x = r.x + a + g_kerf; right.y = r.y; right.w = r.w - a - g_kerf; right.h = b;
            top.x = r.x; top.y = r.y + b + g_kerf; top.w = r.w; top.h = r.h - b - g_kerf;
            if (r.h - b > EPS) add_seg(segs, &nseg, bi, 0, r.y + b, r.x, r.x + r.w);
            if (r.w - a > EPS) add_seg(segs, &nseg, bi, 1, r.x + a, r.y, r.y + b);
        } else {
            right.x = r.x + a + g_kerf; right.y = r.y; right.w = r.w - a - g_kerf; right.h = r.h;
            top.x = r.x; top.y = r.y + b + g_kerf; top.w = a; top.h = r.h - b - g_kerf;
            if (r.w - a > EPS) add_seg(segs, &nseg, bi, 1, r.x + a, r.y, r.y + r.h);
            if (r.h - b > EPS) add_seg(segs, &nseg, bi, 0, r.y + b, r.x, r.x + a);
        }
        if (right.w > EPS && right.h > EPS && bd->n_free < CUT_MAX_FREE) bd->free_rects[bd->n_free++] = right;
        if (top.w > EPS && top.h > EPS && bd->n_free < CUT_MAX_FREE) bd->free_rects[bd->n_free++] = top;

        cut_placement_t *pl = &L->placements[L->n_placed++];
        pl->piece = i; pl->board = bi; pl->shelf = 0;
        pl->x = r.x; pl->y = r.y; pl->a = a; pl->b = b;
        pl->rotated = rot;
        pl->split = split_h ? 1 : 2;
    }
    finish_layout(L, segs, nseg);
}

/* ---- shelf decoder ------------------------------------------------------ */

static void decode_shelf(const double *X, cut_layout_t *L) {
    const int n = g_n;
    const int m = g_m;
    int order[CUT_MAX_PIECES];

    memset(L, 0, sizeof *L);

    sort_order(X, n, order);

    for (int oi = 0; oi < n; oi++) {
        const int i = order[oi];
        const cut_piece_t *p = &g_pieces[i];
        orient_t ors[2];
        const int no = orientations(p, X[n + i], ors);
        int done = 0;

        /* 1. existing shelves, first fit */
        for (int k = 0; k < no && !done; k++) {
            for (int bi = 0; bi < L->n_boards && !done; bi++) {
                cut_board_t *bd = &L->boards[bi];
                const double SL = g_stock[bd->stock].length;
                for (int si = 0; si < bd->n_shelves; si++) {
                    const cut_shelf_t *sh = &bd->shelves[si];
                    const double gap = sh->used > 0 ? g_kerf : 0.0;
                    if (ors[k].b <= sh->h + EPS && sh->used + gap + ors[k].a <= SL + EPS) {
                        put(L, bi, si, &ors[k], i);
                        done = 1;
                        break;
                    }
                }
            }
        }

        /* 2. new shelf on an existing board */
        for (int k = 0; k < no && !done; k++) {
            for (int bi = 0; bi < L->n_boards && !done; bi++) {
                cut_board_t *bd = &L->boards[bi];
                const cut_stock_t *st = &g_stock[bd->stock];
                if (bd->n_shelves >= CUT_MAX_SHELVES) continue;
                if (bd->next_y + ors[k].b <= st->width + EPS && ors[k].a <= st->length + EPS) {
                    cut_shelf_t *sh = &bd->shelves[bd->n_shelves];
                    sh->y = bd->next_y;
                    sh->h = ors[k].b;
                    sh->used = 0.0;
                    bd->next_y = sh->y + sh->h + g_kerf;
                    put(L, bi, bd->n_shelves, &ors[k], i);
                    bd->n_shelves++;
                    done = 1;
                }
            }
        }

        /* 3. open a new board: preferred stock type first, then the others in file order */
        if (!done && L->n_boards < CUT_MAX_BOARDS) {
            const int s_pref = stock_pref(X[2 * n + i], m);
            for (int t = -1; t < m && !done; t++) {
                const int s = t < 0 ? s_pref : t;
                if (t >= 0 && s == s_pref) continue;
                const cut_stock_t *st = &g_stock[s];
                for (int k = 0; k < no; k++) {
                    if (ors[k].a <= st->length + EPS && ors[k].b <= st->width + EPS) {
                        cut_board_t *bd = &L->boards[L->n_boards];
                        bd->stock = s;
                        bd->n_shelves = 1;
                        bd->shelves[0].y = 0.0;
                        bd->shelves[0].h = ors[k].b;
                        bd->shelves[0].used = 0.0;
                        bd->next_y = ors[k].b + g_kerf;
                        put(L, L->n_boards, 0, &ors[k], i);
                        L->n_boards++;
                        done = 1;
                        break;
                    }
                }
            }
        }

        if (!done) L->n_unplaced++;
    }

    /* cuts */
    cut_seg_t segs[CUT_MAX_LINES];
    int nseg = 0;
    for (int bi = 0; bi < L->n_boards; bi++) {
        const cut_board_t *bd = &L->boards[bi];
        const cut_stock_t *st = &g_stock[bd->stock];
        for (int si = 0; si < bd->n_shelves; si++) {
            const cut_shelf_t *sh = &bd->shelves[si];
            if (sh->y + sh->h < st->width - EPS) add_seg(segs, &nseg, bi, 0, sh->y + sh->h, 0.0, st->length);   /* rip that frees the shelf */
        }
    }
    for (int k = 0; k < L->n_placed; k++) {
        const cut_placement_t *pl = &L->placements[k];
        const cut_board_t *bd = &L->boards[pl->board];
        const cut_stock_t *st = &g_stock[bd->stock];
        const cut_shelf_t *sh = &bd->shelves[pl->shelf];
        if (pl->x + pl->a < st->length - EPS) add_seg(segs, &nseg, pl->board, 1, pl->x + pl->a, sh->y, sh->y + sh->h);  /* crosscut after the piece */
        if (pl->b < sh->h - EPS) add_seg(segs, &nseg, pl->board, 0, sh->y + pl->b, pl->x, pl->x + pl->a);                /* trim to width */
    }
    finish_layout(L, segs, nseg);
}

void cutting_decode(const double *X, cut_layout_t *L) {
    if (g_mode == CUT_DECODER_KDTREE) decode_kdtree(X, L);
    else if (g_mode == CUT_DECODER_DIRECT) decode_direct(X, L);
    else decode_shelf(X, L);
}

double cutting_fitness(const double *X) {
    cut_layout_t L;
    cutting_decode(X, &L);
    return L.fitness;
}

/* ------------------------------------------------------------------------- */
/* Reporting                                                                 */
/* ------------------------------------------------------------------------- */

int cutting_write_plan(const double *X, const char *output_dir, const char *tag) {
    cut_layout_t L;
    cutting_decode(X, &L);

    char path[1200];
    snprintf(path, sizeof path, "%s/cutting_plan_%s.csv", output_dir, tag);
    FILE *fp = fopen(path, "w");
    if (!fp) {
        log_err("cutting: cannot write %s", path);
        return -1;
    }
    fprintf(fp, "board,stock,shelf,piece,x,y,length,width,rotated,split\n");
    for (int k = 0; k < L.n_placed; k++) {
        const cut_placement_t *pl = &L.placements[k];
        fprintf(fp, "%d,%s,%d,%s,%.0f,%.0f,%.0f,%.0f,%d,%s\n", pl->board + 1, g_stock[L.boards[pl->board].stock].name,
                pl->shelf + 1, g_pieces[pl->piece].name, pl->x, pl->y, pl->a, pl->b, pl->rotated,
                pl->split == 1 ? "H" : pl->split == 2 ? "V" : "-");
    }
    fclose(fp);
    log_main("Cutting plan written to %s", path);

    snprintf(path, sizeof path, "%s/cutting_cuts_%s.csv", output_dir, tag);
    fp = fopen(path, "w");
    if (!fp) {
        log_err("cutting: cannot write %s", path);
        return -1;
    }
    fprintf(fp, "board,stock,axis,position,from,to\n");
    for (int k = 0; k < L.n_lines; k++) {
        const cut_line_t *ln = &L.lines[k];
        fprintf(fp, "%d,%s,%s,%.0f,%.0f,%.0f\n", ln->board + 1, g_stock[L.boards[ln->board].stock].name,
                ln->axis == 0 ? "H" : "V", ln->c, ln->s0, ln->s1);
    }
    fclose(fp);
    log_main("Saw lines written to %s", path);

    /* SVG: boards stacked vertically, 0.25 px per mm */
    const double S = 0.25, PAD = 20, GAPY = 40, LABEL = 18;
    double max_len = 0, total_h = PAD;
    for (int bi = 0; bi < L.n_boards; bi++) {
        const cut_stock_t *st = &g_stock[L.boards[bi].stock];
        if (st->length > max_len) max_len = st->length;
        total_h += LABEL + st->width * S + GAPY;
    }
    total_h += 60;
    snprintf(path, sizeof path, "%s/cutting_plan_%s.svg", output_dir, tag);
    fp = fopen(path, "w");
    if (!fp) {
        log_err("cutting: cannot write %s", path);
        return -1;
    }
    fprintf(fp, "<svg xmlns='http://www.w3.org/2000/svg' width='%.0f' height='%.0f' font-family='sans-serif' font-size='11'>\n",
            max_len * S + 2 * PAD, total_h);
    fprintf(fp, "<rect width='100%%' height='100%%' fill='white'/>\n");
    double y0 = PAD;
    for (int bi = 0; bi < L.n_boards; bi++) {
        const cut_board_t *bd = &L.boards[bi];
        const cut_stock_t *st = &g_stock[bd->stock];
        double used = 0;
        for (int k = 0; k < L.n_placed; k++) if (L.placements[k].board == bi) used += L.placements[k].a * L.placements[k].b;
        fprintf(fp, "<text x='%.0f' y='%.0f' font-weight='bold'>Board %d: %s (%.0f x %.0f), %.1f%% used</text>\n",
                PAD, y0 + 12, bi + 1, st->name, st->length, st->width, 100.0 * used / (st->length * st->width));
        y0 += LABEL;
        fprintf(fp, "<rect x='%.1f' y='%.1f' width='%.1f' height='%.1f' fill='#e0e0e0' stroke='#333'/>\n",
                PAD, y0, st->length * S, st->width * S);
        for (int k = 0; k < L.n_placed; k++) {
            const cut_placement_t *pl = &L.placements[k];
            if (pl->board != bi) continue;
            const double px = PAD + pl->x * S;
            const double py = y0 + (st->width - pl->y - pl->b) * S;   /* y up on the board, down in SVG */
            fprintf(fp, "<rect x='%.1f' y='%.1f' width='%.1f' height='%.1f' fill='#d9b98a' stroke='#6b4a1e'/>\n",
                    px, py, pl->a * S, pl->b * S);
            fprintf(fp, "<text x='%.1f' y='%.1f' font-size='9'>%s %.0fx%.0f%s</text>\n",
                    px + 3, py + 11, g_pieces[pl->piece].name, pl->a, pl->b, pl->rotated ? " R" : "");
        }
        for (int k = 0; k < L.n_lines; k++) {
            const cut_line_t *ln = &L.lines[k];
            if (ln->board != bi) continue;
            if (ln->axis == 0)
                fprintf(fp, "<line x1='%.1f' y1='%.1f' x2='%.1f' y2='%.1f' stroke='#c62828' stroke-width='1.2' stroke-dasharray='4 2'/>\n",
                        PAD + ln->s0 * S, y0 + (st->width - ln->c) * S, PAD + ln->s1 * S, y0 + (st->width - ln->c) * S);
            else
                fprintf(fp, "<line x1='%.1f' y1='%.1f' x2='%.1f' y2='%.1f' stroke='#c62828' stroke-width='1.2' stroke-dasharray='4 2'/>\n",
                        PAD + ln->c * S, y0 + (st->width - ln->s0) * S, PAD + ln->c * S, y0 + (st->width - ln->s1) * S);
        }
        y0 += st->width * S + GAPY;
    }
    fprintf(fp, "<text x='%.0f' y='%.0f'>%d cuts, %d boards, waste %.3f m2 (%.1f%%), pieces %d/%d placed, fitness %.4f</text>\n",
            PAD, y0 + 10, L.n_cuts, L.n_boards, L.waste_area / 1e6, 100.0 * L.waste_area / L.stock_area,
            L.n_placed, g_n, L.fitness);
    fprintf(fp, "</svg>\n");
    fclose(fp);
    log_main("Cutting layout drawing written to %s", path);

    /* summary to the log */
    int per_stock[CUT_MAX_STOCK] = {0};
    for (int bi = 0; bi < L.n_boards; bi++) per_stock[L.boards[bi].stock]++;
    for (int s = 0; s < g_m; s++) {
        if (per_stock[s]) log_main("  buy %d x %s", per_stock[s], g_stock[s].name);
    }
    log_main("  %d cuts, %d boards, waste %.3f m2 of %.3f m2 (%.1f%%), %d/%d pieces placed, fitness %.6f",
             L.n_cuts, L.n_boards, L.waste_area / 1e6, L.stock_area / 1e6, 100.0 * L.waste_area / L.stock_area,
             L.n_placed, g_n, L.fitness);
    return 0;
}
