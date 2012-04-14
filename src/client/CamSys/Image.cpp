/*
 * Copyright (c) 2009, 2010 Kolja Glogowski
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QDataStream>
#include <QSysInfo>

#include <cstring>    // for std::memcpy
#include <algorithm>  // for std::swap, std::reverse, ...

#include "Image.h"

namespace CamSys {

/*! \class Image
    \brief A class for handling images with various bit depths.

    \todo Implementation:
      - Template specializations for scanLine.
      - Make data() a template function?
      - Move data to an extra ImageData template class.
      - Hide private data.
 */

/*! \enum Image::Format
    \brief The pixel format.

    The data type of the pixels.

    \see FormatInfo, TypeInfo
 */

/*! \struct Image::FormatInfo<int>
    \brief Image pixel format informations.

    This template is useful to get compile time informations on the
    different pixel formats supported by this class.

    \see Format, TypeInfo
 */

/*! \struct Image::TypeInfo<T>
    \brief Image pixel type informations.

    This template is useful to get compile time informations on the
    different pixel types supported by this class.

    \see Format, FormatInfo
 */

/*! \fn T Image::pixel<T>(int x, int y) const
    \brief Access a pixel at position (\a x, \a y).

    The template parameter must be compatible to the particular image
    format (see Image::Format). For example it is possible to read 16bit
    pixel values using a 32bit integer or even a floating point variable,
    but on the other hand, using a 16bit variable with 32bit pixel data
    may result in invalid pixel values.

    \note The performace of this method is rather slow. If you need to
        access more than a couple of pixels, you should use scanLine()
        instead.

    \see pixel(const QPoint &), scanLine(), setPixel()
 */

/*! \fn T Image::pixel<T>(const QPoint &pos) const
    \brief Access a pixel at position \a pos.

    \see pixel(int,int), scanLine(), setPixel()
 */

/*! \fn void Image::setPixel<T>(int x, int y, T value)
    \brief Set a pixel at position (\a x, \a y).

    \see setPixel(const QPoint &), scanLine(), pixel()
 */

/*! \fn void Image::setPixel<T>(const QPoint &pos, T value)
    \brief Set a pixel at position \a pos.

    \see setPixel(int, int), scanLine(), pixel()
 */

/*! \fn T * Image::scanLine<T>(int i);
    \brief Get a pointer to a line of pixels.

    \returns a pointer to the first pixel of line i, where i must be a
        valid line index, i.e. <tt>0 <= i < height</tt>.

    In order to respect an abitrary number of bytes per line, this method
    should be used to access the pixel data.

    \note The template parameter must correspond to the particular image
        format (see Image::Format). To get a pointer to the raw byte
        representation of a line, it is always possible to use
        <tt>char/uchar</tt> or <tt>qint8/quint8</tt> as template parameter.

    <b>Examples:</b>
    - Accessing pixel data of a Uint16 image:
    \code
    quint16 *pixelLine = img.scanLine<quint16>(0);
    for (int i = 0; i < img.width(); ++i)
        pixelLine[i] = 1234;
    \endcode

    - Accessing raw data of a line:
    \code
    uchar *rawLine = img.scanLine<uchar>(0);
    for (int i = 0; i < img.width() * img.bytePerPixel(); ++i)
        rawLine[i] = 0;
    \endcode

    - Accessing pixel data of a Int16 image using the FormatInfo template:
    \code
    typedef Image::FormatInfo<Int16>::Type PixelType;
    PixelType *pixelLine = img.scanLine<PixelType>(0);
    for (int i = 0; i < img.width(); ++i)
        pixelLine[i] = 1337;
    \endcode

    \see data()
 */

/*! \fn const T * Image::scanLine<T>(int i) const
    \brief Get const pointer to a line of pixels.

    \returns a const pointer to the first pixel of line i, where i must be
        a valid line index, i.e. <tt>0 <= i < height</tt>.

    \see scanLine()
 */


//! Magic code: { 'C', 'I', 'M', 'G' }
const quint32 Image::Magic = 0x43494d47;

