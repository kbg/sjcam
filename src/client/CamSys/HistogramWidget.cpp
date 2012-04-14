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

#include <QtDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QRubberBand>
#include <cmath>

#include "Histogram.h"
#include "ColorTable.h"
#include "HistogramWidget.h"
#include "ColorRange.h"

namespace CamSys {

/*! \class HistogramWidget
    \brief A histogram widget class.

    This class provides a basic histogram widget for displaying histograms.

    \see Histogram class for the actual computation of the histogram.

    \todo Implement colorRange selection:
        \code
        public:
            QSizeF colorRange() const;

        public slots:
            void setColorRange(const QSizeF &colorRange);

        signals:
            void colorRangeChanged(QSizeF colorRange);
        \endcode
 */

//! \enum HistogramWidget::ContourStyle

/*! \var HistogramWidget::NoContour
    \brief No contour lines.
 */

/*! \var HistogramWidget::NormalContour
    \brief No line between bins (default).
 */

/*! \var HistogramWidget::BarContour
    \brief Box surrounding each bin.
 */

//! \enum HistogramWidget::FillStyle

/*! \var HistogramWidget::SolidFill
    \brief Fill bins with a solid fillColor (default).
 */

/*! \var HistogramWidget::ColorTableFill
    \brief Fill bins with the color provided by colorTable.
 */

//! \enum HistogramWidget::BinScaling

/*! \var HistogramWidget::LinearScaling
    \brief Linear bin scaling (default).
 */

/*! \var HistogramWidget::LogarithmicScaling
    \brief Logarithmic bin scaling.
 */

 // ---------------------------------------------------------------------------

/*!
    \internal
    \brief This class contains internal data and implementation for the class
           HistogramWidget.
 */
class HistogramWidgetPrivate
{
public:
    HistogramWidgetPrivate();
    virtual ~HistogramWidgetPrivate();

    void drawNormalContour(QPainter *p) const;
    void drawBinsSolid(QPainter *p) const;
    void drawBinsColorTable(QPainter *p) const;

    enum CursorRegion {
        LeftBand,
        RightBand,
        BetweenBands,
        OutsideBands
    };

    enum DraggingMode {
        NoDragging,
        DraggingLeft,
        DraggingRight,
        DraggingBoth
    };

    void updateBands(const QRect &rect);
    CursorRegion cursorRegion(int x) const;

