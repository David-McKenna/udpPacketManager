# Preparing Inputs for `dspsr` / `digifil`

The DSPSR ecosystem is commonly used by low-frequency astronomers for processing data into other formats, through the use of the `digifil`,
`digifits` or `dspsr` CLIs. `udpPacketManager` can produce data in an output voltage format that can be easily consumed by these
command lines using the `DADA` format, either as a ringbuffer or a large on-disk file. This is a walk through on how to produce a
`DADA`-compliant file for use with DSPSR, or any other software which supports it.

## Requirements

To produce an output file, you will need

- Your raw data (raw CEP data)
- An input metadata file (iLISA or manually created)
  - This file must contain the beamctl used to perform the recording, and optionally a line that contains "SOURCE" and a source name,
    separated by a tab, similar to "SOURCE\tSOURCENAME"

## Preparing a `DADA` file

We will be calling `lofar_udp_extractor` in mode 10 (Frequency-major, combined voltages) to produce a single output file, which starts with
a `DADA` header, and then a stream of raw voltage samples.

Following the methodology described in (README_CLI.md)[README_CLI.md], we will need to provide an input file pattern, converting an input
file like this, whereby the last digit of the port is replaced with `[[port]]` so that it can be iterated over.

- `udp_IE613_16130.blc00.bl.pvt.2022-09-01T10:53:00.000.zst"`
- `udp_IE613_1613[[port]].blc00.bl.pvt.2022-09-01T10:53:00.000.zst"`

After that, we build a normal `lofar_udp_extractor` command line call:

```shell
me@machine $ lofar_udp_extractor -i "udp_IE613_1613[[port]].2022-09-01T10:53:00.000.zst"  # IE613 has ports 16130 - 16133, I assume for LV614 this will be 16140 - 16143 instead. Just swap out the last 0 for [[port]]  
                                -u 4 # The number of ports of data to process, reduce this if you didn't record all 4 lanes
                                -o TestObs_2022-09-01T105300.dada # Output file name, as you wish
                                -p 10 # Sets the output data order, mode 10 is the standard DADA Voltages format
                                -I "YYYYMMDD-HHMMSS.h" # Your metadata file
                                -M DADA # The output file having a ".dada" should be enough for the software to produce a DADA header, but it doesn't hurt to be explicit sometimes 
```

After letting it complete, we can then use the output file as an input as you wish. For example, this input can be used to generate a
4-polarisation, 8x channelised output with `digifil` using the following command,

```shell
me@machine $ digifil -F 3904:1 -B 1024 -d 4 TestObs_2022-09-01T105300.dada -o TestObs_2022-09-01T105300.fil
```

## Potential issues

Enabling calibration (`-c`) with the processing run will cause a cryptic error about the machine not being recognised. This is in fact an  
error as there are no 32-bit supported unpackers in the DSPSR backend. You can validate that this is the issue by checking the number of bits
DSPSR is searching for in the "very verbose mode", `-V`, or by using `digihdr` to check the bit mode of the file.