//! \internal
void Image::init(int width, int height, Format format, int bitDepth,
    int bytesPerLine /*= 0*/, int dataSize /*= 0*/, uchar *data /*= 0*/)
{
    // clear all member data
    _width = _height = 0;
    _data = 0;
    _dataSize = _bytesPerLine = _bytesPerPixel = _bitDepth = 0;
    _allocated = false;

    // set format
    _format = format;

    // set bytes per pixel
    _bytesPerPixel = bytesPerPixel(_format);
    Q_ASSERT(_bytesPerPixel >= 0);

    // set bit depth
    setBitDepth(bitDepth);

    // check width and height
    Q_ASSERT(width >= 0);
    Q_ASSERT(height >= 0);
    if (width < 0 || height < 0)
        return;

    // check / calculate the number of bytes per line
    if (bytesPerLine == 0)
        bytesPerLine = width * _bytesPerPixel;
    Q_ASSERT(bytesPerLine >= width * _bytesPerPixel);
    if (bytesPerLine < width * _bytesPerPixel)
        return;

    // check / calculate number data bytes
    Q_ASSERT(dataSize >= 0);
    qint64 size = qint64(bytesPerLine) * qint64(height);
    if (size > std::numeric_limits<int>::max())
        return;
    if (dataSize == 0)
        dataSize = size;
    Q_ASSERT(dataSize >= size);
    if (dataSize < size)
        return;

    // set / allocate data
    if (data != 0) {
        _data = data;
        _allocated = false;
    } else {
        _data = new uchar[dataSize];
        _allocated = true;
    }
    Q_ASSERT(_data);
    if (!_data)
        return;

    // set sizes
    _width = width;
    _height = height;
    _bytesPerLine = bytesPerLine;
    _dataSize = dataSize;
}

/*!
    \brief Constructs a null image.

    This Constructor is usefull for deserialization.

    \see isNull()
 */
Image::Image()
    : _width(0), _height(0), _format(Uint8), _data(0), _dataSize(0),
        _bytesPerLine(0), _bytesPerPixel(0), _bitDepth(0), _allocated(false)
{
}

/*!
    \brief Constructs an image and allocates a new data buffer.

    \todo Documentation.
 */
Image::Image(int width, int height, Format format, int bitDepth)
{
    init(width, height, format, bitDepth);
}

/*!
    \brief Constructs an image and allocates a new data buffer using the
        specified number of bytes per line.

    \todo Documentation.
 */
Image::Image(int width, int height, int bytesPerLine, Format format,
    int bitDepth)
{
    init(width, height, format, bitDepth, bytesPerLine);
}

/*!
    \brief Constructs an image and allocates a new data buffer using the
        specified number of bytes per line and total number of bytes.

    \todo Documentation.
 */
Image::Image(int width, int height, int bytesPerLine, int dataSize,
    Format format, int bitDepth)
{
    init(width, height, format, bitDepth, bytesPerLine, dataSize);
}

/*!
    \brief Constructs an image using the buffer specified by the data
        pointer.

    \todo Documentation.
 */
Image::Image(uchar *data, int width, int height, Format format, int bitDepth)
{
    init(width, height, format, bitDepth, 0, 0, data);
}

/*!
    \todo
     - Implementation?
     - Documentation.
 */
Image::Image(uchar *data, int width, int height, int bytesPerLine,
    Format format, int bitDepth)
{
    init(width, height, format, bitDepth, bytesPerLine, 0, data);
}

/*!
    \todo
     - Implementation?
     - Documentation.
 */
Image::Image(uchar *data, int width, int height, int bytesPerLine,
    int dataSize, Format format, int bitDepth)
{
    init(width, height, format, bitDepth, bytesPerLine, dataSize, data);
}

/*!
    \brief Copy constructor.

    This creates a copy of the supplied other image, performing a deep copy
    of the image data.
 */
Image::Image(const Image &other)
    : _width(other._width), _height(other._height), _format(other._format),
        _data(0), _dataSize(0), _bytesPerLine(other._bytesPerLine),
        _bytesPerPixel(other._bytesPerPixel), _bitDepth(other._bitDepth),
        _allocated(false)
{
    if (other._dataSize == 0)
        return;

    _data = new uchar[other._dataSize];
    if (_data == 0)
    {
        // an error occured, construct null image
        _width = _height = 0;
        _format = Uint8;
        _dataSize = _bytesPerLine = _bytesPerPixel = _bitDepth = 0;
        _allocated = false;
        return;
    }

    _dataSize = other._dataSize;
    _allocated = true;

    // make a deep copy of the image data
    std::memcpy(_data, other._data, _dataSize);
}

