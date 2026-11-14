// histogram.cpp -- histogram computation and text rendering.

#include "histogram.h"

#include <algorithm>
#include <cmath>
#include <iomanip>

namespace ihe {

Histogram computeHistogram(const GrayImage& img) {
    Histogram hist{};
    hist.fill(0);
    for (std::uint8_t v : img.pixels) hist[v]++;
    return hist;
}

std::array<int, 256> cumulativeHistogram(const Histogram& hist) {
    std::array<int, 256> cdf{};
    int running = 0;
    for (int k = 0; k < 256; ++k) {
        running += hist[k];
        cdf[k] = running;
    }
    return cdf;
}

std::array<double, 256> normalizedHistogram(const Histogram& hist) {
    std::array<double, 256> prob{};
    const int total = histogramTotal(hist);
    if (total == 0) {
        prob.fill(0.0);
        return prob;
    }
    for (int k = 0; k < 256; ++k) prob[k] = static_cast<double>(hist[k]) / total;
    return prob;
}

int histogramTotal(const Histogram& hist) {
    int total = 0;
    for (int c : hist) total += c;
    return total;
}

void printHistogram(std::ostream& os, const Histogram& hist, int barWidth, bool showEmpty) {
    int peak = 0;
    for (int c : hist) peak = std::max(peak, c);
    if (peak == 0) {
        os << "(empty histogram)\n";
        return;
    }
    for (int k = 0; k < 256; ++k) {
        if (hist[k] == 0 && !showEmpty) continue;
        int bars = static_cast<int>(std::llround(static_cast<double>(hist[k]) / peak * barWidth));
        os << std::setw(3) << k << " | " << std::string(static_cast<std::size_t>(bars), '#')
           << " (" << hist[k] << ")\n";
    }
}

}  // namespace ihe
