#ifndef BMPIMAGE_H
#define BMPIMAGE_H

#include <cstdint>
#include <string>
#include <vector>

class BMPImage {
public:
    struct Pixel {
        std::uint8_t blue = 0;
        std::uint8_t green = 0;
        std::uint8_t red = 0;
    };

    bool Load(const std::string& filename);
    bool Save(const std::string& filename) const;

    int GetWidth() const;
    int GetHeight() const;
    int GetBitsPerPixel() const;

    std::vector<Pixel>& GetPixels();
    const std::vector<Pixel>& GetPixels() const;
    Pixel GetPixel(int x, int y) const;
    bool SetPixel(int x, int y, const Pixel& pixel);

private:
    int width_ = 0;
    int height_ = 0;
    int bits_per_pixel_ = 0;

    std::vector<Pixel> pixels_;
};

#endif
