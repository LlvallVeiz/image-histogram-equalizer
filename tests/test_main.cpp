// test_main.cpp -- Runs every test suite and reports the total.

#include "test_framework.h"

void test_histogram();
void test_equalize();
void test_clahe();
void test_matching();
void test_stats();
void test_io();

int main() {
    test_histogram();
    test_equalize();
    test_clahe();
    test_matching();
    test_stats();
    test_io();
    return test::report("image-histogram-equalizer unit tests");
}