/*!
    \brief Assignement operator.

    This assigns a copy of other to the current image, performing a deep
    copy of the image data.

    \attention The internal data of the image may be reallocated during
        the assignment operation. So all pointers to the image data which
        were obtained by data() or scanLine() may have become invalid after
        calling this operator.

    \note If an external buffer is used for the image data and the dataSize
        of this buffer doesn't match the dataSize of the other image object,
        then the external buffer will be replaced by a newly allocated
        buffer and needs to be freed by the user. Before freeing the external
        buffer, you should always check via data(), if the buffer has been
        really detached from the image object.
 */
Image & Image::operator= (const Image &other)
{
    if (this == &other)
        return *this;

    // (re)allocate memory, if neccessary
    if (_dataSize != other._dataSize)
    {
        if (_allocated)
            delete [] _data;

        _data = new uchar[other._dataSize];
        if (_data == 0)
        {
            // an error occured, convert to null image
            _width = _height = 0;
            _format = Uint8;
            _dataSize = _bytesPerLine = _bytesPerPixel = _bitDepth = 0;
            _allocated = false;
            return *this;
        }

        _allocated = true;
    }

    _width = other._width;
    _height = other._height;
    _format = other._format;
    _dataSize = other._dataSize;
    _bytesPerLine = other._bytesPerLine;
    _bytesPerPixel = other._bytesPerPixel;
    _bitDepth = other._bitDepth;

    // make a deep copy of the image data
    std::memcpy(_data, other._data, _dataSize);

    return *this;
}

/*!
    \brief Destructor.
 */
Image::~Image()
{
    if (_allocated && _data)
        delete [] _data;
}

/*!
    \brief Clones the current image.

    \returns a copy of the current image, performing a deep copy of the data.
 */
Image * Image::clone() const
{
    return new Image(*this);
}

/*!
    \brief Get the width of the image.

    \returns the width of the image, i.e. the number of pixels per line.
 */
int Image::width() const
{
    return _width;
}

/*!
    \brief Get the height of the image.

    \return the height of the image, i.e. the number of lines.
 */
int Image::height() const
{
    return _height;
}

/*!
    \brief Get the pixel format.

    \returns the image's pixel format.

    \see Format
 */
Image::Format Image::format() const
{
    return _format;
}

/*!
    \brief Check if this is a null image.

    A null image is an image without data.
 */
bool Image::isNull() const
{
    Q_ASSERT(_data == 0
        || (_width != 0 && _height != 0 && _bytesPerLine != 0));
    return _data == 0;
}

/*!
    \brief Clear all data and free the allocated buffer, i.e. convert
        the image to a null image.
 */
void Image::clear()
{
    if (_allocated && _data)
        delete [] _data;

    _data = 0;
    _width = _height = 0;
    _format = Uint8;
    _dataSize = _bytesPerLine = _bytesPerPixel = _bitDepth = 0;
    _allocated = false;
}

/*!
    \brief Reset the image.

    \todo Documentation.
 */
void Image::reset(int width, int height, Format format, int bitDepth)
{
    Q_ASSERT(width >= 0);
    Q_ASSERT(height >= 0);
    if (width < 0 || height < 0) {
        qWarning("Image::reset(): Width and/or height is negative.");
        clear();
        return;
    }

    int bpp = bytesPerPixel(format);
    qint64 size = qint64(width) * qint64(height) * bpp;

    // set to null image, if the new size is to big
    if (size > std::numeric_limits<int>::max()) {
        qWarning("Image::reset(): New data size is too big.");
        clear();
        return;
    }

    // reallocate, if the new size is bigger than the old one
    if (size > _dataSize)
    {
        if (_allocated && _data)
            delete [] _data;

        _data = new uchar[size];
        if (!_data) {
            qWarning("Image::reset(): Memory allocation failed.");
            clear();
            return;
        }

        _allocated = true;
        _dataSize = size;
    }

    // set data members to the new format
    _width = width;
    _height = height;
    _format = format;
    _bytesPerLine = _width * bpp;
    _bytesPerPixel = bpp;
    setBitDepth(bitDepth);
}

