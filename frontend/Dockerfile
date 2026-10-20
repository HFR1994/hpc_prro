# Self-contained image: builds the MPI cutting-stock solver from the hpc_prro sources and
# serves the web front end. Build context must be the parent folder that holds both
# hpc_prro/ and frontend/ (see docker-compose.yml or build.sh).
#
#   docker build -f frontend/Dockerfile -t prro-cutting-frontend .
#   docker run --rm -p 8000:8000 prro-cutting-frontend

ARG SOLVER_SRC=hpc_prro/parallel/mpi
ARG DATASET_SRC=hpc_prro/dataset

# ---------------------------------------------------------------- build the solver
FROM debian:trixie-slim AS build
ARG SOLVER_SRC
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential cmake openmpi-bin libopenmpi-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src/mpi
COPY ${SOLVER_SRC}/CMakeLists.txt ./
COPY ${SOLVER_SRC}/main.c ./
COPY ${SOLVER_SRC}/include ./include
COPY ${SOLVER_SRC}/src ./src
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"$(nproc)"

# ---------------------------------------------------------------- runtime
FROM debian:trixie-slim
ARG DATASET_SRC
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        openmpi-bin python3 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --uid 1000 app
WORKDIR /app
COPY --from=build /src/mpi/bin/rrap_mpi /app/bin/rrap_mpi
COPY frontend/server.py /app/server.py
COPY frontend/static /app/static
COPY ${DATASET_SRC}/cabinet_pieces.csv /app/defaults/pieces.csv
COPY ${DATASET_SRC}/stock.csv /app/defaults/stock.csv
RUN mkdir -p /app/jobs && chown -R app:app /app

ENV SOLVER_BIN=/app/bin/rrap_mpi \
    DEFAULTS_DIR=/app/defaults \
    JOBS_DIR=/app/jobs \
    PORT=8000 \
    PYTHONUNBUFFERED=1 \
    OMPI_MCA_btl_vader_single_copy_mechanism=none \
    OMPI_MCA_btl=vader,self

USER app
EXPOSE 8000
VOLUME ["/app/jobs"]
CMD ["python3", "/app/server.py"]
