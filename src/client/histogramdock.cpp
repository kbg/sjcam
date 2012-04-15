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

#include "histogramdock.h"
#include "ui_histogramdock.h"
#include "CamSys/HistogramWidget.h"
#include "CamSys/ColorBar.h"
#include "CamSys/Histogram.h"
#include <QtGui/QtGui>

HistogramDock::HistogramDock(QWidget *parent)
    : QDockWidget(parent),
      ui(new Ui::HistogramDock),
      m_histWidget(new CamSys::HistogramWidget),
      m_colorBar(new CamSys::ColorBar),
      m_minValue(0),
      m_maxValue(1)
{
    ui->setupUi(this);

    m_histWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_histWidget->setMinimumSize(50, 50);
    m_histWidget->setFillColor(Qt::black);
    m_histWidget->setContourStyle(CamSys::HistogramWidget::NoContour);
    m_histWidget->setInteractive(true);

    m_colorBar->setBarFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_colorBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    //m_colorBar->setContentsMargins(2, 0, 2, 0);

    connect(m_histWidget, SIGNAL(selectionChanging(qreal,qreal)),
            SLOT(histWidget_selectionChanging(qreal,qreal)));
    connect(m_histWidget, SIGNAL(selectionChanged(qreal,qreal)),
            SLOT(histWidget_selectionChanged(qreal,qreal)));

    QVBoxLayout *layout = new QVBoxLayout(ui->dockWidgetContents);
    layout->setSpacing(2);
    layout->addWidget(m_histWidget);
    layout->addWidget(m_colorBar);
}

HistogramDock::~HistogramDock()
{
    delete ui;
}

void HistogramDock::clear()
{
    m_histWidget->clearHistogram();
    setSelection(m_minValue, m_maxValue);
}

void HistogramDock::setImage(CamSys::Image *image)
{
    m_histWidget->setHistogramFromImage(image, m_minValue, m_maxValue, 256);
}

void HistogramDock::setImage(CamSys::Image *image, double minValue,
                             double maxValue)
{
    m_minValue = minValue;
    m_maxValue = maxValue;
    m_colorBar->setRange(minValue, maxValue);
    m_histWidget->setHistogramFromImage(image, minValue, maxValue, 256);
}

void HistogramDock::setColorRange(double minValue, double maxValue)
{
    m_minValue = minValue;
    m_maxValue = maxValue;
    m_histWidget->clearHistogram();
    m_colorBar->setRange(minValue, maxValue);
}

void HistogramDock::setColorTable(const CamSys::ColorTable &colorTable)
{
    m_colorBar->setColorTable(colorTable);
}

double HistogramDock::minColorValue() const
{
    return m_colorBar->minColorValue();
}

double HistogramDock::maxColorValue() const
{
    return m_colorBar->maxColorValue();
}

void HistogramDock::setSelection(double lowerBound, double upperBound)
{
    m_histWidget->setSelection(lowerBound, upperBound);
    m_colorBar->setColorSpread(lowerBound, upperBound);
}

void HistogramDock::histWidget_selectionChanging(qreal lowerBound,
                                                 qreal upperBound)
{
    m_colorBar->setColorSpreadRatio(lowerBound, upperBound);
    emit colorSpreadChanging(m_colorBar->minColorValue(),
                             m_colorBar->maxColorValue());
}

void HistogramDock::histWidget_selectionChanged(qreal lowerBound,
                                                qreal upperBound)
{
    m_colorBar->setColorSpreadRatio(lowerBound, upperBound);
    emit colorSpreadChanged(m_colorBar->minColorValue(),
                             m_colorBar->maxColorValue());
}
