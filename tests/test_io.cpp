// test_io.cpp -- Netpbm reader/writer tests.
//
// Round-trips: P5 and P2 write -> read gives identical pixels.
// PBM: P1 ASCII "0 1 1 0 / 1 0 0 1" reads as white/black/white/black...
//      (PBM 1 = black -> 0, 0 = white -> 255).
// PPM: P3 "255 0 0 / 0 255 0" converts with BT.601 luma:
//      red   -> (299*255+500)/1000 = 76, green -> (587*255+500)/1000 = 150.
// Maxval scaling: P2 with maxval 100 maps 50 -> (50*255+50)/100 = 128.
// Errors: bad magic number and truncated raster throw.

#include <cstdio>
#include <fstream>

#include "image.h"
#include "test_framework.h"

using namespace ihe;

namespace {

const char* kTmpPgm = "/tmp/ihe_test_roundtrip.pgm";
const char* kTmpP2 = "/tmp/ihe_test_ascii.pgm";
const char* kTmpPbm = "/tmp/ihe_test.pbm";
const char* kTmpPpm = "/tmp/ihe_test.ppm";
const char* kTmpScaled = "/tmp/ihe_test_scaled.pgm";
const char* kTmpBad = "/tmp/ihe_test_bad.pgm";

void writeText(const char* path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
}

}  // namespace

void test_io() {
    GrayImage img;
    img.width = 4;
    img.height = 4;
    img.pixels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 100, 100};

    // P5 binary round-trip.
    savePgm(kTmpPgm, img);
    const GrayImage back = loadImage(kTmpPgm);
    CHECK(back.width == 4 && back.height == 4);
    CHECK(back.pixels == img.pixels);

    // P2 ASCII round-trip (also exercises comment skipping on read).
    savePgm(kTmpP2, img, true);
    const GrayImage backAscii = loadImage(kTmpP2);
    CHECK(backAscii.pixels == img.pixels);

    // PBM P1: 1 = black (0), 0 = white (255).
    writeText(kTmpPbm, "P1\n# a comment\n4 2\n0 1 1 0\n1 0 0 1\n");
    const GrayImage pbm = loadImage(kTmpPbm);
    CHECK(pbm.width == 4 && pbm.height == 2);
    CHECK(pbm.pixels ==
          (std::vector<std::uint8_t>{255, 0, 0, 255, 0, 255, 255, 0}));

    // PPM P3 with a comment in the header; BT.601 luma.
    writeText(kTmpPpm, "P3\n2 1 # comment\n255\n255 0 0 0 255 0\n");
    const GrayImage ppm = loadImage(kTmpPpm);
    CHECK(ppm.width == 2 && ppm.height == 1);
    CHECK(ppm.pixels[0] == 76);   // pure red
    CHECK(ppm.pixels[1] == 150);  // pure green

    // P5 binary PPM (P6) round-trip through the same luma path.
    {
        std::ofstream out(kTmpPpm, std::ios::binary);
        out << "P6\n1 1\n255\n";
        const unsigned char px[3] = {0, 0, 255};  // pure blue -> 29
        out.write(reinterpret_cast<const char*>(px), 3);
    }
    const GrayImage ppm6 = loadImage(kTmpPpm);
    CHECK(ppm6.pixels.size() == 1 && ppm6.pixels[0] == 29);

    // Maxval rescaling: maxval 100, sample 50 -> 128.
    writeText(kTmpScaled, "P2\n2 2\n100\n0 50 100 25\n");
    const GrayImage scaled = loadImage(kTmpScaled);
    CHECK(scaled.pixels ==
          (std::vector<std::uint8_t>{0, 128, 255, 64}));  // 25 -> 64

    // 16-bit PGM (maxval 65535): two bytes per sample, big-endian.
    {
        std::ofstream out(kTmpScaled, std::ios::binary);
        out << "P5\n2 1\n65535\n";
        const unsigned char px[4] = {0x00, 0x00, 0xFF, 0xFF};
        out.write(reinterpret_cast<const char*>(px), 4);
    }
    const GrayImage wide = loadImage(kTmpScaled);
    CHECK(wide.pixels == (std::vector<std::uint8_t>{0, 255}));

    // Bad magic number throws.
    writeText(kTmpBad, "P9\n2 2\n255\n0 0 0 0\n");
    bool threw = false;
    try {
        loadImage(kTmpBad);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);

    // Truncated raster throws.
    writeText(kTmpBad, "P5\n4 4\n255\n\x00\x01");
    threw = false;
    try {
        loadImage(kTmpBad);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);

    // Missing file throws.
    threw = false;
    try {
        loadImage("/tmp/ihe_test_does_not_exist_12345.pgm");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);

    std::remove(kTmpPgm);
    std::remove(kTmpP2);
    std::remove(kTmpPbm);
    std::remove(kTmpPpm);
    std::remove(kTmpScaled);
    std::remove(kTmpBad);
}
