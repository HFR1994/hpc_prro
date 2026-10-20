#!/usr/bin/env bash
# Run the front end without Docker against an already built solver (parallel/mpi/bin/rrap_mpi).
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="${REPO:-$DIR/../hpc_prro}"
export SOLVER_BIN="${SOLVER_BIN:-$REPO/parallel/mpi/bin/rrap_mpi}"
export DEFAULTS_DIR="${DEFAULTS_DIR:-$DIR/defaults}"
export JOBS_DIR="${JOBS_DIR:-$DIR/jobs}"
export PORT="${PORT:-8000}"
mkdir -p "$DEFAULTS_DIR"
[ -f "$DEFAULTS_DIR/pieces.csv" ] || cp "$REPO/dataset/cabinet_pieces.csv" "$DEFAULTS_DIR/pieces.csv"
[ -f "$DEFAULTS_DIR/stock.csv" ]  || cp "$REPO/dataset/stock.csv" "$DEFAULTS_DIR/stock.csv"
exec python3 "$DIR/server.py"
