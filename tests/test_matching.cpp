// test_matching.cpp -- Histogram matching tests.
//
// Hand-computed case: source histogram h(0) = 8, h(255) = 8, N = 16.
// Uniform target with total 16: largest-remainder apportionment gives
// one count to bins 0..15 and zero elsewhere, so cdfT(k) = k+1 for
// k < 16 and 16 beyond. Source CDF: cdfS(0) = 8, cdfS(255) = 16.
//   LUT(0)   = smallest k with cdfT(k) >= 8  -> k = 7 (cdfT(7) = 8)
//   LUT(255) = smallest k with cdfT(k) >= 16 -> k = 15
// Note the target is the *discrete* uniform histogram over 256 bins;
// with only 16 counts it is a staircase, not a smooth ramp -- matching
// to uniform equals HE only in the continuous limit (see
// docs/RESEARCH.md).
//
// Reference-image case: reference constant 100 (16 px), so its CDF is
// a step at 100; every source level maps to 100.

#include "matching.h"
#include "test_framework.h"

using namespace ihe;

void test_matching() {
    Histogram source{};
    source.fill(0);
    source[0] = 8;
    source[255] = 8;

    // --- uniform target, hand-computed discrete result ---
    const Histogram uniform = uniformTargetHistogram(16);
    CHECK(histogramTotal(uniform) == 16);
    CHECK(uniform[0] == 1);
    CHECK(uniform[15] == 1);
    CHECK(uniform[16] == 0);
    CHECK(uniform[255] == 0);
    const auto lutU = buildMatchingLut(source, uniform);
    CHECK(lutU[0] == 7);
    CHECK(lutU[255] == 15);
    // The LUT is non-decreasing.
    for (int k = 0; k < 255; ++k) CHECK(lutU[k] <= lutU[k + 1]);

    GrayImage img;
    img.width = 4;
    img.height = 4;
    img.pixels = {0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 0, 255, 0, 255};
    const GrayImage matched = matchHistogram(img, uniform);
    for (std::uint8_t v : matched.pixels) CHECK(v == 7 || v == 15);
    CHECK(computeHistogram(matched)[7] == 8);
    CHECK(computeHistogram(matched)[15] == 8);

    // --- Gaussian target: counts sum exactly, peak near the mean ---
    const Histogram gauss = gaussianTargetHistogram(10000, 127.5, 32.0);
    CHECK(histogramTotal(gauss) == 10000);
    int peakBin = 0;
    for (int k = 1; k < 256; ++k) {
        if (gauss[k] > gauss[peakBin]) peakBin = k;
    }
    CHECK(peakBin >= 120 && peakBin <= 135);
    CHECK(gauss[0] < gauss[peakBin]);
    CHECK(gauss[255] < gauss[peakBin]);

    // --- matching to a constant reference ---
    GrayImage ref;
    ref.width = 4;
    ref.height = 4;
    ref.pixels.assign(16, 100);
    const GrayImage toRef = matchToReference(img, ref);
    for (std::uint8_t v : toRef.pixels) CHECK(v == 100);

    // Matching an image to its own histogram is the identity.
    const GrayImage self = matchToReference(img, img);
    CHECK(self.pixels == img.pixels);
}
