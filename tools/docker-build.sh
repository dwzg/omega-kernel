#!/bin/sh
# Build the kernel inside the official devkitARM container, without
# installing devkitPro. Arguments are passed to make, e.g.:
#   tools/docker-build.sh -j8
#   tools/docker-build.sh clean
set -e
cd "$(dirname "$0")/.."
exec docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/src" -w /src \
    -e HOME=/tmp devkitpro/devkitarm:latest \
    sh -c 'git config --global --add safe.directory /src 2>/dev/null; make "$@"' make "$@"
