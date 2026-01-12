#!/usr/bin/env python3
"""
Simple dataset generator.
Generates pop_size rows each with variables_no columns, values uniform in [lb, ub].
Usage examples:
  python dataset/generate.py --variables 5 --pop 10 --lb -1 --ub 1 --out sample.csv --seed 42
  python dataset/generate.py 10 5 -1 1 > sample.csv  # pop variables lb ub
"""

import sys
import argparse
import csv
import random

def parse_args():
    p = argparse.ArgumentParser(description="Generate a random uniform dataset (CSV)")
    # Positional args now follow the order: pop, variables, lb, ub
    p.add_argument("pop", type=int, help="population size (number of rows)")
    p.add_argument("variables", type=int, help="number of variables (columns) per individual")
    p.add_argument("lb", type=float, help="lower bound (inclusive)")
    p.add_argument("ub", type=float, help="upper bound (inclusive)")
    p.add_argument("--seed", type=int, default=None, help="optional RNG seed for reproducibility")
    return p.parse_args()


def main():
    args = parse_args()

    if args.variables <= 0:
        print("error: variables must be > 0", file=sys.stderr)
        sys.exit(2)
    if args.pop <= 0:
        print("error: pop must be > 0", file=sys.stderr)
        sys.exit(2)
    if args.lb > args.ub:
        print("error: lb must be <= ub", file=sys.stderr)
        sys.exit(2)

    if args.seed is not None:
        random.seed(args.seed)

    # Choose output stream

    out_f = open(f"random-{args.pop}-{args.variables}.csv", "w", newline="")

    writer = csv.writer(out_f)

    lb = args.lb
    ub = args.ub
    rng = random.random

    for i in range(args.pop):
        row = [rng() * (ub - lb) + lb for _ in range(args.variables)]
        writer.writerow(row)

    out_f.close()


if __name__ == "__main__":
    main()
