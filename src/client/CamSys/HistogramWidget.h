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

#ifndef CAMSYS_HISTOGRAM_WIDGET_H
#define CAMSYS_HISTOGRAM_WIDGET_H

#include <QFrame>

class QColor;
class QPaintEvent;

namespace CamSys {

class Image;
class Histogram;
class ColorTable;
class ColorRange;
class HistogramWidgetPrivate;

//! \addtogroup CamSysGui
//! @{

class HistogramWidget : public QFrame
{
    Q_OBJECT

public:
    enum ContourStyle {
        NoContour,
        NormalContour,
        BarContour
    };

    enum FillStyle {
        SolidFill,
        ColorTableFill
    };

    enum BinScaling {
        LinearScaling,
        LogarithmicScaling
    };

public:
    explicit HistogramWidget(QWidget *parent = 0);
    virtual ~HistogramWidget();

    Histogram histogram() const;
    double histogramLowerEdge() const;
    double histogramUpperEdge() const;

    void setHistogram(const Histogram &histogram);
    void clearHistogram();

    void setHistogramFromImage(
            const Image *image, double minValue, double maxValue,
            int numBins = 256);
    void setHistogramFromImage(
            const Image *image, const ColorRange &colorRange,
            int numBins = 256);

    void setImage(const Image *image, double lowerEdge, double upperEdge,
                  int maxBins = 256);

public:
    ContourStyle contourStyle() const;
    void setContourStyle(ContourStyle contourStyle);

    FillStyle fillStyle() const;
    void setFillStyle(FillStyle fillStyle);

    QColor contourColor() const;
    void setContourColor(const QColor &color);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &color);

    CamSys::ColorTable colorTable() const;
    void setColorTable(const CamSys::ColorTable &colorTable);

    BinScaling binScaling() const;
    void setBinScaling(BinScaling binScaling);

    int maxBinHeight() const;
    void setMaxBinHeight(int maxHeight);

    float maxBinHeightScale() const;
    void setMaxBinHeightScale(float scale, bool fixed = false);

    bool isMaxBinHeightFixed() const;
    void setMaxBinHeightFixed(bool fixed);

    void clearMaxBinHeight();

    bool isInteractive() const;
    void setInteractive(bool interactive);

    bool isInteractionHintEnabled() const;
    void setInteractionHintEnabled(bool showHint);

    qreal selectionLowerBound() const;
    qreal selectionUpperBound() const;
    void getSelection(qreal &lowerBound, qreal &upperBound) const;

public slots:
    void setSelection(qreal lowerBound, qreal upperBound);

signals:
    void selectionChanging(qreal lowerBound, qreal upperBound);
    void selectionChanged(qreal lowerBound, qreal upperBound);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    HistogramWidgetPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(HistogramWidget)
    Q_DISABLE_COPY(HistogramWidget)
};

//! @} // end of group CamSysGui

} // namespace CamSys

#endif // CAMSYS_HISTOGRAM_WIDGET_H
