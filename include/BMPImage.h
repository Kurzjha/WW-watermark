#ifndef BMPIMAGE_H
#define BMPIMAGE_H

#include <cstdint>
#include <string>
#include <vector>

class BMPImage {
public:
    bool Load(const std::string& filename);

    int GetWidth() const;
    int GetHeight() const;
    int GetBitsPerPixel() const;

private:
    int width_ = 0;
    int height_ = 0;
    int bits_per_pixel_ = 0;

    std::vector<std::uint8_t> pixels_;
};

#endif