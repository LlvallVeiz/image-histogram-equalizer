// histogram.h -- 256-bin grayscale histograms and cumulative distributions.
// The histogram is the diagnostic that every method in this project is
// built on: HE remaps through its CDF, CLAHE clips it per tile, and
// matching reshapes it towards a target.

#pragma once

#include <array>
#include <ostream>

#include "image.h"

namespace ihe {

// One counter per gray level, 0..255.
using Histogram = std::array<int, 256>;

Histogram computeHistogram(const GrayImage& img);

// cdf[k] = number of pixels with value <= k.
std::array<int, 256> cumulativeHistogram(const Histogram& hist);

// Probability of each level (sums to 1.0 for a non-empty image).
std::array<double, 256> normalizedHistogram(const Histogram& hist);

// Total number of pixels counted in the histogram.
int histogramTotal(const Histogram& hist);

// Print a compact text bar chart (one line per non-empty bin, or every
// bin when showEmpty is true) to the given stream.
void printHistogram(std::ostream& os, const Histogram& hist, int barWidth = 50,
                    bool showEmpty = false);

}  // namespace ihe
