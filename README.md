udpPacketManager
================
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4249771.svg)](https://doi.org/10.5281/zenodo.4249771)

udpPacketManager is a C library developed to handle reading and processing packet streams from international LOFAR
stations. It is used at the Irish LOFAR station (I-LOFAR) in conjunction with Olaf Wucknitz's (MPIfRA) VLBI recording
software, but in principle can be used with any packet capture that keeps the last 16 bytes of the UDP header attached
to CEP packets.

This library allows for the entire or partial extraction and processing of LOFAR CEP packet streams, re-aligning data to
account for packet loss or misalignment on the first packet, to produce one of several data products, ranging from raw
voltages (reordered or not) to a full Stokes vector output.

A guide on how to integrate the software in your project is provided in the [**
README_INTEGRATION.md**](docs/README_INTEGRATION.md) file, and example implementations can be found in the provided [**
lofar_cli_extractor.c**](src/CLI/lofar_cli_extractor.c) and my fork of Cees Bassa's coherent dedispersion GPU
software [CDMT](https://github.com/David-McKenna/cdmt).

Caveats & TODOs
-------

While using the library, do be aware

- CEP packets that are recorded out of order may cause issues, the best way to handle them has not been determined so
  they are currently skipped
- The provided python dummy data script tends to generate errors in the output after around 5,000 packets are generated

Future work should not break the exiting load/process/output loop, and may consist of

- Creating a wrapper python library to allow for easer interfacing within python scripts rather than requiring a C
  program (pybind11?)
- Support more decompression algorithms (arbitrary reader input struct?)

Requirements
------------

### Building / Using the Library
- A modern C and C++ compiler with OpenMP 4.5 and C++17 support (gcc/g++-10 used for development, icc/icpc-2021.01 used
  in production)
- A modern CMake version (>3.14, can be installed with pip)
- CSH, autoconf, libtool for PSRDADA compile
- HDF5 user library and development headers

The library will build fixed version of [Zstandard](https://github.com/facebook/zstd) and [PSRDADA](http://psrdada.sourceforge.net/) which have been tested and found to work. Furuther/past versions can be forced by modifying the [**CMakeLists.txt**](./CMakeLists.txt) file.

To automatically install the dependencies on Debian-based systems, the following commands should suffice.
```shell
apt-get install git autoconf csh libhdf5 libhdf5-dev libtool wget
pip install cmake
```

While we try to ensure full support for both gcc and icc (LLVM derivatives are not tested at the moment), they have
different performance profiles. Due to differences in the OpenMP libraries between GCC GOMP and Intel's Classic OpenMP,
compiling with ICC (not icx) has demonstrated significant performance improvements and advised as the compiler as a
result. Some sample execution times for working on a 1200 second block of compressed data using an Intel Xeon Gold 6130
on version 0.6 using processing mode 154 (Full Stokes Vector, 16x decimation), with and without dreamBeam corrections
applied to the data.

```
v0.6 GCC gcc version 9.3.0 (Ubuntu 9.3.0-11ubuntu0~18.04.1):
dreamBeam: 	Total Read Time:	293.75		Total CPU Ops Time:	376.31	Total Write Time:	0.01
No dreamBeam: 	Total Read Time:	282.78		Total CPU Ops Time:	164.26	Total Write Time:	0.01

v0.6 ICC icc version 2021.1 Beta (gcc version 7.5.0 compatibility):
dreamBeam:	Total Read Time:	290.89		Total CPU Ops Time:	66.14	Total Write Time:	0.01
No dreamBeam:	Total Read Time:	285.35		Total CPU Ops Time:	49.42	Total Write Time:	0.01
```

Performance can be improved in the GCC path by modifying the default THREADS variable to be between 8 and the number of
raw cores (not including hyper-threads) per CPU installed in your machine, though including too many threads causes
performance degradation extremely quickly. The number of threads can be set at run time as well.

The Intel OpenMP library is most easily downloaded by using a modern version of Clang, installing ICC (is now free using the [Intel oneAPI repositories](https://software.intel.com/content/www/us/en/develop/articles/installing-intel-oneapi-toolkits-via-apt.html))
, or just installing the specific IOMP library via oneAPI (`intel-oneapi-openmp intel-oneapi-runtime-openmp`).

#### Using ICC built objects with GCC/NVCC

While ICC offers significant performance improvements, if downstream objects cannot be compiled with ICC/ICPC, you will
need to include extra flags to link in the Intel libraries as they cannot be statically included. As a result, these
flags need to be included.

In the case of NVCC these need to be passed with "-Xlinker" so that the non-CUDA compiler is aware of them, or change
the base compiler to the Intel C++ compiler (`nvcc` tends to have a 1-2 year lag in compiler support, so this is
unlikely to be available).

```
gcc: -L$(ONEAPI_ROOT)/compiler/latest/linux/compiler/lib/intel64_lin/ -liomp5 -lirc
nvcc: -Xlinker "-L$(ONEAPI_ROOT)/compiler/latest/linux/compiler/lib/intel64_lin/ -liomp5 -lirc"
nvcc (alt): -ccbin=icpc
```

### Building / Using the Example CLI

Installing
----------
Once the pre-requisites are met, running the following set of commands will build and install the library.

```shell
mkdir build; cd build
cmake ..
cmake --build . -- -j8
cmake --install . -- -j8
```

### Calibration Installation Notes

If you are also installing the required components for polarmetric calibrations you may receive several errors from
casacore regarding missing ephemeris, leap second catalogues, etc, which can be fixed by following the following help
guide

https://casaguides.nrao.edu/index.php?title=Fixing_out_of_date_TAI_UTC_tables_%28missing_information_on_leap_seconds%29

We have automated this process, along with a few other quick fixes into the `make calibration-prep` target, though this
will only run on Debian-based distributions.


Usage
-----
Please see the [*README_INTEGRATION.md*](docs/README_INTEGRATION.md) file for a guide to implementing the library in
your software, and the [*README_CLI.md*](docs/README_CLI.md) file for the usage guide for the provided CLI.


Funding
-------
This project was written while the author was receiving funding from the Irish Research Council's Government of Ireland
Postgraduate Scholarship Program (GOIPG/2019/2798).
