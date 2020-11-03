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

LIB_VER = 0.4
LIB_VER_MINOR = 0
CLI_VER = 0.2

# Detemrine the max threads per socket to speed up execution via OpenMP with ICC (GCC falls over if we set too many)
THREADS = $(shell cat /proc/cpuinfo | uniq | grep -m 2 "siblings" | cut -d ":" -f 2 | sort --numeric --unique | awk '{printf("%d", $$1);}')

CFLAGS 	+= -W -Wall -O3 -march=native -DVERSION=$(LIB_VER) -DVERSIONCLI=$(CLI_VER) -fPIC # -DBENCHMARKING -g -DALLOW_VERBOSE #-D__SLOWDOWN

ifeq ($(CC), icc)
AR = xiar
CFLAGS += -fast -static -static-intel -qopenmp-link=static -DOMP_THREADS=$(THREADS)
else
AR = ar
CFLAGS += -funswitch-loops -DOMP_THREADS=5
endif

CXXFLAGS += $(CFLAGS) -std=c++17
# -fopt-info-missed=compiler_report_missed.log -fopt-info-vec=compiler_report_vec.log -fopt-info-loop=compiler_report_loop.log -fopt-info-inline=compiler_report_inline.log -fopt-info-omp=compiler_report_omp.log

LFLAGS 	+= -I./src -I./src/lib -I./src/CLI -I/usr/include/ -lzstd -fopenmp #-lefence

OBJECTS = src/lib/lofar_udp_reader.o src/lib/lofar_udp_misc.o src/lib/lofar_udp_backends.o
CLI_META_OBJECTS = src/CLI/lofar_cli_meta.o src/CLI/ascii_hdr_manager.o
CLI_OBJECTS = $(OBJECTS) $(CLI_META_OBJECTS) src/CLI/lofar_cli_extractor.o src/CLI/lofar_cli_guppi_raw.o

LIBRARY_TARGET = liblofudpman.a

PREFIX = /usr/local

%.o: %.c
	$(CC) -c $(CFLAGS) -o ./$@ $< $(LFLAGS)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o ./$@ $< $(LFLAGS)

all: $(CLI_OBJECTS) library
	$(CXX) $(CXXFLAGS) src/CLI/lofar_cli_extractor.o $(CLI_META_OBJECTS) $(LIBRARY_TARGET)  -o ./lofar_udp_extractor $(LFLAGS)
	$(CXX) $(CXXFLAGS) src/CLI/lofar_cli_guppi_raw.o $(CLI_META_OBJECTS) $(LIBRARY_TARGET) -o ./lofar_udp_guppi_raw $(LFLAGS)

library: $(OBJECTS)
	$(AR) rc $(LIBRARY_TARGET).$(LIB_VER).$(LIB_VER_MINOR) $(OBJECTS)
	ln -sf ./$(LIBRARY_TARGET).$(LIB_VER).$(LIB_VER_MINOR) ./$(LIBRARY_TARGET)

install: all
	mkdir -p $(PREFIX)/bin/ && mkdir -p $(PREFIX)/include/
	cp ./lofar_udp_extractor $(PREFIX)/bin/
	cp ./lofar_udp_guppi_raw $(PREFIX)/bin/
	cp ./src/lib/*.h $(PREFIX)/include/
	cp ./src/lib/*.hpp $(PREFIX)/include/
	cp -P ./*.a* ${PREFIX}/lib/
	cp -P ./*.a ${PREFIX}/lib/	
	cp ./mockHeader/mockHeader $(PREFIX)/bin/; exit 0;

install-local: all
	mkdir -p ~/.local/bin/ && mkdir -p ~/.local/include/
	cp ./lofar_udp_extractor ~/.local/bin/
	cp ./lofar_udp_guppi_raw ~/.local/bin/
	cp ./src/lib/*.h ~/.local/include/
	cp ./src/lib/*.hpp ~/.local/include/
	cp -P ./*.a* ~/.local/lib/
	cp -P ./*.a ~/.local/lib/
	cp ./mockHeader/mockHeader ~/.local/bin/; exit 0;

clean:
	rm ./src/CLI/*.o; exit 0;
	rm ./src/lib/*.o; exit 0;
	rm ./*.a; exit 0;
	rm ./*.a.*; exit 0;
	rm ./compiler_report_*.log; exit 0;
	rm ./lofar_udp_extractor; exit 0;
	rm ./lofar_udp_guppi_raw; exit 0;
	rm ./tests/output_*; exit 0;

remove:
	rm $(PREFIX)/bin/lofar_udp_extractor
	rm $(PREFIX)/bin/lofar_udp_guppi_raw
	cd src/lib/; find . -name "*.hpp" -exec rm $(PREFIX)/include/{} \;
	cd src/lib/; find . -name "*.h" -exec rm $(PREFIX)/include/{} \;
	find . -name "*.a" -exec rm $(PREFIX)/lib/{} \;
	find . -name "*.a.*" -exec rm $(PREFIX)/lib/{} \;
	make clean

remove-local:
	rm ~/.local/bin/lofar_udp_extractor
	rm ~/.local/bin/lofar_udp_guppi_raw
	cd src/lib/; find . -name "*.hpp" -exec rm ~/.local/include/{} \;
	cd src/lib/; find . -name "*.h" -exec rm ~/.local/include/{} \;
	find . -name "*.a" -exec rm ~/.local/lib/{} \;
	find . -name "*.a.*" -exec rm ~/.local/lib/{} \;
	make clean


test:
	rm ./tests/output*; exit 0;

	for procMode in 0 1 2 10 11 20 21 30 31 32; do \
		echo "Running lofar_udp_extractor -i ./tests/udp_1613%d_sample.zst -o './tests/output_'$$procModeStokes'_%d' -p $$procMode -m 501"; \
		lofar_udp_extractor -i ./tests/udp_1613%d_sample -o './tests/output_'$$procMode'_%d' -p $$procMode -m 501; \
	done

	for procMode in 100 110 120 130 150 160; do \
		for offset in 0 1 2 3 4; do \
			procModeStokes="`expr $$procMode + $$offset`"; \
			echo "Running lofar_udp_extractor -i ./tests/udp_1613%d_sample.zst -o './tests/output_'$$procModeStokes'_%d' -p $$procModeStokes -m 501"; \
			lofar_udp_extractor -i ./tests/udp_1613%d_sample.zst -o './tests/output_'$$procModeStokes'_%d' -p $$procModeStokes -m 501; \
		done; \
	done

	# . === source
	. hashVariables.txt; for output in ./tests/*; do \
		base=$$(basename $$output); \
		if [ "`md5sum $$output`" != $${!base} ]; then \
			echo "Processed output $$output does not match expected hash. Exiting."; \
		fi; \
	done;

make-test-hashes:
	touch ./tests/hashVariables.txt
	for fil in ./tests/output*; \
		do outp=$$(md5sum $$fil); \
		base=$$(basename $$fil); \
		echo $$base='"'$$outp'"' >> ./tests/hashVariables.txt; \
	done


mockHeader:
	git clone https://github.com/David-McKenna/mockHeader && \
	cd mockHeader && \
	make

