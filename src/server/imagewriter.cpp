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

#include "imagewriter.h"
#include "pvutils.h"
#include "version.h"
#include <QtCore/QDateTime>
#include <QtCore/QtEndian>
#include <QtCore/QDebug>

ImageWriter::ImageWriter(QObject *parent)
    : QObject(parent),
      m_markerEnabled(false),
      m_markerPos(0, 0),
      m_count(0),
      m_stepping(1),
      m_i(0)
{
}

ImageWriter::~ImageWriter()
{
}

void ImageWriter::setDirectory(const QString &directory)
{
    m_directory = QDir(directory);
}

void ImageWriter::setFileNamePrefix(const QString &prefix)
{
    m_fileNamePrefix = prefix;
}

void ImageWriter::processFrame(tPvFrame *frame)
{
    if (frame && (frame->Status == ePvErrSuccess)) {
        if (m_i < m_count * m_stepping) {
            if (m_i % m_stepping == 0)
                if (writeFrame(frame))
                    emit frameWritten(m_i / m_stepping + 1, m_count);
            m_i++;
        }
    }
    emit frameFinished(frame);
}

void ImageWriter::setDeviceName(const QByteArray &deviceName)
{
    m_deviceName = deviceName;
}

void ImageWriter::setTelescopeName(const QByteArray &telescopeName)
{
    m_telescopeName = telescopeName;
}

void ImageWriter::writeNextFrames(int count, int stepping)
{
    m_count = count > 0 ? count : 0;
    m_stepping = stepping > 1 ? stepping : 1;
    m_i = 0;
}

void ImageWriter::setCameraInfo(const CameraInfo &cameraInfo)
{
    m_cameraInfo = cameraInfo;
}

void ImageWriter::setMarkerPos(const QVariant &markerPos)
{
    if (markerPos.isValid()) {
        m_markerPos = markerPos.toPointF();
        m_markerEnabled = true;
    } else {
        m_markerEnabled = false;
    }
}

bool ImageWriter::writeFrame(tPvFrame *frame)
{
    Q_ASSERT(frame);

    QDateTime now = QDateTime::currentDateTimeUtc();
    QString fileName = QString("%1_%2.fits")
            .arg(m_fileNamePrefix)
            .arg(now.toString("yyyyMMdd-hhmmsszzz"));
    QString tempFileName = fileName + ".tmp";
    QString fullTempFileName = m_directory.absoluteFilePath(tempFileName);

    int errcode = 0;
    fitsfile *ff = 0;
    fits_create_diskfile(&ff, fullTempFileName.toAscii(), &errcode);
    if (errcode) {
        sendError("Cannot create the file '" + fullTempFileName + "'.", errcode);
        if (ff) fits_close_file(ff, &errcode);
        return false;
    }

    long naxes[2];
    naxes[0] = long(frame->Width);
    naxes[1] = long(frame->Height);
    int imageType = (frame->BitDepth == 8) ? BYTE_IMG : SHORT_IMG;
    fits_create_img(ff, imageType, 2, naxes, &errcode);
    if (errcode) {
        sendError("Cannot allocate file space.", errcode);
        fits_close_file(ff, &errcode);
        return false;
    }

    // try to remove the 2 default comments entries from the header
    fits_delete_key(ff, "COMMENT", &errcode);
    fits_delete_key(ff, "COMMENT", &errcode);

    writeKey(ff, "CREATOR", QByteArray("SjcServer v") + SJCAM_VERSION_STRING,
             "program that created this file");
    writeKey(ff, "DATE", now.toString("yyyy-MM-ddThh:mm:ss.zzz").toAscii(),
             "[utc] file creation time");
    writeKey(ff, "FILENAME", fileName.toAscii(), "original file name");
    writeKey(ff, "STATUS", "raw", "file status");

    writeKey(ff, "INSTRUME", m_deviceName, "instrument");
    if (!m_telescopeName.isEmpty())
        writeKey(ff, "TELESCOP", m_telescopeName, "telescope name");

    writeKey(ff, "CAMMODEL", m_cameraInfo.pvCameraInfo.ModelName,
             "camera model name");
    writeKey(ff, "CAMSERNO", m_cameraInfo.pvCameraInfo.SerialNumber,
             "camera serial number");
    writeKey(ff, "CAMHWADR", m_cameraInfo.hwAddress.replace('-', ':'),
             "camera hardware address");
    writeKey(ff, "CAMFWVER", m_cameraInfo.pvCameraInfo.FirmwareVersion,
             "camera firmware version");

    writeKey(ff, "FRAME-NO", frame->FrameCount, "frame number (rolls at 65535)");

    uint tsFreq = m_cameraInfo.timeStampFrequency;
    if (tsFreq == 0) tsFreq = 1;
    writeKey(ff, "TIMESTAM", PvFrameTimestamp(frame, tsFreq, 1e6),
             "[us] time stamp (time since camera power on)");

    if (frame->AncillaryBuffer && frame->AncillarySize >= 12) {
        quint32 *buf = reinterpret_cast<quint32 *>(frame->AncillaryBuffer);
        writeKey(ff, "EXPTIME", qFromBigEndian(buf[2]), "[us] exposure time");
    }
    writeKey(ff, "BITDEPTH", frame->BitDepth, "significant bits per pixel");

    if (m_markerEnabled) {
        writeKey(ff, "MARKER-X", m_markerPos.x(),
                 "marker x-coordinate [0, width-1]");
        writeKey(ff, "MARKER-Y", m_markerPos.y(),
                 "marker y-coordinate [0, height-1]");
    }

    //! \todo Add more header entries.

    long fpixel[2] = { 1, 1 };
    LONGLONG nelem = naxes[0] * naxes[1];
    int dataType = (frame->BitDepth == 8) ? TBYTE : TSHORT;
    fits_write_pix(ff, dataType, fpixel, nelem, frame->ImageBuffer, &errcode);
    if (errcode) {
        sendError("Cannot write frame.", errcode);
        fits_close_file(ff, &errcode);
        return false;
    }

    fits_close_file(ff, &errcode);
    if (errcode) {
        sendError("Cannot close file.", errcode);
        return false;
    }

    if (!m_directory.rename(tempFileName, fileName)) {
        sendError("Cannot rename temporary file.");
        return false;
    }

    return true;
}

bool ImageWriter::writeKey(fitsfile *ff, int datatype, const char *keyname,
                           void *value, const char *comment)
{
    int errcode = 0;
    const char *comm = (comment == 0 || *comment == '\0') ? 0 : comment;
    fits_write_key(ff, datatype, keyname, value, comm, &errcode);
    if (errcode != 0) {
        sendError("Cannot write FITS header entry.", errcode);
        return false;
    }
    return true;
}

QString ImageWriter::fitsioErrorString(int errcode) const
{
    char fitsioMsg[31];  // message has max 30 chars
    fits_get_errstatus(errcode, fitsioMsg);
    return QString(fitsioMsg);
}

void ImageWriter::sendError(const QString &msg, int errcode) const
{
    QString fullMsg = msg;
    if (errcode != 0)
        fullMsg += " FITSIO: " + fitsioErrorString(errcode) + ".";
    emit error(fullMsg);
}
