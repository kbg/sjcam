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

#include "camera.h"
#include "pvutils.h"
#include <QtCore/QtCore>

Camera::Camera()
    : m_device(0),
      m_sensorWidth(0),
      m_sensorHeight(0),
      m_sensorBits(0)
{
    qMemSet(&m_cameraInfo, 0, sizeof(m_cameraInfo));
}

Camera::~Camera()
{
    close();
}

bool Camera::open(ulong cameraId)
{
    clearError();

    if (isOpen())
        close();

    CameraInfoList cameraList = availablePvCameras();
    if (cameraList.isEmpty()) {
        setError("No camera found.");
        return false;
    }

    // check if the camera with the given id is available
    if (cameraId != 0) {
        bool cameraFound = false;
        foreach (const tPvCameraInfoEx &info, cameraList)
            if (info.UniqueId == cameraId) {
                cameraFound = true;
                break;
            }
        if (!cameraFound) {
            setError("No camera with unique ID " + QString::number(cameraId)
                     + " found.");
            return false;
        }
    }

    // try to open the camera with the given cameraId, or the first camera
    // with master access.
    tPvErr err = ePvErrSuccess;
    foreach (const tPvCameraInfoEx &info, cameraList) {
        if ((cameraId == 0 || cameraId == info.UniqueId) &&
                (info.PermittedAccess & ePvAccessMaster) != 0)
        {
            err = PvCameraOpen(info.UniqueId, ePvAccessMaster, &m_device);
            if (err != ePvErrSuccess)
                m_device = 0;
            else
                m_cameraInfo = info;
        }
    }

    if (!m_device) {
        clearInfo();
        setError("Cannot open camera.", err);
        return false;
    }

    QByteArray sensorType;
    if (!getAttrEnum("SensorType", &sensorType)) {
        close();
        return false;
    }
    else if (sensorType != "Mono") {
        setError("Sensor type '" + sensorType + "' is not supported.");
        close();
        return false;
    }

    if (!getAttrString("DeviceIPAddress", &m_ipAddress) ||
            !getAttrString("DeviceEthAddress", &m_hwAddress) ||
            !getAttrUint32("SensorWidth", &m_sensorWidth) ||
            !getAttrUint32("SensorHeight", &m_sensorHeight) ||
            !getAttrUint32("SensorBits", &m_sensorBits))
    {
        close();
        return false;
    }

    return true;
}

void Camera::close()
{
    if (m_device != 0) {
        PvCameraClose(m_device);
        m_device = 0;
        clearInfo();
    }
}

bool Camera::resetConfig()
{
    tPvErr err;

    err = PvAttrEnumSet(m_device, "ConfigFileIndex", "Factory");
    if (err != ePvErrSuccess) {
        setError("Cannot select factory settings.", err);
        return false;
    }

    err = PvCommandRun(m_device, "ConfigFileLoad");
    if (err != ePvErrSuccess) {
        setError("Cannot load factory settings.", err);
        return false;
    }

    return true;
}

bool Camera::startCapturing()
{
    tPvErr err = PvCaptureStart(m_device);
    if (err != ePvErrSuccess) {
        setError("Cannot start capturing.", err);
        return false;
    }
    return true;
}

bool Camera::stopCapturing()
{
    tPvErr err = PvCaptureEnd(m_device);
    if (err != ePvErrSuccess) {
        setError("Cannot stop capturing.", err);
        return false;
    }
    return true;
}

bool Camera::isCapturing() const
{
    ulong isStarted;
    tPvErr err = PvCaptureQuery(m_device, &isStarted);
    return (err == ePvErrSuccess) && isStarted;
}

bool Camera::enqueueFrame(tPvFrame *frame)
{
    tPvErr err = PvCaptureQueueFrame(m_device, frame, 0);
    if (err != ePvErrSuccess) {
        setError("Cannot enqueue frame.", err);
        return false;
    }
    return true;
}

bool Camera::clearFrameQueue()
{
    tPvErr err = PvCaptureQueueClear(m_device);
    if (err != ePvErrSuccess) {
        setError("Cannot clear frame queue.", err);
        return false;
    }
    return true;
}

bool Camera::startAcqusition()
{
    tPvErr err = PvCommandRun(m_device, "AcquisitionStart");
    if (err != ePvErrSuccess) {
        setError("Cannot start image acquisition.", err);
        return false;
    }
    return true;
}

bool Camera::stopAcquisition()
{
    tPvErr err = PvCommandRun(m_device, "AcquisitionStop");
    if (err != ePvErrSuccess) {
        setError("Cannot stop image acquisition.", err);
        return false;
    }
    return true;
}

