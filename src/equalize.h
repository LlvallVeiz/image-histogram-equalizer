// equalize.h -- Global histogram equalization (HE).
//
// The classic point operation from Gonzalez & Woods (Digital Image
// Processing, ch. 3): the image's own cumulative distribution function
// becomes the tone-mapping curve, spreading the occupied gray levels
// as uniformly as possible over 0..255:
//
//   s_k = round( (cdf(k) - cdf_min) / (N - cdf_min) * 255 )
//
// where N is the pixel count and cdf_min the first non-zero CDF value.
// Subtracting cdf_min guarantees the darkest occupied level maps to 0
// and the brightest to 255. A constant image (N == cdf_min) maps to
// itself; the LUT is non-decreasing, so HE never inverts brightness
// order.

#pragma once

#include <array>
#include <cstdint>

#include "histogram.h"
#include "image.h"

namespace ihe {

// Build the 256-entry lookup table for one histogram.
std::array<std::uint8_t, 256> buildHeLut(const Histogram& hist, int totalPixels);

// Apply a lookup table to every pixel.
GrayImage applyLut(const GrayImage& img, const std::array<std::uint8_t, 256>& lut);

// Global histogram equalization of a whole image.
GrayImage equalizeHistogram(const GrayImage& img);

}  // namespace ihe
