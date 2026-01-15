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
#    if [ "$JOBS" -eq 1 ]; then
#      echo "$JOBS job still running in iter $1... checking configuration"
#      JOB_ID=$(qstat | grep "${USERNAME}" | awk '{print $1}' | cut -d'.' -f1)
#      JOB_INFO=$(qstat -f "${JOB_ID}")
#
#      if echo "$JOB_INFO" | grep -q "Resource_List.ncpus = 128" && \
#         echo "$JOB_INFO" | grep -q "Resource_List.nodect = 4" && \
#         echo "$JOB_INFO" | grep -q "Resource_List.place = pack:excl"; then
#        echo "Job ${JOB_ID} matches configuration. Deleting..."
#        qdel "${JOB_ID}"
#      fi
#      break
#    fi
    echo "$JOBS jobs still running in iter $1... sleeping 10s"
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

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
echo "Running trial: $TRIAL_NUM"

BUILD_DIR="parallel"
TRIAL="${SCRIPT_DIR}/logs/trial${TRIAL_NUM}"

# Determine if serial or parallel mode
if [[ "$TRIAL_NUM" == "serial" ]]; then
  BUILD_DIR="serial"
  TRIAL="${SCRIPT_DIR}/logs/serial"
else
  # Validate: integer ≥ 1
  if ! [[ "$TRIAL_NUM" =~ ^[0-9]+$ ]]; then
    echo "Error: trial_number must be a positive integer or 'serial'"
    exit 1
  fi
fi

# -------------------------------
# Experiment parameters
# -------------------------------
PROCS=(1 2 4 16 32 64)
RANKS_PER_NODE=16
EXECUTIONS=(1 2 3)
PLACES=("pack" "scatter")

WALLTIME="00:20:00"
MEM_PER_JOB="4gb"

echo "Building project on compute node..."
cd "${SCRIPT_DIR}/${BUILD_DIR}" || exit 1
./build.sh || exit 1
cd "${SCRIPT_DIR}" || exit 1

# -------------------------------
# Paths (relative to project root)
# -------------------------------
APP="${SCRIPT_DIR}/${BUILD_DIR}/bin/rra_${BUILD_DIR}"
DATASET="${SCRIPT_DIR}/dataset/random-128-100.csv"
PBS_SCRIPT="${SCRIPT_DIR}/pbs_scripts/speedup.pbs"

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

        NCPUS=$(( (NP + NODES - 1) / NODES ))

        JOBNAME="rra_t${TRIAL_NUM}_e${EXEC}_${PLACE}_np${NP}"

        qsub \
          -N "${JOBNAME}" \
          -o "${PBS_OUTPUT}/${JOBNAME}.o" \
          -e "${PBS_ERR}/${JOBNAME}.e" \
          -l "select=${NODES}:ncpus=${NCPUS}:mem=${MEM_PER_JOB}:mpiprocs=${NCPUS}" \
          -l "place=${PLACE}:excl" \
          -v NP=${NP},NODES=${NODES},PLACE=${PLACE},TRIAL=${TRIAL_NUM},EXEC=${EXEC},APP=${APP},DATASET=${DATASET},OUTDIR=${OUTDIR} \
          "${PBS_SCRIPT}"

        echo "Submitted ${JOBNAME} (nodes=${NODES})"
      done
  done

  # Sleep 10 seconds
  wait_for_execution "$EXEC"
done