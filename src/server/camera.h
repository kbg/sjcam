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

#ifndef SJCAM_CAMERA_H
#define SJCAM_CAMERA_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <PvApi.h>

class QVariant;

class Camera
{
public:
    Camera();
    ~Camera();

    bool open(ulong cameraId = 0);
    bool isOpen() const { return m_device != 0; }
    void close();
    bool resetConfig();

    bool startCapturing();
    bool stopCapturing();
    bool isCapturing() const;
    bool enqueueFrame(tPvFrame *frame);
    bool clearFrameQueue();
    bool startAcqusition();
    bool stopAcquisition();
    bool waitForFrameDone(tPvFrame *frame, ulong timeout, bool *timedOut);

    bool runCommand(const QByteArray &name);
    bool getAttrString(const QByteArray &name, QByteArray *value) const;
    bool setAttrString(const QByteArray &name, const QByteArray &value);
    bool getAttrEnum(const QByteArray &name, QByteArray *value)  const;
    bool setAttrEnum(const QByteArray &name, const QByteArray &value);
    bool getAttrUint32(const QByteArray &name, quint32 *value) const;
    bool setAttrUint32(const QByteArray &name, quint32 value);
    bool getAttrFloat32(const QByteArray &name, float *value) const;
    bool setAttrFloat32(const QByteArray &name, float value);
    bool getAttrInt64(const QByteArray &name, qint64 *value) const;
    bool setAttrInt64(const QByteArray &name, qint64 value);
    bool getAttrBoolean(const QByteArray &name, bool *value) const;
    bool setAttrBoolean(const QByteArray &name, bool value);
    bool getAttribute(const QByteArray &name, QVariant *value) const;
    bool setAttribute(const QByteArray &name, const QVariant &value);

    bool getFrameStats(float &fps, uint &completed, uint &dropped);

    tPvHandle device() const { return m_device; }
    tPvCameraInfoEx cameraInfo() const { return m_cameraInfo; }
    QString hwAddress() const { return m_hwAddress; }
    QString ipAddress() const { return m_ipAddress; }
    uint sensorWidth() const { return m_sensorWidth; }
    uint sensorHeight() const { return m_sensorHeight; }
    uint sensorBits() const { return m_sensorBits; }

    QString infoString() const;
    QString errorString() const { return m_errorString; }

protected:
    void clearInfo();
    void setError(const QString &errorString,
                  tPvErr errorCode = ePvErrSuccess) const;
    void clearError() const;

private:
    Q_DISABLE_COPY(Camera)
    mutable QString m_errorString;
    tPvHandle m_device;
    tPvCameraInfoEx m_cameraInfo;
    QByteArray m_hwAddress;
    QByteArray m_ipAddress;
    quint32 m_sensorWidth;
    quint32 m_sensorHeight;
    quint32 m_sensorBits;
};

#endif // SJCAM_CAMERA_H
