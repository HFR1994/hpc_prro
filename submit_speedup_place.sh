#!/bin/bash

# -------------------------------
# Experiment parameters
# -------------------------------
PROCS=(1 2 4 8)
PLACES=("pack" "scatter")

QUEUE="short_cpuQ"
WALLTIME="00:20:00"
MEM_PER_JOB="4gb"
MODULES=("mpich-3.2")

echo "Building project on compute node..."
./parallel/build.sh || exit 1

# -------------------------------
# Paths (relative to project root)
# -------------------------------
APP="./parallel/bin/rra_parallel"
DATASET="./datasets/random-128-100.csv"
OUTDIR="./output"

# -------------------------------
# Algorithm parameters
# -------------------------------
LOWER="-600"
UPPER="600"
ITER="1000"
FLIGHT="50"
LOOKOUT="10"
RADIUS="1.0"

mkdir -p pbs_scripts
mkdir -p logs/output
mkdir -p logs/error
mkdir -p output

for PLACE in "${PLACES[@]}"; do
  for NP in "${PROCS[@]}"; do

    JOBSCRIPT="pbs_scripts/rra_${PLACE}_np${NP}.pbs"

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
for MODULE in "${MODULES[@]}"; do
  module load "${MODULE}"
done

export MEASURE_SPEEDUP=1
export PRRO_SEED=42
export PRRO_PLACEMENT=${PLACE}

mpiexec -n ${NP} \\
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
