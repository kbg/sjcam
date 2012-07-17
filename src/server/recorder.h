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

#ifndef SJCAM_RECORDER_H
#define SJCAM_RECORDER_H

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>
#include <QtCore/QQueue>
#include <QtCore/QMetaType>
#include <PvApi.h>

class Camera;

struct CameraInfo
{
    CameraInfo() { clear(); }
    CameraInfo(const tPvCameraInfoEx &pvCameraInfo_,
               const QByteArray &hwAddress_,
               const QByteArray &ipAddress_,
               uint sensorWidth_,
               uint sensorHeight_,
               uint sensorBits_,
               uint timeStampFrequency_)
        : pvCameraInfo(pvCameraInfo_),
          hwAddress(hwAddress_),
          ipAddress(ipAddress_),
          sensorWidth(sensorWidth_),
          sensorHeight(sensorHeight_),
          sensorBits(sensorBits_),
          timeStampFrequency(timeStampFrequency_) {}

    void clear() {
        qMemSet(&pvCameraInfo, 0, sizeof(pvCameraInfo));
        hwAddress.clear();
        ipAddress.clear();
        sensorWidth = 0;
        sensorHeight = 0;
        sensorBits = 0;
        timeStampFrequency = 0;
    }

    tPvCameraInfoEx pvCameraInfo;
    QByteArray hwAddress;
    QByteArray ipAddress;
    uint sensorWidth;
    uint sensorHeight;
    uint sensorBits;
    uint timeStampFrequency;
};

struct FrameInfo
{
    ulong id;
    ulong count;
    int status;
    qint64 timestamp;
    qint64 readoutTimestamp;
    qint64 readoutTimeMs;
};
Q_DECLARE_METATYPE(FrameInfo)

class Recorder : public QThread
{
    Q_OBJECT

public:
    explicit Recorder(QObject *parent = 0);
    ~Recorder();

    bool openCamera(ulong cameraId = 0);
    bool closeCamera();
    bool isCameraOpen() const;

    bool getAttribute(const QByteArray &name, QVariant *value) const;
    bool setAttribute(const QByteArray &name, const QVariant &value);

    bool hasFinishedFrame() const;
    tPvFrame * readFinishedFrame();
    void enqueueFrame(tPvFrame *frame);

    int numBuffers() const;
    bool setNumBuffers(int numBuffers);
    bool isStopRequested() const;

    CameraInfo cameraInfo() const;
    QString cameraInfoString() const;

public slots:
    void start();
    void stop();

signals:
    void frameFinished(FrameInfo frameInfo);
    void info(const QString &infoString) const;
    void error(const QString &errorString) const;

protected:
    void allocateFrames();
    void clearFrameQueues();
    void run();

private:
    Q_DISABLE_COPY(Recorder)
    mutable QMutex m_cameraMutex;
    mutable QMutex m_queueMutex;
    mutable QReadWriteLock m_stopRequestLock;
    Camera * const m_camera;
    CameraInfo m_cameraInfo;
    QQueue<tPvFrame *> m_cameraQueue;
    QQueue<tPvFrame *> m_inputQueue;
    QQueue<tPvFrame *> m_outputQueue;
    bool m_stopRequested;
    int m_numBuffers;
};

inline bool Recorder::isStopRequested() const {
    QReadLocker locker(&m_stopRequestLock);
    return m_stopRequested;
}

#endif // SJCAM_RECORDER_H
