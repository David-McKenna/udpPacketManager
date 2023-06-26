udpPacketManager
================
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4249771.svg)](https://doi.org/10.5281/zenodo.4249771)

udpPacketManager is a C library developed to handle reading and processing CEP packet streams from international Low Frequency Array (LOFAR) 
stations. It is used 
at the Irish LOFAR station (I-LOFAR, [lofar.ie](https://lofar.ie)) for online data processing in conjunction with 
[ILTDada](https://github.com/David-McKenna/ILTDada) for online data reduction to Stokes parameter science ready data products, or 
intermediate voltage formats for use with other software packages.

This library allows for entire, partial beamlet extraction and processing of LOFAR Central Processing (CEP) packet streams, re-aligning data to 
account for packet loss or misalignment on the first packet, to produce one of several data products, ranging from raw voltages (reordered 
or not) to a fully calibrated Stokes vector output.

The intended audience of this software is any observers, or users, that have raw CEP data produced by a LOFAR station and wish to convert it 
into a format usable by standard processing formats (DADA, GUPPI, etc.), or calibrated, science-ready data products (SigProc, HDF5, etc.). 

Caveats & TODOs
-------
While using the library, do be aware
- CEP packets that are recorded out of order may cause issues, the best way to handle them has not been determined, so they are currently 
  dropped. This may cause some time-major formats to change shape (though at least in our experience, packets are more likely to be lost,  
  no packets in 2022 were received out of order at IE613).

Future work should not break the existing load/process/output loop, and may consist of
- Creating a wrapper python library to allow for easer interfacing within python scripts rather than requiring a C program (pybind11?)
- Additional reader/writer format support
  - Adding support for the [Zstandard seekable format](https://github.com/facebook/zstd/blob/dev/contrib/seekable_format/zstd_seekable_compression_format.md)  
- Additional processing modes
- Additional metadata support (FITS frames?)

Requirements
------------
In order to build and use the library, we require
- A modern C and C++ compiler with at least OpenMP 4.5 and C++17 support
  - `gcc-10`/`g++-10` used for development, `clang-12` used in production
  - A range of tested `gcc` and `clang` releases can be found in the [`main.yml`](.github/workflows/main.yml) action file.
- A modern CMake version (>3.14, can be installed with pip)
  - A CMake compatible build system, such as `make` or `ninja`
  - `git` for cloning repositories
- `csh`, `autoconf`, `libtool` (for PSRDADA compile)

To avail of voltage calibration through [dreamBeam](https://github.com/2baOrNot2ba/dreamBeam), we additionally require
- A Python 3.8+ Interpreter available at compile and runtime

To automatically install the required dependencies on Debian-based systems, the following commands should suffice.
```shell
apt-get install git autoconf csh libtool make wget python3 python3-pip
pip install cmake

# Optional, for LLVM (clang) compilation
apt-get install clang libomp-dev libomp5
```

Our CMake configuration will automatically compile several dependencies (though they will not be installed to the system),
- [FFTW3](https://www.fftw.org/), an FFT library for channelisation of data
- [GoogleTest](https://github.com/google/googletest), a testing framework
- [HDF5](https://github.com/HDFGroup/hdf5), a data model
  - [bitshuffle](https://github.com/kiyo-masui/bitshuffle), a HDF5 compression filter
  - [zlib](https://github.com/madler/zlib), a HDF5 dependency
- [PSRDADA](https://psrdada.sourceforge.net/), an astronomical ringbuffer implementation
- [Zstandard](https://github.com/facebook/zstd), a compression library
- Python packages for calibration (and their dependencies, automatic installation available through CMake)
  - astropy
  - dreamBeam
  - lofarantpos

While we aim to have maximum support for common compilers, during the development of the library we have noted that the LLVM (`clang`) 
implementation of OpenMP, `libomp`, has significant performance benefits as compared to the GCC `libgomp` library bundled with GCC, 
especially for higher core count machines. As a result, we strongly recommend using a `clang` compiler when using the library for online 
processing of observations. Further details of this behaviour can be found in **[compilers.md](docs/COMPILERS.md)**.

Building and Installing the Library
-----------------------------------
Once the prerequisites are met, running the following set of commands is sufficient to build and install the library.

```shell
numThreads=8
mkdir build; cd build
cmake ..
cmake --build . -- -j${numThreads}

sudo cmake --install .

# Optional, run the test suite, after performing the install
ctest -V .
```
We provide these commands wrapped in a script at **[build.sh](build.sh)**

Further Calibration Installation Notes
--------------------------------------
You may receive several errors from casacore regarding missing ephemeris, leap second catalogues, etc. when calibration is enabled. These 
can be resolved by following 
[this help guide from the NRAO](https://casaguides.nrao.edu/index.php?title=Fixing_out_of_date_TAI_UTC_tables_%28missing_information_on_leap_seconds%29).


Docker Image
------------
A Dockerfile is provided in **[src/docker/Dockerfile](src/docker/Dockerfile)**, and can be used to build a Docker container that contains the 
latest build of the software, or as a reference for installing the software on a minimal Ubuntu image. 

Like all Docker containers, the **[docker2singularity](https://github.com/singularityhub/docker2singularity)** image can be used to convert it
into a singularity containter for a safer way to provide the software to users.

```shell
# From the root directory of this repo,
docker build -t udppacketmanager/latest . --file src/docker/Dockerfile
docker run -v /var/run/docker.sock:/var/run/docker.sock -v /tmp/test:/output --privileged -t --rm quay.io/singularity/docker2singularity udppacketmanager/latest
```

Usage
-----
A quick-start guide on how to integrate the software in your project is provided in the **[integration](docs/README_INTEGRATION.md)** readme 
file, and  example implementations can be found in the provided CLIs, for example **[lofar_cli_stokes.c](src/CLI/lofar_cli_stokes.c)**, or 
a simplified implementation can be found in **[example_processor.c](docs/examples/example_processor.c)**.

Other documentation can be found in the [docs/](docs) folder.

Contributing
------------

Issues and pull requests are welcomed through GitHub to report or resolve issues with the software, or if you wish to request/add additional 
functionality. All contributions are expected to follow the code of conduct laid out in the 
[Contributor Covenant 2.0](https://www.contributor-covenant.org/version/2/0/code_of_conduct/). Any contributions submitted will be licensed under 
the GPLv3.

Funding
-------
This project was written while the author was receiving funding from the Irish Research Council's Government of Ireland
Postgraduate Scholarship Program (GOIPG/2019/2798).