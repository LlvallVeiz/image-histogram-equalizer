// stats.h -- Image statistics and image comparison metrics.
//
// Mean / standard deviation / min / max describe the brightness and
// contrast of an image; Shannon entropy (base 2, in bits, from the
// normalized histogram) measures how spread out the gray-level
// distribution is -- a successfully equalized image has entropy close
// to 8 bits. MSE and PSNR (peak signal-to-noise ratio, in dB, with
// peak 255) quantify how much an enhancement changed an image.

#pragma once

#include <limits>
#include <string>

#include "image.h"

namespace ihe {

struct ImageStats {
    double mean = 0.0;
    double stddev = 0.0;
    double entropy = 0.0;  // Shannon entropy, bits
    int minValue = 0;
    int maxValue = 0;
    std::size_t pixelCount = 0;
};

ImageStats computeStats(const GrayImage& img);

// Mean squared error between two same-size images.
double meanSquaredError(const GrayImage& a, const GrayImage& b);

// PSNR in dB; +infinity when the images are identical (MSE == 0).
double psnr(const GrayImage& a, const GrayImage& b);

// One human-readable block: label, dimensions, mean, stddev, min/max,
// entropy. Used by the `stats` and `compare` subcommands.
std::string statsReport(const GrayImage& img, const std::string& label);

}  // namespace ihe
