#include "Watermark.h"

#include <algorithm>
#include <cmath>

// Смешивание двух цветовых компонентов.
static std::uint8_t BlendChannel(std::uint8_t original, std::uint8_t mark, double alpha) {
    double result = original * (1.0 - alpha) + mark * alpha;
    return static_cast<std::uint8_t>(std::clamp(std::lround(result), 0L, 255L));
}
// Основная функция наложения водяного знака.
bool Watermark::Apply(BMPImage& original, const BMPImage& watermark, double alpha) {
    // Проверяем корректность коэффициента прозрачности.
    if (!std::isfinite(alpha) || alpha < 0.0 || alpha > 1.0) {
        return false;
    }

    // Получаем размеры изображений.
    const int originalWidth = original.GetWidth();
    const int originalHeight = original.GetHeight();

    const int watermarkWidth = watermark.GetWidth();
    const int watermarkHeight = watermark.GetHeight();

    // Проверяем корректность размеров.
    if (originalWidth <= 0 || originalHeight <= 0 || watermarkWidth <= 0 || watermarkHeight <= 0) {
        return false;
    }

    // Проверяем глубину цвета.
    if (original.GetBitsPerPixel() != 24 || watermark.GetBitsPerPixel() != 24) {
        return false;
    }

    // Водяной знак не должен быть больше оригинала.
    if (watermarkWidth > originalWidth || watermarkHeight > originalHeight) {
        return false;
    }

    // Вычисляем координаты размещения по центру.
    const int startX = (originalWidth - watermarkWidth) / 2;
    const int startY = (originalHeight - watermarkHeight) / 2;

    // Перебираем все пиксели водяного знака.
    for (int y = 0; y < watermarkHeight; ++y) {
        for (int x = 0; x < watermarkWidth; ++x) {

            // Получаем соответствующие пиксели.
            BMPImage::Pixel originalPixel = original.GetPixel(startX + x, startY + y);

            BMPImage::Pixel watermarkPixel = watermark.GetPixel(x, y);

            // Смешиваем синий канал.
            BMPImage::Pixel resultPixel;

            resultPixel.blue = BlendChannel(originalPixel.blue, watermarkPixel.blue, alpha);

            // Смешиваем зелёный канал.
            resultPixel.green = BlendChannel(originalPixel.green, watermarkPixel.green, alpha);

            // Смешиваем красный канал.
            resultPixel.red = BlendChannel(originalPixel.red, watermarkPixel.red, alpha);

            // Записываем изменённый пиксель.
            if (!original.SetPixel(startX + x, startY + y, resultPixel)) {
                return false;
            }
        }
    }

    return true;
}
