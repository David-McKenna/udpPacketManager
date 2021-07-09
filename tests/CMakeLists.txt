project(${CMAKE_PROJECT_NAME}_tests)

include(FetchContent)

FetchContent_Declare(GOOGLETEST
    GIT_REPOSITORY https://github.com/google/googletest.git
)
FetchContent_MakeAvailable(GOOGLETEST)

add_subdirectory(lib_tests)