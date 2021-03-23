rm -rf ./build
mkdir build
cd build

if [[ $1 -eq 1 ]]
then
	cmake -DCMAKE_BUILD_TYPE=Debug ..
	cmake --build . --target all --config Debug -- VERBOSE=1 $BUILD_FLAGS
else
	cmake -DCMAKE_BUILD_TYPE=Release ..
	cmake --build . --target all --config Release
fi
cmake --install .
