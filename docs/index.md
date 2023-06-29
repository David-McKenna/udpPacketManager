# Documentation Index

### User-facing documentation:
- [Main repo README](../README.md)
	- The root folder README
- [Program / CLI flags explains](./README_CLI.md)
	- An overview on how to use the two bundled programs for processing data with the library
- [Preparing data for DSPSR (or any DADA Voltage Input)](./PREPARING_DSPSR_READY_INPUTS.md)
    - An overview on how to prepare an output that can be used in the DSPSR ecosystem, or any other software that supports the DADA format 

### Further Sys-Admin Documentation
- [Compiler Recommendations](./COMPILERS.md)
	- GCC can show reduced performed as compared to the LLVM ecosystem. Further information here.

### Developers Documentation
- [On adding new processing modes](./NEW_PROCESSING_MODE.md)
	- Describing the locations and requirements needed to integrate a new output processing mode, discussing the existing functions and the
	  expected parameters and their use.
- [On using the I/O API](./README_io.md)
	- Describing integrating the I/O API for external work, or just writing outputs from the library to disk.
- [On integrating and using the library](./README_INTEGRATION.md)
	- Describing integrating the library into your own project