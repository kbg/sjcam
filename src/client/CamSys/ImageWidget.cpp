/*
 * Copyright (c) 2009-2011 Kolja Glogowski
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

#include "ImageWidget.h"
#include "Image.h"
#include "ColorTable.h"
#include <QtCore/QtDebug>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QMouseEvent>

namespace CamSys {

/*! \class ImageWidget
    \brief A Widget for displaying images with abritrary bit depth.

    \see Image class, for supported pixel formats.

    \todo
    - Implement OpenGL support to speed up drawing for X11
 */

// ---------------------------------------------------------------------------

/*! \internal
    \brief This class contains internal data and implementation for
           ImageWidget.
 */
class ImageWidgetPrivate
{
public:
    ImageWidgetPrivate();

    ImageRenderer renderer;
    ColorTable colorTable;
    QImage renderedImage;
    qreal imageScale;
    bool markerEnabled;
    QPointF markerPos;
    int markerSize;
    QColor markerInnerColor;
    QColor markerOuterColor;
};

ImageWidgetPrivate::ImageWidgetPrivate()
    : imageScale(1.0),
      markerEnabled(false),
      markerPos(0, 0),
      markerSize(5),
      markerInnerColor(Qt::black),
      markerOuterColor(Qt::white)
{
}

// ---------------------------------------------------------------------------

/*! \brief Default constructor.

    Creates an empty image widget.
 */
ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent),
      d_ptr(new ImageWidgetPrivate)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

//! \brief Destructor.
ImageWidget::~ImageWidget()
{
}

/*! \brief Clear the current image.

    This method sets the currenty shown image to an empty image and updates
    the widget.
 */
void ImageWidget::clear()
{
    Q_D(ImageWidget);

    d->renderedImage = QImage();
    resize(0, 0);
    update();
}

/*! \brief Check if there is no image set.

    Returns true, if the current image is empty image.
    \see clear()
 */
bool ImageWidget::isEmpty() const
{
    Q_D(const ImageWidget);
    return d->renderedImage.isNull();
}

/*! \brief Render and display an Image.

    This method renders the given Image using the currently set colorTable and
    colorRange and displays the resulting image.

    The reference to \a image will not be saved, so it may be modified or even
    destroyed after this method has returned.

    \see setColorTable(), setColorRange(), setMinColorValue(),
        setMaxColorValue(), renderedImage(), clear()
 */
void ImageWidget::setImage(const Image *image)
{
    Q_D(ImageWidget);

    if (!image || image->isNull()) {
        clear();
        return;
    }

    bool updateColorTable = d->renderedImage.isNull();

    d->renderer.render(image, d->renderedImage);

    if (updateColorTable)
        d->renderedImage.setColorTable(d->colorTable);

    update();
}

/*! \brief Get the displayed Image as QImage.

    This method returns the currently displayed QImage that was rendered by
    the preceeding setImage() call.

    \see setImage()
 */
QImage ImageWidget::renderedImage() const
{
    Q_D(const ImageWidget);
    return d->renderedImage;
}

double ImageWidget::minColorValue() const
{
    Q_D(const ImageWidget);
    return d->renderer.minColorValue();
}

void ImageWidget::setMinColorValue(double minColorValue)
{
    Q_D(ImageWidget);
    d->renderer.setMinColorValue(minColorValue);
}

double ImageWidget::maxColorValue() const
{
    Q_D(const ImageWidget);
    return d->renderer.maxColorValue();
}

void ImageWidget::setMaxColorValue(double maxColorValue)
{
    Q_D(ImageWidget);
    d->renderer.setMaxColorValue(maxColorValue);
}

void ImageWidget::setColorRange(double minColorValue, double maxColorValue)
{
    Q_D(ImageWidget);
    d->renderer.setColorRange(minColorValue, maxColorValue);
}

ImageRenderer::ColorScaling ImageWidget::colorScaling() const
{
    Q_D(const ImageWidget);
    return d->renderer.colorScaling();
}

void ImageWidget::setColorScaling(ImageRenderer::ColorScaling colorScaling)
{
    Q_D(ImageWidget);
    d->renderer.setColorScaling(colorScaling);
}

/*! \brief Get the widgets color table.

    \see setColorTable(), ColorTable
 */