/*!
    \brief Reset the image.

    \todo
     - Implementation.
 */
void Image::reset(int width, int height, int bytesPerLine, Format format,
    int bitDepth)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(bytesPerLine);
    Q_UNUSED(format);
    Q_UNUSED(bitDepth);
}

/*!
    \brief Reset the image.

    \todo
     - Implementation.
 */
void Image::reset(int width, int height, int bytesPerLine, int dataSize,
    Format format, int bitDepth)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(bytesPerLine);
    Q_UNUSED(dataSize);
    Q_UNUSED(format);
    Q_UNUSED(bitDepth);
}

//! \internal
template <typename T>
void Image::doFill(T value)
{
    for (int i = 0; i < _height; ++i) {
        T *line = scanLine<T>(i);
        for (int j = 0; j < _width; ++j)
            line[j] = value;
    }
}

/*!
    \brief Set all pixels to the specified value.

    \todo Documentation.
 */
void Image::fill(qint64 value)
{
    switch (_format)
    {
    case Uint8:
        doFill(FormatInfo<Uint8>::type(value));
        return;
    case Int8:
        doFill(FormatInfo<Int8>::type(value));
        return;
    case Uint16:
        doFill(FormatInfo<Uint16>::type(value));
        return;
    case Int16:
        doFill(FormatInfo<Int16>::type(value));
        return;
    case Uint32:
        doFill(FormatInfo<Uint32>::type(value));
        return;
    case Int32:
        doFill(FormatInfo<Int32>::type(value));
        return;
    case Float32:
        doFill(FormatInfo<Float32>::type(value));
        return;
    case Float64:
        doFill(FormatInfo<Float64>::type(value));
        return;
    }

    Q_ASSERT(false);
}

/*!
    \brief Set all pixels to the specified value.

    \todo Documentation.
 */
void Image::fill(double value)
{
    switch (_format)
    {
    case Uint8:
        doFill(FormatInfo<Uint8>::type(value));
        return;
    case Int8:
        doFill(FormatInfo<Int8>::type(value));
        return;
    case Uint16:
        doFill(FormatInfo<Uint16>::type(value));
        return;
    case Int16:
        doFill(FormatInfo<Int16>::type(value));
        return;
    case Uint32:
        doFill(FormatInfo<Uint32>::type(value));
        return;
    case Int32:
        doFill(FormatInfo<Int32>::type(value));
        return;
    case Float32:
        doFill(FormatInfo<Float32>::type(value));
        return;
    case Float64:
        doFill(FormatInfo<Float64>::type(value));
        return;
    }

    Q_ASSERT(false);
}

/*!
    \brief Get the number of bytes per pixel.

    \returns the number of bytes per pixel.
 */
int Image::bytesPerPixel() const
{
    return _bytesPerPixel;
}

/*!
    \brief Get the number of bytes per pixel for the specified Format.
 */
int Image::bytesPerPixel(Format format)
{
    switch (format)
    {
    case Uint8:
        return FormatInfo<Uint8>::bytesPerPixel;
    case Int8:
        return FormatInfo<Int8>::bytesPerPixel;
    case Uint16:
        return FormatInfo<Uint16>::bytesPerPixel;
    case Int16:
        return FormatInfo<Int16>::bytesPerPixel;
    case Uint32:
        return FormatInfo<Uint32>::bytesPerPixel;
    case Int32:
        return FormatInfo<Int32>::bytesPerPixel;
    case Float32:
        return FormatInfo<Float32>::bytesPerPixel;
    case Float64:
        return FormatInfo<Float64>::bytesPerPixel;
    }

    Q_ASSERT(false);
    return 1;
}

/*!
    \brief Check if the pixels have an integral type.
 */
bool Image::hasIntegerPixels() const
{
    switch (_format)
    {
    case Float32:
    case Float64:
        return false;
    case Uint8:
    case Int8:
    case Uint16:
    case Int16:
    case Uint32:
    case Int32:
        return true;
    }

    Q_ASSERT(false);
    return true;
}

