lofar_udp_extractor
===================
The [*lofar_udp_extractor*](../src/CLI/lofar_cli_extractor.c) utility can be used to extract and process LOFAR
beamformed observations from international stations. This file is a 
basic guide on how to use the main CLI. A second CLI, 
[*lofar_stokes_extractor*](../src/CLI/lofar_cli_extractor.c) focuses 
on building on the potential Stokes vector data, for science-ready outputs.


Expected Input Data Format
---------------------
The expected recording format consists of the last 16-bytes of the 
header (all ethernet/udp frames removed) followed by a *N* byte 
data payload (typically 7808). These data streams can be raw files, FIFOs (Unix named pipes), compressed
with [zstandard](https://github.com/facebook/zstd) (ending in *.zst*)
or sourced from [PSRDADA](https://psrdada.sourceforge.net/) ringbuffers.

Expected File Name Formats
------------------------------

We support iteration through multiple different files through the use of a string formatting system. Currently, this supports four different 
values:
- \[\[port\]\] (input only)
  - Increases to handle a monotonic integer difference between input file names
- \[\[idx\]\] (output only)
  - Increases to handle a monotonic integer different output file names
- \[\[iter\]\] (output only)
  - Increases with iterations of the output, typically increasing, but the implementation is controlled by the user
- \[\[pack\]\] (output only)
  - Outputs a packet number, or a provided arbitrary number, in the parsed file name

Provided they are supported by your processing method, each of these can be used multiple times in your format string, and they will all be 
replaced. 

For increasing iteration values, the file name can be followed by up to 3 difference values. For both inputs and outputs, this can be two 
values, representing a base value and the incremental value added for each offset. The input value also has a third option to represent 
an offset from the base value, for handling offsets into the metadata in the case that you wish to only partially process an observation. 

### General Formatting

All option can use the input format in order to determine properties of the reader/writers objects. This is typically performed through the use 
of a 4 letter prefix before the path name, or the present of a file extension. As a result, all of these will result in the detection of 
their respective readers/writers. If no patterns are present, we assume the file is a normal file.

```bash
"FILE:myfile.out" # Normal reader/writer
"myfile.out" # Normal reader/writer
"FIFO:myfile.out" # FIFO named pipe reader/writer
"ZSTD:myfile.out" # Zstandard compressed file
"myfile.zst" # Zstandard compressed file 
"DADA:1000" # DADA ringbuffer
"HDF5:myfile.out" # HDF5 
"myfile.hdf5" # HDF5 
"myfile.h5" # HDF5 
```

### Inputs 

Multiple ports of data can be combined at once by providing a *\[\[port\]\]* 
in the input file name. This will iterate over a specified number of 
ports (controlled by the *-u* flag) and concatenate the beamlets 
into a set output format, the options for which are described below.

Consequently, the default input flag will look something akin to one of these inputs
```bash
-i "./udp_1613[[port]].ucc1.2020-02-22T10:30:00.000.zst" # Standard monotonic increasing of port from 0 - 3
-i "./udp_1613[[port]].ucc1.2020-02-22T10:30:00.000.zst,3" # Base value of 3, increasing across 3-6 
-i "./udp_1613[[port]].ucc1.2020-02-22T10:30:00.000.zst,3,2" # Base value of 3, increasing across 3, 5, 7, 9 
-u 2 -i "./udp_1613[[port]].ucc1.2020-02-22T10:30:00.000.zst,0,1,2" # Base value of 2, iterating up to 3
```

### Outputs

Multiple output ports of data can be handled by providing a *\[\[idx\]\]* in the output format name, which will then follow the same rules 
with respect to increments and offsets as the input files do. The *\[\pack\]\]* string will be replaced with the packet number at the start 
of a processing block. Additionally, if the CLI is set to split files, the writer will write a 4 character padded iteration count to 
replace any *\[\[iter\]\]* variables.

Other Flag Notes
----------------
Any times are handled in the ISOT format (YYYY-MM-DDTHH:mm:ss, 
fractions of seconds are not supported) in UTC+0 (NOT effected by 
local time zone, daylight savings, etc., unless your station clock has 
been modified).

Example Command
---------------

```
./lofar_udp_extractor -i "./udp_1613[[port]].ucc1.2020-02-22T10:30:00.000.zst" 
        -M "./metadata_2020-02-22T10:30:00.h" -c \
        -u 4 \
		-o "./test_output_[[idx]]_[[iter]]_[[pack]]" \
		-m 100000 -p 100 \
		-t "2020-02-22T11:02:00" -s 360.5

```

This command

- Takes an input from 4 ports of data (16130 -> 16133) from a set of 
  files in the local folder
- Sets a local metadata file and enables voltage calibration
- Outputs it to a file, with a suffix containing the output number 
  (0->numOutputs, here it will only be one file at 0),
  starting timestamp (later fixed to 2020-02-22T11:02:00) and the 
  starting packet number
- Sets the number of packets read + processed per iteration to 100,000
- Sets the output to be a Stokes I array, without any downsampling
- Sets the starting time to 2020-02-22T11:02:00, and will only read the next 6 minutes and 0.5 seconds of data

Arguments
--------

#### -i (str)

- Input file name, let it contain "[[port]]" to iterate over a number of 
  ports
- E.g., `-i ./udp_1613[[port]].ucc1_2020-10-20T20:20:20.000.zst`

#### -o (str)

- Output file name, must contain at least "[[idx]]" when generating 
  multiple outputs
- "[[iter]]" will include the iteration number of files are split, "[[pack]]"
  will include the starting packet number for each iteration

#### -I (str)

- The location of a metadata file, which must contain the `beamctl` 
  command used to steer the station, and can additionally support 
  tab-separated-values for
  - "SOURCE": Source name
  - "OBSID": Observation ID
  - "OBSERVER": Name of Observer

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

#### -p (int) [default: 0]

- Sets the processing mode for the output (options listed below)

#### -r

- If set, the last good packet will be repeated when a dropped packet is detected
- Default behaviour is to 0 pad the output

#### -c

- Enables calibration with Jones matrices generated by dreamBeam, 
  using the metadata input for pointing and frequency information. 

#### -z

- If set, change from calculating the start time from assuming we are using the 200MHz clock (Modes 3, 5, 7), to the 160MHz clock (4,6,
  probably others)

#### -q

- If set, silence the output from this CLI. Library error messages will still be displayed.

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

- Copy the input payload data to the output, removing the CEP header, padding dropped packets and dropping out of order
  packets
- 1 input file -> 1 output file

#### 2: "Raw Split Polarisations"

- Copy the input payload, padding where needed, and splitting across each of the (Xr, Xi, Yr, Yi) polarisations
- N input files -> 4 output files

### Re-ordering Operations

#### 10: "Raw To Beamlet-Major"

- Take the input payload, padding where needed, remove the header, reorder the data such that instead of having (f0t0,
  f0t1... f0t15, f1t0...) we have (f0t0, f1t0, f2t0)...
- N input files -> 1 output file

#### 11: "Raw to Beamlet-Major, Split Polarisations"

- Combination of (2) and (10), split output data per (Xr, Xi, Yr, Yi) polarisations
- N input files -> 4 output files

#### 20: "Raw To Beamlet-Major, Frequency Reversed"

- Modified version of (10), where instead of (f0t0, f1t0...) we now output (fNt0, fN-1t0, ...), following the standard
  used for pulsar observations
- N input files -> 1 output file

#### 21: "Raw To Beamlet-Major, Frequency Reversed, Split Polarisations"

- Combination of (2) and (20), where we split the output data per (Xr, Xi, Yr, Yi) polarisation
- N input files -> 4 output files

#### 30: "Raw To Time-Major"

- Take the input payload, padding where needed, remove the header and reorder the data such that we have a single
  channel's full time stream before presenting the net channel (f0t0, f0t1, f0t2... f0tN-1, f0tN)
- N input files -> 1 output file

#### 31: "Raw To Time-Major, Split Polarisations"

- Modified version of (30), where we split the output data per (Xr, Xi, Yr, Yi) polarisation
- N input files -> 1 output file

#### 32: "Raw To Time-Major, Antenna Polarisations"

- Modified version of (30), where we split the output per (X, Y) polarisation (complex elements, FFTWF format)
- N input files -> 2 output files

### Stokes Parameter Processing Operations

#### Base Stokes Modes

By default, we define a number of base Stokes parameter outputs each 
at a multiple of 10 from a base of 100, and increasing by 100 for 
each mode. There are 3 different Stokes 
ordering options:
- 100: Frequency-major, beamlet-order Stokes, where the output 
  frequency order matches the order used in the beamctl command 
  (increasing frequency
- 200: Frequency-major, reversed beamlet-order Stokes, where the 
  output frequency order is reversed from the order used in the 
  beamctl command (decreasing frequency)
- 300: Time-major, where the frequency order matches that of the 
  beamctl command (increasing frequency)

As a result, you can use mode 100 to form a frequency-major, 
increasing frequency, Stokes I output, or mode 310 to form a 
time-major, increasing frequency Stokes Q output.

#### \*00: "Stokes I"

- Take the input data, apply (10, 20 or 30) and then combine the polarisations to form a 32-bit floating point Stokes I for each
  frequency sample
- N input files -> 1 output file

#### \*10: "Stokes Q"

- Take the input data, apply (10, 20 or 30) and then combine the polarisations to form a 32-bit floating point Stokes Q for each
  frequency sample
- N input files -> 1 output file

#### \*20: "Stokes U"

- Take the input data, apply (10, 20 or 30) and then combine the polarisations to form a 32-bit floating point Stokes U for each
  frequency sample
- N input files -> 1 output file

#### \*30: "Stokes V"

- Take the input data, apply (10, 20 or 30) and then combine the polarisations to form a 32-bit floating point Stokes V for each
  frequency sample
- N input files -> 1 output file

#### \*50: "Stokes Vector"

- Take the input data, apply (10, 20 or 30), and then combine the polarisation to form 4 output 32-bit floating point Stokes (I,
  Q, U, V) filterbanks for each frequency sample
- N input files -> 4 output files

#### \*60: "Useful Stokes Vector"

- Take the input data, apply (10, 20 or 30), and then combine the polarisation to form 4 output 32-bit floating point Stokes (I,
  V) filterbanks for each frequency sample
- N input files -> 2 output files

#### Stokes With Time Downsampling

We also offer up to a 16x downsampling during execution (the number of time samples per packet). To select this, choose
a Stokes parameter and add a log 2 of the factor to the mode. The 
same rules with respect to the data ordering apply as above.

So in order to get a Stokes U output, in time-major ordering, with 8x 
downsampling we will pass `320 + log_2(8) = 323` as our processing mode.

Further downsampling should be implemented on the output of a raw data mode, or one of these base execution modes. The `lofar_stokes_cli` 
has an example implementation of extended downsampling for modes (10, 20).

#### \*\*1: "Stokes with 2x downsampling"

- Take the input data, apply (10, 20 or 30) and (\*\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (2x less output samples)

#### \*\*2: "Stokes with 4x downsampling"

- Take the input data, apply (10, 20 or 30) and (\*\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (4x less output samples)

#### \*\*3: "Stokes with 8x downsampling"

- Take the input data, apply (10, 20 or 30) and (\*\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (8x less output samples)

#### \*\*4: "Stokes with 16x downsampling"

- Take the input data, apply (10, 20 or 30) and (\*\*0) to form a Stokes \* sample, and sum it with the next sample
- N input files -> 1 output file (16x less output samples)
