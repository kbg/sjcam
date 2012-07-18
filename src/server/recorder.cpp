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
      m_camera(new Camera),
      m_stopRequested(false),
      m_numBuffers(10)
{
}

Recorder::~Recorder()
{
    if (isRunning()) {
        stop();
        wait();
    }

    closeCamera();
    delete m_camera;
}

bool Recorder::openCamera(ulong cameraId)
{
    if (isRunning()) {
        emit error("Cannot open camera while recorder is running.");
        return false;
    }

    emit info("Opening camera...");

    m_cameraMutex.lock();
    if (!m_camera->open(cameraId)) {
        emit error(m_camera->errorString());
        m_cameraMutex.unlock();
        return false;
    }
    if (!m_camera->resetConfig()) {
        emit error(m_camera->errorString());
        m_camera->close();
        m_cameraMutex.unlock();
        return false;
    }
    m_cameraInfo.pvCameraInfo = m_camera->cameraInfo();
    m_cameraInfo.hwAddress = m_camera->hwAddress().toAscii();
    m_cameraInfo.ipAddress = m_camera->ipAddress().toAscii();
    m_cameraInfo.sensorWidth = m_camera->sensorWidth();
    m_cameraInfo.sensorHeight = m_camera->sensorHeight();
    m_cameraInfo.sensorBits = m_camera->sensorBits();
    quint32 timeStampFrequency;
    if (!m_camera->getAttrUint32("TimeStampFrequency", &timeStampFrequency)) {
        emit error(m_camera->errorString());
        m_camera->close();
        m_cameraInfo.clear();
        m_cameraMutex.unlock();
        return false;
    }
    m_cameraInfo.timeStampFrequency = timeStampFrequency;
    ulong camId = m_cameraInfo.pvCameraInfo.UniqueId;
    m_cameraMutex.unlock();

    emit info(QString("Camera opened [%1].").arg(camId));

    allocateFrames();
    return true;
}

