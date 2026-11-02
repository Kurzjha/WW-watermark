#ifndef WATERMARK_H
#define WATERMARK_H

#include "BMPImage.h"

class Watermark {
public:
    // Накладывает водяной знак на исходное изображение.
    // alpha — прозрачность водяного знака (от 0.0 до 1.0).
    // По умолчанию используется прозрачность 30%.
    static bool Apply(BMPImage& original, const BMPImage& watermark, double alpha = 0.45);
};

#endif
