#!/usr/bin/env python3
"""Reference implementation of the random-key cutting decoder used by the C objective function.

A solution vector X has 3*n entries in [0, 1] for n pieces:
    X[i]        order key      -> pieces are placed in ascending order of this key
    X[n + i]    rotation key   -> >= 0.5 rotates a rotatable piece by 90 degrees
    X[2n + i]   stock key      -> floor(key * m) chooses which stock type to open when a new board is needed

Two decoders share the first three keys:
  shelf   first-fit shelf packing (two-stage guillotine); shelves run along the stock length and are
          stacked across its width.
  kdtree  guillotine free-rectangle packing: every placement splits its free rectangle into two children
          (a KD-tree of cuts), so cuts can nest to any depth. One extra key per piece:
    X[3n + i]  split key  -> < 0.5 splits the leftover horizontally (rip over the full width of the rectangle),
                             otherwise vertically (crosscut over its full height)
  direct  positional encoding, 4 keys per piece: x, y, board, rotated.
    X[i]        x key      -> desired x = key * (board length - piece length)
    X[n + i]    y key      -> desired y = key * (board width - piece width)
    X[2n + i]   board key  -> flat = floor(key * rows * copies); stock row = flat // copies, copy = flat % copies
    X[3n + i]   rotation   -> >= 0.5 rotates a rotatable piece
          Pieces are inserted on their chosen board in bottom-left order of their desired position, into the
          free rectangle whose corner is closest to that position (guillotine, collision-free). The leftover is
          split along the shorter leftover axis. A piece that does not fit spills to the next copy of the same
          stock row; if no board of that row can ever hold it, to the first row that can. copies = number of pieces.
A saw kerf is left between neighbouring pieces, never against a factory edge.
A cut is one straight saw line: collinear cut segments on a board are merged into one line unless a
placed piece lies across the line between them (see merge_lines).

Usage:
  tools/cutting_decoder.py --pieces dataset/cabinet_pieces.csv --stock dataset/stock.csv [--vector v.csv]
  tools/cutting_decoder.py ... --random 2000 --seed 1      # random search, prints the best layout
"""
import argparse
import csv
import math
import random
import sys

EPS = 1e-9
UNPLACED_PENALTY = 1.0e6
W_CUT, W_BOARD, W_WASTE = 0.6, 0.3, 0.1   # weights per saw line, per board, per m2 of waste


def read_rows(path):
    with open(path, newline="") as f:
        for row in csv.reader(f):
            if not row or row[0].lstrip().startswith("#"):
                continue
            yield [c.strip() for c in row]


def load_pieces(path):
    pieces = []
    for row in read_rows(path):
        name, length, width, qty, rot = row[:5]
        for k in range(int(qty)):
            pieces.append({"name": name if int(qty) == 1 else f"{name}_{k + 1}",
                           "length": float(length), "width": float(width), "rotatable": int(rot) != 0})
    return pieces


def load_stock(path):
    stock = []
    for row in read_rows(path):
        name, length, width = row[:3]
        price = float(row[3]) if len(row) > 3 and row[3] else 0.0
        stock.append({"name": name, "length": float(length), "width": float(width), "price": price})
    return stock


