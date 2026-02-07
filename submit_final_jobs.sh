#!/bin/bash

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
THREADS=(2 4 6)
RANKS_PER_NODE=32
DATASETS=(256 512 1024 2048)
PLACE="scatter"

WALLTIME="02:00:00"
MEM_PER_JOB="4gb"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

echo "Building project on compute node..."
cd "${SCRIPT_DIR}/parallel/full" || exit 1
./build.sh || exit 1
cd "${SCRIPT_DIR}" || exit 1

# -------------------------------
# Paths (relative to project root)
# -------------------------------
APP="${SCRIPT_DIR}/parallel/full/bin/rrap_full"
TRIAL="${SCRIPT_DIR}/logs/openmp"
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

for THREAD in "${THREADS[@]}"; do
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

	 OUTFILE="${OUTDIR}/exec_timings_${PLACE}_np${NP}_threads_${THREAD}_iter1000_pop${DATASET}_feat200.log"
                           
         # Skip if file exists and is not empty
      	 if [ -s "${OUTFILE}" ]; then
            echo "Skipping ${OUTFILE} (already exists and not empty)"
            continue
         fi

         JOBNAME="rra_d${DATASET}_np${NP}_t_${THREAD}"
         
         rm -rf "${PBS_OUTPUT}/${JOBNAME}.o"
         rm -rf "${PBS_ERR}/${JOBNAME}.e"

      	 qsub \
           -N "${JOBNAME}" \
           -o "${PBS_OUTPUT}/${JOBNAME}.o" \
           -e "${PBS_ERR}/${JOBNAME}.e" \
           -l "select=${NODES}:ncpus=${NCPUS}:mem=${MEM_PER_JOB}:mpiprocs=${NCPUS}" \
           -l "place=${PLACE}" \
           -v NP=${NP},THREADS=${THREAD},NODES=${NODES},PLACE=${PLACE},APP=${APP},DATASET=${DATASET_NAME},OUTDIR=${OUTDIR} \
           "${PBS_SCRIPT}"

      	 echo "Submitted ${JOBNAME} (nodes=${NODES})"
       done
    done
done

# Sleep 10 seconds
wait_for_execution

