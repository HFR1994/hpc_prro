#!/usr/bin/env bash
# Build the image with the parent folder (hpc_prro + frontend) as context.
set -euo pipefail
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
docker build -f "$DIR/Dockerfile" -t prro-cutting-frontend "$DIR/.."
echo "Run with: docker run --rm -p 8000:8000 prro-cutting-frontend"
