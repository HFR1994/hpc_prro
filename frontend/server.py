#!/usr/bin/env python3
"""
Small web front end for the PRRO cutting-stock solver (parallel/mpi/bin/rrap_mpi).

Standard library only. It serves the static page, accepts a job (objective weights,
stock, pieces, population size and solver settings), writes the CSV inputs plus a
random population file, runs the solver through mpirun in a background thread and
exposes the resulting SVG, plan CSV and saw-line CSV.

Environment:
  SOLVER_BIN    path to rrap_mpi            (default: /app/bin/rrap_mpi)
  DEFAULTS_DIR  folder with pieces.csv and stock.csv used to prefill the page
  JOBS_DIR      where job folders are created (default: /app/jobs)
  PORT          HTTP port (default: 8000)
  MAX_NP        upper limit for MPI processes (default: number of CPUs)
  MPIRUN        mpirun executable (default: mpirun)
"""

import json
import os
import random
import re
import shutil
import subprocess
import sys
import threading
import time
import uuid
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

HERE = os.path.dirname(os.path.abspath(__file__))
STATIC_DIR = os.path.join(HERE, "static")
SOLVER_BIN = os.environ.get("SOLVER_BIN", "/app/bin/rrap_mpi")
DEFAULTS_DIR = os.environ.get("DEFAULTS_DIR", os.path.join(HERE, "defaults"))
JOBS_DIR = os.environ.get("JOBS_DIR", "/app/jobs")
PORT = int(os.environ.get("PORT", "8000"))
MAX_NP = int(os.environ.get("MAX_NP", str(os.cpu_count() or 1)))
MPIRUN = os.environ.get("MPIRUN", "mpirun")

# Limits mirrored from parallel/mpi/include/cutting/decoder.h
CUT_MAX_PIECES = 256
CUT_MAX_STOCK = 16
KEYS_PER_PIECE = {"kdtree": 4, "shelf": 3, "direct": 4}

# Population file format expected by read_dataset_csv: fixed width fields, 18 chars, 13 decimals
FIELD_FMT = "{:+018.13f}"

JOBS = {}
JOBS_LOCK = threading.Lock()
RUN_LOCK = threading.Semaphore(int(os.environ.get("MAX_CONCURRENT", "1")))
ANSI = re.compile(r"\x1b\[[0-9;]*m")


# ----------------------------------------------------------------------------- helpers
def safe_name(s, fallback):
    s = re.sub(r"[^A-Za-z0-9_.\-]+", "_", str(s or "").strip()).strip("_")
    return (s or fallback)[:40]


def read_csv_rows(path, min_fields):
    rows = []
    if not os.path.exists(path):
        return rows
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            s = line.strip()
            if not s or s.startswith("#"):
                continue
            f = [x.strip() for x in s.split(",")]
            if len(f) >= min_fields:
                rows.append(f)
    return rows


def load_defaults():
    pieces = []
    for f in read_csv_rows(os.path.join(DEFAULTS_DIR, "pieces.csv"), 3):
        pieces.append({
            "name": f[0], "length": float(f[1]), "width": float(f[2]),
            "qty": int(f[3]) if len(f) > 3 else 1,
            "rotatable": (int(f[4]) != 0) if len(f) > 4 else True,
        })
    stock = []
    for f in read_csv_rows(os.path.join(DEFAULTS_DIR, "stock.csv"), 3):
        stock.append({
            "name": f[0], "length": float(f[1]), "width": float(f[2]),
            "price": float(f[3]) if len(f) > 3 else 0.0,
        })
    return {"pieces": pieces, "stock": stock, "max_np": MAX_NP, "cpus": os.cpu_count() or 1}


class BadRequest(Exception):
    pass


def num(d, key, lo, hi, cast=float, default=None):
    v = d.get(key, default)
    if v is None or v == "":
        if default is None:
            raise BadRequest(f"missing '{key}'")
        v = default
    try:
        v = cast(v)
    except (TypeError, ValueError):
        raise BadRequest(f"'{key}' must be a number")
    if v < lo or v > hi:
        raise BadRequest(f"'{key}' must be between {lo} and {hi}")
    return v


