set -e

module load gcc91
module load cmake-3.10.2
module load mpich-3.2

mkdir -p build
cd build

cmake ..

make clean all