def decode(X, pieces, stock, kerf=3.0, w_cut=W_CUT, w_board=W_BOARD, w_waste=W_WASTE):
    n, m = len(pieces), len(stock)
    assert len(X) == 3 * n, f"vector has {len(X)} entries, expected {3 * n}"
    order = sorted(range(n), key=lambda i: (X[i], i))
    boards = []          # {"stock": s, "shelves": [...], "next_y": y}
    placements = []      # {"piece", "board", "shelf", "x", "y", "a", "b", "rotated"}
    unplaced = []

    def orientations(p, i):
        a, b = p["length"], p["width"]
        prefer_rot = p["rotatable"] and X[n + i] >= 0.5
        first = [(b, a, True)] if prefer_rot else [(a, b, False)]
        if p["rotatable"]:
            first.append((a, b, False) if prefer_rot else (b, a, True))
        return first

    def put(bi, si, a, b, rot, i):
        board = boards[bi]
        shelf = board["shelves"][si]
        gap = kerf if shelf["used"] > 0 else 0.0
        x = shelf["used"] + gap
        placements.append({"piece": i, "board": bi, "shelf": si, "x": x, "y": shelf["y"], "a": a, "b": b, "rotated": rot})
        shelf["used"] = x + a
        shelf["pieces"].append(len(placements) - 1)

    for i in order:
        p = pieces[i]
        ors = orientations(p, i)
        done = False
        # 1. existing shelves, first fit
        for a, b, rot in ors:
            for bi, board in enumerate(boards):
                SL = stock[board["stock"]]["length"]
                for si, shelf in enumerate(board["shelves"]):
                    gap = kerf if shelf["used"] > 0 else 0.0
                    if b <= shelf["h"] + EPS and shelf["used"] + gap + a <= SL + EPS:
                        put(bi, si, a, b, rot, i)
                        done = True
                        break
                if done:
                    break
            if done:
                break
        # 2. new shelf on an existing board
        if not done:
            for a, b, rot in ors:
                for bi, board in enumerate(boards):
                    st = stock[board["stock"]]
                    y = board["next_y"]
                    if y + b <= st["width"] + EPS and a <= st["length"] + EPS:
                        board["shelves"].append({"y": y, "h": b, "used": 0.0, "pieces": []})
                        board["next_y"] = y + b + kerf
                        put(bi, len(board["shelves"]) - 1, a, b, rot, i)
                        done = True
                        break
                if done:
                    break
        # 3. open a new board: preferred stock type first, then the others in file order
        if not done:
            s_pref = 0 if not X[2 * n + i] > 0.0 else min(m - 1, int(X[2 * n + i] * m))
            for s in [s_pref] + [k for k in range(m) if k != s_pref]:
                st = stock[s]
                for a, b, rot in ors:
                    if a <= st["length"] + EPS and b <= st["width"] + EPS:
                        boards.append({"stock": s, "shelves": [{"y": 0.0, "h": b, "used": 0.0, "pieces": []}], "next_y": b + kerf})
                        put(len(boards) - 1, 0, a, b, rot, i)
                        done = True
                        break
                if done:
                    break
        if not done:
            unplaced.append(i)

    # cut segments, then areas
    segs = []
    for bi, board in enumerate(boards):
        st = stock[board["stock"]]
        for shelf in board["shelves"]:
            if shelf["y"] + shelf["h"] < st["width"] - EPS:
                segs.append((bi, 0, shelf["y"] + shelf["h"], 0.0, st["length"]))          # rip that frees this shelf
            for pi in shelf["pieces"]:
                pl = placements[pi]
                if pl["x"] + pl["a"] < st["length"] - EPS:
                    segs.append((bi, 1, pl["x"] + pl["a"], shelf["y"], shelf["y"] + shelf["h"]))   # crosscut after the piece
                if pl["b"] < shelf["h"] - EPS:
                    segs.append((bi, 0, shelf["y"] + pl["b"], pl["x"], pl["x"] + pl["a"]))         # trim to width
    return _finish(boards, placements, unplaced, segs, stock, w_cut, w_board, w_waste)


def _gap_blocked(placements, board, axis, c, g0, g1):
    """Does a piece on `board` straddle the line (axis, c) somewhere inside the gap (g0, g1)?"""
    for pl in placements:
        if pl["board"] != board:
            continue
        if axis == 0:
            if pl["y"] < c - EPS and pl["y"] + pl["b"] > c + EPS and pl["x"] < g1 - EPS and pl["x"] + pl["a"] > g0 + EPS:
                return True
        else:
            if pl["x"] < c - EPS and pl["x"] + pl["a"] > c + EPS and pl["y"] < g1 - EPS and pl["y"] + pl["b"] > g0 + EPS:
                return True
    return False


def merge_lines(placements, segs):
    """Merge collinear cut segments (board, axis, c, s0, s1) into straight saw lines."""
    lines = []
    for board, axis, c, s0, s1 in sorted(segs):
        if lines:
            cb, ca, cc, cs0, cs1 = lines[-1]
            if cb == board and ca == axis and abs(cc - c) <= EPS and (s0 <= cs1 + EPS or not _gap_blocked(placements, board, axis, c, cs1, s0)):
                lines[-1] = (cb, ca, cc, cs0, max(cs1, s1))
                continue
        lines.append((board, axis, c, s0, s1))
    return lines


