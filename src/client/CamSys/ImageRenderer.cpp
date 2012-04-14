/*
 * Copyright (c) 2009 Kolja Glogowski
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

#include <QtDebug>
#include <QtGui>
#include <limits>
#include <cmath>

#include "Image.h"
#include "ColorTable.h"
#include "ImageRenderer.h"

namespace CamSys {

/*! \class ImageRenderer
    \brief An image rendering class.

    This class provides a way to make Image objects with a depth of more than
    8 bits displayable by rendering it to a 8 bit QImage using a ColorTable.

    The renderer supports different color scaling modes and arbitrary min and
    max colors.

    \see Image, ColorScaling
 */

/*! \enum ImageRenderer::ColorScaling
    \brief Color scaling used by the image renderer.

    \note The \a BitDepth and \a FullRange scaling mode is only available for
          Image objects with integer pixel type. Setting this value for
          floating point images has no effect and the MinMax scaling is
          used instead.

    \see setColorScaling(), colorScaling()
 */

/*! \var ImageRenderer::ColorScaling ImageRenderer::MinMax
    \brief Use \a minColorValue and \a maxColorValue for color scaling.
 */

/*! \var ImageRenderer::ColorScaling ImageRenderer::BitDepth
    \brief Use Image::bitDepth() for color scaling (not implemented yet).
 */

/*! \var ImageRenderer::ColorScaling ImageRenderer::FullRange
    \brief Use the full bit range of the image for color scaling (not
           implement yet).
 */

// ---------------------------------------------------------------------------

/*! \internal
    \brief This class contains internal data and implementation for
           ImageRenderer.
 */
class ImageRendererPrivate
{
public:
    ImageRendererPrivate(ImageRenderer::ColorScaling colorScaling);

    template <typename T> inline T intMin() const;
    template <typename T> inline T intMax() const;

    template <typename T>
    void doRender(const Image *image, QImage &renderedImage,
                  T minv, T maxv) const;

    void renderMinMax(const Image *image, QImage &renderedImage) const;

    ImageRenderer::ColorScaling colorScaling;
    double minValue;
    double maxValue;
    ImageRenderer::ImageFlips flips;
};

ImageRendererPrivate::ImageRendererPrivate(
        ImageRenderer::ColorScaling colorScaling)
    : colorScaling(colorScaling),
      minValue(0.0),
      maxValue(0.0),
      flips(ImageRenderer::NoFlip)
{
}

template <typename T>
inline T ImageRendererPrivate::intMin() const
{
    const T typeMin = std::numeric_limits<T>::min();
    const T typeMax = std::numeric_limits<T>::max();

    if (minValue >= typeMax)
        return typeMax;
    else if (minValue <= typeMin)
        return typeMin;
    else
        return qRound64(minValue);
}

template <typename T>
inline T ImageRendererPrivate::intMax() const
{
    const T typeMin = std::numeric_limits<T>::min();
    const T typeMax = std::numeric_limits<T>::max();

    if (maxValue <= typeMin)
        return typeMin;
    else if (maxValue >= typeMax)
        return typeMax;
    else
        return qRound64(maxValue);
}

