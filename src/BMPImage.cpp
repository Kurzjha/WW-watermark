#include "include/BMPImage.h"

#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>


constexpr std::size_t kFileHeaderSize = 14;
constexpr std::size_t kInfoHeaderSize = 40;
constexpr std::size_t kHeaderSize = kFileHeaderSize + kInfoHeaderSize;

constexpr std::uint16_t kBitsPerPixel = 24;
constexpr std::uint32_t kCompressionRgb = 0;

// Читаем 16-битное число из двух байтов в формате little-endian.
std::uint16_t ReadUint16(const std::uint8_t* data) {
    return static_cast<std::uint16_t>(data[0]) |
           (static_cast<std::uint16_t>(data[1]) << 8);
}

// Читаем 32-битное число из четырёх байтов в формате little-endian.
std::uint32_t ReadUint32(const std::uint8_t* data) {
    return static_cast<std::uint32_t>(data[0]) |
           (static_cast<std::uint32_t>(data[1]) << 8) |
           (static_cast<std::uint32_t>(data[2]) << 16) |
           (static_cast<std::uint32_t>(data[3]) << 24);
}

// Читаем знаковое 32-битное число, сохраняя его битовое представление.
std::int32_t ReadInt32(const std::uint8_t* data) {
    const std::uint32_t value = ReadUint32(data);
    std::int32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

// Записываем 16-битное число в формате little-endian.
void WriteUint16(std::uint8_t* data, std::uint16_t value) {
    data[0] = static_cast<std::uint8_t>(value & 0xFF);
    data[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
}

// Записываем 32-битное число в формате little-endian.
void WriteUint32(std::uint8_t* data, std::uint32_t value) {
    data[0] = static_cast<std::uint8_t>(value & 0xFF);
    data[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    data[2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    data[3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
}

// Записываем знаковое 32-битное число.
void WriteInt32(std::uint8_t* data, std::int32_t value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    WriteUint32(data, bits);
}

bool FitsStreamSize(std::uint64_t size) {
    return size <= static_cast<std::uint64_t>(
        std::numeric_limits<std::streamsize>::max()
    );
}

bool BMPImage::Load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // Читаем стандартные заголовки BMP: 14 + 40 байт.
    std::array<std::uint8_t, kHeaderSize> header{};

    file.read(
        reinterpret_cast<char*>(header.data()),
        static_cast<std::streamsize>(header.size())
    );

    if (!file) {
        return false;
    }

    // Проверяем сигнатуру BMP: символы 'B' и 'M'.
    if (header[0] != 'B' || header[1] != 'M') {
        return false;
    }

    const std::uint32_t pixel_offset = ReadUint32(header.data() + 10);
    const std::uint32_t info_header_size = ReadUint32(header.data() + 14);

    const std::int32_t image_width = ReadInt32(header.data() + 18);
    const std::int32_t image_height = ReadInt32(header.data() + 22);

    const std::uint16_t planes = ReadUint16(header.data() + 26);
    const std::uint16_t bits_per_pixel = ReadUint16(header.data() + 28);
    const std::uint32_t compression = ReadUint32(header.data() + 30);

    // Поддерживаем 24-битные BMP без сжатия
    // и со стандартным 40-байтовым информационным заголовком.
    if (info_header_size != kInfoHeaderSize ||
        planes != 1 ||
        bits_per_pixel != kBitsPerPixel ||
        compression != kCompressionRgb ||
        image_width <= 0 ||
        image_height == 0 ||
        image_height == std::numeric_limits<std::int32_t>::min() ||
        pixel_offset < kHeaderSize) {
        return false;
    }

    const std::uint64_t width =
        static_cast<std::uint64_t>(image_width);

    // Отрицательная высота означает, что строки записаны сверху вниз.
    const std::uint64_t height = image_height > 0
        ? static_cast<std::uint64_t>(image_height)
        : static_cast<std::uint64_t>(
              -static_cast<std::int64_t>(image_height)
          );

    // В 24-битном BMP на каждый пиксель приходится 3 байта.
    const std::uint64_t raw_row_size = width * 3;

    // Каждая строка должна занимать количество байтов,
    // кратное четырём. Лишние байты — padding.
    const std::uint64_t row_size =
        (raw_row_size + 3) & ~std::uint64_t{3};

    if (height >
        std::numeric_limits<std::uint64_t>::max() / row_size) {
        return false;
    }

    const std::uint64_t pixel_data_size = row_size * height;

    if (width >
        std::numeric_limits<std::size_t>::max() / height) {
        return false;
    }

    const std::uint64_t pixel_count = width * height;

    if (pixel_count > std::vector<Pixel>().max_size() ||
        !FitsStreamSize(row_size)) {
        return false;
    }

    const std::uint64_t required_file_size =
        static_cast<std::uint64_t>(pixel_offset) + pixel_data_size;

    file.seekg(0, std::ios::end);

    const std::streampos file_end = file.tellg();

    if (file_end < 0 ||
        static_cast<std::uint64_t>(file_end) < required_file_size) {
        return false;
    }

    file.seekg(static_cast<std::streamoff>(pixel_offset), std::ios::beg);

    if (!file) {
        return false;
    }

    std::vector<Pixel> loaded_pixels(
        static_cast<std::size_t>(pixel_count)
    );

    std::vector<std::uint8_t> row(
        static_cast<std::size_t>(row_size)
    );

    for (std::uint64_t y = 0; y < height; ++y) {
        file.read(
            reinterpret_cast<char*>(row.data()),
            static_cast<std::streamsize>(row.size())
        );

        if (!file) {
            return false;
        }

        // Положительная высота: первая строка в файле — нижняя.
        // Отрицательная высота: первая строка в файле — верхняя.
        const std::uint64_t destination_y = image_height > 0
            ? height - 1 - y
            : y;

        for (std::uint64_t x = 0; x < width; ++x) {
            const std::size_t source_index =
                static_cast<std::size_t>(x * 3);

            const std::size_t destination_index =
                static_cast<std::size_t>(destination_y * width + x);

            // BMP хранит компоненты в порядке BGR.
            loaded_pixels[destination_index] = Pixel{
                row[source_index],
                row[source_index + 1],
                row[source_index + 2]
            };
        }
    }

    // Обновляем объект только после успешного чтения всех пикселей.
    width_ = image_width;
    height_ = static_cast<int>(height);
    bits_per_pixel_ = bits_per_pixel;
    pixels_ = std::move(loaded_pixels);

    return true;
}

bool BMPImage::Save(const std::string& filename) const {
    if (width_ <= 0 ||
        height_ <= 0 ||
        bits_per_pixel_ != kBitsPerPixel) {
        return false;
    }

    const std::uint64_t width =
        static_cast<std::uint64_t>(width_);

    const std::uint64_t height =
        static_cast<std::uint64_t>(height_);

    const std::uint64_t raw_row_size = width * 3;

    const std::uint64_t row_size =
        (raw_row_size + 3) & ~std::uint64_t{3};

    if (height >
        std::numeric_limits<std::uint64_t>::max() / row_size) {
        return false;
    }

    const std::uint64_t pixel_data_size = row_size * height;

    // Полный размер BMP должен помещаться в 32-битное поле заголовка.
    if (pixel_data_size >
        std::numeric_limits<std::uint32_t>::max() - kHeaderSize) {
        return false;
    }

    const std::uint64_t expected_pixel_count = width * height;

    if (expected_pixel_count != pixels_.size() ||
        !FitsStreamSize(row_size)) {
        return false;
    }

    const std::uint32_t file_size =
        static_cast<std::uint32_t>(kHeaderSize + pixel_data_size);

    std::array<std::uint8_t, kHeaderSize> header{};

    // Заголовок файла BMP.
    header[0] = 'B';
    header[1] = 'M';

    WriteUint32(header.data() + 2, file_size);
    WriteUint32(header.data() + 10,
                static_cast<std::uint32_t>(kHeaderSize));

    // Информационный заголовок BITMAPINFOHEADER.
    WriteUint32(header.data() + 14,
                static_cast<std::uint32_t>(kInfoHeaderSize));

    WriteInt32(header.data() + 18, width_);
    WriteInt32(header.data() + 22, height_);

    WriteUint16(header.data() + 26, 1);
    WriteUint16(header.data() + 28, kBitsPerPixel);
    WriteUint32(header.data() + 30, kCompressionRgb);
    WriteUint32(header.data() + 34,
                static_cast<std::uint32_t>(pixel_data_size));

    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    file.write(
        reinterpret_cast<const char*>(header.data()),
        static_cast<std::streamsize>(header.size())
    );

    if (!file) {
        return false;
    }

    // Нулевые значения используются для padding.
    std::vector<std::uint8_t> row(
        static_cast<std::size_t>(row_size), 0
    );

    for (std::uint64_t y = 0; y < height; ++y) {
        // Записываем строки снизу вверх.
        const std::uint64_t source_y = height - 1 - y;

        for (std::uint64_t x = 0; x < width; ++x) {
            const std::size_t source_index =
                static_cast<std::size_t>(source_y * width + x);

            const std::size_t destination_index =
                static_cast<std::size_t>(x * 3);

            const Pixel& pixel = pixels_[source_index];

            row[destination_index] = pixel.blue;
            row[destination_index + 1] = pixel.green;
            row[destination_index + 2] = pixel.red;
        }

        file.write(
            reinterpret_cast<const char*>(row.data()),
            static_cast<std::streamsize>(row.size())
        );

        if (!file) {
            return false;
        }
    }

    return static_cast<bool>(file);
}

int BMPImage::GetWidth() const {
    return width_;
}

int BMPImage::GetHeight() const {
    return height_;
}

int BMPImage::GetBitsPerPixel() const {
    return bits_per_pixel_;
}

std::vector<BMPImage::Pixel>& BMPImage::GetPixels() {
    return pixels_;
}

const std::vector<BMPImage::Pixel>& BMPImage::GetPixels() const {
    return pixels_;
}

BMPImage::Pixel BMPImage::GetPixel(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return Pixel{};
    }

    const std::size_t index =
        static_cast<std::size_t>(y) *
        static_cast<std::size_t>(width_) +
        static_cast<std::size_t>(x);

    return pixels_[index];
}

bool BMPImage::SetPixel(int x, int y, const Pixel& pixel) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return false;
    }

    const std::size_t index =
        static_cast<std::size_t>(y) *
        static_cast<std::size_t>(width_) +
        static_cast<std::size_t>(x);

    pixels_[index] = pixel;

    return true;
}