def _finish(boards, placements, unplaced, segs, stock, w_cut, w_board, w_waste):
    lines = merge_lines(placements, segs)
    cuts = len(lines)
    stock_area = sum(stock[bd["stock"]]["length"] * stock[bd["stock"]]["width"] for bd in boards)
    cost = sum(stock[bd["stock"]]["price"] for bd in boards)
    piece_area = sum(pl["a"] * pl["b"] for pl in placements)
    waste = stock_area - piece_area
    fitness = w_cut * cuts + w_board * len(boards) + w_waste * waste / 1e6 + UNPLACED_PENALTY * len(unplaced)
    return {"boards": boards, "placements": placements, "unplaced": unplaced, "cuts": cuts, "lines": lines,
            "stock_area": stock_area, "piece_area": piece_area, "waste": waste, "cost": cost, "fitness": fitness}


def _orientations(p, i, X, n):
    a, b = p["length"], p["width"]
    prefer_rot = p["rotatable"] and X[n + i] >= 0.5
    ors = [(b, a, True)] if prefer_rot else [(a, b, False)]
    if p["rotatable"]:
        ors.append((a, b, False) if prefer_rot else (b, a, True))
    return ors


def decode_kdtree(X, pieces, stock, kerf=3.0, w_cut=W_CUT, w_board=W_BOARD, w_waste=W_WASTE):
    n, m = len(pieces), len(stock)
    assert len(X) == 4 * n, f"vector has {len(X)} entries, expected {4 * n}"
    order = sorted(range(n), key=lambda i: (X[i], i))
    boards = []          # {"stock": s, "free": [[x, y, w, h], ...]}
    placements = []
    unplaced = []
    segs = []

    for i in order:
        p = pieces[i]
        ors = _orientations(p, i, X, n)
        best = None      # (s1, s2, bi, ri, a, b, rot)
        # best short-side fit over every free rectangle of every open board
        for bi, board in enumerate(boards):
            for ri, (rx, ry, rw, rh) in enumerate(board["free"]):
                for a, b, rot in ors:
                    if a <= rw + EPS and b <= rh + EPS:
                        s1, s2 = min(rw - a, rh - b), max(rw - a, rh - b)
                        if best is None or s1 < best[0] - EPS or (abs(s1 - best[0]) <= EPS and s2 < best[1] - EPS):
                            best = (s1, s2, bi, ri, a, b, rot)
        if best is None:
            s_pref = 0 if not X[2 * n + i] > 0.0 else min(m - 1, int(X[2 * n + i] * m))
            for s in [s_pref] + [k for k in range(m) if k != s_pref]:
                st = stock[s]
                for a, b, rot in ors:
                    if a <= st["length"] + EPS and b <= st["width"] + EPS:
                        boards.append({"stock": s, "free": [[0.0, 0.0, st["length"], st["width"]]]})
                        best = (0.0, 0.0, len(boards) - 1, 0, a, b, rot)
                        break
                if best is not None:
                    break
        if best is None:
            unplaced.append(i)
            continue
        _, _, bi, ri, a, b, rot = best
        rx, ry, rw, rh = boards[bi]["free"].pop(ri)
        split_h = X[3 * n + i] < 0.5
        if split_h:
            right = [rx + a + kerf, ry, rw - a - kerf, b]
            top = [rx, ry + b + kerf, rw, rh - b - kerf]
        else:
            right = [rx + a + kerf, ry, rw - a - kerf, rh]
            top = [rx, ry + b + kerf, a, rh - b - kerf]
        if split_h:
            if rh - b > EPS:
                segs.append((bi, 0, ry + b, rx, rx + rw))       # rip across the rectangle
            if rw - a > EPS:
                segs.append((bi, 1, rx + a, ry, ry + b))        # crosscut inside the row
        else:
            if rw - a > EPS:
                segs.append((bi, 1, rx + a, ry, ry + rh))       # crosscut across the rectangle
            if rh - b > EPS:
                segs.append((bi, 0, ry + b, rx, rx + a))        # rip inside the column
        for r in (right, top):
            if r[2] > EPS and r[3] > EPS:
                boards[bi]["free"].append(r)
        placements.append({"piece": i, "board": bi, "shelf": 0, "x": rx, "y": ry, "a": a, "b": b, "rotated": rot,
                           "split": "H" if split_h else "V"})

    return _finish(boards, placements, unplaced, segs, stock, w_cut, w_board, w_waste)


