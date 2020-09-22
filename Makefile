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

LIB_VER = 0.3
CLI_VER = 0.2

# Detemrine the max threads per socket to speed up execution via OpenMP with ICC (GCC falls over if we set too many)
THREADS = $(shell cat /proc/cpuinfo | uniq | grep -m 2 "siblings" | cut -d ":" -f 2 | sort --numeric --unique | awk '{printf("%d", $$1);}')

CFLAGS 	+= -march=native -W -Wall -O3 -march=native -DVERSION=$(LIB_VER) -DVERSIONCLI=$(CLI_VER) -fPIC # -DBENCHMARK -g -DALLOW_VERBOSE #-D__SLOWDOWN

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
	$(AR) rc $(LIBRARY_TARGET).$(LIB_VER) $(OBJECTS)
	cp ./$(LIBRARY_TARGET).$(LIB_VER) ./$(LIBRARY_TARGET) 

install: all
	mkdir -p $(PREFIX)/bin/ && mkdir -p $(PREFIX)/include/
	cp ./lofar_udp_extractor $(PREFIX)/bin/
	cp ./*.h $(PREFIX)/include/
	cp ./*.a* ${PREFIX}/lib/
	cp ./mockHeader/mockHeader $(PREFIX)/bin/; exit 0;

install-local: all
	mkdir -p ~/.local/bin/ && mkdir -p ~/.local/include/
	cp ./lofar_udp_extractor ~/.local/bin/
	cp ./*.h ~/.local/include/
	cp ./*.a* ~/.local/lib/
	cp ./mockHeader/mockHeader ~/.local/bin/; exit 0;

clean:
	rm ./src/CLI/*.o; exit 0;
	rm ./src/lib/*.o; exit 0;
	rm ./*.a; exit 0;
	rm ./*.a.*; exit 0;
	rm ./compiler_report_*.log; exit 0;
	rm ./lofar_udp_extractor; exit 0;
	rm ./lofar_udp_guppi_raw; exit 0;




mockHeader:
	git clone https://github.com/David-McKenna/mockHeader && \
	cd mockHeader && \
	make

