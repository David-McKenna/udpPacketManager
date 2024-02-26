Using the CLI and Plotting the Outputs
======================================

This will be a summary of how to process the outputs of this program in one of two ways in Python: raw data handling with numpy (applies to all processing modes), or via [sigpyproc](https://github.com/pravirkr/sigpyproc). The numpy approach works best for modes below 100 (non-stokes outputs), though these modes can also be handled through sigpyproc if you are familiar with the indiexing you are dealing with.

While all input, intermediate and output arrays are internally handed as chars (8-bit words), you will need to be aware of your data type when processing the outputs of the CLI. For modes below 100, the the output word size is the same as the input word size. However, for modes above 100 the output will be a float that is 4 times wider than the input, and the type is changed to a float. So an 8-bit input will generate a 32-bit floating point value (float) and 16-bit will generate a 64-bit floating point value (double).

Using the CLI
=============

The Numpy Approach
==================


The SigPyProc Approach
======================
