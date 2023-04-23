include_directories(${GOOGLETEST_DIR}/include ${GOOGLETEST_DIR})

add_executable(lib_tests_run
               lib_main_tests.cpp
               lib_reader_tests.cpp
               lib_cli_tests.cpp
               lib_io_tests.cpp
               lib_metadata_tests.cpp
               lib_structs_tests.cpp
               lib_time_tests.cpp)


option(NO_CAL_TEST "Don't run calibration tests" $ENV{NO_CAL_TEST})

if (NO_CAL_TEST)
	message("NO_CAL_TEST detected; disabling calibration tests.")
	target_compile_definitions(lib_tests_run PRIVATE NO_CAL_TEST)
endif()
add_dependencies(lib_tests_run linkTestData)
target_link_libraries(lib_tests_run gtest_main lofudpman)

add_test(NAME lib_tests COMMAND lib_tests_run)
#set_property(TEST lib_tests PROPERTY ENVIRONMENT_MODIFICATION "PATH=path_list_append:${CMAKE_CURRENT_SOURCE_DIR}/../../src/misc/")