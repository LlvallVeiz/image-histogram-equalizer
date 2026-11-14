// stats.cpp -- image statistics and comparison metrics.

#include "stats.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "histogram.h"

namespace ihe {

ImageStats computeStats(const GrayImage& img) {
    ImageStats s;
    s.pixelCount = img.size();
    if (img.empty()) return s;

    double sum = 0.0;
    double sumSq = 0.0;
    s.minValue = 255;
    s.maxValue = 0;
    for (std::uint8_t v : img.pixels) {
        sum += v;
        sumSq += static_cast<double>(v) * v;
        if (v < s.minValue) s.minValue = v;
        if (v > s.maxValue) s.maxValue = v;
    }
    const double n = static_cast<double>(s.pixelCount);
    s.mean = sum / n;
    double variance = sumSq / n - s.mean * s.mean;
    if (variance < 0.0) variance = 0.0;  // guard against round-off
    s.stddev = std::sqrt(variance);

    // Shannon entropy H = -sum p(k) log2 p(k), from the histogram.
    const auto prob = normalizedHistogram(computeHistogram(img));
    double entropy = 0.0;
    for (double p : prob) {
        if (p > 0.0) entropy -= p * std::log2(p);
    }
    s.entropy = entropy;
    return s;
}

double meanSquaredError(const GrayImage& a, const GrayImage& b) {
    if (a.width != b.width || a.height != b.height) {
        throw std::invalid_argument("MSE: images must have the same dimensions");
    }
    if (a.empty()) throw std::invalid_argument("MSE: empty image");
    double sumSq = 0.0;
    for (std::size_t i = 0; i < a.pixels.size(); ++i) {
        double d = static_cast<double>(a.pixels[i]) - static_cast<double>(b.pixels[i]);
        sumSq += d * d;
    }
    return sumSq / static_cast<double>(a.pixels.size());
}

double psnr(const GrayImage& a, const GrayImage& b) {
    const double mse = meanSquaredError(a, b);
    if (mse == 0.0) return std::numeric_limits<double>::infinity();
    return 10.0 * std::log10(255.0 * 255.0 / mse);
}

std::string statsReport(const GrayImage& img, const std::string& label) {
    const ImageStats s = computeStats(img);
    std::ostringstream os;
    os << std::fixed << std::setprecision(2);
    os << label << " (" << img.width << "x" << img.height << ", " << s.pixelCount << " px)\n"
       << "  mean    : " << s.mean << "\n"
       << "  stddev  : " << s.stddev << "\n"
       << "  min/max : " << s.minValue << " / " << s.maxValue << "\n"
       << "  entropy : " << s.entropy << " bits\n";
    return os.str();
}

}  // namespace ihe
