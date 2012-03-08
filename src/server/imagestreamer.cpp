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

#include "imagestreamer.h"
#include <QtCore/QtCore>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

ImageStreamer::ImageStreamer(QObject *parent)
    : QObject(parent),
      m_tcpServer(new QTcpServer)
{
    m_colorTable.resize(256);
    for (int j = 0; j < 256; ++j)
        m_colorTable[j] = qRgb(j, j, j);
    m_image.setColorTable(m_colorTable);
}

ImageStreamer::~ImageStreamer()
{
    delete m_tcpServer;
    foreach (QTcpSocket *socket, m_socketList)
        delete socket;
}

void ImageStreamer::processFrame(tPvFrame *frame)
{
    if (!frame)
        return;

    renderImage(frame);

    m_image.save(QString("img_%1.jpg").arg(frame->FrameCount, 4, 10, QChar('0')));
    //m_image.save("img_x.jpg");

    emit frameFinished(frame);
}

void ImageStreamer::renderImage(tPvFrame *frame)
{
    Q_ASSERT(frame);
    const int width = int(frame->Width);
    const int height = int(frame->Height);
    const int bitDepth = int(frame->BitDepth);  // 8 or 12

    if (m_image.width() != width || m_image.height() != height) {
        m_image = QImage(width, height, QImage::Format_Indexed8);
        m_image.setColorTable(m_colorTable);
    }

    if (bitDepth == 8)
    {
        const uchar * const buffer = reinterpret_cast<uchar *>(
                    frame->ImageBuffer);
        for (int i = 0; i < height; ++i)
        {
            const uchar *bufferLine = buffer + (i * width);
            qMemCopy(m_image.scanLine(i), bufferLine, width);
        }
    }
    else if (bitDepth == 12)
    {
        const quint16 * const buffer = reinterpret_cast<quint16 *>(
                    frame->ImageBuffer);
        for (int i = 0; i < height; ++i)
        {
            const quint16 * const bufferLine = buffer + (i * width);
            uchar * const imageLine = m_image.scanLine(i);
            for (int j = 0; j < width; ++j)
                imageLine[j] = uchar(bufferLine[j] >> 4);
        }
    }
    else
    {
        m_image.fill(0);
        emit error("Cannot render image, unsupported bit depth.");
    }
}
