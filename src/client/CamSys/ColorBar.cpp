/*
 * Copyright (c) 2008-2010 Kolja Glogowski
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

#include "ColorBar.h"
#include "ColorRange.h"
#include <QtGui/QRgb>
#include <QtGui/QtGui>

namespace CamSys {

/*! \class ColorBar
    \brief A color bar widget.

    This widget displays a color gradient corresponding to a given ColorRange
    and ColorTable.

    By setting setInteractive() to true, the widget may also be used to
    select a color spread interactively. This selection method is disabled
    by default and must be enabled explicitly.

    \todo Draw more value ticks and labels.
 */

// ---------------------------------------------------------------------------

/*! \internal
    \brief This class contains internal data and implementation for ColorBar.
 */
class ColorBarPrivate
{
    Q_DECLARE_PUBLIC(ColorBar)
    ColorBar * const q_ptr;

public:
    explicit ColorBarPrivate(ColorBar *qq,
        const ColorRange &range = ColorRange(0, 1),
        const ColorTable &table = ColorTable());

    void updateLowerLabels();
    void updateUpperLabels();
    void updateLabels();
    void updateBarImage();
    void updateMouseSelection(int x, Qt::MouseButtons buttons);

    ColorRange colorRange;
    ColorTable colorTable;
    bool isInteractive;
    QImage barImage;
    QLabel labelImage;
    QLabel labelLeft;
    //QLabel labelCenter;
    QLabel labelRight;
    QRubberBand rubberBand;
};

ColorBarPrivate::ColorBarPrivate(ColorBar *qq, const ColorRange &range,
                                 const ColorTable &table)
    : q_ptr(qq),
        colorRange(range),
        colorTable(table),
        isInteractive(false),
        barImage(ColorTable::TableSize, 1, QImage::Format_Indexed8),
        rubberBand(QRubberBand::Rectangle, &labelImage)
{
    Q_Q(ColorBar);
    q->setContentsMargins(0, 0, 0, 0);

    labelImage.setScaledContents(true);
    labelImage.setMinimumSize(100, 20);
    labelImage.setFrameStyle(QFrame::Box | QFrame::Plain);

    QVBoxLayout *layout = new QVBoxLayout(q);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(&labelImage);

    QHBoxLayout *labelLayout = new QHBoxLayout();
    labelLayout->addWidget(&labelLeft, 0, Qt::AlignLeft);
    //labelLayout->addWidget(&labelCenter, 0, Qt::AlignHCenter);
    labelLayout->addWidget(&labelRight, 0, Qt::AlignRight);
    labelLayout->setSpacing(0);
    layout->addLayout(labelLayout);

    updateBarImage();
    updateLabels();
}

void ColorBarPrivate::updateLowerLabels()
{
    double minValue = colorRange.minValue();
    //double maxValue = colorRange.maxValue();
    labelLeft.setText(QString::number(minValue));
    //labelCenter.setText(QString::number((minValue + maxValue) / 2.0));
}

void ColorBarPrivate::updateUpperLabels()
{
    //double minValue = colorRange.minValue();
    double maxValue = colorRange.maxValue();
    labelRight.setText(QString::number(maxValue));
    //labelCenter.setText(QString::number((minValue + maxValue) / 2.0));
}

void ColorBarPrivate::updateLabels()
{
    double minValue = colorRange.minValue();
    double maxValue = colorRange.maxValue();
    labelLeft.setText(QString::number(minValue));
    labelRight.setText(QString::number(maxValue));
    //labelCenter.setText(QString::number((minValue + maxValue) / 2.0));
}

void ColorBarPrivate::updateBarImage()
{
    int barWidth = labelImage.contentsRect().width();
    barImage = QImage(barWidth, 1, QImage::Format_Indexed8);
    barImage.setColorTable(colorTable);

    //double totalRange = upperEdge - lowerEdge;

    uchar *bits = barImage.bits();
    double rangeWidth = colorRange.width();

    if (rangeWidth != 0.0)
    {
        double minValue = colorRange.minValue();
        double minColorValue = colorRange.minColorValue();
        double maxColorValue = colorRange.maxColorValue();

        // normalized begin and end of the colored area
        float spreadMin = float((minColorValue - minValue) / rangeWidth);
        float spreadMax = float((maxColorValue - minValue) / rangeWidth);

        // color spread indices and width relative to barWidth
        int minIndex = int(spreadMin * barWidth);
        int maxIndex = int(spreadMax * barWidth);
        int indexWidth = maxIndex - minIndex;

        for (int i = 0; i < minIndex; ++i)
            bits[i] = 0;

        if (indexWidth != 0)
        {
            float dx = float(ColorTable::TableSize) / indexWidth;
            for (int i = 0; i < indexWidth; ++i)
                bits[minIndex + i] = uchar(i * dx);
        }

        for (int i = maxIndex; i < barWidth; ++i)
            bits[i] = 255;
    }
    else
    {
        // empty total range -> draw bar with lowest color
        for (int i = 0; i < barWidth; ++i)
            bits[i] = 0;
    }

    labelImage.setPixmap(QPixmap::fromImage(barImage));
}