template <typename T>
void ImageRendererPrivate::doRender(const Image *image,
                                    QImage &renderedImage,
                                    T minv, T maxv) const
{
    Q_ASSERT(image);
    Q_ASSERT(image->width() == renderedImage.width());
    Q_ASSERT(image->height() == renderedImage.height());
    Q_ASSERT(renderedImage.format() == QImage::Format_Indexed8);
    Q_ASSERT(renderedImage.numColors() == 256);
    Q_ASSERT(minValue < maxValue);

    const int width = image->width();
    const int height = image->height();

    const bool hflip = flips.testFlag(ImageRenderer::HorizontalFlip);
    const bool vflip = flips.testFlag(ImageRenderer::VerticalFlip);

    // \todo implementation without code duplication
    if (colorScaling == ImageRenderer::Linear)
    {
        const float scale = 255.0f / float(maxv - minv);
        for (int j = 0; j < height; ++j)
        {
            const T * const src = image->scanLine<T>(j);
            uchar * const dest = renderedImage.scanLine(
                        vflip ? (height - 1 - j) : j);

            for (int i = 0; i < width; ++i) {
                T v = src[i];
                if (v < minv) v = minv;
                else if (v > maxv) v = maxv;
                dest[hflip ? (width-1-i) : i] = uchar((v - minv) * scale);
            }
        }
    }
    else if (colorScaling == ImageRenderer::Logarithmic)
    {
        const float scale = 255.0f / std::log(float(maxv - minv));
        for (int j = 0; j < height; ++j)
        {
            const T * const src = image->scanLine<T>(j);
            uchar * const dest = renderedImage.scanLine(
                        vflip ? (height - 1 - j) : j);

            for (int i = 0; i < width; ++i) {
                T v = src[i];
                if (v < minv) v = minv;
                else if (v > maxv) v = maxv;
                dest[hflip ? (width-1-i) : i] = uchar(std::log(v-minv) * scale);
            }
        }
    }
    else if (colorScaling == ImageRenderer::SquareRoot)
    {
        const float scale = 255.0f / std::sqrt(float(maxv - minv));
        for (int j = 0; j < height; ++j)
        {
            const T * const src = image->scanLine<T>(j);
            uchar * const dest = renderedImage.scanLine(
                        vflip ? (height - 1 - j) : j);

            for (int i = 0; i < width; ++i) {
                T v = src[i];
                if (v < minv) v = minv;
                else if (v > maxv) v = maxv;
                dest[hflip ? (width-1-i) : i] = uchar(std::sqrt(v-minv) * scale);
            }
        }
    }
    else if (colorScaling == ImageRenderer::Squared)
    {
        const float scale = 255.0f / float(maxv - minv) / float(maxv - minv);
        for (int j = 0; j < height; ++j)
        {
            const T * const src = image->scanLine<T>(j);
            uchar * const dest = renderedImage.scanLine(
                        vflip ? (height - 1 - j) : j);

            for (int i = 0; i < width; ++i) {
                T v = src[i];
                if (v < minv) v = minv;
                else if (v > maxv) v = maxv;
                dest[hflip ? (width-1-i) : i] = uchar((v-minv) * (v-minv) * scale);
            }
        }
    }
    else
        Q_ASSERT(false);
}

void ImageRendererPrivate::renderMinMax(
        const Image *image,
        QImage &renderedImage) const
{
    Q_ASSERT(image);
    Q_ASSERT(image->width() == renderedImage.width());
    Q_ASSERT(image->height() == renderedImage.height());
    Q_ASSERT(renderedImage.format() == QImage::Format_Indexed8);
    Q_ASSERT(renderedImage.numColors() == 256);

    if (minValue == maxValue) {
        renderedImage.fill(0);
        return;
    }

    switch (image->format())
    {
    case Image::Uint8:
        doRender<quint8>(image, renderedImage,
                         intMin<quint8>(), intMax<quint8>());
        break;
    case Image::Int8:
        doRender<qint8>(image, renderedImage,
                        intMin<qint8>(), intMax<qint8>());
        break;
    case Image::Uint16:
        doRender<quint16>(image, renderedImage,
                          intMin<quint16>(), intMax<quint16>());
        break;
    case Image::Int16:
        doRender<qint16>(image, renderedImage,
                         intMin<qint16>(), intMax<qint16>());
        break;
    case Image::Uint32:
        doRender<quint32>(image, renderedImage,
                          intMin<quint32>(), intMax<quint32>());
        break;
    case Image::Int32:
        doRender<qint32>(image, renderedImage,
                         intMin<qint32>(), intMax<qint32>());
        break;
    case Image::Float32:
        doRender<float>(image, renderedImage, minValue, maxValue);
        break;
    case Image::Float64:
        doRender<double>(image, renderedImage, minValue, maxValue);
        break;
    }
}

// ---------------------------------------------------------------------------

/*! \brief Default constructor.

    \see setColorScaling()
 */
ImageRenderer::ImageRenderer(ColorScaling colorScaling)
    : d_ptr(new ImageRendererPrivate(colorScaling))
{
}

/*! \brief Destructor. */
ImageRenderer::~ImageRenderer()
{
    delete d_ptr;
}

/*! \brief Get the color scaling mode.

    \see ColorScaling, setColorScaling()
 */
ImageRenderer::ColorScaling ImageRenderer::colorScaling() const
{
    Q_D(const ImageRenderer);
    return d->colorScaling;
}

