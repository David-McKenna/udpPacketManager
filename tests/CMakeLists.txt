project(${CMAKE_PROJECT_NAME}_tests)
enable_testing()

include(FetchContent)

FetchContent_Declare(GOOGLETEST
    GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(GOOGLETEST)

add_subdirectory(lib_tests)