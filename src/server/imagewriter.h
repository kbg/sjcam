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

#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <PvApi.h>

class ImageWriter : public QObject
{
    Q_OBJECT

public:
    explicit ImageWriter(QObject *parent = 0);
    ~ImageWriter();

    // these methods are NOT thread-safe!
    void setDirectory(const QString &directory);
    void setFileNamePrefix(const QString &prefix);

public slots:
    void processFrame(tPvFrame *frame);
    void writeNextFrames(int count, int stepping);

signals:
    void frameFinished(tPvFrame *frame);
    void frameWritten(int n, int total);
    void info(const QString &infoString) const;
    void error(const QString &errorString) const;

protected:
    bool writeFrame(tPvFrame *frame);
    QString fitsioErrorString(int errcode) const;
    void sendError(const QString &msg, int errcode = 0) const;

private:
    Q_DISABLE_COPY(ImageWriter)
    QDir m_directory;
    QString m_fileNamePrefix;
    int m_count;
    int m_stepping;
    int m_i;
};

#endif // SJCAM_IMAGEWRITER_H
