include_directories(${GOOGLETEST_DIR}/include ${GOOGLETEST_DIR})

add_executable(lib_tests_run lib_main_tests.cpp)

target_link_libraries(lib_tests_run gtest gtest_main)
target_link_libraries(lib_tests_run lofudpman)