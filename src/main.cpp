#include "BMPImage.h"
#include "Watermark.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void PrintUsage(const char* program) {
    std::cerr << "Usage: " << program
              << " <original.bmp> <watermark.bmp> <output.bmp> [alpha]\n"
              << "  alpha: value from 0.0 to 1.0 (default: 0.3)\n";
}

}

int main(int argc, char* argv[]) {
    if (argc != 4 && argc != 5) {
        PrintUsage(argv[0]);
        return 1;
    }

    double alpha = 0.3;
    if (argc == 5) {
        try {
            std::size_t parsedCharacters = 0;
            alpha = std::stod(argv[4], &parsedCharacters);
            if (parsedCharacters != std::string(argv[4]).size() ||
                !std::isfinite(alpha) || alpha < 0.0 || alpha > 1.0) {
                throw std::invalid_argument("alpha must be between 0.0 and 1.0");
            }
        } catch (const std::exception&) {
            std::cerr << "Error: alpha must be a number from 0.0 to 1.0.\n";
            PrintUsage(argv[0]);
            return 1;
        }
    }

    BMPImage original;
    if (!original.Load(argv[1])) {
        std::cerr << "Error: could not load original BMP image: " << argv[1] << '\n';
        return 2;
    }

    BMPImage watermark;
    if (!watermark.Load(argv[2])) {
        std::cerr << "Error: could not load watermark BMP image: " << argv[2] << '\n';
        return 3;
    }

    if (watermark.GetWidth() > original.GetWidth() ||
        watermark.GetHeight() > original.GetHeight()) {
        std::cerr << "Error: watermark dimensions must not exceed the original image.\n";
        return 4;
    }

    std::cout << "Applying watermark...\n";
    if (!Watermark::Apply(original, watermark, alpha)) {
        std::cerr << "Error: could not apply watermark. Check image dimensions and alpha.\n";
        return 5;
    }

    if (!original.Save(argv[3])) {
        std::cerr << "Error: could not save output BMP image: " << argv[3] << '\n';
        return 6;
    }

    std::cout << "Saved watermarked image to " << argv[3] << '\n';
    return 0;
}
