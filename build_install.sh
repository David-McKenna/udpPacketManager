#!/usr/bin/env bash
set -e

cd build
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