/*!
    \brief Check if the pixels have a floating point type.
 */
bool Image::hasFloatPixels() const
{
    switch (_format)
    {
    case Float32:
    case Float64:
        return true;
    case Uint8:
    case Int8:
    case Uint16:
    case Int16:
    case Uint32:
    case Int32:
        return false;
    }

    Q_ASSERT(false);
    return false;
}

/*!
    \brief Get the number of bytes per line.

    \todo Documentation.
 */
int Image::bytesPerLine() const
{
    return _bytesPerLine;
}

/*!
    \brief Get a pointer to the image buffer.

    \returns a pointer to the beginning of the image buffer, i.e. to the
       first pixel of the buffer.

    \note To operate on individual pixels, the scanLine() method should be
       used to respect an abitrary number of bytes per line.

    To access individual pixels using the raw data provided by this method,
    the returned pointer have to be casted to the appropriate pixel type.

    Example for accessing the first pixel of an <tt>Uint16</tt> image:
    \code
    quint16 *pixels = reinterpret_cast<quint16 *>(img.data());
    pixels[0] = 42;
    \endcode
 */
uchar * Image::data()
{
    return _data;
}

/*!
    \brief Get a const pointer to the image buffer.

    \returns a const pointer to the beginning of the image buffer, i.e. to
        the first pixel of the buffer.

    \see data()
 */
const uchar * Image::data() const
{
    return _data;
}

/*!
    \brief Get the total number of bytes used by the image buffer.
 */
int Image::dataSize() const
{
    return _dataSize;
}

/*!
    \brief Get the number of padding bytes.

    \returns the number of bytes succeeding the actual pixel data. This
        is equal to <tt>(dataSize() - bytesPerLine() * height())</tt>.
 */
int Image::numPaddingBytes() const
{
    return _dataSize - _bytesPerLine * _height;
}

/*!
    \brief Get the bit depth of the image.

    \todo Documentation.
 */
int Image::bitDepth() const
{
    return _bitDepth;
}

/*!
    \brief Set the bit depth of the image.

    \todo Documentation.
 */
void Image::setBitDepth(int depth)
{
    int maxDepth = _bytesPerPixel * 8;

    if (hasFloatPixels())
        _bitDepth = maxDepth;
    else
        _bitDepth = (depth < 1 || depth > maxDepth) ? maxDepth : depth;
}

//! \internal
qint64 Image::getIntPixel(int x, int y) const
{
    Q_ASSERT(x >= 0 && x < _width);

    switch (_format)
    {
    case Uint8:
        return scanLine<FormatInfo<Uint8>::type>(y)[x];
    case Int8:
        return scanLine<FormatInfo<Int8>::type>(y)[x];
    case Uint16:
        return scanLine<FormatInfo<Uint16>::type>(y)[x];
    case Int16:
        return scanLine<FormatInfo<Int16>::type>(y)[x];
    case Uint32:
        return scanLine<FormatInfo<Uint32>::type>(y)[x];
    case Int32:
        return scanLine<FormatInfo<Int32>::type>(y)[x];
    case Float32:
    case Float64:
        break;
    }

    Q_ASSERT(false);
    return 0;
}

//! \internal
void Image::setIntPixel(int x, int y, qint64 value)
{
    Q_ASSERT(x >= 0 && x < _width);

    switch (_format)
    {
    case Uint8:
        scanLine<FormatInfo<Uint8>::type>(y)[x] = value;
        return;
    case Int8:
        scanLine<FormatInfo<Int8>::type>(y)[x] = value;
        return;
    case Uint16:
        scanLine<FormatInfo<Uint16>::type>(y)[x] = value;
        return;
    case Int16:
        scanLine<FormatInfo<Int16>::type>(y)[x] = value;
        return;
    case Uint32:
        scanLine<FormatInfo<Uint32>::type>(y)[x] = value;
        return;
    case Int32:
        scanLine<FormatInfo<Int32>::type>(y)[x] = value;
        return;
    case Float32:
    case Float64:
        break;
    }

    Q_ASSERT(false);
}

