# Don't default to sh/dash
SHELL = bash

# If we haven't been provided a compiler, check for icc/gcc
ifeq (,$(CC))
ifneq (,$(shell which icc))
CC		= icc
CXX		= icpc
else ifneq (,$(shell which gcc-9))
CC		= gcc-9
CXX		= g++-9
else
CC		= gcc
CXX		= g++
endif
endif

# Library versions
LIB_VER = 0.6
LIB_VER_MINOR = 0
CLI_VER = 0.4

# Detemrine the max threads per socket to speed up execution via OpenMP with ICC (GCC falls over if we set too many)
THREADS = $(shell cat /proc/cpuinfo | uniq | grep -m 2 "siblings" | cut -d ":" -f 2 | sort --numeric --unique | awk '{printf("%d", $$1);}')

CFLAGS 	+= -W -Wall -Ofast -march=native -fPIC
CFLAGS  += -DVERSION=$(LIB_VER) -DVERSIONCLI=$(CLI_VER) 
#CFLAGS  += -fsanitize=address -DALLOW_VERBOSE -g # -DBENCHMARKING -g -DALLOW_VERBOSE #-D__SLOWDOWN
# -fopt-info-missed=compiler_report_missed.log -fopt-info-vec=compiler_report_vec.log -fopt-info-loop=compiler_report_loop.log -fopt-info-inline=compiler_report_inline.log -fopt-info-omp=compiler_report_omp.log

# Adjust flags based on the compiler
# GCC has negative scaling when the number of threads is greater than twice the number of ports
# ICC will take everything you throw at it.
ifeq ($(CC), icc)
AR = xiar
CFLAGS += -fast -static -static-intel -qopenmp-link=static -DOMP_THREADS=$(THREADS)
else
AR = ar
CFLAGS += -static -DOMP_THREADS=8 -funswitch-loops
LFLAGS += -fopenmp-simd
endif

# Ensure we're using C++17
CXXFLAGS += $(CFLAGS) -std=c++17

LFLAGS 	+= -I./src -I./src/lib -I./src/CLI -I/usr/include/ -lzstd -fopenmp #-lefence

# Define our general build targets
OBJECTS = src/lib/lofar_udp_reader.o src/lib/lofar_udp_misc.o src/lib/lofar_udp_backends.o
CLI_META_OBJECTS = src/CLI/lofar_cli_meta.o src/CLI/ascii_hdr_manager.o
CLI_OBJECTS = $(OBJECTS) $(CLI_META_OBJECTS) src/CLI/lofar_cli_extractor.o src/CLI/lofar_cli_guppi_raw.o

LIBRARY_TARGET = liblofudpman.a

PREFIX = /usr/local

# Local folder for casacore if already installed
CASACOREDIR = /usr/share/casacore/data/


.INTERMEDIATE : ./tests/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)

# C -> CC
%.o: %.c
	$(CC) -c $(CFLAGS) -o ./$@ $< $(LFLAGS)

# C++ -> CXX
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o ./$@ $< $(LFLAGS)

# CLI -> link with C++
all: $(CLI_OBJECTS) library
	$(CXX) $(CXXFLAGS) src/CLI/lofar_cli_extractor.o $(CLI_META_OBJECTS) $(LIBRARY_TARGET)  -o ./lofar_udp_extractor $(LFLAGS)
	$(CXX) $(CXXFLAGS) src/CLI/lofar_cli_guppi_raw.o $(CLI_META_OBJECTS) $(LIBRARY_TARGET) -o ./lofar_udp_guppi_raw $(LFLAGS)

# Library -> *ar
library: $(OBJECTS)
	$(AR) rc $(LIBRARY_TARGET).$(LIB_VER).$(LIB_VER_MINOR) $(OBJECTS)
	ln -sf ./$(LIBRARY_TARGET).$(LIB_VER).$(LIB_VER_MINOR) ./$(LIBRARY_TARGET)

