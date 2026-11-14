// image.h -- Grayscale image type and Netpbm (PGM/PBM/PPM) I/O.
//
// This project works internally on 8-bit grayscale images (0 = black,
// 255 = white). Files are read through the Netpbm family so that no
// external image library is needed:
//   PGM  P5 (binary) / P2 (ASCII)  -- native grayscale, full support
//   PBM  P4 (binary) / P1 (ASCII)  -- 1-bit bitmap, read as 0 / 255
//   PPM  P6 (binary) / P3 (ASCII)  -- colour, converted to gray with the
//                                      ITU-R BT.601 luma weights
// Any maxval in 1..65535 is accepted and linearly rescaled to 0..255.
// Output is always written as PGM (P5 binary by default, P2 with --ascii).
//
// Format reference: Netpbm PGM specification,
// http://netpbm.sourceforge.net/doc/pgm.html

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ihe {

struct GrayImage {
    int width = 0;
    int height = 0;
    // Row-major pixels, 0 = black, 255 = white.
    std::vector<std::uint8_t> pixels;

    std::uint8_t at(int x, int y) const {
        return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                      static_cast<std::size_t>(x)];
    }
    std::uint8_t& at(int x, int y) {
        return pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                      static_cast<std::size_t>(x)];
    }
    std::size_t size() const { return pixels.size(); }
    bool empty() const { return pixels.empty(); }
};

// Load a PGM (P5/P2), PBM (P4/P1) or PPM (P6/P3) file.
// Throws std::runtime_error on missing files, bad magic numbers,
// truncated data or out-of-range header values.
GrayImage loadImage(const std::string& path);

// Save as PGM, binary (P5) by default or ASCII (P2) when ascii == true.
// Throws std::runtime_error if the file cannot be written.
void savePgm(const std::string& path, const GrayImage& img, bool ascii = false);

}  // namespace ihe
