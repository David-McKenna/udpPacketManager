CC 	= gcc-9
CFLAGS 	= -W -Wall -O3 -march=native -DVERSION=0.1 -DVERSIONCLI=0.0 -funswitch-loops #-DALLOW_VERBOSE #-D__SLOWDOWN
# -fopt-info-missed=compiler_report_missed.log -fopt-info-vec=compiler_report_vec.log -fopt-info-loop=compiler_report_loop.log -fopt-info-inline=compiler_report_inline.log -fopt-info-omp=compiler_report_omp.log

LFLAGS 	= -I./ -I /usr/include/ -lzstd -fopenmp

OBJECTS = lofar_cli_extractor.o lofar_udp_reader.o lofar_udp_misc.o


%.o: %.c
	$(CC) -c $(LFLAGS) -o ./$@ $< $(CFLAGS)

# Added a clean in here as -MD wasn't working for regenering objects based on new headers
all: clean $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -o ./lofar_udp_extractor $(LFLAGS)

newlinepad:	printf "\n\n\n\n"

clean:
	rm ./*.o; exit 0;
	rm ./*.d; exit 0;
	rm ./compiler_report_*.log; exit 0;
	rm ./lofar_udp_extractor; exit 0;


-include $(OBJECTS:.o=.d)
