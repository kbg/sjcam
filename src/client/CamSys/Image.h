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

#ifndef CAMSYS_IMAGE_H
#define CAMSYS_IMAGE_H

#include <QtCore/qglobal.h>
#include <QtCore/QPoint>
#include <limits>

class QString;
class QDataStream;

namespace CamSys {

//! \addtogroup CamSysImage
//! @{

class Image
{
public:
    enum Format {
        Uint8,    //!< 8 bits per pixel, unsigned value (<tt>quint8</tt>)
        Int8,     //!< 8 bits per pixel, signed value (<tt>qint8</tt>)
        Uint16,   //!< 16 bits per pixel, unsigned value (<tt>quint16</tt>)
        Int16,    //!< 16 bits per pixel, signed value (<tt>qint16</tt>)
        Uint32,   //!< 32 bits per pixel, unsigned value (<tt>qint32</tt>)
        Int32,    //!< 32 bits per pixel, signed value (<tt>qint32</tt>)
        Float32,  //!< 32 bits per pixel, single precision (<tt>float</tt>)
        Float64   //!< 64 bits per pixel, double precision (<tt>double</tt>)
    };

    template <int Format> struct FormatInfo {};
    template <typename T> struct TypeInfo {};

public:
    Image();
    Image(int width, int height, Format format, int bitDepth = 0);
    Image(int width, int height, int bytesPerLine, Format format,
        int bitDepth = 0);
    Image(int width, int height, int bytesPerLine, int dataSize,
        Format format, int bitDepth = 0);
    Image(uchar *data, int width, int height, Format format, int bitDepth = 0);
    Image(uchar *data, int width, int height, int bytesPerLine, Format format,
        int bitDepth = 0);
    Image(uchar *data, int width, int height, int bytesPerLine, int dataSize,
        Format format, int bitDepth = 0);
    explicit Image(const Image &other);
    virtual ~Image();

    Image & operator= (const Image &other);
    virtual Image * clone() const;

public:
    int width() const;
    int height() const;
    Format format() const;
    bool isNull() const;

    void clear();
    void reset(int width, int height, Format format, int bitDepth = 0);
    void reset(int width, int height, int bytesPerLine, Format format,
        int bitDepth = 0);
    void reset(int width, int height, int bytesPerLine, int dataSize,
        Format format, int bitDepth = 0);

    void fill(qint64 value);
    void fill(double value);

    int bytesPerPixel() const;
    static int bytesPerPixel(Format format);

    bool hasIntegerPixels() const;
    bool hasFloatPixels() const;

    template <typename T> T pixel(int x, int y) const;
    template <typename T> T pixel(const QPoint &pos) const;
    template <typename T> void setPixel(int x, int y, T value);
    template <typename T> void setPixel(const QPoint &pos, T value);

    template <typename T> T * scanLine(int i);
    template <typename T> const T * scanLine(int i) const;
    int bytesPerLine() const;

    uchar * data();
    const uchar * data() const;
    int dataSize() const;
    int numPaddingBytes() const;

    int bitDepth() const;
    void setBitDepth(int depth);

    void getMinMax(qint64 &minValue, qint64 &maxValue) const;
    void getMinMax(double &minValue, double &maxValue) const;

    void mirror(bool horizontal = false, bool vertical = true);
    void swapBytes();

    bool writeToFile(const QString &filename) const;
    bool readFromFile(const QString &filename);

#if 0
    /*!
     * \brief Write image to a stream.
     */
    friend QDataStream & operator<< (QDataStream &out, const Image &image);

    /*!
     * \brief Read image from a stream.
     */
    friend QDataStream & operator>> (QDataStream &in, Image &image);
#endif //0

protected:
    enum { MaxFormat = Int32 };
    static const quint32 Magic;

private:
    void init(int width, int height, Format format, int bitDepth,
        int bytesPerLine = 0, int dataSize = 0, uchar *data = 0);

