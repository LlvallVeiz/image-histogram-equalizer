// image.cpp -- Netpbm (PGM/PBM/PPM) reader and PGM writer.
// See image.h for the supported formats. All values are rescaled to
// the internal 0..255 range; colour input is reduced to luma with the
// ITU-R BT.601 weights (0.299 R + 0.587 G + 0.114 B).

#include "image.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ihe {
namespace {

// Read the next whitespace-separated token, skipping whitespace and
// '#' comments (Netpbm allows a comment anywhere whitespace may appear).
// Returns false at end of stream.
bool readToken(std::istream& in, std::string& token) {
    char c;
    while (in.get(c)) {
        if (c == '#') {
            std::string ignored;
            std::getline(in, ignored);
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        token.clear();
        token.push_back(c);
        while (in.get(c) && !std::isspace(static_cast<unsigned char>(c))) {
            if (c == '#') {
                in.unget();
                break;
            }
            token.push_back(c);
        }
        return true;
    }
    return false;
}

int readHeaderInt(std::istream& in, const std::string& what, const std::string& path) {
    std::string token;
    if (!readToken(in, token)) {
        throw std::runtime_error("truncated header in " + path + " (missing " + what + ")");
    }
    try {
        std::size_t used = 0;
        int value = std::stoi(token, &used);
        if (used != token.size()) throw std::invalid_argument("trailing characters");
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid " + what + " '" + token + "' in " + path);
    }
}

// Rescale a sample in [0, maxval] to [0, 255] with rounding.
std::uint8_t rescale(int value, int maxval) {
    if (maxval == 255) return static_cast<std::uint8_t>(value);
    long scaled = (static_cast<long>(value) * 255 + maxval / 2) / maxval;
    if (scaled < 0) scaled = 0;
    if (scaled > 255) scaled = 255;
    return static_cast<std::uint8_t>(scaled);
}

// ITU-R BT.601 luma, integer arithmetic with rounding:
//   Y = (299 R + 587 G + 114 B + 500) / 1000
std::uint8_t rgbToGray(int r, int g, int b) {
    int y = (299 * r + 587 * g + 114 * b + 500) / 1000;
    if (y < 0) y = 0;
    if (y > 255) y = 255;
    return static_cast<std::uint8_t>(y);
}

}  // namespace

GrayImage loadImage(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open file: " + path);

    std::string magic;
    if (!readToken(in, magic)) throw std::runtime_error("empty file: " + path);
    if (magic.size() != 2 || magic[0] != 'P' || magic[1] < '1' || magic[1] > '6') {
        throw std::runtime_error("unsupported magic number '" + magic + "' in " + path +
                                 " (expected P1..P6)");
    }
    const char kind = magic[1];
    const bool ascii = (kind == '1' || kind == '2' || kind == '3');

    const int width = readHeaderInt(in, "width", path);
    const int height = readHeaderInt(in, "height", path);
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("invalid dimensions in " + path);
    }

    const bool bitmap = (kind == '1' || kind == '4');
    int maxval = 1;
    if (!bitmap) {
        maxval = readHeaderInt(in, "maxval", path);
        if (maxval < 1 || maxval > 65535) {
            throw std::runtime_error("maxval out of range (1..65535) in " + path);
        }
    }

    GrayImage img;
    img.width = width;
    img.height = height;
    const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    img.pixels.reserve(count);

    if (ascii) {
        // ASCII raster: whitespace-separated decimal samples.
        for (std::size_t i = 0; i < count; ++i) {
            if (bitmap) {
                int bit = readHeaderInt(in, "bitmap sample", path);
                if (bit != 0 && bit != 1) {
                    throw std::runtime_error("PBM sample must be 0 or 1 in " + path);
                }
                img.pixels.push_back(bit == 1 ? 0 : 255);  // PBM: 1 = black
            } else if (kind == '3') {
                int r = readHeaderInt(in, "red sample", path);
                int g = readHeaderInt(in, "green sample", path);
                int b = readHeaderInt(in, "blue sample", path);
                if (r < 0 || r > maxval || g < 0 || g > maxval || b < 0 || b > maxval) {
                    throw std::runtime_error("PPM sample out of range in " + path);
                }
                img.pixels.push_back(rgbToGray(rescale(r, maxval), rescale(g, maxval),
                                               rescale(b, maxval)));
            } else {
                int v = readHeaderInt(in, "gray sample", path);
                if (v < 0 || v > maxval) {
                    throw std::runtime_error("PGM sample out of range in " + path);
                }
                img.pixels.push_back(rescale(v, maxval));
            }
        }
        return img;
    }

    // Binary raster. The single whitespace byte after maxval was already
    // consumed by readToken, so the stream is positioned at the raster.
    if (bitmap) {
        const std::size_t rowBytes = (static_cast<std::size_t>(width) + 7) / 8;
        std::vector<char> row(rowBytes);
        for (int y = 0; y < height; ++y) {
            if (!in.read(row.data(), static_cast<std::streamsize>(rowBytes))) {
                throw std::runtime_error("truncated PBM raster in " + path);
            }
            for (int x = 0; x < width; ++x) {
                int byte = static_cast<unsigned char>(row[static_cast<std::size_t>(x) / 8]);
                int bit = (byte >> (7 - (x % 8))) & 1;
                img.pixels.push_back(bit == 1 ? 0 : 255);
            }
        }
        return img;
    }

    const bool twoBytes = maxval >= 256;
    const std::size_t channels = (kind == '6') ? 3 : 1;
    const std::size_t total = count * channels;
    std::vector<unsigned char> raw(total * (twoBytes ? 2 : 1));
    if (!in.read(reinterpret_cast<char*>(raw.data()),
                 static_cast<std::streamsize>(raw.size()))) {
        throw std::runtime_error("truncated raster in " + path);
    }
    auto sampleAt = [&](std::size_t i) {
        if (twoBytes) {
            return static_cast<int>(raw[2 * i]) * 256 + static_cast<int>(raw[2 * i + 1]);
        }
        return static_cast<int>(raw[i]);
    };
    for (std::size_t i = 0; i < count; ++i) {
        if (kind == '6') {
            int r = sampleAt(3 * i);
            int g = sampleAt(3 * i + 1);
            int b = sampleAt(3 * i + 2);
            img.pixels.push_back(rgbToGray(rescale(r, maxval), rescale(g, maxval),
                                           rescale(b, maxval)));
        } else {
            img.pixels.push_back(rescale(sampleAt(i), maxval));
        }
    }
    return img;
}

void savePgm(const std::string& path, const GrayImage& img, bool ascii) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write file: " + path);
    if (ascii) {
        out << "P2\n" << img.width << ' ' << img.height << "\n255\n";
        for (int y = 0; y < img.height; ++y) {
            for (int x = 0; x < img.width; ++x) {
                if (x) out << ' ';
                out << static_cast<int>(img.at(x, y));
            }
            out << '\n';
        }
    } else {
        out << "P5\n" << img.width << ' ' << img.height << "\n255\n";
        out.write(reinterpret_cast<const char*>(img.pixels.data()),
                  static_cast<std::streamsize>(img.pixels.size()));
    }
    if (!out) throw std::runtime_error("failed while writing: " + path);
}

}  // namespace ihe
