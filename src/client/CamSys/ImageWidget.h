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

#ifndef CAMSYS_IMAGE_WIDGET_H
#define CAMSYS_IMAGE_WIDGET_H

#include "ImageRenderer.h"
#include <QtGui/QWidget>

namespace CamSys {

class Image;
class ColorTable;
class ImageWidgetPrivate;

//! \addtogroup CamSysGui
//! @{

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageWidget(QWidget *parent = 0);
    virtual ~ImageWidget();

    void clear();
    bool isEmpty() const;
    void setImage(const Image *image);
    QImage renderedImage() const;

    double minColorValue() const;
    void setMinColorValue(double minColorValue);
    double maxColorValue() const;
    void setMaxColorValue(double maxColorValue);
    void setColorRange(double minColorValue, double maxColorValue);

    ImageRenderer::ColorScaling colorScaling() const;
    void setColorScaling(ImageRenderer::ColorScaling colorScaling);

    ColorTable colorTable() const;
    void setColorTable(const ColorTable &colorTable);

    int imageWidth() const;
    int imageHeight() const;
    QSize imageSize() const;
    qreal imageScale() const;

    QPoint mapToImage(const QPoint &pos) const;
    QPoint mapFromImage(const QPoint &pos) const;

    bool isMarkerEnabled() const;
    void setMarkerEnabled(bool enable = true);
    QPointF markerPos() const;
    void setMarkerPos(const QPointF &markerPos);
    int markerSize() const;
    void setMarkerSize(int markerSize);
    QColor markerInnerColor() const;
    void setMarkerInnerColor(const QColor &color);
    QColor markerOuterColor() const;
    void setMarkerOuterColor(const QColor &color);

public slots:
    void zoom(qreal scale = 1.0);
    void zoomIn(qreal scaleChange = 1.0 / 3.0);
    void zoomOut(qreal scaleChange = 1.0 / 3.0);
    void zoomBestFit(const QRect &rect);
    void flipHorizontal(bool flip);
    void flipVertical(bool flip);

signals:
    void zoomed(double scale);
    void mouseMovedTo(const QPoint &pos);
    void mouseEntered();
    void mouseLeft();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);

private:
    ImageWidgetPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ImageWidget)
    Q_DISABLE_COPY(ImageWidget)
};

//! @} // end of group CamSysGui

} // namespace CamSys

#endif // CAMSYS_IMAGE_WIDGET_H
