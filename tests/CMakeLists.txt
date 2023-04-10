project(${CMAKE_PROJECT_NAME}_tests)

include(FetchContent)

option(BUILD_GMOCK "" OFF)
FetchContent_Declare(GOOGLETEST
    GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(GOOGLETEST)

add_custom_target(generateTestData
                  COMMAND bash ./rebuild.sh
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/referenceFiles)

add_custom_target(linkTestData
                  COMMAND ${CMAKE_COMMAND} -E create_symlink
                    ${CMAKE_CURRENT_SOURCE_DIR}/referenceFiles
                    ${CMAKE_CURRENT_BINARY_DIR}/lib_tests/referenceFiles
                  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/lib_tests/referenceFiles
                  DEPENDS generateTestData)

add_subdirectory(lib_tests)