def decode_direct(X, pieces, stock, kerf=3.0, w_cut=W_CUT, w_board=W_BOARD, w_waste=W_WASTE):
    n, m = len(pieces), len(stock)
    assert len(X) == 4 * n, f"vector has {len(X)} entries, expected {4 * n}"
    copies = n
    nbins = m * copies

    def clamp01(v):
        return 0.0 if not v > 0.0 else (0.999999 if v > 0.999999 else v)   # NaN -> 0

    def dims(i):
        p = pieces[i]
        if p["rotatable"] and X[3 * n + i] >= 0.5:
            return p["width"], p["length"], True
        return p["length"], p["width"], False

    def desired(i, s, a, b):
        st = stock[s]
        return clamp01(X[i]) * max(0.0, st["length"] - a), clamp01(X[n + i]) * max(0.0, st["width"] - b)

    # target board of every piece and placement order: by board, then desired y, x, index
    items = []
    for i in range(n):
        flat = min(nbins - 1, int(clamp01(X[2 * n + i]) * nbins))
        row, copy = flat // copies, flat % copies
        a, b, rot = dims(i)
        dx, dy = desired(i, row, a, b)
        items.append((flat, dy, dx, i))
    items.sort()

    boards = {}          # flat id -> {"stock": row, "free": [[x, y, w, h], ...]}
    placements = []
    unplaced = []
    segs = []
    order_of_boards = []

    def try_board(flat, i, a, b):
        """Best free rectangle on board `flat` for an a x b piece: closest corner to the desired position."""
        row = flat // copies
        st = stock[row]
        if flat not in boards:
            if a <= st["length"] + EPS and b <= st["width"] + EPS:
                return (row, None)
            return None
        dx, dy = desired(i, row, a, b)
        best = None
        for ri, (rx, ry, rw, rh) in enumerate(boards[flat]["free"]):
            if a <= rw + EPS and b <= rh + EPS:
                d = (rx - dx) ** 2 + (ry - dy) ** 2
                if best is None or d < best[0] - EPS:
                    best = (d, ri)
        return (row, best[1]) if best else None

    for flat, _, _, i in items:
        a, b, rot = dims(i)
        row = flat // copies
        target = None
        # own board, then the following copies of the same row
        for copy in range(flat % copies, copies):
            f = row * copies + copy
            hit = try_board(f, i, a, b)
            if hit:
                target = (f, hit[1])
                break
        if target is None:
            for r in range(m):
                if r == row:
                    continue
                for copy in range(copies):
                    f = r * copies + copy
                    hit = try_board(f, i, a, b)
                    if hit:
                        target = (f, hit[1])
                        break
                if target:
                    break
        if target is None:
            unplaced.append(i)
            continue
        f, ri = target
        row = f // copies
        st = stock[row]
        if f not in boards:
            boards[f] = {"stock": row, "free": [[0.0, 0.0, st["length"], st["width"]]]}
            order_of_boards.append(f)
            ri = 0
        rx, ry, rw, rh = boards[f]["free"].pop(ri)
        bi = order_of_boards.index(f)
        split_h = (rw - a) < (rh - b)           # shorter leftover axis
        if split_h:
            right = [rx + a + kerf, ry, rw - a - kerf, b]
            top = [rx, ry + b + kerf, rw, rh - b - kerf]
            if rh - b > EPS:
                segs.append((bi, 0, ry + b, rx, rx + rw))
            if rw - a > EPS:
                segs.append((bi, 1, rx + a, ry, ry + b))
        else:
            right = [rx + a + kerf, ry, rw - a - kerf, rh]
            top = [rx, ry + b + kerf, a, rh - b - kerf]
            if rw - a > EPS:
                segs.append((bi, 1, rx + a, ry, ry + rh))
            if rh - b > EPS:
                segs.append((bi, 0, ry + b, rx, rx + a))
        for r in (right, top):
            if r[2] > EPS and r[3] > EPS:
                boards[f]["free"].append(r)
        placements.append({"piece": i, "board": bi, "shelf": 0, "x": rx, "y": ry, "a": a, "b": b, "rotated": rot,
                           "split": "H" if split_h else "V"})

    board_list = [boards[f] for f in order_of_boards]
    return _finish(board_list, placements, unplaced, segs, stock, w_cut, w_board, w_waste)