//! \internal
double Image::getFloatPixel(int x, int y) const
{
    Q_ASSERT(x >= 0 && x < _width);

    switch (_format)
    {
    case Float32:
        return scanLine<FormatInfo<Float32>::type>(y)[x];
    case Float64:
        return scanLine<FormatInfo<Float64>::type>(y)[x];
    case Uint8:
    case Int8:
    case Uint16:
    case Int16:
    case Uint32:
    case Int32:
        break;
    }

    Q_ASSERT(false);
    return 0.0;
}

//! \internal
void Image::setFloatPixel(int x, int y, double value)
{
    Q_ASSERT(x >= 0 && x < _width);

    switch (_format)
    {
    case Float32:
        scanLine<FormatInfo<Float32>::type>(y)[x] = value;
        return;
    case Float64:
        scanLine<FormatInfo<Float64>::type>(y)[x] = value;
        return;
    case Uint8:
    case Int8:
    case Uint16:
    case Int16:
    case Uint32:
    case Int32:
        break;
    }

    Q_ASSERT(false);
}

//! \internal
template <typename T, typename Ta>
void Image::computeMinMax(Ta &minValue, Ta &maxValue) const
{
    Q_ASSERT(_data != 0 && _width != 0 && _height != 0);

    T minv = std::numeric_limits<T>::max();
    T maxv = std::numeric_limits<T>::min();

    for (int i = 0; i < _height; ++i)
    {
        const T *line = scanLine<T>(i);
        for (int j = 0; j < _width; ++j)
        {
            const T v = line[j];
            minv = std::min(v, minv);
            maxv = std::max(v, maxv);
        }
    }

    minValue = Ta(minv);
    maxValue = Ta(maxv);
}

//! \internal
template <typename Ta>
void Image::doGetMinMax(Ta &minValue, Ta &maxValue) const
{
    minValue = 0;
    maxValue = 0;

    if (_data == 0 || _height == 0 || _width == 0)
        return;

    switch (_format)
    {
    case Uint8:
        computeMinMax<FormatInfo<Uint8>::type>(minValue, maxValue);
        return;
    case Int8:
        computeMinMax<FormatInfo<Int8>::type>(minValue, maxValue);
        return;
    case Uint16:
        computeMinMax<FormatInfo<Uint16>::type>(minValue, maxValue);
        return;
    case Int16:
        computeMinMax<FormatInfo<Int16>::type>(minValue, maxValue);
        return;
    case Uint32:
        computeMinMax<FormatInfo<Uint32>::type>(minValue, maxValue);
        return;
    case Int32:
        computeMinMax<FormatInfo<Int32>::type>(minValue, maxValue);
        return;
    case Float32:
        computeMinMax<FormatInfo<Float32>::type>(minValue, maxValue);
        return;
    case Float64:
        computeMinMax<FormatInfo<Float64>::type>(minValue, maxValue);
        return;
    }

    Q_ASSERT(false);
}

/*!
    \brief Compute the mininum and maximum pixel values.

    This method should be used for images with integral pixel data like
    Uint8, Int8, Uint16, ...

    \note This is an expensive operation. You should cache this value
        if you need to use it again.
 */
void Image::getMinMax(qint64 &minValue, qint64 &maxValue) const
{
    doGetMinMax(minValue, maxValue);
}

/*!
    \brief Compute the mininum and maximum pixel values.

    This method should be used for images with floating point pixel data
    like Float32 and Float64.

    \note This is an expensive operation. You should cache this value
        if you need to use it again.
 */
void Image::getMinMax(double &minValue, double &maxValue) const
{
    doGetMinMax(minValue, maxValue);
}