// ---------------------------------------------------------------------------

ColorBar::ColorBar(QWidget *parent)
    : QWidget(parent),
        d_ptr(new ColorBarPrivate(this))
{
}

ColorBar::ColorBar(const ColorRange &colorRange,
                   const ColorTable &colorTable,
                   QWidget *parent)
    : QWidget(parent),
        d_ptr(new ColorBarPrivate(this, colorRange, colorTable))
{
}

ColorBar::ColorBar(double minValue, double maxValue,
                   const ColorTable &colorTable,
                   QWidget *parent)
    : QWidget(parent),
        d_ptr(new ColorBarPrivate(this,
                ColorRange(minValue, maxValue), colorTable))
{
}

ColorBar::~ColorBar()
{
    delete d_ptr;
}

QSize ColorBar::sizeHint() const
{
    return QSize(256, 0);
}

/*! \brief Set the frame style of the bar part.

    The default frame style of the widget's bar is
    <tt>(QFrame::Box | QFrame::Plain)</tt>

    \see QFrame::setFrameStyle()
 */
void ColorBar::setBarFrameStyle(int style)
{
    Q_D(ColorBar);
    d->labelImage.setFrameStyle(style);
}

int ColorBar::barFrameStyle() const
{
    Q_D(const ColorBar);
    return d->labelImage.frameStyle();
}

void ColorBar::setInteractive(bool active)
{
    Q_D(ColorBar);
    d->isInteractive = active;

    if (active)
        d->labelImage.setCursor(Qt::PointingHandCursor);
    else
        d->labelImage.setCursor(Qt::ArrowCursor);
}

bool ColorBar::isInteractive() const
{
    Q_D(const ColorBar);
    return d->isInteractive;
}

ColorTable ColorBar::colorTable() const
{
    Q_D(const ColorBar);
    return d->colorTable;
}

ColorRange ColorBar::colorRange() const
{
    Q_D(const ColorBar);
    return d->colorRange;
}

double ColorBar::minValue() const
{
    Q_D(const ColorBar);
    return d->colorRange.minValue();
}

double ColorBar::maxValue() const
{
    Q_D(const ColorBar);
    return d->colorRange.maxValue();
}

double ColorBar::minColorValue() const
{
    Q_D(const ColorBar);
    return d->colorRange.minColorValue();
}

double ColorBar::maxColorValue() const
{
    Q_D(const ColorBar);
    return d->colorRange.maxColorValue();
}

double ColorBar::minColorRatio() const
{
    Q_D(const ColorBar);
    return d->colorRange.minColorRatio();
}

double ColorBar::maxColorRatio() const
{
    Q_D(const ColorBar);
    return d->colorRange.maxColorRatio();
}

void ColorBar::setColorTable(const ColorTable &colorTable)
{
    Q_D(ColorBar);
    d->colorTable = colorTable;
    d->barImage.setColorTable(colorTable);
    d->labelImage.setPixmap(QPixmap::fromImage(d->barImage));
}

void ColorBar::setColorRange(const ColorRange &colorRange)
{
    Q_D(ColorBar);

    bool rangeChanged = (colorRange.minValue() != d->colorRange.minValue() ||
                         colorRange.maxValue() != d->colorRange.maxValue());

    d->colorRange = colorRange;
    if (rangeChanged) d->updateLabels();
    d->updateBarImage();
}

void ColorBar::setMinValue(double minValue)
{
    Q_D(ColorBar);

    if (minValue == d->colorRange.minValue())
        return;

    if (minValue > d->colorRange.maxValue())
        minValue = d->colorRange.maxValue();

    d->colorRange.setMinValue(minValue);
    d->updateLowerLabels();
    d->updateBarImage();
}

void ColorBar::setMaxValue(double maxValue)
{
    Q_D(ColorBar);

    if (maxValue == d->colorRange.maxValue())
        return;

    if (maxValue < d->colorRange.minValue())
        maxValue = d->colorRange.maxValue();

    d->colorRange.setMaxValue(maxValue);
    d->updateUpperLabels();
    d->updateBarImage();
}

void ColorBar::setRange(double minValue, double maxValue)
{
    Q_D(ColorBar);

    if (minValue > maxValue)
        return;

    if (minValue == d->colorRange.minValue() &&
            maxValue == d->colorRange.maxValue())
        return;

    d->colorRange.setRange(minValue, maxValue);
    d->updateLabels();
    d->updateBarImage();
}

void ColorBar::setMinColorValue(double minColorValue)
{
    Q_D(ColorBar);

    if (minColorValue == d->colorRange.minColorValue())
        return;

    if (minColorValue > d->colorRange.maxColorValue())
        minColorValue = d->colorRange.maxColorValue();

    d->colorRange.setMinColorValue(minColorValue);
    d->updateBarImage();
}

