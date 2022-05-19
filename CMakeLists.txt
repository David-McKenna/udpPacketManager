# Based on what I learnt from https://cliutils.gitlab.io/modern-cmake
cmake_minimum_required(VERSION 3.14...3.19)

# Create a release output by default
set(default_build_type "Release")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ALLOW_VERBOSE "Enable compile-time verbose mode" OFF)
if (ALLOW_VERBOSE OR (CMAKE_BUILD_TYPE STREQUAL Debug))
	set(CMAKE_VERBOSE_MAKEFILE ON)
	message("Verbose mode enabled")
endif()

# Describe the project
project(udpPacketManager 	VERSION 0.7.0
							DESCRIPTION "C Library for handling CEP packet data from an international LOFAR station"
							LANGUAGES C CXX
)

set(UPM_CLI_VERSION 0.7)

message("Building udpPacketManager Version ${PROJECT_VERSION}\n\n")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -W -Wall -DALLOW_VERBOSE -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -W -Wall -DALLOW_VERBOSE -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

# Find the OpenMP package on the system

message("Configuring OpenMP...")
find_package(OpenMP REQUIRED)

execute_process(COMMAND bash -c "cat /proc/cpuinfo | uniq | grep -m 2 \"siblings\" | cut -d \":\" -f 2 | sort --numeric --unique | xargs echo"
                OUTPUT_VARIABLE OMP_THREADS
                )
