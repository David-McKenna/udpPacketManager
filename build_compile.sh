#!/usr/bin/env bash
set -e
cd build

if [[ $1 -eq 2 ]]; then
  export CPU_OPTS="-march=x86-64"
fi

cmake --build . --target all --config Release