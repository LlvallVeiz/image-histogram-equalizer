// test_equalize.cpp -- Global histogram equalization tests.
//
// Hand-computed LUT for the 4x4 fixture (h(0)=10, h(50)=4, h(100)=2,
// N=16, cdf_min=10), using s_k = round((cdf(k)-10)/(16-10) * 255):
//   LUT(0)   = round( 0/6 * 255) = 0
//   LUT(50)  = round( 4/6 * 255) = round(170) = 170
//   LUT(100) = round( 6/6 * 255) = 255
//   LUT(1)   = 0 (cdf(1) = 10, same as cdf(0))
//   LUT(255) = 255

#include "equalize.h"
#include "test_framework.h"

using namespace ihe;

namespace {

GrayImage fixture4x4() {
    GrayImage img;
    img.width = 4;
    img.height = 4;
    img.pixels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 100, 100};
    return img;
}

}  // namespace

void test_equalize() {
    const GrayImage img = fixture4x4();
    const Histogram h = computeHistogram(img);
    const auto lut = buildHeLut(h, 16);

    CHECK(lut[0] == 0);
    CHECK(lut[1] == 0);
    CHECK(lut[49] == 0);
    CHECK(lut[50] == 170);
    CHECK(lut[99] == 170);
    CHECK(lut[100] == 255);
    CHECK(lut[255] == 255);
    // The LUT never inverts brightness order.
    for (int k = 0; k < 255; ++k) CHECK(lut[k] <= lut[k + 1]);

    const GrayImage eq = equalizeHistogram(img);
    CHECK(eq.width == 4 && eq.height == 4);
    CHECK(eq.at(0, 0) == 0);
    CHECK(eq.at(2, 2) == 170);
    CHECK(eq.at(3, 3) == 255);
    // Only the three mapped levels occur in the output.
    const Histogram hout = computeHistogram(eq);
    CHECK(hout[0] == 10);
    CHECK(hout[170] == 4);
    CHECK(hout[255] == 2);

    // Constant image: HE is the identity (no division by zero, no change).
    GrayImage constant;
    constant.width = 7;
    constant.height = 5;
    constant.pixels.assign(35, 200);
    const GrayImage eqConst = equalizeHistogram(constant);
    for (std::uint8_t v : eqConst.pixels) CHECK(v == 200);

    // A full 0..255 gradient used once each is already uniform:
    // HE is the identity (cdf(k) = k+1, cdf_min = 1, LUT(k) = k).
    GrayImage gradient;
    gradient.width = 256;
    gradient.height = 1;
    gradient.pixels.resize(256);
    for (int k = 0; k < 256; ++k) gradient.pixels[static_cast<std::size_t>(k)] = static_cast<std::uint8_t>(k);
    const GrayImage eqGrad = equalizeHistogram(gradient);
    for (int k = 0; k < 256; ++k) {
        CHECK(eqGrad.pixels[static_cast<std::size_t>(k)] == static_cast<std::uint8_t>(k));
    }

    // Two-by-two {0, 85, 170, 255}: cdf_min = 1, N = 4, so
    // LUT(85) = round(1/3*255) = 85, LUT(170) = round(2/3*255) = 170:
    // identity again.
    GrayImage tiny;
    tiny.width = 2;
    tiny.height = 2;
    tiny.pixels = {0, 85, 170, 255};
    const GrayImage eqTiny = equalizeHistogram(tiny);
    CHECK(eqTiny.pixels == tiny.pixels);

    // applyLut with the identity LUT leaves an image unchanged.
    std::array<std::uint8_t, 256> identity{};
    for (int k = 0; k < 256; ++k) identity[k] = static_cast<std::uint8_t>(k);
    CHECK(applyLut(img, identity).pixels == img.pixels);
}
