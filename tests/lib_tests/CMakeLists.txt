include_directories(${GOOGLETEST_DIR}/include ${GOOGLETEST_DIR})

add_executable(lib_tests_run
               lib_main_tests.cpp
               lib_reader_tests.cpp
               lib_backends_tests.cpp
               lib_cli_tests.cpp
               lib_io_tests.cpp
               lib_metadata_tests.cpp
               lib_structs_tests.cpp
               lib_time_tests.cpp
)

target_link_libraries(lib_tests_run gtest_main lofudpman)

add_test(NAME lib_tests COMMAND lib_tests_run)