/*! \brief Set the color scaling mode.

    \see ColorScaling, colorScaling(), setColorRange()
 */
void ImageRenderer::setColorScaling(ColorScaling colorScaling)
{
    Q_D(ImageRenderer);
    d->colorScaling = colorScaling;
}

/*! \brief Set the color range.

    Sets the color range for the MinMax color scaling mode.

    These values are used for color scaling when color scaling mode is set to
    MinMax.

    \see setMinColorValue(), setMaxColorValue(), ColorScaling.
 */
void ImageRenderer::setColorRange(double minColorValue, double maxColorValue)
{
    Q_D(ImageRenderer);

    Q_ASSERT(minColorValue <= maxColorValue);
    if (minColorValue <= maxColorValue)
    {
        d->minValue = minColorValue;
        d->maxValue = maxColorValue;
    }
}

/*! \brief Get the minimum color value.

    Returns the currently set minColorValue.

    \see maxColorValue(), setMinColorValue()
 */
double ImageRenderer::minColorValue() const
{
    Q_D(const ImageRenderer);
    return d->minValue;
}

/*! \brief Set the minimum color value.

    Sets the minimum color value for the MinMax color scaling mode.

    \see setMaxColorValue(), setColorRange(), minColorValue()
 */
void ImageRenderer::setMinColorValue(double minColorValue)
{
    Q_D(ImageRenderer);

    Q_ASSERT(minColorValue <= d->maxValue);
    if (minColorValue <= d->maxValue)
        d->minValue = minColorValue;
}

/*! \brief Get the maximum color value.

    Returns the currently set maxColorValue.

    \see setMaxColorValue(), minColorValue()
 */
double ImageRenderer::maxColorValue() const
{
    Q_D(const ImageRenderer);
    return d->maxValue;
}

/*! \brief Set the maximum color value.

    Sets the maximum color value for the MinMax color scaling mode.

    \see setMinColorValue(), setColorRange(), maxColorValue()
 */
void ImageRenderer::setMaxColorValue(double maxColorValue)
{
    Q_D(ImageRenderer);

    Q_ASSERT(maxColorValue >= d->minValue);
    if (maxColorValue >= d->minValue)
        d->maxValue = maxColorValue;
}

/*! \brief Select flip directions.

    Flips the rendered image in horizontal and/or vertical direction.

    \see ImageFlip, imageFlips()
 */
void ImageRenderer::setImageFlips(ImageFlips flips)
{
    Q_D(ImageRenderer);
    d->flips = flips;
}

/*! \brief Get the selected flip directions.

    \see ImageFlip, setImageFlips()
 */
ImageRenderer::ImageFlips ImageRenderer::imageFlips() const
{
    Q_D(const ImageRenderer);
    return d->flips;
}

/*! \brief Render an Image to a new QImage.

    This method renders the supplied Image object to a new QImage using the
    provided colorTable.

    \see setColorScaling(), setColorRange()
 */
QImage ImageRenderer::render(const Image *image,
                             const ColorTable &colorTable) const
{
    Q_D(const ImageRenderer);

    if (!image || image->isNull())
        return QImage();

    QImage renderedImage(image->width(), image->height(),
                         QImage::Format_Indexed8);
    renderedImage.setColorTable(colorTable);

    d->renderMinMax(image, renderedImage);
    return renderedImage;
}

/*! \brief Render an Image to an already existing QImage.

    This method renders the supplied Image object to an existing QImage using
    the color table of that QImage object.

    \see setColorScaling(), setColorRange()
 */
void ImageRenderer::render(const Image *image, QImage &renderedImage) const
{
    Q_D(const ImageRenderer);

    if (!image || image->isNull()) {
        renderedImage = QImage();
        return;
    }

    int width = image->width();
    int height = image->height();

    // make compatible target image
    if (renderedImage.format() != QImage::Format_Indexed8
        || renderedImage.width() != width
        || renderedImage.height() != height)
    {
        ColorTable colorTable = renderedImage.colorTable();
        renderedImage = QImage(width, height, QImage::Format_Indexed8);
        renderedImage.setColorTable(colorTable);
    }
    else if (renderedImage.numColors() != 256)
        renderedImage.setNumColors(256);

    d->renderMinMax(image, renderedImage);
}

} // namespace CamSys
