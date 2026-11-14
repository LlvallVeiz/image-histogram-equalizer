cmake_minimum_required(VERSION 3.16)
project(image-histogram-equalizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

add_executable(ihe
    src/main.cpp
    src/image.cpp
    src/histogram.cpp
    src/equalize.cpp
    src/clahe.cpp
    src/matching.cpp
    src/stats.cpp
)

add_executable(ihe_tests
    tests/test_main.cpp
    tests/test_histogram.cpp
    tests/test_equalize.cpp
    tests/test_clahe.cpp
    tests/test_matching.cpp
    tests/test_stats.cpp
    tests/test_io.cpp
    src/image.cpp
    src/histogram.cpp
    src/equalize.cpp
    src/clahe.cpp
    src/matching.cpp
    src/stats.cpp
)

enable_testing()
add_test(NAME unit_tests COMMAND ihe_tests)
