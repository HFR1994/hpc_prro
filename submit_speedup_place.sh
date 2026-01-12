#!/bin/bash

git pull
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

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
DATASET="${SCRIPT_DIR}/datasets/random-128-100.csv"
OUTDIR="${SCRIPT_DIR}/output"

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
mkdir -p "${SCRIPT_DIR}/logs/output"
mkdir -p "${SCRIPT_DIR}/logs/error"
mkdir -p "${SCRIPT_DIR}/output"

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
#PBS -o logs/output/rra_${PLACE}_np${NP}.o
#PBS -e logs/error/rra_${PLACE}_np${NP}.e

cd $PBS_O_WORKDIR || exit 1

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
sleep 10
# See if jobs are finished
qstat | grep "hector.floresreynoso"

cat "${SCRIPT_DIR}/logs/error/*"