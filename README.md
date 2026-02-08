# Raven Roosting Optimization Algorithm

A parallel implementation of the Raven Roosting Optimization Algorithm (PRRO) using MPI and OpenMP for high-performance computing.

## Table of Contents
- [Overview](#overview)
- [What is the Raven Roosting Algorithm?](#what-is-the-raven-roosting-algorithm)
- [Parallelization Strategy](#parallelization-strategy)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Building with CMake](#building-with-cmake)
- [Usage](#usage)
- [Using the DevContainer](#using-the-devcontainer)
- [License](#license)

## Overview

This project implements the Raven Roosting Optimization Algorithm in three variants:
- **Serial**: Single-threaded implementation
- **MPI**: Distributed memory parallelization using Message Passing Interface
- **Full (MPI + OpenMP)**: Hybrid parallelization combining MPI for inter-node communication and OpenMP for intra-node parallelization

## What is the Raven Roosting Algorithm?

The Raven Roosting Optimization Algorithm is a nature-inspired metaheuristic optimization algorithm that mimics the roosting behavior of ravens. Ravens are known for their intelligence and social behavior, particularly their ability to find optimal roosting locations.

### Key Characteristics:
- **Population-based**: Uses multiple ravens (agents) to explore the search space
- **Bio-inspired**: Models natural behaviors such as:
  - Flight patterns towards promising locations
  - Lookout behavior for finding better solutions
  - Social communication between ravens
- **Iterative refinement**: Ravens progressively converge toward optimal solutions through multiple iterations
- **Exploration and exploitation**: Balances searching new areas (exploration) with refining known good solutions (exploitation)

## Parallelization Strategy

- **Scalability**: Efficient use of both distributed and shared memory architectures
- **Performance**: Significant speedup on multi-core clusters
- **Flexibility**: Can run on single machines (serial/OpenMP) or clusters (MPI/hybrid)

## Prerequisites

### System Requirements:
- **C Compiler**: GCC 7+ or compatible C11 compiler
- **CMake**: Version 3.10 or higher
- **MPI**: OpenMPI or MPICH (for parallel versions)
- **OpenMP**: Compiler support (for hybrid version)
- **Make**: Build tool

### Optional:
- **Docker**: For using the DevContainer
- **VS Code** or **JetBrains IDE**: With DevContainer support

## Installation

### 1. Clone the Repository
```bash
git clone <repository-url>
cd hpc_prro
```

### 2. Install Dependencies

#### On Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake gcc openmpi-bin libopenmpi-dev libomp-dev
```

#### On RHEL/CentOS:
```bash
sudo yum install -y gcc cmake openmpi openmpi-devel
module load mpi/openmpi-x86_64
```

#### On macOS:
```bash
brew install gcc cmake open-mpi libomp
```

## Building with CMake

### Serial Version
```bash
cd serial
mkdir -p build && cd build
cmake ..
make
```
The executable will be created at `serial/bin/rra_serial`

### MPI Version
```bash
cd parallel/mpi
mkdir -p build && cd build
cmake ..
make
```
The executable will be created at `parallel/mpi/bin/rrap_mpi`

### Full (MPI + OpenMP) Version
```bash
cd parallel/full
mkdir -p build && cd build
cmake ..
make
```
The executable will be created at `parallel/full/bin/rrap_full`

### Build All Versions (from project root)
```bash
# Serial
cd serial && mkdir -p build && cd build && cmake .. && make && cd ../..

# MPI
cd parallel/mpi && mkdir -p build && cd build && cmake .. && make && cd ../..

# Full (MPI + OpenMP)
cd parallel/full && mkdir -p build && cd build && cmake .. && make && cd ../..
```

## Usage

### Serial Version
```bash
./serial/bin/rra_serial <dataset_path> <lower_bound> <upper_bound> <iterations> <flight_steps> <lookout_steps> <radius> [output_dir]
```

### MPI Version
```bash
mpirun -np <num_processes> ./parallel/mpi/bin/rrap_mpi <dataset_path> <lower_bound> <upper_bound> <iterations> <flight_steps> <lookout_steps> <radius> [output_dir]
```

### Full (MPI + OpenMP) Version
```bash
export OMP_NUM_THREADS=<num_threads>
mpirun -np <num_processes> ./parallel/full/bin/rrap_full <dataset_path> <lower_bound> <upper_bound> <iterations> <flight_steps> <lookout_steps> <radius> [output_dir]
```

### Parameters:
- `dataset_path`: Path to CSV file with initial raven population (NxM: N ravens, M dimensions)
- `lower_bound`: Lower bound for search space
- `upper_bound`: Upper bound for search space
- `iterations`: Number of optimization iterations
- `flight_steps`: Number of steps ravens take to reach destinations
- `lookout_steps`: Number of search steps per raven
- `radius`: Looking radius for exploring solutions
- `output_dir`: (Optional) Directory for output files (default: ./output)

### Environment Variables:
- `PRRO_SEED`: Set random seed for reproducibility
- `PRRO_PLACEMENT`: Process placement strategy
- `NP`: Number of processes
- `OMP_NUM_THREADS`: Number of OpenMP threads (for hybrid version)

### Example:
```bash
# Serial execution
./serial/bin/rra_serial dataset/ravens.csv -100 100 1000 50 10 5.0 ./output

# MPI with 4 processes
mpirun -np 4 ./parallel/mpi/bin/rrap_mpi dataset/ravens.csv -100 100 1000 50 10 5.0 ./output

# Hybrid: 4 MPI processes with 8 OpenMP threads each
export OMP_NUM_THREADS=8
mpirun -np 4 ./parallel/full/bin/rrap_full dataset/ravens.csv -100 100 1000 50 10 5.0 ./output
```

## Using the DevContainer

The project includes a DevContainer configuration that provides a complete development environment with all dependencies pre-installed.

### What's Included:
- Debian 13 base image
- GCC compiler and build tools
- CMake 3.10+
- OpenMPI and OpenMP libraries
- Zsh with Oh My Zsh
- Git and development utilities

### Using with VS Code:
1. Install the **Remote - Containers** extension
2. Open the project folder in VS Code
3. When prompted, click "Reopen in Container" or run the command:
   ```
   Remote-Containers: Reopen in Container
   ```
4. VS Code will build the container and set up the environment automatically
5. Once inside the container, you can build and run the project normally

### Using with JetBrains IDEs (CLion, IntelliJ):
1. Ensure Docker is installed and running
2. Open the project in your JetBrains IDE
3. The IDE will detect the `.devcontainer` configuration
4. Select "Run in DevContainer" when prompted
5. The IDE will build the container and configure the environment

### Manual Container Usage:
```bash
# Build the container
cd .devcontainer
docker build -t hpc-prro .

# Run the container
docker run -it -v $(pwd)/..:/home/vscode/hpc_unitn hpc-prro

# Inside the container, build and run as normal
cd /home/vscode/hpc_unitn
# Follow the building instructions above
```

### Benefits of Using DevContainer:
- **Reproducibility**: Consistent environment across all developers
- **No local setup**: All dependencies are containerized
- **Isolation**: Doesn't interfere with your local system
- **Quick start**: Get coding immediately without manual dependency installation

## License

See the [LICENSE](LICENSE) file for details.