ColorTable ImageWidget::colorTable() const
{
    Q_D(const ImageWidget);
    return d->colorTable;
}

/*! \brief Sets the color table.

    This method sets the color table which is used for displaying
    the images and updates the widget.

    \see colorTable(), ColorTable
 */
void ImageWidget::setColorTable(const ColorTable &colorTable)
{
    Q_D(ImageWidget);

    d->colorTable = colorTable;
    d->renderedImage.setColorTable(colorTable);
    update();
}

/*! \brief Get the image width in pixels.

    \see imageHeight(), imageSize()
 */
int ImageWidget::imageWidth() const
{
    Q_D(const ImageWidget);
    return d->renderedImage.width();
}

/*! \brief Get the image height in pixels.

    \see imageWidth(), imageSize()
 */
int ImageWidget::imageHeight() const
{
    Q_D(const ImageWidget);
    return d->renderedImage.height();
}

/*! \brief Get the image width and height in pixels.

    \see imageWidth(), imageHeight()
 */
QSize ImageWidget::imageSize() const
{
    Q_D(const ImageWidget);
    return d->renderedImage.size();
}

/*! \brief Get the current scale factor of the image.

    \see zoom()
 */
qreal ImageWidget::imageScale() const
{
    Q_D(const ImageWidget);
    return d->imageScale;
}

QPoint ImageWidget::mapToImage(const QPoint &pos) const
{
    Q_D(const ImageWidget);

    qreal scale = d->imageScale;
    if (scale <= 0.0)
        return QPoint();

    QPoint rp = pos;
    ImageRenderer::ImageFlips flips = d->renderer.imageFlips();
    if (flips.testFlag(ImageRenderer::HorizontalFlip))
        rp.setX(width() - rp.x() - 1);
    if (flips.testFlag(ImageRenderer::VerticalFlip))
        rp.setY(height() - rp.y() - 1);

    // don't use the '/' operator because it rounds the result
    return QPoint(rp.x() / scale, rp.y() / scale);
}

QPoint ImageWidget::mapFromImage(const QPoint &pos) const
{
    Q_D(const ImageWidget);

    qreal scale = d->imageScale;
    if (scale <= 0.0)
        return QPoint();

    // don't use the '/' operator because it rounds the result
    QPoint rp(pos.x() * scale, pos.y() * scale);
    ImageRenderer::ImageFlips flips = d->renderer.imageFlips();
    if (flips.testFlag(ImageRenderer::HorizontalFlip))
        rp.setX(width() - rp.x() - 1);
    if (flips.testFlag(ImageRenderer::VerticalFlip))
        rp.setY(height() - rp.y() - 1);
    return rp;
}

bool ImageWidget::isMarkerEnabled() const
{
    Q_D(const ImageWidget);
    return d->markerEnabled;
}

void ImageWidget::setMarkerEnabled(bool enable)
{
    Q_D(ImageWidget);
    d->markerEnabled = enable;
}

QPointF ImageWidget::markerPos() const
{
    Q_D(const ImageWidget);
    return d->markerPos;
}

void ImageWidget::setMarkerPos(const QPointF &markerPos)
{
    Q_D(ImageWidget);
    d->markerPos = markerPos;
}

int ImageWidget::markerSize() const
{
    Q_D(const ImageWidget);
    return d->markerSize;
}

void ImageWidget::setMarkerSize(int markerSize)
{
    Q_D(ImageWidget);
    d->markerSize = markerSize > 0 ? markerSize : 0;
}

QColor ImageWidget::markerInnerColor() const
{
    Q_D(const ImageWidget);
    return d->markerInnerColor;
}

void ImageWidget::setMarkerInnerColor(const QColor &color)
{
    Q_D(ImageWidget);
    d->markerInnerColor = color;
}

QColor ImageWidget::markerOuterColor() const
{
    Q_D(const ImageWidget);
    return d->markerOuterColor;
}

void ImageWidget::setMarkerOuterColor(const QColor &color)
{
    Q_D(ImageWidget);
    d->markerOuterColor = color;
}

/*! \brief Scale the displayed image.

    \see imageScale(), zoomIn(), zoomOut(), zoomBestFit()
 */
