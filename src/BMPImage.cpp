#include "BMPImage.h"

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
