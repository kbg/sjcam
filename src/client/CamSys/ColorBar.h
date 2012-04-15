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

#ifndef CAMSYS_COLOR_BAR_H
#define CAMSYS_COLOR_BAR_H

#include "ColorTable.h"
#include <QtGui/QWidget>

namespace CamSys {

class ColorRange;
class ColorBarPrivate;

//! \addtogroup CamSysGui
//! @{

class ColorBar : public QWidget
{
    Q_OBJECT

public:
    explicit ColorBar(QWidget *parent = 0);

    explicit ColorBar(const ColorRange &colorRange,
             const ColorTable &colorTable = ColorTable(),
             QWidget *parent = 0);

    ColorBar(double minValue, double maxValue,
             const ColorTable &colorTable = ColorTable(),
             QWidget *parent = 0);

    virtual ~ColorBar();
    QSize sizeHint() const;

    void setBarFrameStyle(int style);
    int barFrameStyle() const;

    void setInteractive(bool active = true);
    bool isInteractive() const;

    ColorTable colorTable() const;
    ColorRange colorRange() const;

    double minValue() const;
    double maxValue() const;

    double minColorValue() const;
    double maxColorValue() const;

    double minColorRatio() const;
    double maxColorRatio() const;

public slots:
    void setColorTable(const ColorTable &colorTable);
    void setColorRange(const ColorRange &colorRange);

    void setMinValue(double minValue);
    void setMaxValue(double maxValue);
    void setRange(double minValue, double maxValue);

    void setMinColorValue(double minColorValue);
    void setMaxColorValue(double maxColorValue);
    void setColorSpread(double minColorValue, double maxColorValue);

    void setMinColorRatio(double minColorRatio);
    void setMaxColorRatio(double maxColorRatio);
    void setColorSpreadRatio(double minColorRatio, double maxColorRatio);

signals:
    void colorSpreadSelected(double minColorValue, double maxColorValue);
    void colorSpreadRatioSelected(double minColorRatio, double maxColorRatio);

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    ColorBarPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ColorBar)
    Q_DISABLE_COPY(ColorBar)
};

//! @} // end of group CamSysGui

} // namespace CamSys

#endif // CAMSYS_COLOR_BAR_H
