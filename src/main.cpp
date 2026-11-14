// main.cpp -- Command-line interface for image-histogram-equalizer.
//
//   ihe he      <in> <out>            global histogram equalization
//   ihe clahe   <in> <out>            CLAHE (tile-based, clip limit)
//   ihe match   <in> <ref> <out>      histogram matching to a reference image
//   ihe match   <in> <out> --target uniform|gaussian [--mu M] [--sigma S]
//   ihe stats   <in> [--hist]         image statistics (+ optional histogram)
//   ihe compare <a> <b>               stats for both + MSE / PSNR
//
// Batch mode: give several input files and --out-dir <dir> (or a single
// --batch <dir> that processes every .pgm/.ppm/.pbm inside it); outputs
// are named <stem>_<method>.pgm. Single-file mode writes to <out> or,
// when -o is omitted, to <stem>_<method>.pgm next to the input.
//
// No external libraries: images are Netpbm (PGM/PBM/PPM), see image.h.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "clahe.h"
#include "equalize.h"
#include "histogram.h"
#include "image.h"
#include "matching.h"
#include "stats.h"

namespace fs = std::filesystem;

namespace {

const char* kVersion = "1.0.0";

void printUsage(std::ostream& os) {
    os << "image-histogram-equalizer " << kVersion
       << " -- histogram equalization, CLAHE and matching from scratch (C++17)\n"
       << "\nUsage:\n"
       << "  ihe he      <in> [out] [--ascii] [--out-dir DIR] [--batch DIR]\n"
       << "  ihe clahe   <in> [out] [--tiles NxM] [--clip L] [--ascii] [--out-dir DIR] [--batch DIR]\n"
       << "  ihe match   <in> <ref> <out> [--ascii]\n"
       << "  ihe match   <in> [out] --target uniform|gaussian [--mu M] [--sigma S]\n"
       << "  ihe stats   <in> [--hist]\n"
       << "  ihe compare <a> <b>\n"
       << "  ihe --help | --version\n"
       << "\nOptions:\n"
       << "  --tiles NxM    CLAHE tile grid, e.g. 8x8 (default 8x8)\n"
       << "  --clip L       CLAHE clip limit, multiple of average bin count (default 2.0)\n"
       << "  --target T     match target: uniform or gaussian (default gaussian, mu=127.5, sigma=64)\n"
       << "  --mu M         Gaussian target mean (default 127.5)\n"
       << "  --sigma S      Gaussian target stddev (default 64)\n"
       << "  --ascii        write PGM as ASCII (P2) instead of binary (P5)\n"
       << "  --hist         stats: also print the histogram\n"
       << "  --batch DIR    process every .pgm/.ppm/.pbm in DIR (needs --out-dir)\n"
       << "  --out-dir DIR  batch output directory (created if missing)\n"
       << "\nExamples:\n"
       << "  ihe he examples/low_contrast.pgm examples/outputs/low_contrast_he.pgm\n"
       << "  ihe clahe examples/dark.pgm out.pgm --tiles 8x8 --clip 2.0\n"
       << "  ihe match a.pgm b.pgm matched.pgm\n"
       << "  ihe stats examples/bimodal.pgm --hist\n"
       << "  ihe compare original.pgm enhanced.pgm\n";
}

struct Options {
    std::vector<std::string> positionals;
    std::string outDir;
    std::string batchDir;
    std::string tiles = "8x8";
    double clip = 2.0;
    std::string target = "gaussian";
    double mu = 127.5;
    double sigma = 64.0;
    bool ascii = false;
    bool hist = false;
};

bool parseDouble(const std::string& s, double& out) {
    try {
        std::size_t used = 0;
        out = std::stod(s, &used);
        return used == s.size();
    } catch (const std::exception&) {
        return false;
    }
}

bool parseTiles(const std::string& s, int& tx, int& ty) {
    const auto x = s.find('x');
    if (x == std::string::npos) return false;
    try {
        tx = std::stoi(s.substr(0, x));
        ty = std::stoi(s.substr(x + 1));
        return tx >= 1 && tx <= 64 && ty >= 1 && ty <= 64;
    } catch (const std::exception&) {
        return false;
    }
}

bool parseOptions(int argc, char** argv, Options& opt, std::string& error) {
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        auto needValue = [&](const char* flag, std::string& dst) {
            if (i + 1 >= argc) {
                error = std::string("missing value for ") + flag;
                return false;
            }
            dst = argv[++i];
            return true;
        };
        if (arg == "--out-dir") {
            if (!needValue("--out-dir", opt.outDir)) return false;
        } else if (arg == "--batch") {
            if (!needValue("--batch", opt.batchDir)) return false;
        } else if (arg == "--tiles") {
            if (!needValue("--tiles", opt.tiles)) return false;
        } else if (arg == "--target") {
            if (!needValue("--target", opt.target)) return false;
        } else if (arg == "--clip" || arg == "--mu" || arg == "--sigma") {
            std::string raw;
            if (!needValue(arg.c_str(), raw)) return false;
            double v = 0.0;
            if (!parseDouble(raw, v)) {
                error = "invalid number for " + arg + ": " + raw;
                return false;
            }
            if (arg == "--clip") opt.clip = v;
            if (arg == "--mu") opt.mu = v;
            if (arg == "--sigma") opt.sigma = v;
        } else if (arg == "--ascii") {
            opt.ascii = true;
        } else if (arg == "--hist") {
            opt.hist = true;
        } else if (arg.rfind("--", 0) == 0) {
            error = "unknown option: " + arg;
            return false;
        } else {
            opt.positionals.push_back(arg);
        }
    }
    return true;
}