    Histogram histogram;
    HistogramWidget::ContourStyle contourStyle;
    HistogramWidget::FillStyle fillStyle;
    HistogramWidget::BinScaling binScaling;
    QColor contourColor;
    QColor fillColor;
    QColor backgroundColor;
    ColorTable colorTable;
    int maxBinHeight;
    float maxBinHeightScale;
    bool isInteractive;
    bool showInteractionHint;
    QRubberBand leftBand;
    QRubberBand rightBand;
    DraggingMode draggingMode;
    int draggingStartPos;
    int draggingLeftPos;
    int draggingRightPos;
    qreal lowerBound;
    qreal upperBound;
};

HistogramWidgetPrivate::HistogramWidgetPrivate()
    : contourStyle(HistogramWidget::NormalContour),
      fillStyle(HistogramWidget::SolidFill),
      binScaling(HistogramWidget::LinearScaling),
      contourColor(Qt::black),
      fillColor(Qt::transparent),
      backgroundColor(Qt::transparent),
      maxBinHeight(0),
      maxBinHeightScale(1.0),
      isInteractive(false),
      showInteractionHint(true),
      leftBand(QRubberBand::Line),
      rightBand(QRubberBand::Line),
      draggingMode(NoDragging),
      draggingStartPos(0),
      draggingLeftPos(0),
      draggingRightPos(0),
      lowerBound(0.0),
      upperBound(1.0)
{
}

HistogramWidgetPrivate::~HistogramWidgetPrivate()
{
}

void HistogramWidgetPrivate::drawNormalContour(QPainter *p) const
{
    p->setPen(contourColor);

    double y1 = 0.0;
    int bins = histogram.bins();

    switch (binScaling)
    {
    case HistogramWidget::LinearScaling:
        for (int i = 0; i < bins; ++i) {
            int y2 = histogram.binEntries(i);
            p->drawLine(QLineF(i, y1, i, y2));
            p->drawLine(QLineF(i, y2, i + 1, y2));
            y1 = y2;
        }
        break;
    case HistogramWidget::LogarithmicScaling:
        for (int i = 0; i < bins; ++i) {
            double y2 = std::log10(histogram.binEntries(i) + 1.0);
            p->drawLine(QLineF(i, y1, i, y2));
            p->drawLine(QLineF(i, y2, i + 1, y2));
            y1 = y2;
        }
        break;
    }

    p->drawLine(QLineF(bins, y1, bins, 0));
}

void HistogramWidgetPrivate::drawBinsSolid(QPainter *p) const
{
    switch (contourStyle)
    {
    case HistogramWidget::NoContour:
    case HistogramWidget::NormalContour:
        p->setPen(fillColor);
        break;
    case HistogramWidget::BarContour:
        p->setPen(contourColor);
        break;
    }

    p->setBrush(fillColor);

    int bins = histogram.bins();

    switch (binScaling)
    {
    case HistogramWidget::LinearScaling:
        for (int i = 0; i < bins; ++i) {
            int y = histogram.binEntries(i);
            p->drawRect(QRectF(i, 0, 1, y));
        }
        break;
    case HistogramWidget::LogarithmicScaling:
        for (int i = 0; i < bins; ++i) {
            double y = std::log10(histogram.binEntries(i) + 1.0);
            p->drawRect(QRectF(i, 0, 1, y));
        }
    }

    if (contourStyle == HistogramWidget::NormalContour)
        drawNormalContour(p);
}

//! \todo needs implementation
void HistogramWidgetPrivate::drawBinsColorTable(QPainter *p) const
{
    drawNormalContour(p);
}

void HistogramWidgetPrivate::updateBands(const QRect &rect)
{
    if (rect.width() < 1)
        return;

    if (lowerBound > upperBound) qSwap(lowerBound, upperBound);
    if (lowerBound < 0.0) lowerBound = 0.0;
    if (upperBound > 1.0) upperBound = 1.0;

    int maxx = rect.width() - 1;
    int lx = qRound(lowerBound * maxx) + rect.left();
    int rx = qRound(upperBound * maxx) + rect.left();

    leftBand.setGeometry(lx, rect.top(), 1, rect.height());
    rightBand.setGeometry(rx, rect.top(), 1, rect.height());
}

HistogramWidgetPrivate::CursorRegion
HistogramWidgetPrivate::cursorRegion(int x) const
{
    Q_ASSERT(leftBand.x() <= rightBand.x());

    const int dx = 4;
    const int lx = leftBand.x();
    const int rx = rightBand.x();

    if (x < lx - dx || x > rx + dx)
        return OutsideBands;
    else if (x > lx + dx && x < rx - dx)
        return BetweenBands;

    const int dl = qAbs(lx - x);
    const int dr = qAbs(rx - x);

    if (dl < dr)
        return LeftBand;
    else if (dr < dl)
        return RightBand;

    // dl == dr or lx == rx
    if (x < lx)
        return LeftBand;
    else
        return RightBand;
}

// ---------------------------------------------------------------------------

HistogramWidget::HistogramWidget(QWidget *parent)
    : QFrame(parent),
        d_ptr(new HistogramWidgetPrivate())
{
    Q_D(HistogramWidget);
    d->leftBand.setParent(this);
    d->rightBand.setParent(this);
}

HistogramWidget::~HistogramWidget()
{
    delete d_ptr;
}

Histogram HistogramWidget::histogram() const
{
    Q_D(const HistogramWidget);
    return d->histogram;
}

double HistogramWidget::histogramLowerEdge() const
{
    Q_D(const HistogramWidget);
    return d->histogram.lowerEdge();
}

double HistogramWidget::histogramUpperEdge() const
{
    Q_D(const HistogramWidget);
    return d->histogram.upperEdge();
}

void HistogramWidget::setHistogram(const Histogram &histogram)
{
    Q_D(HistogramWidget);
    d->histogram = histogram;
    update();
}

void HistogramWidget::clearHistogram()
{
    Q_D(HistogramWidget);
    d->histogram.reset();
    update();
}

void HistogramWidget::setHistogramFromImage(
        const Image *image, double minValue, double maxValue, int numBins)
{
    Q_D(HistogramWidget);
    Histogram &hist = d->histogram;

    if (!image || image->isNull()) {
        hist.reset();
        update();
        return;
    }

    if (hist.lowerEdge() != minValue ||
        hist.upperEdge() != maxValue ||
        hist.bins() != numBins)
    {
        hist.reset(numBins, minValue, maxValue);
    }
    else
        hist.reset();

    hist.fill(image);
    update();
}

void HistogramWidget::setHistogramFromImage(
        const Image *image, const ColorRange &colorRange, int numBins)
{
    Q_D(HistogramWidget);
    setHistogramFromImage(
            image, colorRange.minValue(), colorRange.maxValue(), numBins);
    if (d->isInteractive)
        setSelection(colorRange.minColorRatio(), colorRange.maxColorRatio());
}

/*! \deprecated
    This method is deprecated. It will be removed after the transition to
    the setHistogramFromImage() is done.

    \note This implementation does not exactly the same as the old one. It
          is just a wrapper to setHistogramFromImage() which has a slightly
          changed behaviour.
 */
void HistogramWidget::setImage(const Image *image,
                               double lowerEdge, double upperEdge,
                               int maxBins)
{
    setHistogramFromImage(image, lowerEdge, upperEdge, maxBins);
}

HistogramWidget::ContourStyle HistogramWidget::contourStyle() const
{
    Q_D(const HistogramWidget);
    return d->contourStyle;
}

void HistogramWidget::setContourStyle(
        HistogramWidget::ContourStyle contourStyle)
{
    Q_D(HistogramWidget);
    d->contourStyle = contourStyle;
    update();
}

HistogramWidget::FillStyle HistogramWidget::fillStyle() const
{
    Q_D(const HistogramWidget);
    return d->fillStyle;
}

void HistogramWidget::setFillStyle(HistogramWidget::FillStyle fillStyle)
{
    Q_D(HistogramWidget);
    d->fillStyle = fillStyle;
    update();
}

QColor HistogramWidget::contourColor() const
{
    Q_D(const HistogramWidget);
    return d->contourColor;
}

void HistogramWidget::setContourColor(const QColor &color)
{
    Q_D(HistogramWidget);
    d->contourColor = color;
    update();
}

QColor HistogramWidget::fillColor() const
{
    Q_D(const HistogramWidget);
    return d->fillColor;
}

void HistogramWidget::setFillColor(const QColor &color)
{
    Q_D(HistogramWidget);
    d->fillColor = color;
    update();
}

QColor HistogramWidget::backgroundColor() const
{
    Q_D(const HistogramWidget);
    return d->backgroundColor;
}

void HistogramWidget::setBackgroundColor(const QColor &color)
{
    Q_D(HistogramWidget);
    d->backgroundColor = color;
    update();
}

CamSys::ColorTable HistogramWidget::colorTable() const
{
    Q_D(const HistogramWidget);
    return d->colorTable;
}

void HistogramWidget::setColorTable(const CamSys::ColorTable &colorTable)
{
    Q_D(HistogramWidget);
    d->colorTable = colorTable;
    update();
}

HistogramWidget::BinScaling HistogramWidget::binScaling() const
{
    Q_D(const HistogramWidget);
    return d->binScaling;
}

void HistogramWidget::setBinScaling(
        HistogramWidget::BinScaling binScaling)
{
    Q_D(HistogramWidget);
    d->binScaling = binScaling;
    update();
}

int HistogramWidget::maxBinHeight() const
{
    Q_D(const HistogramWidget);
    return d->maxBinHeight;
}

void HistogramWidget::setMaxBinHeight(int maxHeight)
{
    Q_D(HistogramWidget);
    d->maxBinHeight = maxHeight > 0 ? maxHeight : 0;
    d->maxBinHeightScale = 1.0;
    update();
}

float HistogramWidget::maxBinHeightScale() const
{
    Q_D(const HistogramWidget);
    return d->maxBinHeightScale;
}

void HistogramWidget::setMaxBinHeightScale(float scale, bool fixed)
{
    Q_D(HistogramWidget);
    if (scale < 0.0) scale = 0.0;

    if (fixed) {
        d->maxBinHeight = qRound(scale * d->histogram.maxBinEntries());
        d->maxBinHeightScale = 1.0;
    }
    else {
        d->maxBinHeight = 0;
        d->maxBinHeightScale = scale;
    }

    update();
}

bool HistogramWidget::isMaxBinHeightFixed() const
{
    Q_D(const HistogramWidget);
    return d->maxBinHeight != 0;
}

void HistogramWidget::setMaxBinHeightFixed(bool fixed)
{
    Q_D(HistogramWidget);

    if (fixed)
    {
        if (d->maxBinHeight != 0)
            return;

        d->maxBinHeight = qRound(
            d->maxBinHeightScale * d->histogram.maxBinEntries());
        d->maxBinHeightScale = 1.0;
    }
    else
    {
        if (d->maxBinHeight == 0)
            return;

        int maxEntries = d->histogram.maxBinEntries();
        d->maxBinHeightScale = (maxEntries != 0) ?
            float(d->maxBinHeight) / maxEntries : 1.0;
        d->maxBinHeight = 0;
    }
}

void HistogramWidget::clearMaxBinHeight()
{
    Q_D(HistogramWidget);
    d->maxBinHeight = 0;
    d->maxBinHeightScale = 1.0;
    update();
}

bool HistogramWidget::isInteractive() const
{
    Q_D(const HistogramWidget);
    return d->isInteractive;
}

void HistogramWidget::setInteractive(bool interactive)
{
    Q_D(HistogramWidget);
    if (interactive != d->isInteractive)
    {
        d->isInteractive = interactive;

        d->updateBands(contentsRect());
        d->leftBand.setVisible(interactive);
        d->rightBand.setVisible(interactive);

        setMouseTracking(interactive);
    }
}

bool HistogramWidget::isInteractionHintEnabled() const
{
    Q_D(const HistogramWidget);
    return d->showInteractionHint;
}

void HistogramWidget::setInteractionHintEnabled(bool showHint)
{
    Q_D(HistogramWidget);
    if (!showHint && d->showInteractionHint)
        setToolTip(QString());
    d->showInteractionHint = showHint;
}

qreal HistogramWidget::selectionLowerBound() const
{
    Q_D(const HistogramWidget);
    return d->lowerBound;
}

qreal HistogramWidget::selectionUpperBound() const
{
    Q_D(const HistogramWidget);
    return d->upperBound;
}

void HistogramWidget::getSelection(qreal &lowerBound, qreal &upperBound) const
{
    Q_D(const HistogramWidget);
    lowerBound = d->lowerBound;
    upperBound = d->upperBound;
}

void HistogramWidget::setSelection(qreal lowerBound, qreal upperBound)
{
    Q_D(HistogramWidget);
    d->lowerBound = lowerBound;
    d->upperBound = upperBound;
    d->updateBands(contentsRect());
}

void HistogramWidget::paintEvent(QPaintEvent *event)
{
    Q_D(const HistogramWidget);
    QFrame::paintEvent(event);

    QPainter p(this);

    QColor bgColor = backgroundColor();
    if (bgColor != Qt::transparent)
        p.fillRect(rect(), bgColor);

    QRect crect = contentsRect();
    p.setViewport(crect.x(), crect.y(), crect.width() - 1, crect.height() - 1);

    int width = d->histogram.bins();
    int height = d->maxBinHeight > 0 ? d->maxBinHeight :
        qRound(d->maxBinHeightScale * d->histogram.maxBinEntries());

    if (d->binScaling == LogarithmicScaling)
        height = qRound(std::ceil(std::log10(height + 1.0)));

    // scale to width/height and flip the y-axis
    p.setWindow(0, 0, width, height);
    p.translate(0, height);
    p.scale(1, -1);

    switch (d->fillStyle)
    {
    case SolidFill:
        d->drawBinsSolid(&p);
        break;
    case ColorTableFill:
        d->drawBinsColorTable(&p);
        break;
    }
}

void HistogramWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    Q_D(HistogramWidget);

