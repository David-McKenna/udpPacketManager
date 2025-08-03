#!/usr/bin/env bash
rm -rf ./build

set -e

mkdir build
cd build

if [[ $1 -eq 2 ]]; then
  export CPU_OPTS="-march=x86-64"
fi

echo "Preparing build with CC=${CC} CXX=${CXX}"
cmake -DCMAKE_BUILD_TYPE=Release ..