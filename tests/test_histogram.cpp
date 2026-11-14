// test_histogram.cpp -- Histogram and CDF tests.
//
// Hand-computed fixture: a 4x4 image with known pixels
//   row 0: 0,   0,   0,   0
//   row 1: 0,   0,   0,   0
//   row 2: 0,   0,   50,  50
//   row 3: 50,  50,  100, 100
// Counts: h(0) = 10, h(50) = 4, h(100) = 2, everything else 0, N = 16.
// CDF: cdf(0) = 10, cdf(50) = 14, cdf(100) = 16, cdf(255) = 16.
// Normalised: p(0) = 10/16 = 0.625, p(50) = 0.25, p(100) = 0.125.

#include "histogram.h"
#include "test_framework.h"

using namespace ihe;

namespace {

// The 4x4 hand-computed fixture, shared with the other suites.
GrayImage fixture4x4() {
    GrayImage img;
    img.width = 4;
    img.height = 4;
    img.pixels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 100, 100};
    return img;
}

}  // namespace

void test_histogram() {
    const GrayImage img = fixture4x4();
    const Histogram h = computeHistogram(img);

    CHECK(h[0] == 10);
    CHECK(h[50] == 4);
    CHECK(h[100] == 2);
    CHECK(h[1] == 0);
    CHECK(h[255] == 0);
    CHECK(histogramTotal(h) == 16);

    const auto cdf = cumulativeHistogram(h);
    CHECK(cdf[0] == 10);
    CHECK(cdf[1] == 10);
    CHECK(cdf[49] == 10);
    CHECK(cdf[50] == 14);
    CHECK(cdf[99] == 14);
    CHECK(cdf[100] == 16);
    CHECK(cdf[255] == 16);

    const auto prob = normalizedHistogram(h);
    CHECK_NEAR(prob[0], 0.625, 1e-12);
    CHECK_NEAR(prob[50], 0.25, 1e-12);
    CHECK_NEAR(prob[100], 0.125, 1e-12);
    double sum = 0.0;
    for (double p : prob) sum += p;
    CHECK_NEAR(sum, 1.0, 1e-12);

    // Empty image: all-zero histogram, zero total, zero probabilities.
    GrayImage empty;
    empty.width = 0;
    empty.height = 0;
    const Histogram he = computeHistogram(empty);
    CHECK(histogramTotal(he) == 0);
    CHECK(cumulativeHistogram(he)[255] == 0);
}
