set -e

module load gcc91
module load mpich-3.2
module load cmake-3.10.2

mkdir -p build
cd build

cmake -DCMAKE_C_COMPILER=mpicc -DCMAKE_BUILD_TYPE=Release ..
make -j

