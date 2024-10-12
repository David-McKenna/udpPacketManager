rm -rf ./build
mkdir build
cd build

if [[ $1 -eq 2 ]]; then
  export CPU_OPTS="-march=x86-64"
fi

echo "Preparing build with CC=${CC} CXX=${CXX}"
cmake -DCMAKE_BUILD_TYPE=Release ..
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