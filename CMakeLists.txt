cmake_minimum_required(VERSION 3.10)

project(json_parser)


add_library(json_parser STATIC
    src/json_parser/json.cpp
    src/json_parser/detail.cpp
)

target_include_directories(json_parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/detail
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(json_parser PUBLIC cxx_std_20)

enable_testing()

add_executable(test_arrays tests/test_arrays.cpp)
target_link_libraries(test_arrays json_parser)
add_test(NAME test_arrays COMMAND test_arrays)

add_executable(test_objects tests/test_objects.cpp)
target_link_libraries(test_objects json_parser)
add_test(NAME test_objects COMMAND test_objects)

add_executable(test_invalid tests/test_invalid.cpp)
target_link_libraries(test_invalid json_parser)
add_test(NAME test_invalid COMMAND test_invalid)

add_executable(test_casting tests/test_casting.cpp)
target_link_libraries(test_casting json_parser)
add_test(NAME test_casting COMMAND test_casting)

add_executable(test_memory tests/test_memory.cpp)
target_link_libraries(test_memory json_parser)
add_test(NAME test_memory COMMAND test_memory)

add_executable(test_strings tests/test_strings.cpp)
target_link_libraries(test_strings json_parser)
add_test(NAME test_strings COMMAND test_strings)