bool Camera::waitForFrameDone(tPvFrame *frame, ulong timeout, bool *timedOut)
{
    tPvErr err = PvCaptureWaitForFrameDone(m_device, frame, timeout);
    if (timedOut)
        *timedOut = (err == ePvErrTimeout);
    if (err != ePvErrSuccess) {
        setError("Failed to wait for frame.", err);
        return false;
    }
    return true;

}

bool Camera::runCommand(const QByteArray &name)
{
    tPvErr err = PvCommandRun(m_device, name);
    if (err != ePvErrSuccess) {
        setError(QString("Cannot run command %1.").arg(QString(name)), err);
        return false;
    }
    return true;
}

bool Camera::getAttrString(const QByteArray &name, QByteArray *value) const
{
    char buf[32];
    tPvErr err = PvAttrStringGet(m_device, name, buf, 32, 0);
    if (err != ePvErrSuccess) {
        *value = QByteArray();
        setError(QString("Cannot get attribute %1.").arg(QString(name)), err);
        return false;
    }
    *value = QByteArray(buf);
    return true;
}

bool Camera::setAttrString(const QByteArray &name, const QByteArray &value)
{
    tPvErr err = PvAttrStringSet(m_device, name, value);
    if (err != ePvErrSuccess) {
        setError(QString("Cannot set attribute %1.").arg(QString(name)), err);
        return false;
    }
    return true;
}

bool Camera::getAttrEnum(const QByteArray &name, QByteArray *value) const
{
    char buf[32];
    tPvErr err = PvAttrEnumGet(m_device, name, buf, 32, 0);
    if (err != ePvErrSuccess) {
        *value = QByteArray();
        setError(QString("Cannot get attribute %1.").arg(QString(name)), err);
        return false;
    }
    *value = QByteArray(buf);
    return true;
}

bool Camera::setAttrEnum(const QByteArray &name, const QByteArray &value)
{
    tPvErr err = PvAttrEnumSet(m_device, name, value);
    if (err != ePvErrSuccess) {
        setError(QString("Cannot set attribute %1.").arg(QString(name)), err);
        return false;
    }
    return true;
}

bool Camera::getAttrUint32(const QByteArray &name, quint32 *value) const
{
    tPvUint32 pvval;
    tPvErr err = PvAttrUint32Get(m_device, name, &pvval);
    if (err != ePvErrSuccess) {
        *value = 0;
        setError(QString("Cannot get attribute %1.").arg(QString(name)), err);
        return false;
    }
    *value = pvval;
    return true;
}

bool Camera::setAttrUint32(const QByteArray &name, quint32 value)
{
    tPvErr err = PvAttrUint32Set(m_device, name, value);
    if (err != ePvErrSuccess) {
        setError(QString("Cannot set attribute %1.").arg(QString(name)), err);
        return false;
    }
    return true;
}

bool Camera::getAttrFloat32(const QByteArray &name, float *value) const
{
    tPvFloat32 pvval;
    tPvErr err = PvAttrFloat32Get(m_device, name, &pvval);
    if (err != ePvErrSuccess) {
        *value = 0;
        setError(QString("Cannot get attribute %1.").arg(QString(name)), err);
        return false;
    }
    *value = pvval;
    return true;
}

bool Camera::setAttrFloat32(const QByteArray &name, float value)
{
    tPvErr err = PvAttrFloat32Set(m_device, name, value);
    if (err != ePvErrSuccess) {
        setError(QString("Cannot set attribute %1.").arg(QString(name)), err);
        return false;
    }
    return true;
}

bool Camera::getAttrInt64(const QByteArray &name, qint64 *value) const
{
    tPvInt64 pvval;
    tPvErr err = PvAttrInt64Get(m_device, name, &pvval);
    if (err != ePvErrSuccess) {
        *value = 0;
        setError(QString("Cannot get attribute %1.").arg(QString(name)), err);
        return false;
    }
    *value = pvval;
    return true;
}

bool Camera::setAttrInt64(const QByteArray &name, qint64 value)
{
    tPvErr err = PvAttrInt64Set(m_device, name, value);
    if (err != ePvErrSuccess) {
        setError(QString("Cannot set attribute %1.").arg(QString(name)), err);
        return false;
    }
    return true;
}

