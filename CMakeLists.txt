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

message("Attempting to detect HDF5 libraries")
include(FetchContent)
#set(HDF5_USE_STATIC_LIBRARIES ON)
#find_package(HDF5 COMPONENTS C CXX)
find_package(HDF5 REQUIRED COMPONENTS C CXX)
#if (NOT 0)
#	message("HDF5 libraries not found, prepping CMake build.")
#	set(HDF5_EXTERNALLY_CONFIGURED 1)
#	set(HDF5_INSTALL_NO_DEVELOPMENT 1)
#	FetchContent_Declare(HDF5
#                     GIT_REPOSITORY https://github.com/HDFGroup/hdf5
#                     GIT_TAG 1.12/master
#)
#	FetchContent_MakeAvailable(HDF5)
#	set(HDF5_C_LIBRARIES ${HDF5_LIB_TARGET})
#	set(HDF5_INCLUDE_DIRS "${HDF5_SRC_DIR};${HDF5_SRC_BINARY_DIR}")
#endif()
message("")

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
message("")

#message("Configuring CSpice")
#FetchContent_Declare(cspice
#                     GIT_REPOSITORY https://github.com/tudat-team/cspice-cmake
#                     )
#FetchContent_MakeAvailable(cspice)

message("")

message ("Configuring PSRDADA")
include(ExternalProject)
# Find the PSRDADA libraries on the system (or set NODADA if not found)
ExternalProject_Add(internal_PSRDADA
                    GIT_REPOSITORY https://git.code.sf.net/p/psrdada/code
                    GIT_TAG ba2b88
                    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/_deps/psrdada
                    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/psrdada/src/internal_PSRDADA/src/
                    CONFIGURE_COMMAND cd .. && ./bootstrap && ./configure --with-cuda-dir=no
                    BUILD_COMMAND "make" "libpsrdada.la"
                    INSTALL_COMMAND ""
                    UPDATE_COMMAND ""
                    )
ExternalProject_Get_Property(internal_PSRDADA install_dir)
set(psrdada_INSTALL_DIR ${install_dir})
add_library(psrdada STATIC IMPORTED)
set_property(TARGET psrdada PROPERTY IMPORTED_LOCATION ${psrdada_INSTALL_DIR}/src/internal_PSRDADA/src/.libs/libpsrdada.a)
add_dependencies(psrdada internal_PSRDADA)
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

add_dependencies(lofudpman libzstd_static psrdada) #yaml) #CSpice::cspice)

# Include all of our library headers
target_include_directories(lofudpman PUBLIC
		${PROJECT_SOURCE_DIR}/src/lib
		${PROJECT_SOURCE_DIR}/src/CLI
		${PROJECT_SOURCE_DIR}/src/metadata
)
# Include all of our dependency headers
target_include_directories(lofudpman PUBLIC ${HDF5_INCLUDE_DIRS})
target_include_directories(lofudpman PUBLIC ${psrdada_INSTALL_DIR}/src/internal_PSRDADA/src/)
target_include_directories(lofudpman PUBLIC ${zstd_SOURCE_DIR}/lib/)
#target_include_directories(lofudpman PUBLIC ${yaml_SOURCE_DIR}/include/)

file(GLOB_RECURSE UDP_INCLUDE_FILES "${PROJECT_SOURCE_DIR}/src/*/*.h" "${PROJECT_SOURCE_DIR}/src/*/*.hpp")

target_link_libraries(lofudpman PUBLIC ${HDF5_C_LIBRARIES})
target_link_libraries(lofudpman PUBLIC psrdada)
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


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
		lofudpmanConfigVersion.cmake
		VERSION ${PACKAGE_VERSION}
		COMPATIBILITY AnyNewerVersion
)

# Install everything
install(TARGETS lofudpman lofar_udp_extractor
		EXPORT lofudpman
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
)
install(FILES ${UDP_INCLUDE_FILES} DESTINATION include)
install(FILES src/misc/dreamBeamJonesGenerator.py DESTINATION bin)

# Add the tests directory
add_subdirectory(tests EXCLUDE_FROM_ALL)


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