bool isImageFile(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".pgm" || ext == ".ppm" || ext == ".pbm";
}

// Resolve the list of (input, output) pairs for a transform command.
// Returns false (with `error` set) when the arguments are inconsistent.
bool resolveJobs(const Options& opt, const std::string& method, std::string& error,
                 std::vector<std::pair<std::string, std::string>>& jobs) {
    std::vector<std::string> inputs;
    if (!opt.batchDir.empty()) {
        if (!fs::is_directory(opt.batchDir)) {
            error = "batch directory not found: " + opt.batchDir;
            return false;
        }
        for (const auto& entry : fs::directory_iterator(opt.batchDir)) {
            if (entry.is_regular_file() && isImageFile(entry.path())) {
                inputs.push_back(entry.path().string());
            }
        }
        std::sort(inputs.begin(), inputs.end());
        if (inputs.empty()) {
            error = "no .pgm/.ppm/.pbm files in " + opt.batchDir;
            return false;
        }
        if (opt.outDir.empty()) {
            error = "--batch needs --out-dir";
            return false;
        }
    } else {
        if (!opt.outDir.empty()) {
            // --out-dir given: every positional is an input file.
            inputs = opt.positionals;
            if (inputs.empty()) {
                error = "no input file given";
                return false;
            }
        } else if (opt.positionals.size() == 2) {
            // Single file with an explicit output path: <in> <out>.
            jobs.emplace_back(opt.positionals[0], opt.positionals[1]);
            return true;
        } else if (opt.positionals.size() == 1) {
            inputs = opt.positionals;
        } else if (opt.positionals.size() > 2) {
            error = "several inputs need --out-dir";
            return false;
        } else {
            error = "no input file given";
            return false;
        }
    }

    if (inputs.size() > 1 || !opt.batchDir.empty()) {
        if (opt.outDir.empty()) {
            error = "several inputs need --out-dir";
            return false;
        }
        std::error_code ec;
        fs::create_directories(opt.outDir, ec);
        if (ec) {
            error = "cannot create output directory: " + opt.outDir;
            return false;
        }
        for (const auto& in : inputs) {
            fs::path out = fs::path(opt.outDir) /
                           (fs::path(in).stem().string() + "_" + method + ".pgm");
            jobs.emplace_back(in, out.string());
        }
        return true;
    }

    // Single file.
    std::string out;
    if (!opt.outDir.empty()) {
        std::error_code ec;
        fs::create_directories(opt.outDir, ec);
        if (ec) {
            error = "cannot create output directory: " + opt.outDir;
            return false;
        }
        out = (fs::path(opt.outDir) /
               (fs::path(inputs[0]).stem().string() + "_" + method + ".pgm"))
                  .string();
    } else if (opt.positionals.size() >= 2) {
        // For `match`, positional[1] is the reference, not the output;
        // the caller handles that case and never reaches here with 2 args.
        out = opt.positionals[1];
    } else {
        out = (fs::path(inputs[0]).parent_path() /
               (fs::path(inputs[0]).stem().string() + "_" + method + ".pgm"))
                  .string();
    }
    jobs.emplace_back(inputs[0], out);
    return true;
}