bool Camera::getAttrBoolean(const QByteArray &name, bool *value) const
{
    tPvBoolean pvval;
    tPvErr err = PvAttrBooleanGet(m_device, name, &pvval);
    if (err != ePvErrSuccess) {
        *value = false;
        setError(QString("Cannot get attribute %1.").arg(QString(name)), err);
        return false;
    }
    *value = pvval ? true : false;
    return true;
}

bool Camera::setAttrBoolean(const QByteArray &name, bool value)
{
    tPvErr err = PvAttrBooleanSet(m_device, name, value ? 1 : 0);
    if (err != ePvErrSuccess) {
        setError(QString("Cannot set attribute %1.").arg(QString(name)), err);
        return false;
    }
    return true;
}

bool Camera::getAttribute(const QByteArray &name, QVariant *value) const
{
    tPvAttributeInfo info;
    tPvErr err = PvAttrInfo(m_device, name, &info);

    if (err == ePvErrSuccess)
    {
        switch(info.Datatype)
        {
        case ePvDatatypeString: {
            QByteArray v;
            bool ok = getAttrString(name, &v);
            *value = ok ? v : QVariant();
            return ok;
        }
        case ePvDatatypeEnum: {
            QByteArray v;
            bool ok = getAttrEnum(name, &v);
            *value = ok ? v : QVariant();
            return ok;
        }
        case ePvDatatypeUint32: {
            quint32 v;
            bool ok = getAttrUint32(name, &v);
            *value = ok ? v : QVariant();
            return ok;
        }
        case ePvDatatypeFloat32: {
            float v;
            bool ok = getAttrFloat32(name, &v);
            *value = ok ? v : QVariant();
            return ok;
        }
        case ePvDatatypeInt64: {
            qint64 v;
            bool ok = getAttrInt64(name, &v);
            *value = ok ? v : QVariant();
            return ok;
        }
        case ePvDatatypeBoolean: {
            bool v;
            bool ok = getAttrBoolean(name, &v);
            *value = ok ? v : QVariant();
            return ok;
        }
        default:
            break;
        }
    }

    *value = QVariant();
    setError(QString("Cannot get attribute %1.").arg(QString(name)), err);
    return false;
}

bool Camera::setAttribute(const QByteArray &name, const QVariant &value)
{
    tPvAttributeInfo info;
    tPvErr err = PvAttrInfo(m_device, name, &info);

    if (err == ePvErrSuccess)
    {
        switch(info.Datatype)
        {
        case ePvDatatypeString:
            return setAttrString(name, value.toByteArray());
        case ePvDatatypeEnum:
            return setAttrString(name, value.toByteArray());
        case ePvDatatypeUint32: {
            bool ok;
            quint32 v = value.toUInt(&ok);
            if (ok) return setAttrUint32(name, v);
            break;
        }
        case ePvDatatypeFloat32: {
            bool ok;
            float v = value.toFloat(&ok);
            if (ok) return setAttrFloat32(name, v);
            break;
        }
        case ePvDatatypeInt64: {
            bool ok;
            qint64 v = value.toLongLong(&ok);
            if (ok) return setAttrInt64(name, v);
            break;
        }
        case ePvDatatypeBoolean:
            return setAttrBoolean(name, value.toBool());
        default:
            break;
        }
    }

    setError(QString("Cannot set attribute %1.").arg(QString(name)), err);
    return false;
}

QString Camera::infoString() const
{
    QString result;
    QTextStream ts(&result);
    ts << "Camera infos:"
       << "\n    UniqueId .......... " << m_cameraInfo.UniqueId
       << "\n    CameraName ........ " << m_cameraInfo.CameraName
       << "\n    ModelName ......... " << m_cameraInfo.ModelName
       << "\n    SerialNumber ...... " << m_cameraInfo.SerialNumber
       << "\n    FirmwareVersion ... " << m_cameraInfo.FirmwareVersion
       << "\n    HwAddress ......... " << m_hwAddress
       << "\n    IpAddress ......... " << m_ipAddress
       << "\n    Sensor ............ " << m_sensorWidth << "x"
                                       << m_sensorHeight << "@"
                                       << m_sensorBits;
    return result;
}

void Camera::clearInfo()
{
    qMemSet(&m_cameraInfo, 0, sizeof(m_cameraInfo));
    m_hwAddress.clear();
    m_ipAddress.clear();
    m_sensorWidth = 0;
    m_sensorHeight = 0;
    m_sensorBits = 0;
}

void Camera::setError(const QString &errorString, tPvErr errorCode) const
{
    m_errorString = (errorCode == ePvErrSuccess) ?
                errorString : formatErrorMessage(errorString, errorCode);
}

void Camera::clearError() const
{
    m_errorString.clear();
}
