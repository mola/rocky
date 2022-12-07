/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "Image.h"
#include "Notify.h"
//#include <stb_image.h>

using namespace rocky;

namespace
{
    using uchar = unsigned char;
    using ushort = unsigned short;

    constexpr float norm_8 = 255.0f;
    constexpr float denorm_8 = 1.0 / norm_8;

    template<typename T>
    struct NORM8 {
        static void read(Image::Pixel& pixel, unsigned char* ptr, int n) {
            for (int i = 0; i < n; ++i)
                pixel[i] = (float)(*ptr++) * denorm_8;
        }
        static void write(const Image::Pixel& pixel, unsigned char* ptr, int n) {
            for (int i = 0; i < n; ++i)
                *ptr++ = (T)(pixel[i] * norm_8);
        }
    };

    constexpr float norm_16 = 65535.0f;
    constexpr float denorm_16 = 1.0 / norm_16;

    template<typename T>
    struct NORM16 {
        static void read(Image::Pixel& pixel, unsigned char* ptr, int n) {
            T* sptr = (T*)ptr;
            for (int i = 0; i < n; ++i)
                pixel[i] = (float)(*sptr++) * denorm_16;
        }
        static void write(const Image::Pixel& pixel, unsigned char* ptr, int n) {
            T* sptr = (T*)ptr;
            for (int i = 0; i < n; ++i)
                *sptr++ = (T)(pixel[i] * norm_16);
        }
    };

    template<typename T>
    struct FLOAT {
        static void read(Image::Pixel& pixel, unsigned char* ptr, int n) {
            T* sptr = (T*)ptr;
            for (int i = 0; i < n; ++i)
                pixel[i] = (float)(*sptr++);
        }
        static void write(const Image::Pixel& pixel, unsigned char* ptr, int n) {
            T* sptr = (T*)ptr;
            for (int i = 0; i < n; ++i)
                *sptr++ = (T)pixel[i];
        }
    };
}

// static member
Image::Layout Image::_layouts[7] =
{
    { &NORM8<uchar>::read, &NORM8<uchar>::write, 1, 1, R8_UNORM },
    { &NORM8<uchar>::read, &NORM8<uchar>::write, 2, 2, R8G8_UNORM },
    { &NORM8<uchar>::read, &NORM8<uchar>::write, 3, 3, R8G8B8_UNORM },
    { &NORM8<uchar>::read, &NORM8<uchar>::write, 4, 4, R8G8B8A8_UNORM },
    { &NORM16<ushort>::read, &NORM16<ushort>::write, 1, 2, R16_UNORM },
    { &FLOAT<float>::read, &FLOAT<float>::write, 1, 4, R32_SFLOAT },
    { &FLOAT<double>::read, &FLOAT<double>::write, 1, 8, R64_SFLOAT }
};

Image::Image() :
    super(),
    _width(0), _height(0), _depth(0),
    _pixelFormat(R8G8B8A8_UNORM)
{
    //todo
}

Image::Image(
    PixelFormat format,
    unsigned cols,
    unsigned rows,
    unsigned depth) :
    
    super(),
    _width(0), _height(0), _depth(0),
    _pixelFormat(R8G8B8A8_UNORM)
{
    allocate(cols, rows, depth, format);
}

shared_ptr<Image>
Image::clone() const
{
    auto clone = Image::create(
        pixelFormat(), width(), height(), depth());

    memcpy(
        clone->data<unsigned char*>(),
        _data.get(),
        sizeInBytes());

    return clone;
}

void
Image::allocate(
    unsigned width_,
    unsigned height_,
    unsigned depth_,
    PixelFormat pixelFormat_)
{
    ROCKY_SOFT_ASSERT_AND_RETURN(
        width_ > 0 && height_ > 0 && depth_ > 0 &&
        (unsigned)pixelFormat_ >= 0 && pixelFormat_ < NUM_PIXEL_FORMATS,
        void());
    
    _width = width_;
    _height = height_;
    _depth = depth_;
    _pixelFormat = pixelFormat_;

    auto layout = _layouts[pixelFormat()];

    _data = std::unique_ptr<unsigned char>(
        new unsigned char[sizeInBytes()]);
}

bool
Image::copyAsSubImage(
    Image* dst,
    unsigned dst_start_col,
    unsigned dst_start_row) const
{
    if (!valid() || !dst || !dst->valid() ||
        dst_start_col + width() > dst->width() ||
        dst_start_row + height() > dst->height() ||
        depth() != dst->depth())
    {
        return false;
    }

    Pixel pixel;
    for (unsigned r = 0; r < depth(); ++r)
    {
        for (unsigned src_t = 0, dst_t = dst_start_row; src_t < height(); src_t++, dst_t++)
        {
            for (unsigned src_s = 0, dst_s = dst_start_col; src_s < width(); src_s++, dst_s++)
            {
                read(pixel, src_s, src_t, r);
                dst->write(pixel, dst_s, dst_t, r);
            }
        }
    }

    return true;
}

void
Image::flipVerticalInPlace()
{
    ROCKY_TODO("handle compressed pixel formats");

    auto rowBytes = rowSizeInBytes();

    for (unsigned d = 0; d < depth(); ++d)
    {
        auto halfRows = height() / 2;
        for (unsigned row = 0; row < halfRows; ++row)
        {
            auto row1 = data<uchar*>(0, row, d);
            auto row2 = data<uchar*>(0, ((height()) - 1 - row), d);
            for (unsigned b = 0; b < rowBytes; ++b, ++row1, ++row2)
                std::swap(*row1, *row2);
        }
    }
}

void
Image::fill(const Image::Pixel& value)
{
    for (unsigned r = 0; r < depth(); ++r)
        for (unsigned t = 0; t < height(); ++t)
            for (unsigned s = 0; s < width(); ++s)
                write(value, s, t, r);
}
