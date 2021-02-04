lofar_udp_extractor
===================
The [*lofar_udp_extractor*](../src/CLI/lofar_cli_extractor.c) utility can be used to extract and process LOFAR beamformed observations from international stations. This file is a basic guide on how to use the main CLI, while [*README_CLI_GUPPI_RAW.md*](README_CLI_GUPPI_RAW.md) will describe the changes made to support the LOFAR -> GUPPI RAW formatting in [*lofar_udp_guppi_raw*](../src/CLI/lofar_cli_guppi_raw.c).


Expected Input Data Format
---------------------
The expected recording format consists of the last 16-bytes of the header (all ethernet/udp frames removed) followed by a *N* byte data payload. These data streams can be raw files, or files that have been compressed with [zstandard](https://github.com/facebook/zstd) (ending in *.zst*).

Multiple ports of data can be combined at once by providing a *%d* in the input file name. This will iterate over a specified number of ports (controlled by the *-u* flag) and concatenate the beamlets into a single output outside of processing mode 0.

Any times are handled in the ISOT format (YYYY-MM-DDTHH:mm:ss, fractions of seconds are not supported) in UTC+0 (NOT effected by local time zone, daylight savings, etc unless your station clock has been modified).

The events file can allow for several smaller chunks of data to be removed from an observation, the format for these files is described at the end of this document.

Example Command
---------------
```
./lofar_udp_extractor -i "./udp_1613%d.ucc1.2020-02-22T10:30:00.000.zst" -u 4 \
		-o "./test_output_%d_%s_%ld" \
		-m 100000 -p 100\
		-t "2020-02-22T11:02:00" -s 360.5

```
This command
- Takes an input from 4 ports of data (16130 -> 16133) from a set of files in the local folder
- Outputs it to a file, with a suffix containing the output number (0->numOutputs, here it will only be one file at 0), starting timestamp (later fixed to 2020-02-22T11:02:00) and the starting packet number
- Sets the number of packets read + processed per iteration to 100,000
- Sets the output to be a Stokes I array, without any downsampling
- Sets the starting time to 2020-02-22T11:02:00, and will only read the next 6 minutes and 0.5 seconds of data

Arguments
--------

#### -i (str)
- Input file name, let it contain *%d* to iterate over a number of ports
- E.g., `-i ./udp_1613%d.ucc1_2020-10-20T20:20:20.000.zst`

#### -o (str) [default: "./output_%d_%s_%ld"]
- Output file name, must contain at least *%d* when generating multiple outputs
- *%s* will include the starting time stamp, *%ld* will include the starting packet number
- These values must be added in order, so *%d_%s* is allowed to not attach the packet number but *%ld_%d_%s* will not work.

#### -m (int) [default: 65536]
- Number of packets to read and processed per iteration
- Be considerate of the memory requirements for loading / processing the data when setting this value

#### -u (int) [default: 4]
- Number of input files to iterate over

#### -b (int),(int) [default: 0,0 === all inputs]
- Indices of beamlets to extract from the input dataset. Lower value is inclusive, higher value is exclusive
- Eg, `-b 0,300` will return 300 beamlets, at indices 0:299.
- I wanted this to be inclusive on both ends but couldn't find a solid way to just index it as intended.

#### -t (str) [default: '']
- Starting time string, in UTC+0 and ISOT format (YYYY-MM-DDTHH:mm:ss)

#### -s (float) [default: FLOAT_MAX]
- Maximum amount of data (in seconds) to process before exiting

#### -e (str) 
- Location of an events file, for processing multiple time / extraction lengths as once, file format described below
- Must have at least *%d* and *%s* in the output name to prevent overwriting each event with the next one

#### -p (int) [default: 0]
- Sets the processing mode for the output (options listed below)

#### -r
- If set, the last good packet will be repeated when a dropped packet is detected
- Default behaviour is to 0 pad the output

#### -c (str) [default: '']
- Provide comma separated information on the instrument and subbands for calibrating the data using generating Jones matrices from dreamBeam
- General syntax: 'INST,<lo,hi>' where inst is 'LBA' or 'HBA' and lo, hi are the same as used in beamctl commands
- HBA mode 7 is accessed by adding 512 to the base subband (e.g., mode 7 subband 12 is 524)
- Multiple instruments (eg, mode 357) can be specified similarly by command separating each variable, eg "LBA,0:200,HBA,100:300"
- Requires -d is provided in order for calibration to be enabled

#### -d (str) [default: '']
- Provide a comma separate direction to generate Jones matrices for with dreamBeam
- General syntax is 'RAD1,RAD2,COORD', where RAD1/2 are values in radians with respect to the coordinate system
- All standard casacore coordinate systems are supported (J2000, SUN, JUPITER, AZELGO), but non-J2000 coordinate system will be processed slowly as they must be recalculated for each timestep
- Requires -c is provided in order for calibration to be enabled

#### -z
- If set, change from calculating the start time from the RSP 200MHz clock (Modes 3, 5, 7) to the 160MHz clock (4,6, probably others)

#### -q 
- If set, silence the output from this CLI. Library error messages will still be displayed.

#### -a (str) [default: '']
- Call mockHeader to generate a header for new files, provide mockHeader enclosed by \".
- E.g., '-a "-fch1 150 -fo -0.19 -tel 1916 -source Sun"'
- By default, we will pass in the number of channels, starting time and sampling time

#### -f
- If set, we will append to an existing output file rather than exiting when they exist
- Do note, using this in conjunction with *-a* will replace files rather than appending them.



Processing Modes
----------------
### Default Ordering Operations
#### 0: "Raw Copy"
- Copy the input to the output, padding dropped packets and dropping out of order packets
- 1 input file -> 1 output file


#### 1: "Raw Headerless Copy"
- Copy the input payload data to the output, removing the CEP header, padding dropped packets and dropping out of order packets
- 1 input file -> 1 output file

#### 2: "Raw Split Polarizations"
- Copy the input payload, padding where needed, and splitting across each of the (Xr, Xi, Yr, Yi) polarisations
- N input files -> 4 output files


### Re-ordering Operations
#### 10: "Raw To Beamlet-Major"
- Take the input payload, padding where needed, remove the header, reorder the data such that instead of having (f0t0, f0t1... f0t15, f1t0...) we have (f0t0, f1t0, f2t0)...
- N input files -> 1 output file

#### 11: "Raw to Beamlet-Major, Split Polarizations"
- Combination of (2) and (10), split output data per (Xr, Xi, Yr, Yi) polarizations
- N input files -> 4 output files

#### 20: "Raw To Beamlet-Major, Frequency Reversed"
- Modified version of (10), where instead of (f0t0, f1t0...) we now output (fNt0, fN-1t0, ...), following the standard used for pulsar observations
- N input files -> 1 output file

#### 21: "Raw To Beamlet-Major, Frequency Reversed, Split Polarizations"
- Combination of (2) and (20), where we split the output data per (Xr, Xi, Yr, Yi) polarization
- N input files -> 4 output files

#### 30: "Raw To Time-Major"
- Take the input payload, padding where needed, remove the header and reorder the data such that we have a single channel's full time stream before presenting the net channel (f0t0, f0t1, f0t2... f0tN-1, f0tN)
- N input files -> 1 output file

#### 31: "Raw To Time-Major, Split Polarizations"
- Modified version of (30), where we split the output data per (Xr, Xi, Yr, Yi) polarisation
- N input files -> 1 output file

#### 32: "Raw To Time-Major, Antenna Polarizations"
- Modified version of (30), where we split the output per (X, Y) polarisation (complex elements, FFTWF format)
- N input files -> 2 output files


### Processing Operations

There is currently an untested implementation of time-major Stokes outputs in the library, but is has not been tested or fully implemented in the reader as of yet.

#### Base Modes
By default, we define a number of base Stokes parameter outputs each at a multiple of 10 from 100.

#### 100: "Stokes I"
- Take the input data, apply (20) and then combine the polarizations to form a 32-bit floating point Stokes I for each frequency sample
- N input files -> 1 output file

#### 110: "Stokes Q"
- Take the input data, apply (20) and then combine the polarizations to form a 32-bit floating point Stokes Q for each frequency sample
- N input files -> 1 output file

#### 120: "Stokes U"
- Take the input data, apply (20) and then combine the polarizations to form a 32-bit floating point Stokes U for each frequency sample
- N input files -> 1 output file

#### 130: "Stokes V"
- Take the input data, apply (20) and then combine the polarizations to form a 32-bit floating point Stokes V for each frequency sample
- N input files -> 1 output file

#### 150: "Stokes Vector"
- Take the input data, apply (20), and then combine the polarisation to form 4 output 32-bit floating point Stokes (I, Q, U, V) filterbanks for each frequency sample
- N input files -> 4 output files

#### 160: "Useful Stokes Vector"
- Take the input data, apply (20), and then combine the polarisation to form 4 output 32-bit floating point Stokes (I, V) filterbanks for each frequency sample
- N input files -> 2 output files

#### Time downsampling
We also offer up to a 16x downsampling during execution (the number of time samples per packet). To select this, choose a Stokes parameter and add a log 2 of the factor to the mode.

So in order to get a Stokes U output, with 8x downsampling we will pass `120 + log_2(8) = 123` as our processing mode.

#### 1\*1: "Stokes with 2x downsampling"
- Take the input data, apply (20) and (1\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (2x less output samples)

#### 1\*2: "Stokes with 4x downsampling"
- Take the input data, apply (20) and (1\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (4x less output samples)

#### 1\*3: "Stokes with 8x downsampling"
- Take the input data, apply (20) and (1\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (8x less output samples)

#### 1\*4: "Stokes with 16x downsampling"
- Take the input data, apply (20) and (1\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (16x less output samples)


Event Files
-----------
Event files can be used to extract multiple, non-continuous elements of an observation. The file must
- Have the number of events to process in the first line
- List the events in order from start to finish of the observation
- Not contain events that overlap
```
NUM_EVENTS
ISOT0 LENGTH0
ISOT1 LENGTH1
...
```

For example, in a 2 event case, we could use the file
```
2
2020-02-22T10:02:00 100.33
2020-02-22T11:04:00 300.5
```

While it is possible to extract multiple events from a given second, you will need all three output file characters (*%d*, *%s*, *%ld*) in order to prevent the output files from overlapping (only the *%ld* variable will be changing).