#include "BMPImage.h"
#include "Watermark.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

using Pixel = BMPImage::Pixel;

void WriteU16(std::ostream& out, std::uint16_t value) {
    out.put(static_cast<char>(value & 0xff));
    out.put(static_cast<char>((value >> 8) & 0xff));
}

void WriteU32(std::ostream& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.put(static_cast<char>((value >> shift) & 0xff));
    }
}

// Creates a small bottom-up BMP fixture, including row padding.
bool WriteFixture(const std::filesystem::path& path, int width, int height,
                 const std::vector<Pixel>& pixels) {
    const std::uint32_t rowBytes = static_cast<std::uint32_t>(width * 3);
    const std::uint32_t stride = (rowBytes + 3u) & ~3u;
    const std::uint32_t dataSize = stride * static_cast<std::uint32_t>(height);
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    out.put('B'); out.put('M');
    WriteU32(out, 54u + dataSize);
    WriteU16(out, 0); WriteU16(out, 0);
    WriteU32(out, 54);
    WriteU32(out, 40);
    WriteU32(out, static_cast<std::uint32_t>(width));
    WriteU32(out, static_cast<std::uint32_t>(height));
    WriteU16(out, 1); WriteU16(out, 24);
    WriteU32(out, 0); WriteU32(out, dataSize);
    WriteU32(out, 2835); WriteU32(out, 2835);
    WriteU32(out, 0); WriteU32(out, 0);

    std::vector<char> row(stride, 0);
    for (int fileY = 0; fileY < height; ++fileY) {
        const int y = height - 1 - fileY;
        for (int x = 0; x < width; ++x) {
            const Pixel& p = pixels[static_cast<std::size_t>(y * width + x)];
            const auto i = static_cast<std::size_t>(x * 3);
            row[i] = static_cast<char>(p.blue);
            row[i + 1] = static_cast<char>(p.green);
            row[i + 2] = static_cast<char>(p.red);
        }
        out.write(row.data(), static_cast<std::streamsize>(row.size()));
    }
    return static_cast<bool>(out);
}

bool Equal(const Pixel& a, const Pixel& b) {
    return a.blue == b.blue && a.green == b.green && a.red == b.red;
}

bool Run() {
    const auto uniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = std::filesystem::temp_directory_path() /
        ("ww-watermark-tests-" + std::to_string(uniqueSuffix));
    std::filesystem::create_directories(dir);
    const auto originalPath = dir / "original.bmp";
    const auto markPath = dir / "mark.bmp";
    const auto outputPath = dir / "output.bmp";

    const Pixel base{20, 40, 60};
    const Pixel mark{220, 240, 100};
    if (!WriteFixture(originalPath, 3, 2, std::vector<Pixel>(6, base)) ||
        !WriteFixture(markPath, 1, 1, {mark})) {
        std::cerr << "Failed to create BMP fixtures\n";
        return false;
    }

    BMPImage original;
    BMPImage watermark;
    if (!original.Load(originalPath.string()) || !watermark.Load(markPath.string())) {
        std::cerr << "Failed to load BMP fixtures\n";
        return false;
    }
    if (original.GetWidth() != 3 || original.GetHeight() != 2 ||
        original.GetBitsPerPixel() != 24 || !Equal(original.GetPixel(2, 1), base)) {
        std::cerr << "BMP dimensions, padding, or pixel order are incorrect\n";
        return false;
    }
    if (!Watermark::Apply(original, watermark, 0.5)) {
        std::cerr << "Watermark application failed\n";
        return false;
    }
    const Pixel blended{120, 140, 80};
    if (!Equal(original.GetPixel(1, 0), blended) ||
        !Equal(original.GetPixel(0, 0), base) ||
        !Equal(original.GetPixel(1, 1), base)) {
        std::cerr << "Blend value, placement, or unaffected pixels are incorrect\n";
        return false;
    }
    if (!original.Save(outputPath.string())) {
        std::cerr << "Failed to save BMP result\n";
        return false;
    }
    BMPImage roundTrip;
    if (!roundTrip.Load(outputPath.string()) || roundTrip.GetWidth() != 3 ||
        roundTrip.GetHeight() != 2 || !Equal(roundTrip.GetPixel(1, 0), blended)) {
        std::cerr << "Saved BMP did not survive a load/save round trip\n";
        return false;
    }
    if (Watermark::Apply(roundTrip, watermark, 1.5)) {
        std::cerr << "Invalid alpha was accepted\n";
        return false;
    }

    std::filesystem::remove_all(dir);
    return true;
}

} // namespace

int main() {
    if (!Run()) return 1;
    std::cout << "All watermark checks passed\n";
    return 0;
}
