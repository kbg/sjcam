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
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <fitsio.h>

ImageWriter::ImageWriter(QObject *parent)
    : QObject(parent),
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

void ImageWriter::writeNextFrames(int count, int stepping)
{
    m_count = count > 0 ? count : 0;
    m_stepping = stepping > 1 ? stepping : 1;
    m_i = 0;
}

bool ImageWriter::writeFrame(tPvFrame *frame)
{
    Q_ASSERT(frame);

    QDateTime now = QDateTime::currentDateTimeUtc();
    QString fileName = QString("%1_%2.fits")
            .arg(m_fileNamePrefix)
            .arg(now.toString("yyyyMMdd-hhmmsszzz"));
    QString fullFileName = m_directory.absoluteFilePath(fileName);

    int errcode = 0;
    fitsfile *ff = 0;
    fits_create_diskfile(&ff, fullFileName.toAscii(), &errcode);
    if (errcode) {
        sendError("Cannot create the file '" + fullFileName + "'.", errcode);
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

    //! \todo Insert some more header entries.

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
