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

#ifndef CAMSYS_IMAGE_RENDERER_H
#define CAMSYS_IMAGE_RENDERER_H

#include <QtCore/qglobal.h>

class QImage;

namespace CamSys {

class Image;
class ColorTable;
class ImageRendererPrivate;

//! \addtogroup CamSysGui
//! @{

class ImageRenderer
{
public:
    enum ColorScaling {
        Linear,
        Logarithmic,
        SquareRoot,
        Squared
    };

    enum ImageFlip {
        NoFlip         = 0x00,
        HorizontalFlip = 0x01,
        VerticalFlip   = 0x02
    };
    Q_DECLARE_FLAGS(ImageFlips, ImageFlip)
    
public:
    explicit ImageRenderer(ColorScaling colorScaling = Linear);
    virtual ~ImageRenderer();

    ColorScaling colorScaling() const;
    void setColorScaling(ColorScaling colorScaling);

    void setColorRange(double minColorValue, double maxColorValue);

    double minColorValue() const;
    void setMinColorValue(double minColorValue);

    double maxColorValue() const;
    void setMaxColorValue(double maxColorValue);

    void setImageFlips(ImageFlips flips);
    ImageFlips imageFlips() const;

    QImage render(const Image *image, const ColorTable &colorTable) const;
    void render(const Image *image, QImage &renderedImage) const;

private:
    ImageRendererPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ImageRenderer)
    Q_DISABLE_COPY(ImageRenderer)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ImageRenderer::ImageFlips)

//! @} // end of group CamSysGui

} // namespace CamSys

#endif // CAMSYS_IMAGE_RENDERER_H
