/*
 * Copyright (c) 2012 Kolja Glogowski
 * Kiepenheuer-Institut fuer Sonnenphysik
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

#ifndef SJCAM_HISTOGRAMDOCK_H
#define SJCAM_HISTOGRAMDOCK_H

#include <QtGui/QDockWidget>

namespace CamSys {
    class HistogramWidget;
    class ColorBar;
    class ColorTable;
    class Image;
}

namespace Ui {
    class HistogramDock;
}

class HistogramDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit HistogramDock(QWidget *parent = 0);
    ~HistogramDock();

    void clear();
    void setImage(CamSys::Image *image);
    void setImage(CamSys::Image *image, double minValue, double maxValue);
    void setColorRange(double minValue, double maxValue);
    void setColorTable(const CamSys::ColorTable &colorTable);
    double minColorValue() const;
    double maxColorValue() const;

public slots:
    void setSelection(double lowerBound, double upperBound);

signals:
    void colorSpreadChanging(double minColorValue, double maxColorValue);
    void colorSpreadChanged(double minColorValue, double maxColorValue);

private slots:
    void histWidget_selectionChanging(qreal lowerBound, qreal upperBound);
    void histWidget_selectionChanged(qreal lowerBound, qreal upperBound);

private:
    Ui::HistogramDock *ui;
    CamSys::HistogramWidget *m_histWidget;
    CamSys::ColorBar *m_colorBar;
    double m_minValue;
    double m_maxValue;
};

#endif // SJCAM_HISTOGRAMDOCK_H