bool Recorder::closeCamera()
{
    if (isRunning()) {
        emit error("Cannot close camera while recorder is running.");
        return false;
    }

    m_cameraMutex.lock();
    if (m_camera->isOpen())
        emit info("Closing camera.");
    m_camera->close();
    m_cameraInfo.clear();
    m_cameraMutex.unlock();

    clearFrameQueues();
    return true;
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

bool Recorder::getFrameStats(float &fps, uint &completed, uint &dropped)
{
    QMutexLocker locker(&m_cameraMutex);
    if (!m_camera->getFrameStats(fps, completed, dropped)) {
        emit error(m_camera->errorString());
        return false;
    }
    return true;
}

int Recorder::numBuffers() const
{
    QMutexLocker locker(&m_cameraMutex);
    return m_numBuffers;
}

bool Recorder::setNumBuffers(int numBuffers)
{
    QMutexLocker locker(&m_cameraMutex);
    if (m_camera->isOpen()) {
        emit error("Cannot set number of buffers while camera is opened.");
        return false;
    }
    m_numBuffers = (numBuffers >= 1 ? numBuffers : 1);
    return true;
}

CameraInfo Recorder::cameraInfo() const
{
    QMutexLocker locker(&m_cameraMutex);
    return m_cameraInfo;
}

QString Recorder::cameraInfoString() const
{
    QMutexLocker locker(&m_cameraMutex);
    return m_camera->infoString();
}

bool Recorder::hasFinishedFrame() const
{
    QMutexLocker locker(&m_queueMutex);
    return !m_outputQueue.isEmpty();
}

tPvFrame * Recorder::readFinishedFrame()
{
    QMutexLocker locker(&m_queueMutex);
    return m_outputQueue.isEmpty() ? 0 : m_outputQueue.dequeue();
}

void Recorder::enqueueFrame(tPvFrame *frame)
{
    Q_ASSERT(frame);
    QMutexLocker locker(&m_queueMutex);
    return m_inputQueue.enqueue(frame);
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

void Recorder::allocateFrames()
{
    Q_ASSERT(!isRunning());

    // create frames with the full sensor size and 2 bytes per pixel to make
    // sure that all possible frames fit to the allocated buffers
    uint width = m_camera->sensorWidth();
    uint height = m_camera->sensorHeight();
    ulong bufferSize = 2L * width * height;

    m_queueMutex.lock();
    for (int i = 0; i < m_numBuffers; ++i)
        m_inputQueue.enqueue(allocPvFrame(bufferSize));
    m_queueMutex.unlock();
}

void Recorder::clearFrameQueues()
{
    Q_ASSERT(!isRunning());

    m_cameraMutex.lock();
    while (!m_cameraQueue.isEmpty())
        freePvFrame(m_cameraQueue.dequeue());
    m_cameraMutex.unlock();

    m_queueMutex.lock();
    while (!m_inputQueue.isEmpty())
        freePvFrame(m_inputQueue.dequeue());
    while (!m_outputQueue.isEmpty())
        freePvFrame(m_outputQueue.dequeue());
    m_queueMutex.unlock();
}

/*
  == Capture Loop ==

     Start capturing
     Register frames in camera queue
     Move and register frames: input queue -> camera queue
     Start acquisition

     while not done:
         Move and register frames: input queue -> camera queue
         Wait for first frame in camera queue to be done
         Move finished frame: camera queue -> output queue
         emit frameFinished()

     Stop acquisition
     Cancel pending frames (stored in camera queue)
     Stop capturing
*/
void Recorder::run()
{
    // list of frames, used to move frames between queues
    QList<tPvFrame *> frameList;

// +++ queue
    // read all frames from the input queue
    m_queueMutex.lock();
    while (!m_inputQueue.isEmpty())
        frameList.append(m_inputQueue.dequeue());
    m_queueMutex.unlock();
// --- queue

// +++ camera
    m_cameraMutex.lock();
    if (!m_camera->startCapturing()) {
        emit error(m_camera->errorString());
        m_cameraMutex.unlock();
        return;
    }

    // move all frames from the input to the camera queue
    foreach (tPvFrame *frame, frameList)
        m_cameraQueue.enqueue(frame);
    frameList.clear();

    // register all frames in the camera queue
    foreach (tPvFrame *frame, m_cameraQueue) {
        if (!m_camera->enqueueFrame(frame)) {
            emit error(m_camera->errorString());
            m_camera->clearFrameQueue();
            m_camera->stopCapturing();
            m_cameraMutex.unlock();
            return;
        }
    }

    if (!m_camera->startAcqusition()) {
        emit error(m_camera->errorString());
        m_camera->clearFrameQueue();
        m_camera->stopCapturing();
        m_cameraMutex.unlock();
        return;
    }
    m_cameraMutex.unlock();
// --- camera

    QElapsedTimer clock;
    clock.start();

    ulong id = 1;
    while (!isStopRequested())
    {
        // make sure that other threads can get a mutex lock; this seems to
        // be only neccessary in some pathological cases, but waiting 1 ms
        // doesn't hurt considering the maximum possible frame rates
        pvmsleep(1);

// +++ queue
        // read all frames from the input queue
        m_queueMutex.lock();
        while (!m_inputQueue.isEmpty())
            frameList.append(m_inputQueue.dequeue());
        m_queueMutex.unlock();
// --- queue

// +++ camera
        m_cameraMutex.lock();

        // move all frames from the input to the camera queue
        foreach (tPvFrame *frame, frameList)
            m_cameraQueue.enqueue(frame);

        // register new frames
        foreach (tPvFrame *frame, frameList) {
            if (!m_camera->enqueueFrame(frame)) {
                emit error(m_camera->errorString());
                m_camera->stopAcquisition();
                m_camera->clearFrameQueue();
                m_camera->stopCapturing();
                m_cameraMutex.unlock();
                return;
            }
        }
        frameList.clear();

        if (m_cameraQueue.isEmpty()) {
            emit error("Capture queue is empty.");
            m_cameraMutex.unlock();
            pvmsleep(10);
            continue;
        }

        bool timeout = false;
        tPvFrame *frame = m_cameraQueue.first();
        if (!m_camera->waitForFrameDone(frame, 150, &timeout)) {
            if (timeout) {
                m_cameraMutex.unlock();
                continue;
            } else {
                emit error(m_camera->errorString());
                m_camera->stopAcquisition();
                m_camera->clearFrameQueue();
                m_camera->stopCapturing();
                m_cameraMutex.unlock();
                return;
            }
        }
        m_cameraQueue.dequeue();
        FrameInfo frameInfo;
        frameInfo.readoutTimestamp = clock.elapsed();
        frameInfo.readoutTimeMs = QDateTime::currentDateTimeUtc()
                .toMSecsSinceEpoch();
        frameInfo.id = id;
        frameInfo.count = frame->FrameCount;
        frameInfo.status = frame->Status;
        frameInfo.timestamp = PvFrameTimestamp(
                    frame, m_cameraInfo.timeStampFrequency, 1e3);
        m_cameraMutex.unlock();
// --- camera

// +++ queue
        // enqueue the finished frame to the output queue
        m_queueMutex.lock();
        m_outputQueue.enqueue(frame);
        m_queueMutex.unlock();
// --- queue

        emit frameFinished(frameInfo);
        ++id;
    }

// +++ camera
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
// --- camera
}
