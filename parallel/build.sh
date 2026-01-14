set -e

module load gcc91
module load cmake-3.10.2
module load openmpi-3.0.0

mkdir -p build
cd build

cmake ..

make clean all

