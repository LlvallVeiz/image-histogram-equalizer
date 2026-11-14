// clahe.h -- Contrast-Limited Adaptive Histogram Equalization (CLAHE).
//
// After Pizer et al. (1987) and Zuiderveld (1994, Graphics Gems IV):
// the image is split into a grid of tiles, each tile gets its own
// equalization curve from its own histogram, and the curves are blended
// with bilinear interpolation between neighbouring tile centres so no
// seams appear. Before the CDF of a tile is computed, its histogram is
// clipped at the clip limit and the clipped-off counts are redistributed
// uniformly, which caps the CDF slope and therefore the contrast gain --
// the mechanism that stops CLAHE from amplifying noise in flat regions
// the way plain adaptive HE does.
//
// The clip limit is expressed the OpenCV way: a multiple of the average
// bin count of a tile histogram (clipLimit = 2.0 is the usual default).
// A tile grid of 1x1 with a very large clip limit reduces exactly to
// global histogram equalization (verified in tests/test_clahe.cpp).

#pragma once

#include "histogram.h"
#include "image.h"

namespace ihe {

struct ClaheParams {
    int tilesX = 8;       // tile columns (1..64)
    int tilesY = 8;       // tile rows (1..64)
    double clipLimit = 2.0;  // multiple of the tile histogram's average bin count
};

// Clip one tile histogram at clipLimit * (tilePixels / 256) and
// redistribute the excess uniformly. The total count is preserved.
// Exposed for unit testing.
Histogram clipHistogram(const Histogram& hist, int tilePixels, double clipLimit);

// Apply CLAHE to a whole image.
GrayImage applyClahe(const GrayImage& img, const ClaheParams& params = {});

}  // namespace ihe
