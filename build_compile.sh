#!/usr/bin/env bash
set -e
cd build

if [[ $1 -eq 2 ]]; then
  export CPU_OPTS="-march=x86-64"
fi

cmake --build . --target all --config Release

cmake --install .
exitCode="$?"

if [[ $1 -gt 0 ]]; then
  ctest -V .
  exitTest="$?"
  if [[ $exitCode -eq 0 ]]; then
    exitCode="${exitTest}"
  fi
fi

exit ${exitCode}