void ImageWidget::zoom(qreal scale)
{
    Q_D(ImageWidget);
    d->imageScale = scale;

    resize(imageSize() * scale);
    update();

    emit zoomed(scale);
}

/*! \brief Zoom in by the given scale factor.

    \see zoomOut(), zoom()
 */
void ImageWidget::zoomIn(qreal scaleChange)
{
    Q_D(ImageWidget);

    if (scaleChange <= 0.0)
        return;

    d->imageScale *= 1.0 + scaleChange;

    resize(imageSize() * d->imageScale);
    update();

    emit zoomed(d->imageScale);
}

/*! \brief Zoom out by the given scale factor.

    \see zoomIn(), zoom()
 */
void ImageWidget::zoomOut(qreal scaleChange)
{
    Q_D(ImageWidget);

    if (scaleChange <= 0.0)
        return;

    d->imageScale /= 1.0 + scaleChange;

    resize(imageSize() * d->imageScale);
    update();

    emit zoomed(d->imageScale);
}

/*! \brief Scale the displayed image to the rectangle \a rect by
        conserving the aspect ratio.

    \see zoom()
 */
void ImageWidget::zoomBestFit(const QRect &rect)
{
    // real image size
    QSize sz = imageSize();

    if (sz.width() != 0 && sz.height() != 0 && rect.isValid())
    {
        // ratio between image width/height and area width/height
        qreal fx = qreal(rect.width()) / qreal(sz.width());
        qreal fy = qreal(rect.height()) / qreal(sz.height());

        // use scale factor of the bigger side
        zoom(qMin(fx, fy));
    }
}

void ImageWidget::flipHorizontal(bool flip)
{
    Q_D(ImageWidget);
    ImageRenderer::ImageFlips flips = d->renderer.imageFlips();
    if (flips.testFlag(ImageRenderer::HorizontalFlip) != flip) {
        d->renderer.setImageFlips(flips ^ ImageRenderer::HorizontalFlip);
        d->renderedImage = d->renderedImage.mirrored(true, false);
        update();
    }
}

void ImageWidget::flipVertical(bool flip)
{
    Q_D(ImageWidget);
    ImageRenderer::ImageFlips flips = d->renderer.imageFlips();
    if (flips.testFlag(ImageRenderer::VerticalFlip) != flip) {
        d->renderer.setImageFlips(flips ^ ImageRenderer::VerticalFlip);
        d->renderedImage = d->renderedImage.mirrored(false, true);
        update();
    }
}

void ImageWidget::paintEvent(QPaintEvent *event)
{
    Q_D(ImageWidget);

    QSizeF is = d->renderedImage.size();
    QSizeF ws = size();
    QRectF er = event->rect();

    if (is.isEmpty() || ws.isEmpty() || er.isEmpty())
        return;

    qreal sx = is.width() / ws.width();
    qreal sy = is.height() / ws.height();

    QRectF r(sx * er.x(), sy * er.y(),
        sx * er.width(), sy * er.height());

    //qDebug() << "paintEvent:" << er << ws << is << sx << sy << " -> " << r;

    QPainter painter(this);
    painter.drawImage(er, d->renderedImage, r);

    // draw marker
    if (!d->markerEnabled)
        return;

    QPointF markerPos = d->markerPos;
    //QPointF markerPos(0.5 * (is.width()-1), 0.5 * (is.height()-1));
    markerPos += QPointF(0.5, 0.5);

    const int cx = int(markerPos.x() / sx);
    const int cy = int(markerPos.y() / sy);
    const int markerSize = d->markerSize;

    painter.setPen(d->markerOuterColor);
    painter.drawRect(cx-markerSize-1, cy-1, 2*markerSize+2, 2);
    painter.drawRect(cx-1, cy-markerSize-1, 2, 2*markerSize+2);
    painter.setPen(d->markerInnerColor);
    painter.drawLine(cx-markerSize, cy, cx+markerSize, cy);
    painter.drawLine(cx, cy-markerSize, cx, cy+markerSize);
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    emit mouseMovedTo(event->pos());
}

void ImageWidget::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
    if (event->type() == QEvent::Enter)
        emit mouseEntered();
}

void ImageWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (event->type() == QEvent::Leave)
        emit mouseLeft();
}

} // namespace CamSys
