cmake_minimum_required(VERSION 3.11)
project(sevdb LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

#-----------------
#- THE LIBRARIES -
#-----------------

add_library(sevdb STATIC
    src/internal.c
    src/distance.c
    src/pqueue.c
)

target_include_directories(sevdb PUBLIC include)
target_include_directories(sevdb PRIVATE src)
# math standard library
target_link_libraries(sevdb PRIVATE m)

#---------------------
#- THE TESTING SUITE -
#---------------------

enable_testing()
add_executable(internal_test_suite tests/internal_test.c)
add_executable(distance_test_suite tests/distance_test.c)
add_executable(priority_queue_test_suite tests/pqueue_test.c)
find_library(CRITERION_LIB criterion REQUIRED)
target_link_libraries(internal_test_suite PRIVATE sevdb ${CRITERION_LIB})
target_link_libraries(distance_test_suite PRIVATE sevdb ${CRITERION_LIB})
target_link_libraries(priority_queue_test_suite PRIVATE sevdb ${CRITERION_LIB})
add_test(NAME run_tests COMMAND internal_test_suite && distance_test_suite && priority_queue_test_suite)
