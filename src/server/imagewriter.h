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

#ifndef SJCAM_IMAGEWRITER_H
#define SJCAM_IMAGEWRITER_H

#include "recorder.h"
#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QVariant>
#include <QtCore/QPointF>
#include <fitsio.h>

class ImageWriter : public QObject
{
    Q_OBJECT

public:
    explicit ImageWriter(QObject *parent = 0);
    ~ImageWriter();

    // these methods are NOT thread-safe!
    void setDirectory(const QString &directory);
    void setFileNamePrefix(const QString &prefix);
    void setDeviceName(const QByteArray &deviceName);
    void setTelescopeName(const QByteArray &telescopeName);

public slots:
    void processFrame(tPvFrame *frame);
    void writeNextFrames(int count, int stepping);
    void setCameraInfo(const CameraInfo &cameraInfo);
    void setMarkerPos(const QVariant &markerPos);

signals:
    void frameFinished(tPvFrame *frame);
    void frameWritten(int n, int total, const QByteArray &fileId);
    void info(const QString &infoString) const;
    void error(const QString &errorString) const;

protected:
    bool writeFrame(tPvFrame *frame);

    bool writeKey(fitsfile *ff, const QByteArray &key, short value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, ushort value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, int value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, uint value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, long value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, ulong value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, qint64 value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, float value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, double value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, const QByteArray &key, QByteArray value, const QByteArray &comment = QByteArray());
    bool writeKey(fitsfile *ff, int datatype, const char *keyname, void *value, const char *comment);

    QString fitsioErrorString(int errcode) const;
    void sendError(const QString &msg, int errcode = 0) const;

private:
    Q_DISABLE_COPY(ImageWriter)
    QDir m_directory;
    QString m_fileNamePrefix;
    QByteArray m_deviceName;
    QByteArray m_telescopeName;
    CameraInfo m_cameraInfo;
    bool m_markerEnabled;
    QPointF m_markerPos;
    int m_count;
    int m_stepping;
    int m_i;
};

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  short value, const QByteArray &comment)
{
    return writeKey(ff, TSHORT, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  ushort value, const QByteArray &comment)
{
    return writeKey(ff, TUSHORT, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  int value, const QByteArray &comment)
{
    return writeKey(ff, TINT, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  uint value, const QByteArray &comment)
{
    return writeKey(ff, TUINT, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  long value, const QByteArray &comment)
{
    return writeKey(ff, TLONG, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  ulong value, const QByteArray &comment)
{
    return writeKey(ff, TULONG, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
              qint64 value, const QByteArray &comment)
{
    return writeKey(ff, TLONGLONG, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  float value, const QByteArray &comment)
{
    return writeKey(ff, TFLOAT, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  double value, const QByteArray &comment)
{
    return writeKey(ff, TDOUBLE, key, &value, comment);
}

inline bool ImageWriter::writeKey(fitsfile *ff, const QByteArray &key,
                                  QByteArray value, const QByteArray &comment)
{
    return writeKey(ff, TSTRING, key, value.data(), comment);
}


#endif // SJCAM_IMAGEWRITER_H
