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

    FIELD_WIDTH = 18     # total characters per number
    PRECISION   = 13     # digits after decimal
    DELIM       = ','    # single-byte delimiter
    NEWLINE     = '\n'   # enforce LF only

    FMT = f"{{:+0{FIELD_WIDTH}.{PRECISION}f}}"

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

    lb = args.lb
    ub = args.ub
    population = args.pop
    features = args.variables

    expected_line_len = (
            features * FIELD_WIDTH +
            (features - 1) * len(DELIM) +
            len(NEWLINE)
    )

    filename = f"random-{population}-{features}.csv"
    with open(filename, "w", newline="") as f:
        for _ in range(population):
            row = [
                FMT.format(random.uniform(lb, ub))
                for _ in range(features)
            ]

            line = DELIM.join(row) + NEWLINE

            # HARD ASSERT: all lines identical length
            if len(line) != expected_line_len:
                raise RuntimeError(
                    f"Line length mismatch: {len(line)} != {expected_line_len}"
                )

            f.write(line)

    print(f"Generated {filename}")
    print(f"Each line = {expected_line_len} bytes")


if __name__ == "__main__":
    main()
