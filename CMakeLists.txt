# Based on what I learnt from https://cliutils.gitlab.io/modern-cmake
cmake_minimum_required(VERSION 3.14...3.19)

# Create a release output by default
set(default_build_type "Release")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

# Describe the project
project(udpPacketManager 	VERSION 0.7.0
							DESCRIPTION "C Library for handling CEP packet data from an international LOFAR station"
							LANGUAGES C CXX
)

set(UPM_CLI_VERSION 0.6)

message("Building udpPacketManager Version ${PROJECT_VERSION}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -W -Wall -DALLOW_VERBOSE -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -W -Wall -DALLOW_VERBOSE -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

# Find the OpenMP package on the system

message("Configuring OpenMP...")
find_package(OpenMP)
if(OpenMP_C_LIB_NAMES MATCHES ".*(iomp).*")
	message ("Intel OpenMP Detected.")
	# Determine the optimal number of threads for this machine (when using Intel OpenMP Libraries)
	execute_process(COMMAND bash -c "cat /proc/cpuinfo | uniq | grep -m 2 \"siblings\" | cut -d \":\" -f 2 | sort --numeric --unique | xargs echo"
			OUTPUT_VARIABLE OMP_THREADS
	)
else()
	message(WARNING "Other OpenMP implementation Detected (performance may be degraded, we highly recommend using the Intel OpenMP libraries)")
	set(OMP_THREADS 8)
endif()

# Find the zstandard libraries on the system
message("Configuring Zstandard")
include(FetchContent)
FetchContent_Declare(zstd
                     GIT_REPOSITORY https://github.com/facebook/zstd.git
                     GIT_TAG v1.4.9
                     )
FetchContent_MakeAvailable(zstd)
add_subdirectory(${zstd_SOURCE_DIR}/build/cmake/)
message("")

message ("Configuring PSRDADA")
include(ExternalProject)
# Find the PSRDADA libraries on the system (or set NODADA if not found)
ExternalProject_Add(internal_PSRDADA
                    GIT_REPOSITORY https://git.code.sf.net/p/psrdada/code
                    GIT_TAG ba2b88
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_deps/psrdada
                    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/psrdada/src/internal_PSRDADA/src/
                    CONFIGURE_COMMAND cd .. && ./bootstrap && ./configure
                    BUILD_COMMAND "make" "libpsrdada.la"
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    )
ExternalProject_Get_Property(internal_PSRDADA install_dir)
add_library(psrdada STATIC IMPORTED)
set_property(TARGET psrdada PROPERTY IMPORTED_LOCATION ${install_dir}/src/internal_PSRDADA/src/.libs/libpsrdada.a)
add_dependencies(psrdada internal_PSRDADA)
message("")





# Include compile-time parameters into the headers
configure_file(	"${PROJECT_SOURCE_DIR}/src/lib/lofar_udp_general.h.in"
				"${PROJECT_SOURCE_DIR}/src/lib/lofar_udp_general.h"
)
configure_file(	"${PROJECT_SOURCE_DIR}/src/CLI/lofar_cli_meta.h.in"
				"${PROJECT_SOURCE_DIR}/src/CLI/lofar_cli_meta.h"
)


# Setup the base library object
add_library(lofudpman STATIC
            src/lib/lofar_udp_backends.cpp
            src/lib/lofar_udp_misc.c
            src/lib/lofar_udp_io.c
            src/lib/lofar_udp_reader.c
            src/lib/lofar_udp_structs.c
            src/CLI/lofar_cli_meta.c
            src/lib/lofar_udp_metadata.c
            src/lib/metadata/ascii_hdr_manager.c
            src/lib/metadata/sigproc_hdr_manager.c src/lib/metadata/metadata_structs.h src/lib/metadata/metadata_structs.c
            src/lib/metadata/dada_hdr_manager.c)
# Include all of our headers
target_include_directories(lofudpman PUBLIC
		${PROJECT_SOURCE_DIR}/src/lib
		${PROJECT_SOURCE_DIR}/src/CLI
		${PROJECT_SOURCE_DIR}/src/metadata
)
file(GLOB_RECURSE UDP_INCLUDE_FILES "${PROJECT_SOURCE_DIR}/src/*/*.h" "${PROJECT_SOURCE_DIR}/src/*/*.hpp")

target_link_libraries(lofudpman INTERFACE psrdada)
target_link_libraries(lofudpman INTERFACE zstd)

# Set a few extra compiler options
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
set_property(TARGET lofudpman PROPERTY LINKER_LANGUAGE C)
set_property(TARGET lofudpman PROPERTY LINK_WHAT_YOU_USE ON)
set_property(TARGET lofudpman PROPERTY POSITION_INDEPENDENT_CODE ON)
#set_property(TARGET lofudpman PROPERTY INTERPROCEDURAL_OPTIMIZATION ON) # Static + IPO -> build failures?

# Link OpenMP
if(OpenMP_CXX_FOUND)
	target_link_libraries(lofudpman PUBLIC OpenMP::OpenMP_CXX OpenMP::OpenMP_C)
else()
	message(FATAL_ERROR "ERROR: OpenMP Not found.")
endif()


#get_target_property(OLD_SUFFIX lofudpman SUFFIX)
# Version the library suffix
#get_target_property(LIBTYPE lofudpman TYPE)
#if(LIBTYPE STREQUAL STATIC_LIBRARY)
#	set_property(TARGET lofudpman PROPERTY SUFFIX ".a.${PROJECT_VERSION}")
#elseif(LIBTYPE STREQUAL SHARED_LIBRARY)
#	set_property(TARGET lofudpman PROPERTY SUFFIX ".so.${PROJECT_VERSION}")
#endif()


# Setup the CLIs
add_executable(lofar_udp_extractor src/CLI/lofar_cli_extractor.c)
target_link_libraries(lofar_udp_extractor PUBLIC lofudpman)

add_executable(lofar_udp_guppi_raw src/CLI/lofar_cli_guppi_raw.c)
target_link_libraries(lofar_udp_guppi_raw PUBLIC lofudpman)



include(CMakePackageConfigHelpers)
write_basic_package_version_file(
		lofudpmanConfigVersion.cmake
		VERSION ${PACKAGE_VERSION}
		COMPATIBILITY AnyNewerVersion
)

# Install everything
install(TARGETS lofudpman lofar_udp_extractor lofar_udp_guppi_raw
		EXPORT lofudpman
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
)
install(FILES ${UDP_INCLUDE_FILES} DESTINATION include)

# Add the tests directory
add_subdirectory(tests EXCLUDE_FROM_ALL)