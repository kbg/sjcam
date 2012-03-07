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
#include <PvApi.h>

class Camera;

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

    bool isStopRequested() const;

public slots:
    void start();
    void stop();

signals:
    void frameDone(ulong id, int status);
    void info(const QString &infoString) const;
    void error(const QString &errorString) const;

protected:
    void run();

private:
    Q_DISABLE_COPY(Recorder)
    mutable QMutex m_cameraMutex;
    mutable QMutex m_queueMutex;
    mutable QReadWriteLock m_stopRequestLock;
    Camera * const m_camera;
    QQueue<tPvFrame *> m_cameraQueue;
    QQueue<tPvFrame *> m_inputQueue;
    QQueue<tPvFrame *> m_outputQueue;
    bool m_stopRequested;
};

inline bool Recorder::isStopRequested() const {
    QReadLocker locker(&m_stopRequestLock);
    return m_stopRequested;
}

#endif // SJCAM_RECORDER_H