# Install CLI, headers, library
install: all
	mkdir -p $(PREFIX)/bin/ && mkdir -p $(PREFIX)/include/
	cp ./lofar_udp_extractor $(PREFIX)/bin/
	cp ./lofar_udp_guppi_raw $(PREFIX)/bin/
	cp ./src/misc/dreamBeamJonesGenerator.py $(PREFIX)/bin/
	cp ./src/lib/*.h $(PREFIX)/include/
	cp ./src/lib/*.hpp $(PREFIX)/include/
	cp -P ./*.a* ${PREFIX}/lib/
	cp -P ./*.a ${PREFIX}/lib/
	-cp ./mockHeader/mockHeader $(PREFIX)/bin/

# Install CLI, headers, library, locally
install-local: all
	mkdir -p ~/.local/bin/ && mkdir -p ~/.local/include/
	cp ./lofar_udp_extractor ~/.local/bin/
	cp ./lofar_udp_guppi_raw ~/.local/bin/
	cp ./src/misc/dreamBeamJonesGenerator.py ~/.local/bin/
	cp ./src/lib/*.h ~/.local/include/
	cp ./src/lib/*.hpp ~/.local/include/
	cp -P ./*.a* ~/.local/lib/
	cp -P ./*.a ~/.local/lib/
	-cp ./mockHeader/mockHeader ~/.local/bin/


calibration-prep:
	# Install the python dependencies \
	pip3 install lofarantpos python-casacore astropy git+https://github.com/2baOrNot2ba/AntPat.git git+https://github.com/2baOrNot2ba/dreamBeam.git; \
	# Get the base casacore-data \
	apt-get install -y --upgrade rsync casacore-data; \
	# Update the out-of-date components of casacore-data \
	rsync -avz rsync://casa-rsync.nrao.edu/casa-data/ephemerides rsync://casa-rsync.nrao.edu/casa-data/geodetic $(CASACOREDIR); \
	wget ftp://ftp.astron.nl/outgoing/Measures/WSRT_Measures.ztar -O $(CASACOREDIR)WSRT_Measures.ztar; \
	tar -xzvf $(CASACOREDIR)WSRT_Measures.ztar -C /usr/share/casacore/data/; \

# Remove local build arifacts
clean:
	-rm ./src/CLI/*.o
	-rm ./src/lib/*.o
	-rm ./*.a
	-rm ./*.a.*
	-rm ./compiler_report_*.log
	-rm ./lofar_udp_extractor
	-rm ./lofar_udp_guppi_raw
	-rm ./tests/output_*

# Uninstall the software from the system
remove:
	rm $(PREFIX)/bin/lofar_udp_extractor
	rm $(PREFIX)/bin/lofar_udp_guppi_raw
	rm $(PREFIX)/bin/dreamBeamJonesGenerator.py
	cd src/lib/; find . -name "*.hpp" -exec rm $(PREFIX)/include/{} \;
	cd src/lib/; find . -name "*.h" -exec rm $(PREFIX)/include/{} \;
	find . -name "*.a" -exec rm $(PREFIX)/lib/{} \;
	find . -name "*.a.*" -exec rm $(PREFIX)/lib/{} \;
	make clean

# Uninstall the software from the local user
remove-local:
	rm ~/.local/bin/lofar_udp_extractor
	rm ~/.local/bin/lofar_udp_guppi_raw
	rm ~/.local/bin/dreamBeamJonesGenerator.py
	cd src/lib/; find . -name "*.hpp" -exec rm ~/.local/include/{} \;
	cd src/lib/; find . -name "*.h" -exec rm ~/.local/include/{} \;
	find . -name "*.a" -exec rm ~/.local/lib/{} \;
	find . -name "*.a.*" -exec rm ~/.local/lib/{} \;
	make clean


# Generate test outputs to ensure we haven't broken anything
# Works based on output file md5 hashes, should be stable between
# versions and builds.
# 
# The generated hashes were generated using -ffast-math and will
# not be correct without that flag.
test: ./tests/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)
	# . === source
	. ./tests/hashVariables.txt; for output in ./tests/output*; do \
		base=$$(basename $$output); \
		md5hash=($$(md5sum $$output)); \
		echo "$$base: $${md5hash[0]}, $${!base}"; \
		if [[ "$${md5hash[0]}" != "$${!base}" ]]; then \
			echo "##### Processed output $$output does not match expected hash. #####"; \
		fi; \
	done; \
	\
	# TODO: check overlapping outputs, eg 100, 150, 160 against each other.

	rm ./tests/output*


# Build the objects to test
# Stress test: multiple ports, compressed and uncompressed, every processing mode, an odd/small number of packets
# In futre: consider dropping a few packets from the test case
./tests/obj-generated-$(LIB_VER).$(LIB_VER_MINOR): test-samples
	-rm ./tests/output*


	for procMode in 0 1 2 10 11 20 21 30 31 32; do \
		echo "Running lofar_udp_extractor -i ./tests/udp_1613%d_sample.zst -o './tests/output_'$$procModeStokes'_%d' -p $$procMode -m 501 -u 2"; \
		lofar_udp_extractor -i ./tests/udp_1613%d_sample -o './tests/output_'$$procMode'_%d' -p $$procMode -m 501 -u 2; \
	done

	for procMode in 100 110 120 130 150 160; do \
		for order in 0; do \
		#for order in 0 100; do \
			workingMode="`expr $$procMode + $$order`"; \
			for offset in 0 1 2 3 4; do \
				procModeStokes="`expr $$workingMode + $$offset`"; \
				echo "Running lofar_udp_extractor -i ./tests/udp_1613%d_sample.zst -o './tests/output_'$$procModeStokes'_%d' -p $$procModeStokes -m 501 -u 2"; \
				lofar_udp_extractor -i ./tests/udp_1613%d_sample.zst -o './tests/output_'$$procModeStokes'_%d' -p $$procModeStokes -m 501 -u 2; \
			done; \
		done; \
	done

	touch ./tests/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)
	rm ./tests/udp_*_sample

# Decompress the input data
test-samples:
	for fil in ./tests/*zst; do \
		zstd -d $$fil; \
	done;

# Generate hashes for the current output files
test-make-hashes: ./tests/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)
	-rm ./tests/hashVariables.txt
	touch ./tests/hashVariables.txt
	for fil in ./tests/output*; \
		do outp=($$(md5sum $$fil)); \
		base=$$(basename $$fil); \
		echo $$base='"'"$${outp[0]}"'"' >> ./tests/hashVariables.txt; \
	done




# Optional: build mockHeader
mockHeader:
	git clone https://github.com/David-McKenna/mockHeader && \
	cd mockHeader && \
	make

