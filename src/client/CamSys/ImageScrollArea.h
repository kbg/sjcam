/*
 * Copyright (c) 2011 Kolja Glogowski
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

#ifndef CAMSYS_IMAGE_SCROLL_AREA_H
#define CAMSYS_IMAGE_SCROLL_AREA_H

#include <QScrollArea>

class QEvent;
class QMouseEvent;

namespace CamSys {

class ImageWidget;
class ImageScrollAreaPrivate;

//! \addtogroup CamSysGui
//! @{

class ImageScrollArea : public QScrollArea
{
    Q_OBJECT
    Q_PROPERTY(bool cursorLines READ hasCursorLines WRITE setCursorLines)

public:
    explicit ImageScrollArea(QWidget *parent = 0);
    virtual ~ImageScrollArea();

    void setWidget(QWidget *widget);
    void setImageWidget(ImageWidget *imageWidget);
    ImageWidget * imageWidget();
    const ImageWidget * imageWidget() const;

    bool hasCursorLines() const;

public slots:
    void zoomIn();
    void zoomOut();
    void zoomNormalSize();
    void zoomBestFit();

    void showCursorLines();
    void hideCursorLines();
    void setCursorLines(bool enable);
    void setCursorLineSize(int lineSize);
    void setCursorLineColor(const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    bool viewportEvent(QEvent *event);

private:
    ImageScrollAreaPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ImageScrollArea)
    Q_DISABLE_COPY(ImageScrollArea)
};

//! @} // end of group CamSysGui

} // namespace CamSys

#endif // CAMSYS_IMAGE_SCROLL_AREA_H
