// matching.h -- Histogram matching (histogram specification).
//
// Generalises equalization: instead of flattening the histogram towards
// uniform, the image is remapped so its histogram follows a chosen
// target -- a uniform distribution, a Gaussian, or the histogram of a
// reference image (Gonzalez & Woods, Digital Image Processing, ch. 3).
//
// The mapping is the standard CDF inversion: each source level s maps
// to the smallest target level t whose target CDF is >= the source CDF
// at s. The LUT is non-decreasing, so brightness order is preserved.
// With a uniform target over a large image this reduces (in the
// continuous limit) to plain histogram equalization; for small images
// the discrete target histogram matters -- see tests/test_matching.cpp
// for a hand-computed 4x4 case.

#pragma once

#include "histogram.h"
#include "image.h"

namespace ihe {

// Build the source -> target lookup table by CDF inversion.
std::array<std::uint8_t, 256> buildMatchingLut(const Histogram& sourceHist,
                                               const Histogram& targetHist);

// Target histogram with the given total spread as evenly as possible
// over all 256 levels (largest-remainder apportionment, so the counts
// sum to exactly totalPixels).
Histogram uniformTargetHistogram(int totalPixels);

// Target histogram shaped as a Gaussian with the given mean and
// standard deviation, apportioned to sum to exactly totalPixels.
Histogram gaussianTargetHistogram(int totalPixels, double mean, double stddev);

// Remap img so its histogram follows targetHist.
GrayImage matchHistogram(const GrayImage& img, const Histogram& targetHist);

// Remap img so its histogram follows the histogram of reference.
GrayImage matchToReference(const GrayImage& img, const GrayImage& reference);

}  // namespace ihe
