#!/bin/bash

git pull

wait_for_execution() {
  local USERNAME="hector.floresrey"

  # White line
  echo ""
  echo "Waiting for PBS jobs of user ${USERNAME} to finish..."

  while qstat | grep -q "${USERNAME}"; do
    JOBS=$(qstat | grep "${USERNAME}" | wc -l)
    echo "Jobs still running: $JOBS jobs... sleeping 10s"
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

TRIAL_NUM="$1"

# Validate: integer ≥ 1
if ! [[ "$TRIAL_NUM" =~ ^[0-9]+$ ]]; then
  echo "Error: trial_number must be a positive integer"
  exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
echo "Running trial: $TRIAL_NUM"

# -------------------------------
# Experiment parameters
# -------------------------------
PROCS=(1 2 4 16 32 64 128)
RANKS_PER_NODE=32
EXECUTIONS=(1 2 3)
PLACES=("pack" "scatter")

QUEUE="short_cpuQ"
WALLTIME="00:20:00"
MEM_PER_JOB="4gb"

echo "Building project on compute node..."
cd "${SCRIPT_DIR}/parallel" || exit 1
./build.sh || exit 1
cd "${SCRIPT_DIR}" || exit 1

# -------------------------------
# Paths (relative to project root)
# -------------------------------
APP="${SCRIPT_DIR}/parallel/bin/rra_parallel"
DATASET="${SCRIPT_DIR}/dataset/random-128-100.csv"
TRIAL="${SCRIPT_DIR}/logs/trial${TRIAL_NUM}"
PBS_SCRIPT="${SCRIPT_DIR}/pbs_scripts/speedup.pbs"

# -------------------------------
# Algorithm parameters
# -------------------------------
LOWER="-600"
UPPER="600"
ITER="1000"
FLIGHT="10"
LOOKOUT="10"
RADIUS="600"

for EXEC in "${EXECUTIONS[@]}"; do

  # -------------------------------
  # Main loops
  # -------------------------------

  EXEC_DIR="${TRIAL}/execution${EXEC}"
  PBS_OUTPUT="${EXEC_DIR}/pbs/output"
  PBS_ERR="${EXEC_DIR}/pbs/error"
  OUTDIR="${EXEC_DIR}/output"

  mkdir -p "${PBS_OUTPUT}"
  mkdir -p "${PBS_ERR}"
  mkdir -p "${OUTDIR}"

  for PLACE in "${PLACES[@]}"; do
    for NP in "${PROCS[@]}"; do

      # Derive nodes
      if (( NP <= RANKS_PER_NODE )); then
        NODES=1
      else
        NODES=$(( (NP + RANKS_PER_NODE - 1) / RANKS_PER_NODE ))
      fi

      JOBNAME="rra_t${TRIAL_NUM}_e${EXEC}_${PLACE}_np${NP}"

      qsub \
        -N "${JOBNAME}" \
        -o "${PBS_OUTPUT}/${JOBNAME}.o" \
        -e "${PBS_ERR}/${JOBNAME}.e" \
        -l "select=${NODES}:ncpus=${RANKS_PER_NODE}:mem=${MEM_PER_JOB}" \
        -l "place=${PLACE}" \
        -v NP=${NP},NODES=${NODES},PLACE=${PLACE},TRIAL=${TRIAL_NUM},EXEC=${EXEC},APP=${APP},DATASET=${DATASET},OUTDIR=${OUTDIR} \
        "${PBS_SCRIPT}"

      echo "Submitted ${JOBNAME} (nodes=${NODES})"
    done
  done

  # Sleep 10 seconds
  wait_for_execution

  cat "${PBS_ERR}/*"

done


