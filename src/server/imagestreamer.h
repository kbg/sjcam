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

#ifndef SJCAM_IMAGESTREAMER_H
#define SJCAM_IMAGESTREAMER_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtGui/QImage>
#include <PvApi.h>

class QString;
class QTcpServer;
class QTcpSocket;

class ImageStreamer : public QObject
{
    Q_OBJECT

public:
    explicit ImageStreamer(QObject *parent = 0);
    ~ImageStreamer();

public slots:
    void processFrame(tPvFrame *frame);

signals:
    void frameFinished(tPvFrame *frame);
    void info(const QString &infoString) const;
    void error(const QString &errorString) const;

protected:
    void renderImage(tPvFrame *frame);

private:
    Q_DISABLE_COPY(ImageStreamer)
    QTcpServer * const m_tcpServer;
    QList<QTcpSocket *> m_socketList;
    QImage m_image;
    QVector<QRgb> m_colorTable;
};

#endif // SJCAM_IMAGESTREAMER_H
