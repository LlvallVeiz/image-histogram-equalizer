// test_clahe.cpp -- CLAHE tests.
//
// 1. clipHistogram: hand-computed redistribution.
//    hist[100] = 1000, everything else 0, tilePixels = 1000, clipLimit = 2.
//    average bin count = 1000/256 = 3.90625, clip = round(2 * 3.90625) = 8.
//    Excess = 1000 - 8 = 992 = 3 * 256 + 224, so every bin gains 3 and
//    bins 0..223 gain one more: hist[100] = 8+3+1 = 12, hist[0] = 4,
//    hist[224] = 3, hist[255] = 3, total still 1000.
//
// 2. Bilinear interpolation: a 64x32 image, left half constant 50 and
//    right half constant 200, CLAHE with tiles 2x1, clip 2.0.
//    Each tile has 1024 pixels; average = 4, clip = 8, excess = 1016
//    = 3*256 + 248, so bins 0..247 gain 4, bins 248..255 gain 3, and the
//    occupied bin gains its clipped 8 on top:
//      left tile:  hist[k] = 4 (k < 50), hist[50] = 12, hist[k] = 4 (51..247)
//      CDF_left(50) = 50*4 + 12 = 212, cdf_min = 4
//      LUT_left(50)  = round((212-4)/(1024-4)*255) = round(52.0) = 52
//      CDF_left(200) = 199*4 + 12 = 808 (bin 50 holds 12, the rest 4)
//      LUT_left(200) = round((808-4)/1020*255) = round(201.0) = 201
//      right tile: same shape centred on 200:
//      LUT_right(200) = 202, LUT_right(50) = round((204-4)/1020*255) = 50
//      (CDF_right(200) = 200*4 + 12 = 812; CDF_right(50) = 51*4 = 204.)
//    Pixel (15,16) sits on the left tile centre -> 52.
//    Pixel (31,16) is between the centres (fx = 0.484375, value 50):
//      (1-fx)*52 + fx*50 = 51.03 -> 51.
//    Pixel (32,16) is just past the midpoint (value 200):
//      (1-fx)*201 + fx*202 = 201.52 -> 202.
//
// 3. Degenerate case: 1x1 tiles with a huge clip limit is exactly
//    global HE (single tile, no clipping, no interpolation).

#include <cmath>

#include "clahe.h"
#include "equalize.h"
#include "stats.h"
#include "test_framework.h"

using namespace ihe;

void test_clahe() {
    // --- 1. clipHistogram, hand-computed ---
    Histogram spike{};
    spike.fill(0);
    spike[100] = 1000;
    const Histogram clipped = clipHistogram(spike, 1000, 2.0);
    CHECK(clipped[100] == 12);
    CHECK(clipped[0] == 4);
    CHECK(clipped[223] == 4);
    CHECK(clipped[224] == 3);
    CHECK(clipped[255] == 3);
    CHECK(histogramTotal(clipped) == 1000);
    // No bin keeps more than the clip plus the redistributed share.
    for (int k = 0; k < 256; ++k) CHECK(clipped[k] <= 12);

    // Clipping a flat histogram changes nothing (nothing above the clip).
    Histogram flat{};
    flat.fill(4);
    CHECK(clipHistogram(flat, 1024, 2.0) == flat);

    // --- 2. bilinear interpolation, hand-computed ---
    GrayImage split;
    split.width = 64;
    split.height = 32;
    split.pixels.resize(64u * 32u);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 64; ++x)
            split.at(x, y) = (x < 32) ? 50 : 200;
    const GrayImage out = applyClahe(split, ClaheParams{2, 1, 2.0});
    CHECK(out.width == 64 && out.height == 32);
    CHECK(out.at(15, 16) == 52);
    CHECK(out.at(31, 16) == 51);
    CHECK(out.at(32, 16) == 202);
    CHECK(out.at(47, 16) == 202);
    CHECK(out.at(0, 0) == 52);
    CHECK(out.at(63, 31) == 202);

    // --- 3. 1x1 tiles + huge clip == global HE ---
    GrayImage fixture;
    fixture.width = 4;
    fixture.height = 4;
    fixture.pixels = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 100, 100};
    const GrayImage claheOne = applyClahe(fixture, ClaheParams{1, 1, 1000.0});
    CHECK(claheOne.pixels == equalizeHistogram(fixture).pixels);

    // --- 4. CLAHE raises the contrast of a low-contrast image ---
    GrayImage low;
    low.width = 64;
    low.height = 64;
    low.pixels.resize(64u * 64u);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            low.at(x, y) = static_cast<std::uint8_t>(100 + (x + y) % 41);  // 100..140
    const ImageStats before = computeStats(low);
    const ImageStats after = computeStats(applyClahe(low, ClaheParams{8, 8, 2.0}));
    CHECK(after.stddev > before.stddev);
    CHECK(after.maxValue - after.minValue > before.maxValue - before.minValue);
    // No entropy assertion here: the source already uses its 41 levels
    // almost uniformly (entropy 5.36 of a log2(41) = 5.36 maximum), so
    // spreading it over more levels can slightly *lower* the entropy
    // while still raising the contrast. Entropy is a diagnostic, not
    // a score to maximise.
    CHECK(after.entropy > 4.0);  // still an informative distribution

    // A higher clip limit gives at least as much spread as a lower one
    // on the same low-contrast image (clip 1.0 nearly flattens the gain).
    const ImageStats mild = computeStats(applyClahe(low, ClaheParams{8, 8, 1.0}));
    CHECK(after.stddev >= mild.stddev);

    // Invalid parameters are rejected instead of silently misbehaving.
    bool threw = false;
    try {
        applyClahe(low, ClaheParams{0, 8, 2.0});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
    threw = false;
    try {
        applyClahe(low, ClaheParams{8, 8, 0.0});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}