/*
void Image::getMinMax(qint64 &minValue, qint64 &maxValue) const
{
    minValue = 0;
    maxValue = 0;

    if (_data == 0 || _height == 0 || _width == 0)
        return;

    switch (_format)
    {
    case Uint8:
        computeMinMax<FormatInfo<Uint8>::type>(minValue, maxValue);
        return;
    case Int8:
        computeMinMax<FormatInfo<Int8>::type>(minValue, maxValue);
        return;
    case Uint16:
        computeMinMax<FormatInfo<Uint16>::type>(minValue, maxValue);
        return;
    case Int16:
        computeMinMax<FormatInfo<Int16>::type>(minValue, maxValue);
        return;
    case Uint32:
        computeMinMax<FormatInfo<Uint32>::type>(minValue, maxValue);
        return;
    case Int32:
        computeMinMax<FormatInfo<Int32>::type>(minValue, maxValue);
        return;
    case Float32:
        computeMinMax<FormatInfo<Float32>::type>(minValue, maxValue);
        return;
    case Float64:
        computeMinMax<FormatInfo<Float64>::type>(minValue, maxValue);
        return;
    }

    Q_ASSERT(false);
}
*/

//! \internal
template <typename T>
void Image::doMirror(bool horizontal, bool vertical)
{
    Q_ASSERT(_data != 0 && _width != 0 && _height != 0);

    if (horizontal && vertical)
    {
        // mirror along both axes
        for (int i = 0; i < _height / 2; ++i)
        {
            T *line1 = scanLine<T>(i);
            std::reverse_iterator<T*> line2(
                scanLine<T>(_height - 1 - i) + _width);
            std::swap_ranges(line1, line1 + _width, line2);
        }
    }
    else if (horizontal)
    {
        // mirror along the horizontal axis
        for (int i = 0; i < _height / 2; ++i)
        {
            T *line1 = scanLine<T>(i);
            T *line2 = scanLine<T>(_height - 1 - i);
            std::swap_ranges(line1, line1 + _width, line2);
        }
    }
    else if (vertical)
    {
        // mirror along the vertical axis
        for (int i = 0; i < _height; ++i)
        {
            T *line = scanLine<T>(i);
            std::reverse(line, line + _width);
        }
    }
}

/*!
    \brief Mirrors the image along the horizontal and/or the vertical axis.
 */
void Image::mirror(bool horizontal, bool vertical)
{
    if (_data == 0 || _width == 0 || _height == 0)
        return;

    switch (_format)
    {
    case Uint8:
        doMirror<FormatInfo<Uint8>::type>(horizontal, vertical);
        return;
    case Int8:
        doMirror<FormatInfo<Int8>::type>(horizontal, vertical);
        return;
    case Uint16:
        doMirror<FormatInfo<Uint16>::type>(horizontal, vertical);
        return;
    case Int16:
        doMirror<FormatInfo<Int16>::type>(horizontal, vertical);
        return;
    case Uint32:
        doMirror<FormatInfo<Uint32>::type>(horizontal, vertical);
        return;
    case Int32:
        doMirror<FormatInfo<Int32>::type>(horizontal, vertical);
        return;
    case Float32:
        doMirror<FormatInfo<Float32>::type>(horizontal, vertical);
        return;
    case Float64:
        doMirror<FormatInfo<Float64>::type>(horizontal, vertical);
        return;
    }

    Q_ASSERT(false);
}

/*!
    \brief Swaps the bytes of all pixels (Little Endian <-> Big Endian)
 */
void Image::swapBytes()
{
    if (_data == 0 || _width == 0 || _height == 0)
        return;

    if (_bytesPerPixel == 1)
        return;
    else if (_bytesPerPixel == 2)
    {
        const int bpp = 2;
        int bytesWidth = bpp * _width;

        for (int i = 0; i < _height; ++i)
        {
            uchar *line = _data + (_bytesPerLine * i);
            for (int j = 0; j < bytesWidth; j += bpp)
                std::swap(line[j], line[j+1]);
        }
    }
    else if (_bytesPerPixel == 4)
    {
        const int bpp = 4;
        int bytesWidth = bpp * _width;

        for (int i = 0; i < _height; ++i)
        {
            uchar *line = _data + (_bytesPerLine * i);
            for (int j = 0; j < bytesWidth; j += bpp)
            {
                std::swap(line[j], line[j+3]);
                std::swap(line[j+1], line[j+2]);
            }
        }
    }
    else if (_bytesPerPixel == 8)
    {
        const int bpp = 8;
        int bytesWidth = bpp * _width;

        for (int i = 0; i < _height; ++i)
        {
            uchar *line = _data + (_bytesPerLine * i);
            for (int j = 0; j < bytesWidth; j += bpp)
            {
                std::swap(line[j], line[j+7]);
                std::swap(line[j+1], line[j+6]);
                std::swap(line[j+2], line[j+5]);
                std::swap(line[j+3], line[j+4]);
            }
        }
    }
    else
        Q_ASSERT(false);
}