    qint64 getIntPixel(int x, int y) const;
    void setIntPixel(int x, int y, qint64 value);
    double getFloatPixel(int x, int y) const;
    void setFloatPixel(int x, int y, double value);

    template <typename T, typename Ta>
    void computeMinMax(Ta &minValue, Ta &maxValue) const;

    template <typename Ta>
    void doGetMinMax(Ta &minValue, Ta &maxValue) const;

    template <typename T>
    void doMirror(bool horizontal, bool vertical);

    template <typename T>
    void doFill(T value);

private:
    int _width;
    int _height;
    Format _format;
    uchar *_data;
    int _dataSize;
    int _bytesPerLine;
    int _bytesPerPixel;
    int _bitDepth;
    bool _allocated;
};

//! @} // end of group CamSysImage

template <typename T>
inline T Image::pixel(int x, int y) const {
    return hasFloatPixels() ? T(getFloatPixel(x, y)) : T(getIntPixel(x, y));
}

template <typename T>
inline T Image::pixel(const QPoint &pos) const {
    return hasFloatPixels() ? T(getFloatPixel(pos.x(), pos.y())) :
                              T(getIntPixel(pos.x(), pos.y()));
}

template <typename T>
inline void Image::setPixel(int x, int y, T value) {
    if (hasFloatPixels()) setFloatPixel(x, y, value);
    else setIntPixel(x, y, value);
}

template <typename T>
inline void Image::setPixel(const QPoint &pos, T value) {
    if (hasFloatPixels()) setFloatPixel(pos.x(), pos.y(), value);
    else setIntPixel(pos.x(), pos.y(), value);
}

template <typename T>
inline T * Image::scanLine(int i)
{
    Q_ASSERT(i >= 0 && i < _height);
    return reinterpret_cast<T *>(_data + (_bytesPerLine * i));
}

template <typename T>
inline const T * Image::scanLine(int i) const
{
    Q_ASSERT(i >= 0 && i < _height);
    return reinterpret_cast<T *>(_data + (_bytesPerLine * i));
}

//! \cond

template <>
struct Image::FormatInfo<Image::Uint8> {
    typedef quint8 type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::FormatInfo<Image::Int8> {
    typedef qint8 type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::FormatInfo<Image::Uint16> {
    typedef quint16 type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::FormatInfo<Image::Int16> {
    typedef qint16 type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::FormatInfo<Image::Uint32> {
    typedef quint32 type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::FormatInfo<Image::Int32> {
    typedef qint32 type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::FormatInfo<Image::Float32> {
    typedef float type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::FormatInfo<Image::Float64> {
    typedef double type;
    static const int bytesPerPixel = sizeof(type);
};

template <>
struct Image::TypeInfo<quint8> {
    static const Image::Format format = Image::Uint8;
    static const int bytesPerPixel = sizeof(quint8);
};

template <>
struct Image::TypeInfo<qint8> {
    static const Image::Format format = Image::Int8;
    static const int bytesPerPixel = sizeof(qint8);
};

template <>
struct Image::TypeInfo<quint16> {
    static const Image::Format format = Image::Uint16;
    static const int bytesPerPixel = sizeof(quint16);
};

template <>
struct Image::TypeInfo<qint16> {
    static const Image::Format format = Image::Int16;
    static const int bytesPerPixel = sizeof(qint16);
};

template <>
struct Image::TypeInfo<quint32> {
    static const Image::Format format = Image::Uint32;
    static const int bytesPerPixel = sizeof(quint32);
};

template <>
struct Image::TypeInfo<qint32> {
    static const Image::Format format = Image::Int32;
    static const int bytesPerPixel = sizeof(qint32);
};

template <>
struct Image::TypeInfo<float> {
    static const Image::Format format = Image::Float32;
    static const int bytesPerPixel = sizeof(float);
};

template <>
struct Image::TypeInfo<double> {
    static const Image::Format format = Image::Float64;
    static const int bytesPerPixel = sizeof(double);
};

//! \endcond

} // namespace CamSys

#endif // CAMSYS_IMAGE_H
