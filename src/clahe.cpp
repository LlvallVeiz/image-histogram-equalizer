// clahe.cpp -- contrast-limited adaptive histogram equalization.

#include "clahe.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "equalize.h"

namespace ihe {

Histogram clipHistogram(const Histogram& hist, int tilePixels, double clipLimit) {
    if (tilePixels <= 0) throw std::invalid_argument("clipHistogram: tilePixels must be > 0");
    if (!(clipLimit > 0.0)) throw std::invalid_argument("clipHistogram: clipLimit must be > 0");

    const double average = static_cast<double>(tilePixels) / 256.0;
    // At least 1, so tiny tiles (average < 0.5) still keep one count per bin.
    const int clip = std::max(1, static_cast<int>(std::llround(clipLimit * average)));

    Histogram clipped = hist;
    int excess = 0;
    for (int k = 0; k < 256; ++k) {
        if (clipped[k] > clip) {
            excess += clipped[k] - clip;
            clipped[k] = clip;
        }
    }
    // Redistribute the excess uniformly: the same amount to every bin,
    // plus one extra to the first (excess % 256) bins, in order.
    // This keeps the redistribution deterministic.
    const int perBin = excess / 256;
    const int remainder = excess % 256;
    for (int k = 0; k < 256; ++k) {
        clipped[k] += perBin;
        if (k < remainder) clipped[k] += 1;
    }
    return clipped;
}

GrayImage applyClahe(const GrayImage& img, const ClaheParams& params) {
    if (img.empty()) throw std::invalid_argument("applyClahe: empty image");
    if (params.tilesX < 1 || params.tilesX > 64 || params.tilesY < 1 || params.tilesY > 64) {
        throw std::invalid_argument("applyClahe: tiles must be in 1..64");
    }
    if (!(params.clipLimit > 0.0)) {
        throw std::invalid_argument("applyClahe: clipLimit must be > 0");
    }

    const int W = img.width;
    const int H = img.height;
    const int tileW = (W + params.tilesX - 1) / params.tilesX;
    const int tileH = (H + params.tilesY - 1) / params.tilesY;

    // One equalization LUT per tile, from that tile's clipped histogram.
    using Lut = std::array<std::uint8_t, 256>;
    std::vector<std::vector<Lut>> luts(static_cast<std::size_t>(params.tilesY),
                                       std::vector<Lut>(static_cast<std::size_t>(params.tilesX)));
    for (int ty = 0; ty < params.tilesY; ++ty) {
        const int y0 = ty * tileH;
        const int y1 = std::min(y0 + tileH, H);
        for (int tx = 0; tx < params.tilesX; ++tx) {
            const int x0 = tx * tileW;
            const int x1 = std::min(x0 + tileW, W);
            Histogram hist{};
            hist.fill(0);
            for (int y = y0; y < y1; ++y)
                for (int x = x0; x < x1; ++x) hist[img.at(x, y)]++;
            const int tilePixels = (x1 - x0) * (y1 - y0);
            luts[ty][tx] = buildHeLut(clipHistogram(hist, tilePixels, params.clipLimit),
                                      tilePixels);
        }
    }

    // Map each pixel through the four surrounding tile-centre LUTs and
    // blend bilinearly. Tile centres sit at (tx + 0.5) * tileW in pixel
    // coordinates; outside the outer centres the nearest tile is used.
    GrayImage out = img;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const double txf = (x + 0.5) / tileW - 0.5;
            const double tyf = (y + 0.5) / tileH - 0.5;
            int tx0, ty0;
            double fx, fy;
            if (params.tilesX == 1) {
                tx0 = 0;
                fx = 0.0;
            } else {
                tx0 = static_cast<int>(std::floor(txf));
                fx = txf - tx0;
                if (tx0 < 0) {
                    tx0 = 0;
                    fx = 0.0;
                } else if (tx0 > params.tilesX - 2) {
                    tx0 = params.tilesX - 2;
                    fx = 1.0;
                }
            }
            if (params.tilesY == 1) {
                ty0 = 0;
                fy = 0.0;
            } else {
                ty0 = static_cast<int>(std::floor(tyf));
                fy = tyf - ty0;
                if (ty0 < 0) {
                    ty0 = 0;
                    fy = 0.0;
                } else if (ty0 > params.tilesY - 2) {
                    ty0 = params.tilesY - 2;
                    fy = 1.0;
                }
            }
            const int tx1 = std::min(tx0 + 1, params.tilesX - 1);
            const int ty1 = std::min(ty0 + 1, params.tilesY - 1);
            const std::uint8_t v = img.at(x, y);
            const double top = (1.0 - fx) * luts[ty0][tx0][v] + fx * luts[ty0][tx1][v];
            const double bottom = (1.0 - fx) * luts[ty1][tx0][v] + fx * luts[ty1][tx1][v];
            const double blended = (1.0 - fy) * top + fy * bottom;
            out.at(x, y) = static_cast<std::uint8_t>(std::llround(blended));
        }
    }
    return out;
}

}  // namespace ihe