/*!
    \brief Write image to a file.
    \todo Needs implementation.
 */
bool Image::writeToFile(const QString &filename) const
{
    Q_UNUSED(filename);
    return false;
}

/*!
    \brief Read image from a file.
    \todo Needs implementation.
 */
bool Image::readFromFile(const QString &filename)
{
    Q_UNUSED(filename);
    return false;
}

#if 0
bool Image::writeToFile(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDataStream out(&file);
    out << *this;

    return out.status() == QDataStream::Ok;
}

bool Image::readFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&file);
    in >> *this;

    return in.status() == QDataStream::Ok;
}

QDataStream & operator<< (QDataStream &out, const Image &image)
{
    out << quint32(Image::Magic)
        << qint32(image._format)
        << qint32(image._width)
        << qint32(image._height)
        << qint32(image._dataSize)
        << qint32(image._bytesPerLine)
        << qint32(image._bitDepth);

    if (image._dataSize != 0)
    {
        // writing raw pixel endianess
        out << qint32(QSysInfo::ByteOrder);

        // writing raw data
        char *data = reinterpret_cast<char *>(image._data);
        out.writeRawData(data, image._dataSize);
    }

    return out;
}

QDataStream & operator>> (QDataStream &in, Image &image)
{
    image.clear();

    quint32 magic;
    in >> magic;

    if (in.status() != QDataStream::Ok)
        return in;

    if (magic != Image::Magic) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    qint32 format, width, height, dataSize, bytesPerLine, bitDepth;

    in >> format;

    if (in.status() != QDataStream::Ok)
        return in;

    if (format < 0 || format > Image::MaxFormat) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    qint32 bytesPerPixel = 0;
    switch (format)
    {
    case Image::Uint8:
        bytesPerPixel = Image::FormatInfo<Image::Uint8>::bytesPerPixel;
        break;
    case Image::Int8:
        bytesPerPixel = Image::FormatInfo<Image::Int8>::bytesPerPixel;
        break;
    case Image::Uint16:
        bytesPerPixel = Image::FormatInfo<Image::Uint16>::bytesPerPixel;
        break;
    case Image::Int16:
        bytesPerPixel = Image::FormatInfo<Image::Int16>::bytesPerPixel;
        break;
    case Image::Uint32:
        bytesPerPixel = Image::FormatInfo<Image::Uint32>::bytesPerPixel;
        break;
    case Image::Int32:
        bytesPerPixel = Image::FormatInfo<Image::Int32>::bytesPerPixel;
        break;
    }
    Q_ASSERT(bytesPerPixel > 0);

    in >> width >> height >> dataSize >> bytesPerLine >> bitDepth;

    if (in.status() != QDataStream::Ok)
        return in;

    if (width < 0 || height < 0
        || bytesPerLine < width * bytesPerPixel
        || bitDepth < 0 || bitDepth > 32
        || qint64(dataSize) < qint64(bytesPerLine) * qint64(height))
    {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    if (dataSize == 0)
    {
        // no data, so we're done (image is already a null image)
        return in;
    }

    // initialize image
    image.init(width, height, Image::Format(format), bitDepth, bytesPerLine,
        dataSize);

    if (image.isNull()) {
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }
    Q_ASSERT(image._dataSize == dataSize);

    qint32 endian;
    in >> endian;

    if (in.status() != QDataStream::Ok)
        return in;

    if (endian != QSysInfo::BigEndian && endian != QSysInfo::LittleEndian) {
        image.clear();
        in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    char *data = reinterpret_cast<char *>(image._data);
    int bytesRead = in.readRawData(data, image._dataSize);

    if (bytesRead < dataSize) {
        image.clear();
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    if (endian != QSysInfo::ByteOrder)
        image.swapBytes();

    return in;
}
#endif //0

} // namespace CamSys