    if (!d->isInteractive)
        return;

    d->updateBands(contentsRect());
}

void HistogramWidget::mousePressEvent(QMouseEvent *event)
{
    Q_D(HistogramWidget);

    if (!d->isInteractive)
        return;

    if (event->button() != Qt::LeftButton)
        return;

    d->draggingStartPos = event->x();
    d->draggingLeftPos = d->leftBand.x();
    d->draggingRightPos = d->rightBand.x();

    int cx = event->x() + contentsRect().x();
    switch (d->cursorRegion(cx))
    {
    case HistogramWidgetPrivate::OutsideBands:
        break;
    case HistogramWidgetPrivate::BetweenBands:
        if (d->lowerBound != 0.0 || d->upperBound != 1.0) {
            setCursor(Qt::ClosedHandCursor);
            d->draggingMode = HistogramWidgetPrivate::DraggingBoth;
        }
        break;
    case HistogramWidgetPrivate::LeftBand:
        setCursor(Qt::SizeHorCursor);
        d->draggingMode = HistogramWidgetPrivate::DraggingLeft;
        break;
    case HistogramWidgetPrivate::RightBand:
        setCursor(Qt::SizeHorCursor);
        d->draggingMode = HistogramWidgetPrivate::DraggingRight;
        break;
    }
}

void HistogramWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(HistogramWidget);

    if (!d->isInteractive)
        return;

    if ((event->buttons() & Qt::LeftButton) != 0)
    {
        QRect crect = contentsRect();
        int ldx = d->draggingLeftPos + event->x() - d->draggingStartPos;
        int rdx = d->draggingRightPos + event->x() - d->draggingStartPos;
        qreal lb = (ldx - crect.x()) / qreal(crect.width());
        qreal ub = (rdx - crect.x() + 1) / qreal(crect.width());

        switch (d->draggingMode)
        {
        case HistogramWidgetPrivate::DraggingLeft:
            if (lb < 0.0) d->lowerBound = 0.0;
            else if (lb > d->upperBound) d->lowerBound = d->upperBound;
            else d->lowerBound = lb;
            d->updateBands(crect);
            break;
        case HistogramWidgetPrivate::DraggingRight:
            if (ub > 1.0) d->upperBound = 1.0;
            else if (ub < d->lowerBound) d->upperBound = d->lowerBound;
            else d->upperBound = ub;
            d->updateBands(crect);
            break;
        case HistogramWidgetPrivate::DraggingBoth:
            if (lb < 0.0) {
                d->lowerBound = 0.0;
                d->upperBound = ub - lb;
            }
            else if (ub > 1.0) {
                d->lowerBound = lb + 1.0 - ub;
                d->upperBound = 1.0;
            }
            else {
                d->lowerBound = lb;
                d->upperBound = ub;
            }
            d->updateBands(crect);
            break;
        default:
            break;
        }

        if (d->draggingMode != HistogramWidgetPrivate::NoDragging)
            emit selectionChanging(d->lowerBound, d->upperBound);
    }
    else // left mouse button not pressed
    {
        int cx = event->x() + contentsRect().x();
        HistogramWidgetPrivate::CursorRegion cursorRegion = d->cursorRegion(cx);
        switch (cursorRegion)
        {
        case HistogramWidgetPrivate::OutsideBands:
            unsetCursor();
            break;
        case HistogramWidgetPrivate::BetweenBands:
            if (d->lowerBound != 0.0 || d->upperBound != 1.0)
                setCursor(Qt::OpenHandCursor);
            else
                unsetCursor();

            break;
        case HistogramWidgetPrivate::LeftBand:
        case HistogramWidgetPrivate::RightBand:
            setCursor(Qt::SizeHorCursor);
            break;
        }

        if (d->showInteractionHint
              && cursorRegion == HistogramWidgetPrivate::BetweenBands
              && d->lowerBound == 0.0 && d->upperBound == 1.0)
            setToolTip(
                tr("The color range can be selected by dragging the\n"\
                "lines at the left and right side of the histogram."));
        else
            setToolTip(QString());

    }
}

void HistogramWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(HistogramWidget);

    if (!d->isInteractive)
        return;

    if (event->button() != Qt::LeftButton)
        return;

    int cx = event->x() + contentsRect().x();
    switch (d->cursorRegion(cx))
    {
    case HistogramWidgetPrivate::OutsideBands:
        unsetCursor();
        break;
    case HistogramWidgetPrivate::BetweenBands:
        if (d->lowerBound != 0.0 || d->upperBound != 1.0)
            setCursor(Qt::OpenHandCursor);
        else
            unsetCursor();
        break;
    case HistogramWidgetPrivate::LeftBand:
    case HistogramWidgetPrivate::RightBand:
        setCursor(Qt::SizeHorCursor);
        break;
    }

    if (d->draggingMode != HistogramWidgetPrivate::NoDragging)
    {
        d->draggingMode = HistogramWidgetPrivate::NoDragging;
        d->draggingStartPos = 0;
        d->draggingLeftPos = 0;
        d->draggingRightPos = 0;

        emit selectionChanged(d->lowerBound, d->upperBound);
    }
}

} // namespace CamSys
