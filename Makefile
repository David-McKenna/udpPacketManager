# Don't default to sh/dash
SHELL = bash

# Library versions
LIB_VER = 0.7
LIB_VER_MINOR = 0
CLI_VER = 0.6

# Detemrine the max threads per socket to speed up execution via OpenMP with ICC (GCC falls over if we set too many)
THREADS ?= $(shell cat /proc/cpuinfo | uniq | grep -m 2 "siblings" | cut -d ":" -f 2 | sort --numeric --unique | xargs echo)

# Docker compilation variables
NPROC ?= $(shell nproc)
BUILD_CORES ?= $(shell echo $(NPROC) '0.75' | awk '{printf "%1.0f", $$1*$$2}')
OPT_ARCH ?= "native"


BUILD_COMMIT ?= $(shell git log -1 --pretty=format:"%H")


# Local folder for casacore if already installed
CASACOREDIR ?= /usr/share/casacore/data/


.INTERMEDIATE : ./test_old/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)

calibration-prep:
	# Install the python dependencies \
	pip3 install lofarantpos python-casacore astropy git+https://github.com/2baOrNot2ba/AntPat.git git+https://github.com/2baOrNot2ba/dreamBeam.git; \
	# Get the base casacore-data \
	apt-get install -y --upgrade rsync casacore-data; \
	# Update the out-of-date components of casacore-data \
	rsync -avz rsync://casa-rsync.nrao.edu/casa-data/ephemerides rsync://casa-rsync.nrao.edu/casa-data/geodetic $(CASACOREDIR); \
	wget ftp://ftp.astron.nl/outgoing/Measures/WSRT_Measures.ztar -O $(CASACOREDIR)WSRT_Measures.ztar; \
	tar -xzvf $(CASACOREDIR)WSRT_Measures.ztar -C /usr/share/casacore/data/; \


docker-build:
	docker build --build-arg BUILD_CORES=$(BUILD_CORES) --build-arg BUILD_COMMIT=$(BUILD_COMMIT) --build-arg ARCH=$(ARCH) -t lofar-upm:$(LIB_VER).$(LIB_VER_MINOR) -f src/docker/Dockerfile .



# Generate test outputs to ensure we haven't broken anything
# Works based on output file md5 hashes, should be stable between
# versions and builds.
# 
# The generated hashes were generated using -ffast-math and will
# not be correct without that flag.
test: ./test_old/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)
	# . === source
	. ./test_old/hashVariables.txt; for output in ./test_old/output*; do \
		base=$$(basename $$output); \
		md5hash=($$(md5sum $$output)); \
		echo "$$base: $${md5hash[0]}, $${!base}"; \
		if [[ "$${md5hash[0]}" != "$${!base}" ]]; then \
			echo "##### Processed output $$output does not match expected hash. #####"; \
		fi; \
	done; \
	\
	# TODO: check overlapping outputs, eg 100, 150, 160 against each other.

	rm ./test_old/output*


# Build the objects to test
# Stress test: multiple ports, compressed and uncompressed, every processing mode, an odd/small number of packets
# In futre: consider dropping a few packets from the test case
./test_old/obj-generated-$(LIB_VER).$(LIB_VER_MINOR): test-samples
	-rm ./test_old/output*


	for procMode in 0 1 2 10 11 20 21 30 31 32; do \
		echo "Running lofar_udp_extractor -i ./test_old/udp_1613%d_sample.zst -o './test_old/output_'$$procModeStokes'_%d' -p $$procMode -m 501 -u 2"; \
		./build/lofar_udp_extractor -v -i ./test_old/udp_1613[[port]]_sample -o './test_old/output_'$$procMode'_[[outp]]' -p $$procMode -m 501 -u 2; \
	done

	for procMode in 100 110 120 130 150 160; do \
		for order in 0; do \
		#for order in 0 100; do \
			workingMode="`expr $$procMode + $$order`"; \
			for offset in 0 1 2 3 4; do \
				procModeStokes="`expr $$workingMode + $$offset`"; \
				echo "Running lofar_udp_extractor -i ./test_old/udp_1613%d_sample.zst -o './test_old/output_'$$procModeStokes'_%d' -p $$procModeStokes -m 501 -u 2"; \
				./build/lofar_udp_extractor -v -i ./test_old/udp_1613[[port]]_sample -o './test_old/output_'$$procModeStokes'_[[outp]]' -p $$procModeStokes -m 501 -u 2; \
			done; \
		done; \
	done

	touch ./test_old/obj-generated-$(LIB_VER).$(LIB_VER_MINOR)
	rm ./test_old/udp_*_sample

# Decompress the input data
test-samples:
	for fil in ./test_old/*zst; do \
		zstd -d $$fil; \
	done;

# Generate hashes for the current output files
test-make-hashes: test_old
	-rm ./test_old/hashVariables.txt
	touch ./test_old/hashVariables_tmp.txt
	for fil in ./test_old/output*; \
		do outp=($$(md5sum $$fil)); \
		base=$$(basename $$fil); \
		echo $$base='"'"$${outp[0]}"'"' >> ./test_old/hashVariables_tmp.txt; \
	done; \
	cat ./test_old/hashVariables_tmp.txt | sort -t_ -k 2 -n  > ./test_old/hashVariables.txt; \
	rm ./test_old/hashVariables_tmp.txt 