DECODERS = {"shelf": (decode, 3), "kdtree": (decode_kdtree, 4), "direct": (decode_direct, 4)}


def describe(layout, pieces, stock):
    out = []
    for bi, board in enumerate(layout["boards"]):
        st = stock[board["stock"]]
        used = sum(pl["a"] * pl["b"] for pl in layout["placements"] if pl["board"] == bi)
        if "shelves" not in board:
            out.append(f"board {bi + 1}: {st['name']}  utilisation {100 * used / (st['length'] * st['width']):.1f}%")
            for pl in layout["placements"]:
                if pl["board"] == bi:
                    out.append(f"   {pieces[pl['piece']]['name']} {pl['a']:.0f}x{pl['b']:.0f}{'R' if pl['rotated'] else ''}"
                               f" at ({pl['x']:.0f},{pl['y']:.0f}) split {pl['split']}")
            continue
        out.append(f"board {bi + 1}: {st['name']}  utilisation {100 * used / (st['length'] * st['width']):.1f}%")
        for si, shelf in enumerate(board["shelves"]):
            names = ", ".join(f"{pieces[layout['placements'][pi]['piece']]['name']}"
                              f"({layout['placements'][pi]['a']:.0f}x{layout['placements'][pi]['b']:.0f}"
                              f"{'R' if layout['placements'][pi]['rotated'] else ''})" for pi in shelf["pieces"])
            out.append(f"   shelf {si + 1} y={shelf['y']:.0f} h={shelf['h']:.0f}: {names}")
    for board, axis, c, s0, s1 in layout["lines"]:
        out.append(f"   cut board {board + 1} {'H y' if axis == 0 else 'V x'}={c:.0f} from {s0:.0f} to {s1:.0f}")
    out.append(f"cuts {layout['cuts']}, boards {len(layout['boards'])}, waste {layout['waste'] / 1e6:.3f} m2 "
               f"({100 * layout['waste'] / layout['stock_area']:.1f}%), fitness {layout['fitness']:.4f}, "
               f"unplaced {len(layout['unplaced'])}")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pieces", required=True)
    ap.add_argument("--stock", required=True)
    ap.add_argument("--kerf", type=float, default=3.0)
    ap.add_argument("--w-cut", type=float, default=W_CUT)
    ap.add_argument("--w-board", type=float, default=W_BOARD)
    ap.add_argument("--w-waste", type=float, default=W_WASTE)
    ap.add_argument("--vector", help="CSV file with one row of 3n keys (e.g. a row of the population file)")
    ap.add_argument("--row", type=int, default=0, help="row of --vector to use")
    ap.add_argument("--random", type=int, default=0, help="evaluate this many random vectors and report the best")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--decoder", choices=sorted(DECODERS), default="kdtree")
    args = ap.parse_args()

    pieces = load_pieces(args.pieces)
    stock = load_stock(args.stock)
    n = len(pieces)
    decode_fn, keys = DECODERS[args.decoder]
    print(f"{n} pieces -> {keys * n} features ({args.decoder}), {len(stock)} stock types, "
          f"piece area {sum(p['length'] * p['width'] for p in pieces) / 1e6:.3f} m2")

    if args.vector:
        rows = list(csv.reader(open(args.vector)))
        X = [float(v) for v in rows[args.row] if v.strip()]
        layout = decode_fn(X, pieces, stock, args.kerf, args.w_cut, args.w_board, args.w_waste)
        print(describe(layout, pieces, stock))
        print(f"FITNESS {layout['fitness']:.12f}")
    if args.random:
        rng = random.Random(args.seed)
        best = None
        for _ in range(args.random):
            X = [rng.random() for _ in range(keys * n)]
            layout = decode_fn(X, pieces, stock, args.kerf, args.w_cut, args.w_board, args.w_waste)
            if best is None or layout["fitness"] < best["fitness"]:
                best = layout
        print(f"best of {args.random} random vectors:")
        print(describe(best, pieces, stock))


if __name__ == "__main__":
    main()
