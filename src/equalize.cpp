// equalize.cpp -- global histogram equalization.

#include "equalize.h"

#include <algorithm>
#include <cmath>

namespace ihe {

std::array<std::uint8_t, 256> buildHeLut(const Histogram& hist, int totalPixels) {
    std::array<std::uint8_t, 256> lut{};
    const auto cdf = cumulativeHistogram(hist);

    int cdfMin = 0;
    for (int k = 0; k < 256; ++k) {
        if (cdf[k] > 0) {
            cdfMin = cdf[k];
            break;
        }
    }
    // Constant (or empty) image: the CDF is a single step, so the only
    // sensible mapping is the identity.
    if (totalPixels <= 0 || totalPixels == cdfMin) {
        for (int k = 0; k < 256; ++k) lut[k] = static_cast<std::uint8_t>(k);
        return lut;
    }
    const double denom = static_cast<double>(totalPixels - cdfMin);
    for (int k = 0; k < 256; ++k) {
        double mapped = (static_cast<double>(cdf[k]) - cdfMin) / denom * 255.0;
        if (mapped < 0.0) mapped = 0.0;
        if (mapped > 255.0) mapped = 255.0;
        lut[k] = static_cast<std::uint8_t>(std::llround(mapped));
    }
    return lut;
}

GrayImage applyLut(const GrayImage& img, const std::array<std::uint8_t, 256>& lut) {
    GrayImage out = img;
    for (std::size_t i = 0; i < out.pixels.size(); ++i) out.pixels[i] = lut[out.pixels[i]];
    return out;
}

GrayImage equalizeHistogram(const GrayImage& img) {
    const Histogram hist = computeHistogram(img);
    const auto lut = buildHeLut(hist, static_cast<int>(img.size()));
    return applyLut(img, lut);
}

}  // namespace ihe
