cmake_minimum_required(VERSION 3.11)
project(sevdb VERSION 0.1.0 LANGUAGES C DESCRIPTION "one small embedded vector database")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)


#-----------------
#- THE LIBRARIES -
#-----------------

add_library(sevdb STATIC
    src/sevdb.c
    src/distance.c
    src/pqueue.c
    src/checksum.c
    src/persistence.c
)
target_include_directories(sevdb PUBLIC include)
target_include_directories(sevdb PRIVATE src)
# math standard library
target_link_libraries(sevdb PRIVATE m)

#--------------
#- VERSIONING -
#--------------

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sevdb/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/sevdb/version.h
)
target_include_directories(sevdb PUBLIC ${CMAKE_BINARY_DIR}/include)

#---------------------
#- THE TESTING SUITE -
#---------------------
option(SEVDB_BUILD_TESTS "build sevdb test suite" ON)
if(SEVDB_BUILD_TESTS)
    enable_testing()
    add_executable(internal_test_suite tests/internal_test.c)
    add_executable(distance_test_suite tests/distance_test.c)
    add_executable(priority_queue_test_suite tests/pqueue_test.c)
    add_executable(benchmark tests/benchmark.c)
    find_library(CRITERION_LIB criterion REQUIRED)
    target_link_libraries(internal_test_suite PRIVATE sevdb ${CRITERION_LIB})
    target_link_libraries(distance_test_suite PRIVATE sevdb ${CRITERION_LIB})
    target_link_libraries(priority_queue_test_suite PRIVATE sevdb ${CRITERION_LIB})
    target_link_libraries(benchmark PRIVATE sevdb)
    add_test(NAME internal_tests COMMAND internal_test_suite)
    add_test(NAME distance_tests COMMAND distance_test_suite)
    add_test(NAME priority_queue_tests COMMAND priority_queue_test_suite)
endif()