def validate(req):
    """Turn the JSON request into a clean job spec or raise BadRequest."""
    spec = {}
    w = req.get("weights") or {}
    spec["w_cut"] = num(w, "cut", 0, 1000, default=0.6)
    spec["w_board"] = num(w, "board", 0, 1000, default=0.3)
    spec["w_waste"] = num(w, "waste", 0, 1000, default=0.1)
    if spec["w_cut"] + spec["w_board"] + spec["w_waste"] <= 0:
        raise BadRequest("at least one objective weight must be positive")

    spec["kerf"] = num(req, "kerf", 0, 50, default=3.0)
    decoder = str(req.get("decoder") or "kdtree").lower()
    if decoder not in KEYS_PER_PIECE:
        raise BadRequest("decoder must be kdtree, shelf or direct")
    spec["decoder"] = decoder

    spec["pop_size"] = num(req, "pop_size", 2, 20000, int, 128)
    spec["iterations"] = num(req, "iterations", 1, 100000, int, 500)
    spec["flight_steps"] = num(req, "flight_steps", 1, 10000, int, 20)
    spec["lookout_steps"] = num(req, "lookout_steps", 1, 10000, int, 10)
    spec["radius"] = num(req, "radius", 0.000001, 1000, float, 1.0)
    spec["np"] = num(req, "np", 1, max(1, MAX_NP), int, min(4, MAX_NP))
    if spec["np"] > spec["pop_size"]:
        raise BadRequest("MPI processes cannot exceed the population size")
    seed = req.get("seed")
    spec["seed"] = None if seed in (None, "", "random") else num(req, "seed", 0, 2**63 - 1, int)

    stock = req.get("stock") or []
    if not stock:
        raise BadRequest("add at least one stock board")
    if len(stock) > CUT_MAX_STOCK:
        raise BadRequest(f"at most {CUT_MAX_STOCK} stock types")
    spec["stock"] = []
    for i, s in enumerate(stock):
        spec["stock"].append({
            "name": safe_name(s.get("name"), f"board_{i + 1}"),
            "length": num(s, "length", 1, 100000),
            "width": num(s, "width", 1, 100000),
            "price": num(s, "price", 0, 1e9, default=0.0),
        })

    pieces = req.get("pieces") or []
    if not pieces:
        raise BadRequest("add at least one piece")
    spec["pieces"] = []
    total = 0
    for i, p in enumerate(pieces):
        qty = num(p, "qty", 1, CUT_MAX_PIECES, int, 1)
        total += qty
        spec["pieces"].append({
            "name": safe_name(p.get("name"), f"piece_{i + 1}"),
            "length": num(p, "length", 1, 100000),
            "width": num(p, "width", 1, 100000),
            "qty": qty,
            "rotatable": bool(p.get("rotatable", True)),
        })
    if total > CUT_MAX_PIECES:
        raise BadRequest(f"at most {CUT_MAX_PIECES} pieces in total (quantities included), you have {total}")
    spec["num_pieces"] = total
    spec["features"] = total * KEYS_PER_PIECE[decoder]
    return spec


def write_inputs(job_dir, spec):
    with open(os.path.join(job_dir, "pieces.csv"), "w", encoding="utf-8") as fh:
        fh.write("# name,length,width,qty,rotatable\n")
        for p in spec["pieces"]:
            fh.write(f"{p['name']},{p['length']:g},{p['width']:g},{p['qty']},{1 if p['rotatable'] else 0}\n")
    with open(os.path.join(job_dir, "stock.csv"), "w", encoding="utf-8") as fh:
        fh.write("# name,length,width,price\n")
        for s in spec["stock"]:
            fh.write(f"{s['name']},{s['length']:g},{s['width']:g},{s['price']:g}\n")
    # Population: the solver reads rows/cols from the file name and fixed-width fields from the content
    pop_path = os.path.join(job_dir, f"random-{spec['pop_size']}-{spec['features']}.csv")
    rng = random.Random(spec["seed"])
    with open(pop_path, "w", encoding="utf-8", newline="") as fh:
        for _ in range(spec["pop_size"]):
            fh.write(",".join(FIELD_FMT.format(rng.uniform(0.0, 1.0)) for _ in range(spec["features"])) + "\n")
    return pop_path


SUMMARY_RE = re.compile(
    r"(\d+) cuts, (\d+) boards, waste ([\d.]+) m2 of ([\d.]+) m2 \(([\d.]+)%\), (\d+)/(\d+) pieces placed, fitness ([\d.eE+-]+)")