template <typename Transform>
int runTransform(const Options& opt, const std::string& method, Transform&& transform) {
    std::string error;
    std::vector<std::pair<std::string, std::string>> jobs;
    if (!resolveJobs(opt, method, error, jobs)) {
        std::cerr << "error: " << error << "\n";
        return 2;
    }
    try {
        for (const auto& [in, out] : jobs) {
            ihe::GrayImage img = ihe::loadImage(in);
            ihe::GrayImage result = transform(img);
            ihe::savePgm(out, result, opt.ascii);
            const ihe::ImageStats before = ihe::computeStats(img);
            const ihe::ImageStats after = ihe::computeStats(result);
            std::cout << in << " -> " << out << "\n"
                      << "  mean " << before.mean << " -> " << after.mean
                      << ", stddev " << before.stddev << " -> " << after.stddev
                      << ", entropy " << before.entropy << " -> " << after.entropy
                      << " bits\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

int cmdStats(const Options& opt) {
    if (opt.positionals.size() != 1) {
        std::cerr << "error: stats needs exactly one input file\n";
        return 2;
    }
    try {
        ihe::GrayImage img = ihe::loadImage(opt.positionals[0]);
        std::cout << ihe::statsReport(img, opt.positionals[0]);
        if (opt.hist) {
            std::cout << "\nHistogram:\n";
            ihe::printHistogram(std::cout, ihe::computeHistogram(img));
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

int cmdCompare(const Options& opt) {
    if (opt.positionals.size() != 2) {
        std::cerr << "error: compare needs two image files\n";
        return 2;
    }
    try {
        ihe::GrayImage a = ihe::loadImage(opt.positionals[0]);
        ihe::GrayImage b = ihe::loadImage(opt.positionals[1]);
        std::cout << ihe::statsReport(a, opt.positionals[0]);
        std::cout << ihe::statsReport(b, opt.positionals[1]);
        const double mse = ihe::meanSquaredError(a, b);
        const double p = ihe::psnr(a, b);
        std::cout << "MSE  : " << mse << "\n";
        std::cout << "PSNR : ";
        if (std::isinf(p)) {
            std::cout << "inf dB (identical images)\n";
        } else {
            std::cout << p << " dB\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    std::cout.setf(std::ios::fixed);
    std::cout.precision(2);

    if (argc < 2) {
        printUsage(std::cerr);
        return 2;
    }
    std::string cmd = argv[1];
    if (cmd == "--help" || cmd == "-h" || cmd == "help") {
        printUsage(std::cout);
        return 0;
    }
    if (cmd == "--version" || cmd == "-V") {
        std::cout << "image-histogram-equalizer " << kVersion << "\n";
        return 0;
    }

    Options opt;
    std::string error;
    if (!parseOptions(argc, argv, opt, error)) {
        std::cerr << "error: " << error << "\n";
        printUsage(std::cerr);
        return 2;
    }

    if (cmd == "he") {
        if (opt.positionals.empty() && opt.batchDir.empty()) {
            std::cerr << "error: he needs an input file\n";
            return 2;
        }
        return runTransform(opt, "he",
                            [](const ihe::GrayImage& img) { return ihe::equalizeHistogram(img); });
    }

    if (cmd == "clahe") {
        int tx = 8, ty = 8;
        if (!parseTiles(opt.tiles, tx, ty)) {
            std::cerr << "error: invalid --tiles value: " << opt.tiles << " (expected NxM)\n";
            return 2;
        }
        if (!(opt.clip > 0.0)) {
            std::cerr << "error: --clip must be > 0\n";
            return 2;
        }
        ihe::ClaheParams params{tx, ty, opt.clip};
        return runTransform(opt, "clahe",
                            [params](const ihe::GrayImage& img) { return ihe::applyClahe(img, params); });
    }

    if (cmd == "match") {
        // match <in> <ref> <out>  (reference-image mode)
        if (opt.target == "reference" || (opt.positionals.size() == 3 && opt.batchDir.empty())) {
            if (opt.positionals.size() != 3) {
                std::cerr << "error: match with a reference needs <in> <ref> <out>\n";
                return 2;
            }
            try {
                ihe::GrayImage img = ihe::loadImage(opt.positionals[0]);
                ihe::GrayImage ref = ihe::loadImage(opt.positionals[1]);
                ihe::GrayImage result = ihe::matchToReference(img, ref);
                ihe::savePgm(opt.positionals[2], result, opt.ascii);
                std::cout << opt.positionals[0] << " -> " << opt.positionals[2]
                          << " (matched to " << opt.positionals[1] << ")\n";
            } catch (const std::exception& e) {
                std::cerr << "error: " << e.what() << "\n";
                return 1;
            }
            return 0;
        }
        // match <in> [out] --target uniform|gaussian
        if (opt.target != "uniform" && opt.target != "gaussian") {
            std::cerr << "error: --target must be uniform or gaussian\n";
            return 2;
        }
        if (opt.positionals.empty() && opt.batchDir.empty()) {
            std::cerr << "error: match needs an input file\n";
            return 2;
        }
        const std::string method = "match-" + opt.target;
        const double mu = opt.mu;
        const double sigma = opt.sigma;
        const std::string target = opt.target;
        return runTransform(opt, method, [target, mu, sigma](const ihe::GrayImage& img) {
            const int total = static_cast<int>(img.size());
            ihe::Histogram targetHist = (target == "uniform")
                                            ? ihe::uniformTargetHistogram(total)
                                            : ihe::gaussianTargetHistogram(total, mu, sigma);
            return ihe::matchHistogram(img, targetHist);
        });
    }

    if (cmd == "stats") return cmdStats(opt);
    if (cmd == "compare") return cmdCompare(opt);

    std::cerr << "error: unknown subcommand: " << cmd << "\n";
    printUsage(std::cerr);
    return 2;
}
