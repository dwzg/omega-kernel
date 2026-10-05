#!/bin/sh
# Build the kernel with a local devkitPro installation (see docs/building.md).
# Without devkitPro, use tools/docker-build.sh instead.
set -e
: "${DEVKITPRO:=/opt/devkitpro}"
: "${DEVKITARM:=$DEVKITPRO/devkitARM}"
export DEVKITPRO DEVKITARM
cd "$(dirname "$0")"
exec make "$@"
