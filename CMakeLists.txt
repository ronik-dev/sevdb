cmake_minimum_required(VERSION 3.11)
project(MyC17Project LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

#-----------------
#- THE LIBRARIES -
#-----------------

add_library(sevdb STATIC
    src/internal.c
)

target_include_directories(sevdb PUBLIC include)
target_include_directories(sevdb PRIVATE src)

#---------------------
#- THE TESTING SUITE -
#---------------------

enable_testing()
add_executable(test_suite tests/test.c)
find_library(CRITERION_LIB criterion REQUIRED)
target_link_libraries(test_suite PRIVATE sevdb ${CRITERION_LIB})
add_test(NAME run_tests COMMAND test_suite)
