// matching.cpp -- histogram matching (specification).

#include "matching.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace ihe {

std::array<std::uint8_t, 256> buildMatchingLut(const Histogram& sourceHist,
                                               const Histogram& targetHist) {
    const auto cdfS = cumulativeHistogram(sourceHist);
    const auto cdfT = cumulativeHistogram(targetHist);
    std::array<std::uint8_t, 256> lut{};
    int t = 0;
    for (int s = 0; s < 256; ++s) {
        // Smallest target level whose CDF reaches the source CDF at s.
        // t only moves forward because cdfS is non-decreasing.
        while (t < 255 && cdfT[t] < cdfS[s]) ++t;
        lut[s] = static_cast<std::uint8_t>(t);
    }
    return lut;
}

namespace {

// Apportion `total` counts over 256 bins in proportion to `weights`,
// using largest-remainder rounding so the counts sum to exactly total.
Histogram apportion(const std::array<double, 256>& weights, int total) {
    double weightSum = 0.0;
    for (double w : weights) weightSum += w;
    Histogram counts{};
    counts.fill(0);
    if (total <= 0 || weightSum <= 0.0) return counts;

    std::vector<double> frac(256, 0.0);
    int assigned = 0;
    for (int k = 0; k < 256; ++k) {
        double exact = weights[k] / weightSum * total;
        int base = static_cast<int>(std::floor(exact));
        counts[k] = base;
        frac[k] = exact - base;
        assigned += base;
    }
    int remainder = total - assigned;
    std::vector<int> order(256);
    std::iota(order.begin(), order.end(), 0);
    // Ties break towards the lower bin so the result is deterministic
    // regardless of the sort implementation.
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (frac[a] != frac[b]) return frac[a] > frac[b];
        return a < b;
    });
    for (int i = 0; i < remainder; ++i) counts[order[static_cast<std::size_t>(i)]]++;
    return counts;
}

}  // namespace

Histogram uniformTargetHistogram(int totalPixels) {
    std::array<double, 256> weights{};
    weights.fill(1.0);
    return apportion(weights, totalPixels);
}

Histogram gaussianTargetHistogram(int totalPixels, double mean, double stddev) {
    if (!(stddev > 0.0)) throw std::invalid_argument("gaussian target: stddev must be > 0");
    std::array<double, 256> weights{};
    for (int k = 0; k < 256; ++k) {
        double z = (k - mean) / stddev;
        weights[k] = std::exp(-0.5 * z * z);
    }
    return apportion(weights, totalPixels);
}

GrayImage matchHistogram(const GrayImage& img, const Histogram& targetHist) {
    const Histogram sourceHist = computeHistogram(img);
    const auto lut = buildMatchingLut(sourceHist, targetHist);
    GrayImage out = img;
    for (std::size_t i = 0; i < out.pixels.size(); ++i) out.pixels[i] = lut[out.pixels[i]];
    return out;
}

GrayImage matchToReference(const GrayImage& img, const GrayImage& reference) {
    return matchHistogram(img, computeHistogram(reference));
}

}  // namespace ihe
