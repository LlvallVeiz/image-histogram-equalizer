// test_framework.h -- Minimal assert-based test framework (no external
// dependencies). Each check increments a counter; failures print the
// expression, file and line, and the process exits non-zero.

#pragma once

#include <cmath>
#include <iostream>

namespace test {

inline int& checks() {
    static int n = 0;
    return n;
}
inline int& failures() {
    static int n = 0;
    return n;
}

inline void check(bool cond, const char* expr, const char* file, int line) {
    ++checks();
    if (!cond) {
        ++failures();
        std::cerr << "FAIL " << file << ':' << line << ": " << expr << '\n';
    }
}

inline void checkNear(double actual, double expected, double tol, const char* expr,
                      const char* file, int line) {
    ++checks();
    if (!(std::fabs(actual - expected) <= tol)) {
        ++failures();
        std::cerr << "FAIL " << file << ':' << line << ": " << expr << " -- got " << actual
                  << ", expected " << expected << " (+/- " << tol << ")\n";
    }
}

inline int report(const char* suite) {
    std::cout << suite << ": " << (checks() - failures()) << '/' << checks()
              << " checks passed";
    if (failures() == 0) {
        std::cout << " -- OK\n";
        return 0;
    }
    std::cout << " -- " << failures() << " FAILURES\n";
    return 1;
}

}  // namespace test

#define CHECK(cond) ::test::check((cond), #cond, __FILE__, __LINE__)
#define CHECK_NEAR(actual, expected, tol) \
    ::test::checkNear((actual), (expected), (tol), #actual " ~= " #expected, __FILE__, __LINE__)
