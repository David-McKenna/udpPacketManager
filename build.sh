rm -rf ./build
mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target all --config Release

cmake --install .

if [[ $1 -eq 1 ]]; then
  ctest -V .
fi