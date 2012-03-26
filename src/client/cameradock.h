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

#ifndef SJCAM_CAMERADOCK_H
#define SJCAM_CAMERADOCK_H

#include <QtGui/QDockWidget>

namespace Ui {
    class CameraDock;
}

class CameraDock : public QDockWidget
{
    Q_OBJECT

public:
    enum CameraState {
        ClosedState,
        OpenedState,
        CapturingState,
        UnknownState
    };

    explicit CameraDock(QWidget *parent = 0);
    ~CameraDock();

    void reset();

    CameraState cameraState() const { return m_state; }
    void setCameraState(CameraState state);

    double exposureTime() const;
    void setExposureTime(double exposureTime);

    double frameRate() const;
    void setFrameRate(double frameRate);

    void setCameraName(const QString &cameraName);
    void setCameraId(const QString &cameraId);
    void setCameraSensor(const QString &cameraSensor);

signals:
    void openButtonClicked(bool checked);
    void captureButtonClicked(bool checked);
    void exposureTimeChanged(double ms);
    void frameRateChanged(double hz);

private slots:
    void on_buttonOpen_clicked(bool checked);
    void on_buttonCapture_clicked(bool checked);
    void on_spinExposure_editingFinished();
    void on_buttonExposure_clicked();
    void on_spinFrameRate_editingFinished();
    void on_buttonFrameRate_clicked();

private:
    Ui::CameraDock *ui;
    CameraState m_state;
    double m_exposureTime;
    double m_frameRate;
};

#endif // SJCAM_CAMERADOCK_H
