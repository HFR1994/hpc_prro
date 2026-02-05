set -e

module load GCC/10.3.0
module load CMake/3.20.1-GCCcore-10.3.0
module load OpenMPI/4.1.1-GCC-10.3.0

mkdir -p build
cd build

cmake ..

make clean all

