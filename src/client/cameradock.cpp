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

#include "cameradock.h"
#include "ui_cameradock.h"
#include <QtCore/QtCore>

CameraDock::CameraDock(QWidget *parent)
    : QDockWidget(parent),
      ui(new Ui::CameraDock),
      m_exposureTime(1),
      m_frameRate(1)
{
    ui->setupUi(this);

    ui->comboTrigger->addItem("Fixed Rate", "fixedrate");
    ui->comboTrigger->addItem("Trigger Bus", "syncin1");
    ui->comboTrigger->addItem("GSJC", "syncin2");

    connect(ui->buttonOpen, SIGNAL(clicked(bool)), SIGNAL(openButtonClicked(bool)));
    connect(ui->buttonCapture, SIGNAL(clicked(bool)), SIGNAL(captureButtonClicked(bool)));
    setCameraState(UnknownState);
}

CameraDock::~CameraDock()
{
    delete ui;
}

void CameraDock::reset()
{
    setExposureTime(1);
    setFrameRate(1);
    ui->labelCamera->setText("-");
    ui->labelCameraId->setText("-");
    ui->labelSensor->setText("-");
}

void CameraDock::setCameraState(CameraDock::CameraState state)
{
    m_state = state;

    switch (m_state)
    {
    case UnknownState:
        ui->buttonOpen->setChecked(false);
        ui->buttonCapture->setChecked(false);
        ui->buttonOpen->setEnabled(false);
        ui->buttonCapture->setEnabled(false);
        ui->comboTrigger->setEnabled(false);
        ui->spinExposure->setEnabled(false);
        ui->spinFrameRate->setEnabled(false);
        ui->buttonExposure->setEnabled(false);
        ui->buttonFrameRate->setEnabled(false);
        reset();
        break;
    case ClosedState:
        ui->buttonOpen->setChecked(false);
        ui->buttonCapture->setChecked(false);
        ui->buttonOpen->setEnabled(true);
        ui->buttonCapture->setEnabled(false);
        ui->comboTrigger->setEnabled(false);
        ui->spinExposure->setEnabled(false);
        ui->spinFrameRate->setEnabled(false);
        ui->buttonExposure->setEnabled(false);
        ui->buttonFrameRate->setEnabled(false);
        reset();
        break;
    case OpenedState:
        ui->buttonOpen->setChecked(true);
        ui->buttonCapture->setChecked(false);
        ui->buttonOpen->setEnabled(true);
        ui->buttonCapture->setEnabled(true);
        ui->comboTrigger->setEnabled(true);
        ui->spinExposure->setEnabled(true);
        ui->spinFrameRate->setEnabled(true);
        ui->buttonExposure->setEnabled(true);
        ui->buttonFrameRate->setEnabled(true);
        break;
    case CapturingState:
        ui->buttonOpen->setChecked(true);
        ui->buttonCapture->setChecked(true);
        ui->buttonOpen->setEnabled(true);
        ui->buttonCapture->setEnabled(true);
        ui->comboTrigger->setEnabled(true);
        ui->spinExposure->setEnabled(true);
        ui->spinFrameRate->setEnabled(true);
        ui->buttonExposure->setEnabled(true);
        ui->buttonFrameRate->setEnabled(true);
        break;
    }
}

double CameraDock::exposureTime() const
{
    return m_exposureTime;
}

void CameraDock::setExposureTime(double exposureTime)
{
    m_exposureTime = exposureTime;
    ui->spinExposure->setValue(exposureTime);
}

double CameraDock::frameRate() const
{
    return m_frameRate;
}

void CameraDock::setFrameRate(double frameRate)
{
    m_frameRate = frameRate;
    ui->spinFrameRate->setValue(frameRate);
}

QByteArray CameraDock::triggerMode() const
{
    int i = ui->comboTrigger->currentIndex();
    if (i >= 0) {
        QVariant data = ui->comboTrigger->itemData(i);
        if (data.isValid())
            return data.toByteArray();
    }
    return QByteArray();
}

void CameraDock::setTriggerMode(const QByteArray &triggerMode)
{
    int i = ui->comboTrigger->findData(triggerMode);
    if (i >= 0)
        ui->comboTrigger->setCurrentIndex(i);
}

void CameraDock::setCameraName(const QString &cameraName)
{
    ui->labelCamera->setText(cameraName);
}

void CameraDock::setCameraId(const QString &cameraId)
{
    ui->labelCameraId->setText(cameraId);
}

void CameraDock::setCameraSensor(const QString &cameraSensor)
{
    ui->labelSensor->setText(cameraSensor);
}

void CameraDock::on_buttonOpen_clicked(bool checked)
{
    if (!checked)
        setCameraState(ClosedState);
}

void CameraDock::on_buttonCapture_clicked(bool checked)
{
    Q_UNUSED(checked)
}

void CameraDock::on_spinExposure_editingFinished()
{
    double value = ui->spinExposure->value();
    if (value != m_exposureTime) {
        m_exposureTime = value;
        emit exposureTimeChanged(m_exposureTime);
    }
}

void CameraDock::on_buttonExposure_clicked()
{
    m_exposureTime = ui->spinExposure->value();
    emit exposureTimeChanged(m_exposureTime);
}

void CameraDock::on_spinFrameRate_editingFinished()
{
    double value = ui->spinFrameRate->value();
    if (value != m_frameRate) {
        m_frameRate = value;
        emit frameRateChanged(m_frameRate);
    }
}

void CameraDock::on_buttonFrameRate_clicked()
{
    m_frameRate = ui->spinFrameRate->value();
    emit frameRateChanged(m_frameRate);
}

void CameraDock::on_comboTrigger_activated(int index)
{
    QVariant data = ui->comboTrigger->itemData(index);
    if (data.isValid())
        emit triggerModeChanged(data.toByteArray());
}

void CameraDock::on_comboTrigger_currentIndexChanged(int index)
{
    QByteArray mode = ui->comboTrigger->itemData(index).toByteArray();
    bool enable = !(mode == "syncin1" || mode == "syncin2");
    ui->spinFrameRate->setEnabled(enable);
    ui->buttonFrameRate->setEnabled(enable);
}