BUY_RE = re.compile(r"buy (\d+) x (\S+)")


def parse_log(text):
    buy = [{"qty": int(m.group(1)), "name": m.group(2)} for m in BUY_RE.finditer(text)]
    summary = None
    m = SUMMARY_RE.search(text)
    if m:
        summary = {
            "cuts": int(m.group(1)), "boards": int(m.group(2)), "waste_m2": float(m.group(3)),
            "stock_m2": float(m.group(4)), "waste_pct": float(m.group(5)),
            "placed": int(m.group(6)), "total": int(m.group(7)), "fitness": float(m.group(8)),
        }
    return buy, summary


def run_job(job):
    spec = job["spec"]
    job_dir = job["dir"]
    out_dir = os.path.join(job_dir, "out")
    os.makedirs(out_dir, exist_ok=True)
    pop_path = write_inputs(job_dir, spec)

    env = dict(os.environ)
    env.update({
        "PRRO_PLACEMENT": "local",
        "PRRO_PIECES": os.path.join(job_dir, "pieces.csv"),
        "PRRO_STOCK": os.path.join(job_dir, "stock.csv"),
        "PRRO_KERF": f"{spec['kerf']:g}",
        "PRRO_W_CUT": f"{spec['w_cut']:g}",
        "PRRO_W_BOARD": f"{spec['w_board']:g}",
        "PRRO_W_WASTE": f"{spec['w_waste']:g}",
        "PRRO_DECODER": spec["decoder"],
    })
    if spec["seed"] is not None:
        env["PRRO_SEED"] = str(spec["seed"])

    cmd = [MPIRUN, "-np", str(spec["np"])]
    if spec["np"] > (os.cpu_count() or 1):
        cmd.append("--oversubscribe")
    cmd += [SOLVER_BIN, pop_path, "0", "1", str(spec["iterations"]), str(spec["flight_steps"]),
            str(spec["lookout_steps"]), f"{spec['radius']:g}", out_dir]
    job["command"] = " ".join(cmd)

    with RUN_LOCK:
        if job["status"] == "cancelled":
            return
        job["status"] = "running"
        job["started"] = time.time()
        log_path = os.path.join(job_dir, "solver.log")
        try:
            with open(log_path, "w", encoding="utf-8") as log:
                proc = subprocess.Popen(cmd, cwd=job_dir, env=env, stdout=log, stderr=subprocess.STDOUT)
                job["proc"] = proc
                rc = proc.wait()
        except OSError as exc:
            job["status"] = "failed"
            job["error"] = f"could not start solver: {exc}"
            return
        finally:
            job["proc"] = None
        job["elapsed"] = time.time() - job["started"]
        with open(log_path, encoding="utf-8", errors="replace") as fh:
            text = ANSI.sub("", fh.read())
        job["log"] = text
        if job["status"] == "cancelled":
            return
        tag = f"local_np{spec['np']}_iter{spec['iterations']}_pop{spec['pop_size']}"
        files = {
            "svg": os.path.join(out_dir, f"cutting_plan_{tag}.svg"),
            "plan": os.path.join(out_dir, f"cutting_plan_{tag}.csv"),
            "cuts": os.path.join(out_dir, f"cutting_cuts_{tag}.csv"),
        }
        if rc != 0 or not all(os.path.exists(p) for p in files.values()):
            job["status"] = "failed"
            job["error"] = f"solver exited with code {rc}" if rc != 0 else "solver produced no output files"
            return
        result = {}
        for key, path in files.items():
            with open(path, encoding="utf-8") as fh:
                result[key] = fh.read()
            shutil.copyfile(path, os.path.join(job_dir, f"cutting_{'plan' if key == 'svg' else key}.{'svg' if key == 'svg' else 'csv'}"))
        result["buy"], result["summary"] = parse_log(text)
        job["result"] = result
        job["status"] = "done"


def job_view(job, with_result=True):
    view = {k: job.get(k) for k in ("id", "status", "error", "elapsed", "command", "created")}
    view["spec"] = {k: v for k, v in job["spec"].items() if k not in ("stock", "pieces")}
    log = job.get("log")
    if log is None and os.path.exists(os.path.join(job["dir"], "solver.log")):
        try:
            with open(os.path.join(job["dir"], "solver.log"), encoding="utf-8", errors="replace") as fh:
                log = ANSI.sub("", fh.read())
        except OSError:
            log = ""
    view["log_tail"] = "\n".join((log or "").splitlines()[-40:])
    if job.get("started"):
        view["running_for"] = time.time() - job["started"]
    if with_result and job.get("result"):
        view["result"] = job["result"]
    return view


