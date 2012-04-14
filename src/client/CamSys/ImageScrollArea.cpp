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

#include "ImageScrollArea.h"
#include "ImageWidget.h"

#include <QtDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QRubberBand>
#include <QStyle>
#include <QScrollBar>
#include <QColor>
#include <QCursor>

namespace CamSys {

// ---------------------------------------------------------------------------

/*! \internal
    \brief This class contains internal data and implementation for
           ImageScrollArea.
 */
class ImageScrollAreaPrivate
{
public:
    ImageScrollAreaPrivate();
    void initCursorLines(QWidget *parent);
    void updateCursorLinePos(const QRect &rect, const QPoint &pos);
    void setCursorLineColor(const QColor &color);

    bool cursorLinesVisible;
    int cursorLineSize;
    QColor cursorLineColor;
    QRubberBand *horizontalLine;
    QRubberBand *verticalLine;
    bool insideViewArea;
    QPoint clickPos;
    QPoint clickScrollBarValues;
    QCursor clickOldCursor;
};

ImageScrollAreaPrivate::ImageScrollAreaPrivate()
    : cursorLinesVisible(false),
      cursorLineSize(0),
      horizontalLine(0),
      verticalLine(0),
      insideViewArea(false),
      clickPos(0, 0),
      clickScrollBarValues(0, 0)
{
}

void ImageScrollAreaPrivate::initCursorLines(QWidget *parent)
{
    if (!horizontalLine) {
        horizontalLine = new QRubberBand(QRubberBand::Line, parent);
        horizontalLine->setGeometry(0, 0, 0, 0);
    }
    if (!verticalLine) {
        verticalLine = new QRubberBand(QRubberBand::Line, parent);
        verticalLine->setGeometry(0, 0, 0, 0);
    }
    if (cursorLineColor.isValid())
        setCursorLineColor(cursorLineColor);
}

void ImageScrollAreaPrivate::updateCursorLinePos(const QRect &rect,
                                                 const QPoint &pos)
{
    int lineWidth = 2 * cursorLineSize + 1;
    horizontalLine->setGeometry(
        0, pos.y() - cursorLineSize, rect.width(), lineWidth);
    verticalLine->setGeometry(
        pos.x() - cursorLineSize, 0, lineWidth, rect.height());
}

void ImageScrollAreaPrivate::setCursorLineColor(const QColor &color)
{
    QPalette palette;
    palette.setBrush(QPalette::Highlight, QBrush(color));
    palette.setBrush(QPalette::Base, QBrush(color));
    if (horizontalLine)
        horizontalLine->setPalette(palette);
    if (verticalLine)
        verticalLine->setPalette(palette);
    cursorLineColor = color;
}

// ---------------------------------------------------------------------------

ImageScrollArea::ImageScrollArea(QWidget *parent)
    : QScrollArea(parent),
      d_ptr(new ImageScrollAreaPrivate)
{
}

ImageScrollArea::~ImageScrollArea()
{
    delete d_ptr;
}

void ImageScrollArea::setWidget(QWidget *widget)
{
    Q_D(ImageScrollArea);

    if (d->cursorLinesVisible)
        widget->setMouseTracking(true);

    QScrollArea::setWidget(widget);
}

void ImageScrollArea::setImageWidget(ImageWidget *imageWidget)
{
    setWidget(imageWidget);
}

ImageWidget * ImageScrollArea::imageWidget()
{
    return qobject_cast<ImageWidget *>(widget());
}

const ImageWidget * ImageScrollArea::imageWidget() const
{
    return qobject_cast<const ImageWidget *>(widget());
}

bool ImageScrollArea::hasCursorLines() const
{
    Q_D(const ImageScrollArea);
    return d->cursorLinesVisible;
}

void ImageScrollArea::setCursorLines(bool enable)
{
    Q_D(ImageScrollArea);

    if (!d->cursorLinesVisible)
    {
        d->initCursorLines(viewport());
        setMouseTracking(true);
        if (widget())
            widget()->setMouseTracking(true);
    }

    QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
    d->updateCursorLinePos(viewport()->rect(), pos);

    if (enable) {
        if (d->insideViewArea) {
            d->horizontalLine->show();
            d->verticalLine->show();
        }
    } else {
        d->horizontalLine->hide();
        d->verticalLine->hide();
    }

    d->cursorLinesVisible = enable;
}

/*! \todo Update lines, if they are visible. */
void ImageScrollArea::setCursorLineSize(int lineSize)
{
    Q_D(ImageScrollArea);
    if (lineSize >= 0)
        d->cursorLineSize = lineSize;
}

/*! \todo Update lines, if they are visible. */
void ImageScrollArea::setCursorLineColor(const QColor &color)
{
    Q_D(ImageScrollArea);
    if (color.isValid())
        d->setCursorLineColor(color);
}

void ImageScrollArea::zoomIn()
{
    QWidget *vp = viewport();
    ImageWidget *iw = imageWidget();

    if (!vp || !iw)
        return;

    QPointF center = iw->mapFrom(vp, vp->rect().center());
    center /= iw->imageScale();

    iw->zoomIn();

    center *= iw->imageScale();
    ensureVisible(qRound(center.x()), qRound(center.y()),
                  vp->width() / 2, vp->height() / 2);
}

void ImageScrollArea::zoomOut()
{
    QWidget *vp = viewport();
    ImageWidget *iw = imageWidget();

    if (!vp || !iw)
        return;

    QPointF center = iw->mapFrom(vp, vp->rect().center());
    center /= iw->imageScale();

    iw->zoomOut();

    center *= iw->imageScale();
    ensureVisible(qRound(center.x()), qRound(center.y()),
                  vp->width() / 2, vp->height() / 2);
}

void ImageScrollArea::zoomNormalSize()
{
    QWidget *vp = viewport();
    ImageWidget *iw = imageWidget();

    if (!vp || !iw || iw->imageScale() == 1.0)
        return;

    QPointF center = iw->mapFrom(vp, vp->rect().center());
    center /= iw->imageScale();

    iw->zoom(1.0);

    center *= iw->imageScale();
    ensureVisible(qRound(center.x()), qRound(center.y()),
                  vp->width() / 2, vp->height() / 2);
}

void ImageScrollArea::zoomBestFit()
{
    QWidget *vp = viewport();
    ImageWidget *iw = imageWidget();

    if (!vp || !iw)
        return;

    QRect rect = vp->rect();
    int barExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents))
        barExtent += style()->pixelMetric(
                QStyle::PM_ScrollView_ScrollBarSpacing);

    if (horizontalScrollBar()->isVisible())
        rect.setWidth(rect.width() + barExtent);
    if (verticalScrollBar()->isVisible())
        rect.setHeight(rect.height() + barExtent);

    iw->zoomBestFit(rect);
}

