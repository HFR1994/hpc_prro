#!/usr/bin/env python3
"""Draw a cutting plan CSV (written by the solver) as a PNG, one board per row.
Usage: tools/plot_cutting_plan.py output/cutting_plan_<tag>.csv --stock dataset/stock.csv [--out plan.png]"""
import argparse
import csv
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("plan")
    ap.add_argument("--stock", required=True)
    ap.add_argument("--out")
    ap.add_argument("--cuts", help="saw-line CSV (default: the plan path with cutting_plan_ replaced by cutting_cuts_)")
    args = ap.parse_args()

    stock = {}
    for row in csv.reader(open(args.stock)):
        if not row or row[0].lstrip().startswith("#"):
            continue
        stock[row[0].strip()] = (float(row[1]), float(row[2]))

    boards = defaultdict(list)
    board_stock = {}
    for r in csv.DictReader(open(args.plan)):
        b = int(r["board"])
        board_stock[b] = r["stock"]
        boards[b].append(r)

    cuts_path = args.cuts or args.plan.replace("cutting_plan_", "cutting_cuts_")
    lines = defaultdict(list)
    try:
        for r in csv.DictReader(open(cuts_path)):
            lines[int(r["board"])].append(r)
    except OSError:
        pass

    ids = sorted(boards)
    max_len = max(stock[board_stock[b]][0] for b in ids)
    heights = [stock[board_stock[b]][1] for b in ids]
    fig_h = sum(h / 1000 * 2.2 + 0.6 for h in heights) + 0.6
    fig, axes = plt.subplots(len(ids), 1, figsize=(max_len / 1000 * 2.2 + 0.6, fig_h), dpi=130,
                             gridspec_kw={"height_ratios": [h + 150 for h in heights]})
    if len(ids) == 1:
        axes = [axes]
    total_stock = total_used = 0.0
    for ax, b in zip(axes, ids):
        L, Wd = stock[board_stock[b]]
        ax.add_patch(Rectangle((0, 0), L, Wd, fc="#e0e0e0", ec="#333", lw=1))
        used = 0.0
        for r in boards[b]:
            x, y, a, w = (float(r[k]) for k in ("x", "y", "length", "width"))
            used += a * w
            ax.add_patch(Rectangle((x, y), a, w, fc="#d9b98a", ec="#6b4a1e", lw=0.8))
            ax.text(x + a / 2, y + w / 2, f"{r['piece']}\n{a:.0f} x {w:.0f}" + (" R" if r["rotated"] == "1" else ""),
                    ha="center", va="center", fontsize=6.5)
        for r in lines[b]:
            c, s0, s1 = float(r["position"]), float(r["from"]), float(r["to"])
            if r["axis"] == "H":
                ax.plot([s0, s1], [c, c], color="#c62828", lw=1.0, ls="--")
            else:
                ax.plot([c, c], [s0, s1], color="#c62828", lw=1.0, ls="--")
        total_stock += L * Wd
        total_used += used
        ax.set_title(f"Board {b}: {board_stock[b]} ({L:.0f} x {Wd:.0f}), {100 * used / (L * Wd):.1f}% used", fontsize=9, loc="left")
        ax.set_xlim(-20, max_len + 20); ax.set_ylim(-20, Wd + 20)
        ax.set_aspect("equal"); ax.set_axis_off()
    waste = total_stock - total_used
    n_cuts = sum(len(v) for v in lines.values())
    fig.suptitle(f"{n_cuts} saw lines (red), {len(ids)} boards, waste {waste / 1e6:.3f} m2 of {total_stock / 1e6:.3f} m2 ({100 * waste / total_stock:.1f}%)", fontsize=10)
    fig.tight_layout(rect=(0, 0, 1, 0.97))
    out = args.out or args.plan.rsplit(".", 1)[0] + ".png"
    fig.savefig(out)
    print(out)


if __name__ == "__main__":
    main()
