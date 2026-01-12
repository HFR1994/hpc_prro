#!/bin/bash

git pull

wait_for_jobs() {
  local USERNAME="hector.floresrey"

  echo "Waiting for PBS jobs of user ${USERNAME} to finish..."

  while qstat | grep -q "${USERNAME}"; do
    echo "Jobs still running... sleeping 10s"
    sleep 10
  done

  echo "No running jobs detected. Continuing."
}

# -------------------------------
# Argument handling
# -------------------------------
if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <trial_number>"
  exit 1
fi

TRIAL="$1"

# Validate: integer ≥ 1
if ! [[ "$TRIAL" =~ ^[0-9]+$ ]]; then
  echo "Error: trial_number must be a positive integer"
  exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
echo "Running trial: $TRIAL"

# -------------------------------
# Experiment parameters
# -------------------------------
PROCS=(1 2 4 8)
PLACES=("pack" "scatter")

QUEUE="short_cpuQ"
WALLTIME="00:20:00"
MEM_PER_JOB="4gb"

echo "Building project on compute node..."
cd parallel
./build.sh || exit 1
cd ..

# -------------------------------
# Paths (relative to project root)
# -------------------------------
APP="${SCRIPT_DIR}/parallel/bin/rra_parallel"
DATASET="${SCRIPT_DIR}/dataset/random-128-100.csv"
PBS_OUTPUT="${SCRIPT_DIR}/logs/trial${TRIAL}/pbs/output"
PBS_ERR="${SCRIPT_DIR}/logs/trial${TRIAL}/pbs/error"
OUTDIR="${SCRIPT_DIR}/logs/trial${TRIAL}/output"

# -------------------------------
# Algorithm parameters
# -------------------------------
LOWER="-600"
UPPER="600"
ITER="1000"
FLIGHT="10"
LOOKOUT="10"
RADIUS="600"

mkdir -p "${SCRIPT_DIR}/pbs_scripts"
mkdir -p "${PBS_OUTPUT}"
mkdir -p "${PBS_ERR}"
mkdir -p "${OUTDIR}"

for PLACE in "${PLACES[@]}"; do
  for NP in "${PROCS[@]}"; do

    JOBSCRIPT="${SCRIPT_DIR}/pbs_scripts/rra_${PLACE}_np${NP}.pbs"

    cat > "$JOBSCRIPT" <<EOF
#!/bin/bash
#PBS -N rra_${PLACE}_np${NP}
#PBS -q ${QUEUE}
#PBS -l select=1:ncpus=${NP}:mem=${MEM_PER_JOB}
#PBS -l place=${PLACE}
#PBS -l walltime=${WALLTIME}
#PBS -o ${PBS_OUTPUT}/rra_${PLACE}_np${NP}.o
#PBS -e ${PBS_ERR}/rra_${PLACE}_np${NP}.e

module purge
module load "mpich-3.2"

export MEASURE_SPEEDUP=1
export PRRO_SEED=42
export PRRO_PLACEMENT=${PLACE}

mpirun.actual -n ${NP} \\
    ${APP} \\
    ${DATASET} \\
    ${LOWER} ${UPPER} \\
    ${ITER} \\
    ${FLIGHT} \\
    ${LOOKOUT} \\
    ${RADIUS} \\
    ${OUTDIR}
EOF

    chmod +x "$JOBSCRIPT"

    echo "Submitting: placement=${PLACE}, np=${NP}"
    qsub "$JOBSCRIPT"

  done
done

# Sleep 10 seconds
wait_for_jobs

cat "${SCRIPT_DIR}/logs/error/*"