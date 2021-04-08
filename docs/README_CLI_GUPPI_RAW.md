lofar_udp_guppi_raw
===================
This is a modified version of the [*lofar_udp_extrator*](../src/CLI/lofar_cli_extractor.c), where instead of producing a
variable mode output with / without a [SigProc](https://github.com/SixByNine/sigproc) compatible header, the output is
always time-major (30) and has a GUPPI RAW / FITS compatible header attached between iterations.


Injecting Metadata
------------------
In order to populate the metadata in the headers, a file must be provided with lines each containing a keyword/value
pair, which will be copied to the header on start-up. Several keys (*OBSNCHAN, OBSBW, CHAN_BW, NBITS, TBIN, NPOL,
OVERLAP, PKTFMT, PKTSIZE, STT_IMJD, STT_SMJD, STT_OFFS*) are overwritten and inferred from the raw data, and others (*
BLOCSIZE, DAQPULSE, DROPBLK, DROPTOT, PKTIDX*) are updated between iterations. The remaining supported keywords and
their default values are listed below, in the format expected of the input metadata file.

```
--src_name J0000+0000
--ra_str 00:00:00.0000
--dec_str +00:00:00.0000
--obsfreq 149.90234375
--fd_poln LIN
--trk_mode TRACK
--obs_mode RAW
--cal_mode OFF
--scanlen 0.0
--projid UDP2RAW
--observer Unknown
--telescop ILT
--frontend LOFAR-RSP
--backend LOFAR-UDP
--datahost 0.0.0.0
--dataport 16130
```

Example Command
---------------

```
$ lofar_udp_guppi_raw -i "./udp_1613%d.ucc1.2020-02-22T10:30:00.000.zst" -u 4 \
						-o "./target.%04d.raw" -a "./headerMetadata" \
						-e 32

```

This command

- Takes an input from 4 ports of date (16130 -> 16133) from a set of files in the local folder
- Outputs it to a file, with a file name dependant on the number of iterations performed, and breaks taken
- Loads data from a local file "headerMetadata" to modify the default values of the GUPPI RAW frame headers
- Split into a new file every 32 iterations

Modified Arguments
------------------
While most of the arguments are the same as the default CLI, as described in [*README_CLI.md*](README_CLI.md), some
changes have been made to better support the goal of this CLI.

#### -o (str) [default: ./output_%d]

- Output file name, must contain at least *%d* when generating multiple outputs
- Both *%s* for the starting time stamp, *%ld* for the packet number **are not supported by this CLI**

#### -e (int) [default: INT_MAX]

- Number of iterations to perform before closing the current file and opening a new one

#### -p <REMOVED>

- As we only expect a single output, the *-p* flag has been removed from this CLI

#### -a (str) [default: ""]

- Location of the file containing modifications to the default header attributes
- If not provided, it will use the default values listed above.