if(OpenMP_C_LIB_NAMES MATCHES ".*(iomp).*" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message ("Intel-like OpenMP Detected.")
	# Determine the optimal number of threads for this machine (when using Intel OpenMP Libraries)

else()
	message(WARNING "Other OpenMP implementation Detected (performance may be degraded, we highly recommend using the Intel OpenMP libraries)")
	set(OMP_THREADS 8)
endif()
message("")

# Find the zstandard libraries on the system
message("Configuring Zstandard")
include(FetchContent)
set(ZSTD_BUILD_STATIC ON CACHE INTERNAL "")
set(ZSTD_BUILD_SHARED OFF CACHE INTERNAL "")
FetchContent_Declare(zstd
                     GIT_REPOSITORY https://github.com/facebook/zstd.git
                     GIT_TAG v1.4.9
                     )
FetchContent_MakeAvailable(zstd)
# MakeAvailable doesn't allow for libzstd_static to be used as a target
# Related? https://github.com/facebook/zstd/issues/2377
add_subdirectory(${zstd_SOURCE_DIR}/build/cmake/ ${zstd_BINARY_DIR} EXCLUDE_FROM_ALL)
message("Hello ${zstd_SOURCE_DIR}/lib/")
message("")

#message("Configuring CSpice")
#FetchContent_Declare(cspice
#                     GIT_REPOSITORY https://github.com/tudat-team/cspice-cmake
#                     )
#FetchContent_MakeAvailable(cspice)

message("")

message("Configuring HDF5 libraries")

# zlib is "required" by hdf5. If it is present anywhere on the user's path it will be enabled and included, I cannot disable this for the life of me.
include(ExternalProject)
ExternalProject_Add(internal_zlib
                    GIT_REPOSITORY https://github.com/madler/zlib
                    GIT_TAG v1.2.12
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_deps/zlib
                    CONFIGURE_COMMAND ./configure --static --prefix=./
                    BUILD_COMMAND "make" "test" "install" "-j8"
                    BUILD_IN_SOURCE TRUE # Keep source beside install prefix
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    )
ExternalProject_Get_Property(internal_zlib INSTALL_DIR)
# ${HDF5_INCLUDE_DIRS}
set(zlib_INSTALL_DIR ${INSTALL_DIR})
add_library(libz STATIC IMPORTED)
set_property(TARGET libz PROPERTY IMPORTED_LOCATION ${zlib_INSTALL_DIR}/src/internal_zlib/libz.a)
add_dependencies(libz internal_zlib)
message("")
#find_package(HDF5 REQUIRED COMPONENTS C CXX)
#set(HDF5_EXTERNALLY_CONFIGURED ON CACHE INTERNAL "")
#set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
#set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
#set(HDF5_EXTERNALLY_CONFIGURED ON CACHE INTERNAL "")
#set(HDF5_INSTALL_NO_DEVELOPMENT ON CACHE INTERNAL "")
#set(HDF5_BUILD_CPP_LIB ON CACHE INTERNAL "")
#set(HDF5_ENABLE_THREADSAFE OFF CACHE INTERNAL "")
#set(HDF5_BUILD_EXAMPLES OFF CACHE INTERNAL "")
#set(HDF5_BUILD_TOOLS OFF CACHE INTERNAL "")
#set(HDF5_BUILD_UTILS OFF CACHE INTERNAL "")
#set(HDF5_BUILD_HL_LIB OFF CACHE INTERNAL "")
#set(HDF5_BUILD_HL_TOOLS OFF CACHE INTERNAL "")
#set(BUILD_TESTING OFF CACHE INTERNAL "")
#FetchContent_Declare(HDF5
#                     GIT_REPOSITORY https://github.com/HDFGroup/hdf5
#                     GIT_TAG hdf5-1_12_2
#                     )
#FetchContent_MakeAvailable(HDF5)
#set(HDF5_INCLUDE_DIRS "${HDF5_SRC_SOURCE_DIR};${HDF5_SRC_BINARY_DIR}")
ExternalProject_Add(internal_hdf5
                    GIT_REPOSITORY https://github.com/HDFGroup/hdf5
                    GIT_TAG hdf5-1_12_2
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_deps/hdf5
                    CONFIGURE_COMMAND ./configure --disable-dependency-tracking --enable-optimization=high --disable-shared --disable-cxx --disable-hl --disable-hltools --disable-tools --enable-threadsafe --disable-fortran --enable-build-mode=production --with-zlib=/dev/null
                    BUILD_COMMAND "make" "all" "-j8"
                    BUILD_IN_SOURCE TRUE # Header gets moved to incorrect folder otherwise
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    DEPENDS internal_zlib
)
ExternalProject_Get_Property(internal_hdf5 INSTALL_DIR)
set(hdf5_INSTALL_DIR ${INSTALL_DIR})
add_library(libhdf5 STATIC IMPORTED)
set_property(TARGET libhdf5 PROPERTY IMPORTED_LOCATION ${hdf5_INSTALL_DIR}/src/internal_hdf5/src/.libs/libhdf5.a)
add_dependencies(libhdf5 internal_hdf5)
message("")

message("Configuring HDF5 plugins")
ExternalProject_Add(internal_bitshuffle
                    GIT_REPOSITORY https://github.com/kiyo-masui/bitshuffle
                    GIT_TAG fdfcd404ac8dcb828857a90c559d36d8ac4c2968
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_deps/bitshuffle
                    CONFIGURE_COMMAND cp ${PROJECT_SOURCE_DIR}/src/misc/bitshuffleMakefile ${CMAKE_CURRENT_BINARY_DIR}/_deps/bitshuffle/src/internal_bitshuffle/Makefile
                    BUILD_COMMAND CMAKE_BASE_DIR=${CMAKE_CURRENT_BINARY_DIR} make link -j8
                    BUILD_IN_SOURCE TRUE
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    DEPENDS internal_hdf5 zstd internal_zlib
                    )
ExternalProject_Get_Property(internal_bitshuffle INSTALL_DIR)
set(bitshuffle_INSTALL_DIR ${INSTALL_DIR})
add_library(libh5bshuf STATIC IMPORTED)
set_property(TARGET libh5bshuf PROPERTY IMPORTED_LOCATION ${bitshuffle_INSTALL_DIR}/src/internal_bitshuffle/libh5bshuf.a)
add_dependencies(libh5bshuf internal_bitshuffle)
message("")


message ("Configuring PSRDADA")
# Find the PSRDADA libraries on the system (or set NODADA if not found)
ExternalProject_Add(internal_PSRDADA
                    GIT_REPOSITORY https://git.code.sf.net/p/psrdada/code
                    GIT_TAG ba2b88
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_deps/psrdada
                    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/psrdada/src/internal_PSRDADA/src/
                    CONFIGURE_COMMAND cd .. && ./bootstrap && ./configure --disable-dependency-tracking --with-cuda-dir=no
                    BUILD_COMMAND "make" "libpsrdada.la" "dada_db" "dada_dbmeminfo" "dada_dbmetric" "dada_dbmonitor" "-j4"
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    )
ExternalProject_Get_Property(internal_PSRDADA INSTALL_DIR)
set(psrdada_INSTALL_DIR ${INSTALL_DIR})
add_library(libpsrdada STATIC IMPORTED)
set_property(TARGET libpsrdada PROPERTY IMPORTED_LOCATION ${psrdada_INSTALL_DIR}/src/internal_PSRDADA/src/.libs/libpsrdada.a)
add_dependencies(libpsrdada internal_PSRDADA)
message("")


# Configure FFTW3 (w/ floats + omp support, optional)

## CMake config is outdated as of FFTW3 3.3.10, manually building to get AVX-512 support for REALTA
message("Configuring FFTW3 (static, floats, OpenMP support enabled)")
#set(ENABLE_FLOAT ON CACHE INTERNAL "")
#set(ENABLE_OPENMP OFF CACHE INTERNAL "")
#set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
#set(BUILD_TESTS OFF CACHE INTERNAL "")
#
#FetchContent_Declare(fftw3f
#                     URL "https://www.fftw.org/fftw-3.3.10.tar.gz"
#                     )
#FetchContent_MakeAvailable(fftw3f)
# I do not understand why, but the clean build process fails without the "echo $(pwd)" command.
ExternalProject_ADD(internal_FFTW3F
                    URL "https://www.fftw.org/fftw-3.3.10.tar.gz"
                    URL_HASH "MD5=8ccbf6a5ea78a16dbc3e1306e234cc5c"
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_deps/fftw3
                    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/fftw3/src/internal_FFTW3F/
                    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/fftw3/src/internal_FFTW3F/build/
                    CONFIGURE_COMMAND echo $(pwd) && mkdir -p ./build/ && ./configure --disable-dependency-tracking --enable-float --enable-openmp --enable-static --prefix ${CMAKE_CURRENT_BINARY_DIR}/_deps/fftw3/src/internal_FFTW3F/build/
                    BUILD_COMMAND "make" "-j4" "&&" "make" "install" "-j4"
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    )
ExternalProject_Get_Property(internal_FFTW3F install_dir)
set(fftw3f_INSTALL_DIR ${install_dir})
add_library(libfftw3fomp STATIC IMPORTED)
set_property(TARGET libfftw3fomp PROPERTY IMPORTED_LOCATION ${fftw3f_INSTALL_DIR}/lib/libfftw3f_omp.a)
add_dependencies(libfftw3fomp internal_FFTW3F)
message("")


#message("Configuring LibYAML")
#FetchContent_Declare(yaml
#                     GIT_REPOSITORY https://github.com/yaml/libyaml.git
#                     GIT_TAG 2c891fc7a770e8ba2fec34fc6b545c672beb37e6 # Git tag for 0.2.5 is erroring
#                     )
#FetchContent_MakeAvailable(yaml)


# Include compile-time parameters into the headers
configure_file(	"${PROJECT_SOURCE_DIR}/src/lib/lofar_udp_general.h.in"
				"${PROJECT_SOURCE_DIR}/src/lib/lofar_udp_general.h"
				@ONLY
)

configure_file(	"${PROJECT_SOURCE_DIR}/src/CLI/lofar_cli_meta.h.in"
				"${PROJECT_SOURCE_DIR}/src/CLI/lofar_cli_meta.h"
)


# Setup the base library object
add_library(lofudpman STATIC
            src/lib/lofar_udp_general.c
            src/lib/lofar_udp_backends.cpp
            src/lib/lofar_udp_io.c
            src/lib/lofar_udp_reader.c
            src/lib/lofar_udp_structs.c
            src/CLI/lofar_cli_meta.c
            src/lib/lofar_udp_metadata.c
            src/lib/lofar_udp_time.c)

add_dependencies(lofudpman libfftw3fomp libzstd_static libpsrdada libhdf5 libh5bshuf) #yaml) #CSpice::cspice)

# Include all of our library headers
target_include_directories(lofudpman PUBLIC
		${PROJECT_SOURCE_DIR}/src/lib
		${PROJECT_SOURCE_DIR}/src/CLI
		${PROJECT_SOURCE_DIR}/src/metadata
)
# Include all of our dependency headers
target_include_directories(lofudpman PUBLIC ${fftw3f_INSTALL_DIR}/include/)
target_include_directories(lofudpman PUBLIC ${hdf5_INSTALL_DIR}/src/internal_hdf5/src/)
target_include_directories(lofudpman PUBLIC ${bitshuffle_INSTALL_DIR}/src/internal_bitshuffle/src/)
target_include_directories(lofudpman PUBLIC ${psrdada_INSTALL_DIR}/src/internal_PSRDADA/src/)
target_include_directories(lofudpman PUBLIC ${zstd_SOURCE_DIR}/lib/)
#target_include_directories(lofudpman PUBLIC ${yaml_SOURCE_DIR}/include/)

file(GLOB_RECURSE UDP_INCLUDE_FILES "${PROJECT_SOURCE_DIR}/src/*/*.h" "${PROJECT_SOURCE_DIR}/src/*/*.hpp")

target_link_libraries(lofudpman PUBLIC libfftw3fomp)
target_link_libraries(lofudpman PUBLIC libz) # hdf5 requirement, not ours
target_link_libraries(lofudpman PUBLIC libhdf5)
target_link_libraries(lofudpman PUBLIC libh5bshuf)
target_link_libraries(lofudpman PUBLIC libpsrdada)
target_link_libraries(lofudpman PUBLIC libzstd_static)
#target_link_libraries(lofudpman PUBLIC yaml)

# Set a few extra compiler options
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
set_property(TARGET lofudpman PROPERTY LINKER_LANGUAGE CXX)
set_property(TARGET lofudpman PROPERTY LINK_WHAT_YOU_USE ON)
#set_property(TARGET lofudpman PROPERTY INTERPROCEDURAL_OPTIMIZATION ON) # Static + IPO -> build failures?

# Link OpenMP
target_link_libraries(lofudpman PUBLIC OpenMP::OpenMP_CXX OpenMP::OpenMP_C)


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
add_executable(lofar_stokes_extractor src/CLI/lofar_cli_stokes.c)
target_link_libraries(lofar_stokes_extractor PUBLIC lofudpman)


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
		lofudpmanConfigVersion.cmake
		VERSION ${PACKAGE_VERSION}
		COMPATIBILITY AnyNewerVersion
)

# Install everything
install(TARGETS lofudpman lofar_udp_extractor lofar_stokes_extractor
		EXPORT lofudpman
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
)
install(FILES ${UDP_INCLUDE_FILES} DESTINATION include)
install(FILES src/misc/dreamBeamJonesGenerator.py DESTINATION bin)

# Add the tests directory
#add_subdirectory(tests EXCLUDE_FROM_ALL)


#include(CMakePackageConfigHelpers)
#write_basic_package_version_file(
#		lofudpmanConfig.cmake
#		VERSION ${PACKAGE_VERSION}
#		COMPATIBILITY SameMajorVersion
#)
#set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/lofudpman)
#install(EXPORT lofudpman
#        FILE lofudpmanTargets.cmake
#        NAMESPACE lofudpman::
#        DESTINATION ${ConfigPackageLocation}
#)
#install(FILES
#        lofudpmanConfig.cmake
#        "${CMAKE_CURRENT_BINARY_DIR}/lofudpmanConfigVersion.cmake"
#        DESTINATION ${ConfigPackageLocation}
#)