# ----------------------------------------------------------------------------- HTTP
class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=STATIC_DIR, **kw)

    def log_message(self, fmt, *args):
        sys.stderr.write("%s - %s\n" % (self.address_string(), fmt % args))

    def send_json(self, obj, status=HTTPStatus.OK):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def send_file(self, path, ctype, download_name=None):
        if not os.path.exists(path):
            return self.send_json({"error": "not found"}, HTTPStatus.NOT_FOUND)
        with open(path, "rb") as fh:
            data = fh.read()
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        if download_name:
            self.send_header("Content-Disposition", f'attachment; filename="{download_name}"')
        self.end_headers()
        self.wfile.write(data)

    def read_json(self):
        length = int(self.headers.get("Content-Length") or 0)
        if length > 5_000_000:
            raise BadRequest("request too large")
        raw = self.rfile.read(length) if length else b"{}"
        try:
            return json.loads(raw or b"{}")
        except json.JSONDecodeError:
            raise BadRequest("invalid JSON body")

    def do_GET(self):
        path = urlparse(self.path).path
        if path == "/api/defaults":
            return self.send_json(load_defaults())
        m = re.fullmatch(r"/api/jobs/([0-9a-f]{12})(?:/(plan\.csv|cuts\.csv|plan\.svg|solver\.log|pieces\.csv|stock\.csv))?", path)
        if m:
            with JOBS_LOCK:
                job = JOBS.get(m.group(1))
            if not job:
                return self.send_json({"error": "unknown job"}, HTTPStatus.NOT_FOUND)
            if m.group(2) is None:
                return self.send_json(job_view(job))
            name = m.group(2)
            local = {"plan.csv": "cutting_plan.csv", "cuts.csv": "cutting_cuts.csv", "plan.svg": "cutting_plan.svg"}.get(name, name)
            ctype = "image/svg+xml" if name.endswith(".svg") else "text/csv" if name.endswith(".csv") else "text/plain"
            return self.send_file(os.path.join(job["dir"], local), ctype, download_name=f"{job['id']}_{name}")
        if path.startswith("/api/"):
            return self.send_json({"error": "not found"}, HTTPStatus.NOT_FOUND)
        if path == "/":
            self.path = "/index.html"
        return super().do_GET()

    def do_POST(self):
        path = urlparse(self.path).path
        try:
            if path == "/api/run":
                spec = validate(self.read_json())
                if not os.path.exists(SOLVER_BIN):
                    raise BadRequest(f"solver binary not found at {SOLVER_BIN}")
                job_id = uuid.uuid4().hex[:12]
                job_dir = os.path.join(JOBS_DIR, job_id)
                os.makedirs(job_dir, exist_ok=True)
                job = {"id": job_id, "dir": job_dir, "spec": spec, "status": "queued",
                       "created": time.time(), "proc": None, "error": None}
                with JOBS_LOCK:
                    JOBS[job_id] = job
                threading.Thread(target=run_job, args=(job,), daemon=True).start()
                return self.send_json(job_view(job), HTTPStatus.ACCEPTED)
            m = re.fullmatch(r"/api/jobs/([0-9a-f]{12})/cancel", path)
            if m:
                with JOBS_LOCK:
                    job = JOBS.get(m.group(1))
                if not job:
                    return self.send_json({"error": "unknown job"}, HTTPStatus.NOT_FOUND)
                if job["status"] in ("queued", "running"):
                    job["status"] = "cancelled"
                    proc = job.get("proc")
                    if proc:
                        proc.terminate()
                return self.send_json(job_view(job))
            return self.send_json({"error": "not found"}, HTTPStatus.NOT_FOUND)
        except BadRequest as exc:
            return self.send_json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)


def main():
    os.makedirs(JOBS_DIR, exist_ok=True)
    server = ThreadingHTTPServer(("0.0.0.0", PORT), Handler)
    print(f"PRRO cutting front end on http://0.0.0.0:{PORT}  solver={SOLVER_BIN} jobs={JOBS_DIR} max_np={MAX_NP}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
