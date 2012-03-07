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

#include "recorder.h"
#include "camera.h"
#include "pvutils.h"
#include <QtCore/QtCore>

Recorder::Recorder(QObject *parent)
    : QThread(parent),
      m_camera(new Camera)
{
}

Recorder::~Recorder()
{
    if (isRunning()) {
        stop();
        wait();
        closeCamera();
    }

    delete m_camera;
}

bool Recorder::openCamera(ulong cameraId)
{
    if (isRunning()) {
        emit error("Cannot open camera while recorder is running.");
        return false;
    }

    emit info("Opening camera...");

    QMutexLocker locker(&m_cameraMutex);
    if (!m_camera->open(cameraId)) {
        emit error(m_camera->errorString());
        return false;
    }
    if (!m_camera->resetConfig()) {
        emit error(m_camera->errorString());
        m_camera->close();
        return false;
    }

    emit info("\n" + m_camera->infoString() + "\n");

    return true;
}

bool Recorder::closeCamera()
{
    if (isRunning()) {
        emit error("Cannot close camera while recorder is running.");
        return false;
    }

    QMutexLocker locker(&m_cameraMutex);
    if (m_camera->isOpen())
        emit info("Closing camera.");
    m_camera->close();
}

bool Recorder::isCameraOpen() const
{
    QMutexLocker locker(&m_cameraMutex);
    return m_camera->isOpen();
}

bool Recorder::getAttribute(const QByteArray &name, QVariant *value) const
{
    QMutexLocker locker(&m_cameraMutex);
    if (!m_camera->getAttribute(name, value)) {
        emit error(m_camera->errorString());
        return false;
    }
    return true;
}

bool Recorder::setAttribute(const QByteArray &name, const QVariant &value)
{
    QMutexLocker locker(&m_cameraMutex);
    if (!m_camera->setAttribute(name, value)) {
        emit error(m_camera->errorString());
        return false;
    }
    return true;
}

void Recorder::start()
{
    if (isRunning())
        return;

    m_stopRequestLock.lockForWrite();
    m_stopRequested = false;
    m_stopRequestLock.unlock();

    QThread::start();
}

void Recorder::stop()
{
    QWriteLocker locker(&m_stopRequestLock);
    m_stopRequested = true;
}

void Recorder::run()
{
    QList<tPvFrame *> frameList;

// +++ queue
    m_queueMutex.lock();

    // read all frames from the input queue
    frameList.reserve(m_inputQueue.size());
    while (!m_inputQueue.isEmpty())
        frameList.append(m_inputQueue.dequeue());

    m_queueMutex.unlock();
// --- queue

// +++ cam
    m_cameraMutex.lock();

    // test
    uint width = m_camera->sensorWidth();
    uint height = m_camera->sensorHeight();
    ulong bufferSize = 2L * width * height;
    for (int i = 0; i < 10; ++i)
        frameList.append(allocPvFrame(bufferSize));

    if (!m_camera->startCapturing()) {
        emit error(m_camera->errorString());
        m_cameraMutex.unlock();
        return;
    }

    // enqueue all frames from the input queue to the camera queue
    foreach (tPvFrame *frame, frameList) {
        m_cameraQueue.enqueue(frame);
        if (!m_camera->enqueueFrame(frame))
            emit error(m_camera->errorString());
    }
    frameList.clear();

    if (!m_camera->startAcqusition()) {
        emit error(m_camera->errorString());
        if (!m_camera->clearFrameQueue())
            emit error(m_camera->errorString());
        if (!m_camera->stopCapturing())
            emit error(m_camera->errorString());
        m_cameraMutex.unlock();
        return;
    }

    m_cameraMutex.unlock();
// --- cam

    ulong i = 1;
    while (!isStopRequested())
    {
// +++ cam
        m_cameraMutex.lock();

        if (m_cameraQueue.isEmpty()) {
            emit error("Capture queue is empty.");
            m_cameraMutex.unlock();
            continue;
        }

        bool timeout;
        tPvFrame *frame = m_cameraQueue.dequeue();
        if (m_camera->waitForFrameDone(frame, 100, &timeout)) {
            emit frameDone(i, frame->Status);
            m_cameraQueue.enqueue(frame);
            if (!m_camera->enqueueFrame(frame))
                emit error(m_camera->errorString());
        } else {
            if (!timeout)
                emit error(m_camera->errorString());
            m_cameraQueue.prepend(frame);
        }


        m_cameraMutex.unlock();
// --- cam

        msleep(1);
    }

// +++ cam
    m_cameraMutex.lock();

    if (!m_camera->stopAcquisition())
        emit error(m_camera->errorString());

    // aborts all frames in the camera queue; with frame->Status
    // ePvErrDataMissing or ePvErrCancelled
    if (!m_camera->clearFrameQueue())
        emit error(m_camera->errorString());

    if (!m_camera->stopCapturing())
        emit error(m_camera->errorString());

    m_cameraMutex.unlock();
// --- cam
}
