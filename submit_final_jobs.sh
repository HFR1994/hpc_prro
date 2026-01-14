#!/bin/bash

git pull
rm -rf parallel/bin
rm -rf parallel/build

wait_for_execution() {
  local USERNAME="hector.floresrey"

  # White line
  echo ""
  echo "Waiting for PBS jobs of user ${USERNAME} to finish..."

  while qstat | grep -q "${USERNAME}"; do
    JOBS=$(qstat | grep "${USERNAME}" | wc -l)
    echo "$JOBS jobs still running in iter $1... sleeping 10s"
    sleep 10
  done

  echo "No running jobs detected. Continuing."
}

# -------------------------------
# Experiment parameters
# -------------------------------
PROCS=(16 32 64 128)
RANKS_PER_NODE=32
DATASETS=(256 512 1024 2048)
PLACE="scatter"

WALLTIME="00:40:00"
MEM_PER_JOB="4gb"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

echo "Building project on compute node..."
cd "${SCRIPT_DIR}/parallel" || exit 1
./build.sh || exit 1
cd "${SCRIPT_DIR}" || exit 1

# -------------------------------
# Paths (relative to project root)
# -------------------------------
APP="${SCRIPT_DIR}/parallel/bin/rra_parallel"
TRIAL="${SCRIPT_DIR}/logs/final"
PBS_SCRIPT="${SCRIPT_DIR}/pbs_scripts/final.pbs"

# -------------------------------
# Main loops
# -------------------------------

PBS_OUTPUT="${TRIAL}/pbs/output"
PBS_ERR="${TRIAL}/pbs/error"
OUTDIR="${TRIAL}/output"

mkdir -p "${PBS_OUTPUT}"
mkdir -p "${PBS_ERR}"
mkdir -p "${OUTDIR}"

for NP in "${PROCS[@]}"; do
    for DATASET in "${DATASETS[@]}"; do

      DATASET_NAME="${SCRIPT_DIR}/dataset/random-${DATASET}-200.csv"

      # Derive nodes
      if (( NP <= RANKS_PER_NODE )); then
        NODES=1
      else
        NODES=$(( (NP + RANKS_PER_NODE - 1) / RANKS_PER_NODE ))
      fi

      NCPUS=$(( (NP + NODES - 1) / NODES ))

      JOBNAME="rra_d${DATASET}_np${NP}"

      qsub \
        -N "${JOBNAME}" \
        -o "${PBS_OUTPUT}/${JOBNAME}.o" \
        -e "${PBS_ERR}/${JOBNAME}.e" \
        -l "select=${NODES}:ncpus=${NCPUS}:mem=${MEM_PER_JOB}:mpiprocs=${NCPUS}" \
        -l "place=${PLACE}" \
        -v NP=${NP},NODES=${NODES},PLACE=${PLACE},APP=${APP},DATASET=${DATASET_NAME},OUTDIR=${OUTDIR} \
        "${PBS_SCRIPT}"

      echo "Submitted ${JOBNAME} (nodes=${NODES})"
    done
done

# Sleep 10 seconds
wait_for_execution