void ImageScrollArea::showCursorLines()
{
    setCursorLines(true);
}

void ImageScrollArea::hideCursorLines()
{
    setCursorLines(false);
}

void ImageScrollArea::mousePressEvent(QMouseEvent *event)
{
    Q_D(ImageScrollArea);

    if (event->button() == Qt::LeftButton) {
        d->clickPos = event->pos();
        d->clickScrollBarValues = QPoint(
            horizontalScrollBar() ? horizontalScrollBar()->value() : 0,
            verticalScrollBar() ? verticalScrollBar()->value() : 0);
        if (imageWidget()) {
            d->clickOldCursor = imageWidget()->cursor();
            imageWidget()->setCursor(Qt::ClosedHandCursor);
        }
    }

    QScrollArea::mousePressEvent(event);
}

void ImageScrollArea::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(ImageScrollArea);

    if (imageWidget())
        imageWidget()->setCursor(d->clickOldCursor);

    QScrollArea::mouseReleaseEvent(event);
}

void ImageScrollArea::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(ImageScrollArea);

    if (d->cursorLinesVisible) {
        QRect rect = viewport()->rect();
        QPoint pos = event->pos();
        d->updateCursorLinePos(rect, pos);
    }

    if (event->buttons() & Qt::LeftButton) {
        QPoint diff = d->clickPos - event->pos();
        QPoint newScrollPos = d->clickScrollBarValues + diff;
        if (horizontalScrollBar())
            horizontalScrollBar()->setValue(newScrollPos.x());
        if (verticalScrollBar())
            verticalScrollBar()->setValue(newScrollPos.y());
    }
    else
        QScrollArea::mouseMoveEvent(event);
}

/*! \todo Implement zoom support for mice with higher resolution wheels. */
void ImageScrollArea::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() != Qt::ControlModifier) {
        QScrollArea::wheelEvent(event);
        return;
    }

    // currently only wheel events with |delta| >= 120 are supported.
    int n = event->delta() / 120;

    if (n < 0) {
        for (int i = 0; i < -n; ++i)
            zoomOut();
    } else {
        for (int i = 0; i < n; ++i)
            zoomIn();
    }

    event->accept();
}

bool ImageScrollArea::viewportEvent(QEvent *event)
{
    Q_D(ImageScrollArea);

    switch (event->type())
    {
    case QEvent::Enter:
        d->insideViewArea = true;
        if (d->cursorLinesVisible) {
            d->horizontalLine->show();
            d->verticalLine->show();
        }
        break;
    case QEvent::Leave:
        d->insideViewArea = false;
        if (d->cursorLinesVisible) {
            d->horizontalLine->hide();
            d->verticalLine->hide();
        }
        break;
    default:
        break;
    }

    return QScrollArea::viewportEvent(event);
}

} // namespace CamSys
