cmake_minimum_required(VERSION 3.16)
project(WWWatermark LANGUAGES CXX)

include(CTest)

add_executable(ww-watermark
    src/main.cpp
    src/BMPImage.cpp
    src/Watermark.cpp
)

target_include_directories(ww-watermark PRIVATE include)
target_compile_features(ww-watermark PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(ww-watermark PRIVATE /W4 /permissive-)
else()
    target_compile_options(ww-watermark PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(ww-watermark-tests
        tests/test_watermark.cpp
        src/BMPImage.cpp
        src/Watermark.cpp
    )
    target_include_directories(ww-watermark-tests PRIVATE include)
    target_compile_features(ww-watermark-tests PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(ww-watermark-tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ww-watermark-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME watermark_unit_tests COMMAND ww-watermark-tests)
endif()
