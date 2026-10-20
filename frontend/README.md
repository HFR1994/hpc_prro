# Cutting Planner front end

A small web page that drives the MPI cutting-stock solver from `hpc_prro/parallel/mpi` (`rrap_mpi`).
You set the objective priorities, the stock boards you can buy, the pieces you need, and the raven
population; the solver runs through `mpirun` and the page shows the layout SVG, the shopping list,
and gives the plan and saw-line CSV files for download.

## Run with Docker (self-contained)

The image compiles the solver from the sibling `hpc_prro` folder and ships the server, so the build
context is the parent folder that contains both `hpc_prro/` and `frontend/`:

```bash
cd frontend
docker compose up --build          # http://localhost:8000
# or
./build.sh && docker run --rm -p 8000:8000 prro-cutting-frontend
```

`MAX_NP` caps the "MPI processes" field (default: CPUs visible in the container) and
`MAX_CONCURRENT` limits simultaneous solver runs (default 1, extra runs queue). Job folders are kept
in the `/app/jobs` volume.

## Run without Docker

Build the solver first (`cd hpc_prro/parallel/mpi && cmake -S . -B build && cmake --build build`), then:

```bash
cd frontend
./run_local.sh                     # http://localhost:8000, needs python3 and mpirun on PATH
```

## What the page does

1. **Priorities**: three sliders (fewer saw cuts, fewer boards, less waste) plus presets. They are
   normalised to weights that sum to 1 and passed as `PRRO_W_CUT`, `PRRO_W_BOARD`, `PRRO_W_WASTE`.
   Fitness = w_cut x saw lines + w_board x boards + w_waste x waste in m² (lower is better).
2. **Stock**: editable table (add, edit, remove) of board types, name/length/width/price in mm.
   Import/export uses the same CSV layout as `hpc_prro/dataset/stock.csv`.
3. **Pieces**: editable table of pieces with quantity and rotatable flag; CSV layout of
   `hpc_prro/dataset/cabinet_pieces.csv`.
4. **Solver**: population size, iterations, MPI processes, decoder (kdtree / shelf / direct),
   kerf, seed and the raven parameters (flight steps, lookout steps, radius).
5. **Result**: summary tiles, shopping list, the SVG drawing, the plan and cuts tables, download
   links for `plan.csv`, `cuts.csv` and `plan.svg`, and the solver log.

The page is prefilled with the PC cabinet pieces and stock from `hpc_prro/dataset`.

## HTTP API

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/defaults` | prefill data, CPU count, max MPI processes |
| POST | `/api/run` | start a job; body is the JSON the page sends, returns `{id, status}` |
| GET | `/api/jobs/<id>` | status, log tail and, when done, `result` (svg, plan, cuts, summary, buy) |
| GET | `/api/jobs/<id>/plan.csv`, `cuts.csv`, `plan.svg`, `solver.log` | downloads |
| POST | `/api/jobs/<id>/cancel` | terminate a running job |

For each job the server writes `pieces.csv`, `stock.csv` and a population file
`random-<pop>-<features>.csv` (features = pieces x keys per piece, fixed-width fields as
`dataset/generate.py` produces) into its job folder and runs:

```
PRRO_PLACEMENT=local PRRO_PIECES=... PRRO_STOCK=... PRRO_KERF=... PRRO_W_CUT=... PRRO_W_BOARD=... PRRO_W_WASTE=... PRRO_DECODER=... \
mpirun -np <np> rrap_mpi random-<pop>-<features>.csv 0 1 <iterations> <flight> <lookout> <radius> <job>/out
```

## Files

- `server.py`: Python standard-library HTTP server (no pip dependencies)
- `static/`: the page (`index.html`, `app.js`, `style.css`)
- `Dockerfile`, `Dockerfile.dockerignore`, `docker-compose.yml`, `build.sh`
- `run_local.sh`: run against an already built `rrap_mpi`