void ColorBar::setMaxColorValue(double maxColorValue)
{
    Q_D(ColorBar);

    if (maxColorValue == d->colorRange.maxColorValue())
        return;

    if (maxColorValue < d->colorRange.minColorValue())
        maxColorValue = d->colorRange.minColorValue();

    d->colorRange.setMaxColorValue(maxColorValue);
    d->updateBarImage();
}

void ColorBar::setColorSpread(double minColorValue, double maxColorValue)
{
    Q_D(ColorBar);

    if (minColorValue > maxColorValue)
        return;

    if (minColorValue == d->colorRange.minColorValue() &&
            maxColorValue == d->colorRange.maxColorValue())
        return;

    d->colorRange.setColorSpread(minColorValue, maxColorValue);
    d->updateBarImage();
}

void ColorBar::setMinColorRatio(double minColorRatio)
{
    Q_D(ColorBar);

    if (minColorRatio == d->colorRange.minColorRatio())
        return;

    if (minColorRatio > d->colorRange.maxColorRatio())
        minColorRatio = d->colorRange.maxColorRatio();

    d->colorRange.setMinColorRatio(minColorRatio);
    d->updateBarImage();
}

void ColorBar::setMaxColorRatio(double maxColorRatio)
{
    Q_D(ColorBar);

    if (maxColorRatio == d->colorRange.maxColorRatio())
        return;

    if (maxColorRatio < d->colorRange.minColorRatio())
        maxColorRatio = d->colorRange.minColorRatio();

    d->colorRange.setMaxColorRatio(maxColorRatio);
    d->updateBarImage();
}

void ColorBar::setColorSpreadRatio(double minColorRatio, double maxColorRatio)
{
     Q_D(ColorBar);

     if (minColorRatio > maxColorRatio)
         return;

     if (minColorRatio == d->colorRange.minColorRatio() &&
             maxColorRatio == d->colorRange.maxColorRatio())
         return;

     d->colorRange.setColorSpreadRatio(minColorRatio, maxColorRatio);
     d->updateBarImage();
}

void ColorBar::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    Q_D(ColorBar);
    d->updateBarImage();
}

void ColorBarPrivate::updateMouseSelection(int x, Qt::MouseButtons buttons)
{
    Q_Q(ColorBar);

    int width = labelImage.contentsRect().width();

    double minValue = colorRange.minValue();
    double rangeWidth = colorRange.width();

    // update color spread
    if (buttons == Qt::LeftButton)
    {
        double value = minValue + x * rangeWidth / width;
        q->setMinColorValue(value);
    }
    else if (buttons == Qt::RightButton)
    {
        double value = minValue + (x + 1) * rangeWidth / width;
        q->setMaxColorValue(value);
    }
    else if (buttons == Qt::MidButton)
    {
        double value = minValue + (x + 0.5) * rangeWidth / width;
        double spread = colorRange.colorSpreadWidth() / 2.0;
        q->setColorSpread(value - spread, value + spread);
    }

    // update rubber band
    int left = 0;
    int right = 0;
    int height = labelImage.height();

    if (rangeWidth != 0)
    {
        double minColorValue = colorRange.minColorValue();
        double maxColorValue = colorRange.maxColorValue();
        left = qRound((minColorValue - minValue) / rangeWidth * width);
        right = qRound((maxColorValue - minValue) / rangeWidth * width);
    }

    rubberBand.setGeometry(left, 0, right - left + 2, height);
}

void ColorBar::mousePressEvent(QMouseEvent *event)
{
    Q_D(ColorBar);

    if (!d->isInteractive)
        return;

    // continue only, if a single button is pressed
    Qt::MouseButtons buttons = event->buttons();
    if (buttons != Qt::RightButton && buttons != Qt::LeftButton &&
            buttons != Qt::MidButton)
        return;

    int x = d->labelImage.mapFrom(this, event->pos()).x() -
            d->labelImage.frameWidth();

    d->updateMouseSelection(x, buttons);
    d->rubberBand.show();

    event->accept();
    emit colorSpreadSelected(d->colorRange.minColorValue(),
                             d->colorRange.maxColorValue());
    emit colorSpreadRatioSelected(d->colorRange.minColorRatio(),
                                  d->colorRange.maxColorRatio());
}

void ColorBar::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(ColorBar);

    if (!d->isInteractive)
        return;

    // continue only, if a single button is pressed
    Qt::MouseButtons buttons = event->buttons();
    if (buttons != Qt::RightButton && buttons != Qt::LeftButton &&
            buttons != Qt::MidButton)
        return;

    int x = d->labelImage.mapFrom(this, event->pos()).x() -
            d->labelImage.frameWidth();

    d->updateMouseSelection(x, buttons);

    event->accept();
    emit colorSpreadSelected(d->colorRange.minColorValue(),
                             d->colorRange.maxColorValue());
    emit colorSpreadRatioSelected(d->colorRange.minColorRatio(),
                                  d->colorRange.maxColorRatio());
}

void ColorBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    Q_D(ColorBar);
    d->rubberBand.hide();
}

} // namespace CamSys
