// test_stats.cpp -- Statistics and comparison-metric tests.
//
// Hand-computed values for the 4x4 fixture (0x10, 50x4, 100x2, N=16):
//   mean     = (10*0 + 4*50 + 2*100) / 16 = 400/16 = 25.0
//   variance = (10*25^2 + 4*25^2 + 2*75^2) / 16
//            = (6250 + 2500 + 11250) / 16 = 20000/16 = 1250
//   stddev   = sqrt(1250) = 35.35533905932738
//   entropy  = -(0.625*log2 0.625 + 0.25*log2 0.25 + 0.125*log2 0.125)
//            = 1.2987949406953985 bits
//   min = 0, max = 100
//
// MSE/PSNR: two 2x2 images differing by 1 in a single pixel:
//   MSE = 1/4 = 0.25
//   PSNR = 10*log10(255^2 / 0.25) = 10*log10(260100) = 54.1514 dB

#include <cmath>
#include <limits>

#include "stats.h"
#include "test_framework.h"

using namespace ihe;

void test_stats() {
    GrayImage img;
    img.width = 4;
    img.height = 4;
    img.pixels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 100, 100};
    const ImageStats s = computeStats(img);
    CHECK(s.pixelCount == 16);
    CHECK_NEAR(s.mean, 25.0, 1e-9);
    CHECK_NEAR(s.stddev, 35.35533905932738, 1e-6);
    CHECK_NEAR(s.entropy, 1.2987949406953985, 1e-9);
    CHECK(s.minValue == 0);
    CHECK(s.maxValue == 100);

    // Constant image: zero spread, zero entropy.
    GrayImage constant;
    constant.width = 5;
    constant.height = 5;
    constant.pixels.assign(25, 77);
    const ImageStats sc = computeStats(constant);
    CHECK_NEAR(sc.mean, 77.0, 1e-9);
    CHECK_NEAR(sc.stddev, 0.0, 1e-12);
    CHECK_NEAR(sc.entropy, 0.0, 1e-12);
    CHECK(sc.minValue == 77 && sc.maxValue == 77);

    // MSE / PSNR.
    GrayImage a, b;
    a.width = b.width = 2;
    a.height = b.height = 2;
    a.pixels = {10, 20, 30, 40};
    b.pixels = {10, 20, 30, 41};
    CHECK_NEAR(meanSquaredError(a, a), 0.0, 1e-12);
    CHECK(std::isinf(psnr(a, a)));
    CHECK_NEAR(meanSquaredError(a, b), 0.25, 1e-12);
    CHECK_NEAR(psnr(a, b), 54.1514, 0.01);

    // Mismatched dimensions are an error, not a silent wrong answer.
    GrayImage c;
    c.width = 3;
    c.height = 2;
    c.pixels.assign(6, 0);
    bool threw = false;
    try {
        meanSquaredError(a, c);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    // statsReport mentions the numbers it claims.
    const std::string report = statsReport(img, "fixture");
    CHECK(report.find("25.00") != std::string::npos);
    CHECK(report.find("35.36") != std::string::npos);
    CHECK(report.find("1.30") != std::string::npos);
}
