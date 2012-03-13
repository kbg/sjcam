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

#ifndef SJCAM_SJCSERVER_H
#define SJCAM_SJCSERVER_H

#include "cmdlineoptions.h"
#include <dcpclient/dcpclient.h>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QTextStream>
#include <QtCore/QVariant>
#include <QtCore/QList>
#include <PvApi.h>

class Recorder;
class ImageStreamer;
class ImageWriter;
class QThread;

struct CamAttr {
    QByteArray name;
    QVariant value;
};

class SjcServer : public QObject
{
    Q_OBJECT

public:
    explicit SjcServer(const CmdLineOptions &opts, QObject *parent = 0);
    ~SjcServer();

public slots:
    bool openCamera();
    void connectToDcpServer();

protected:
    void loadConfigFile();
    bool verbose() { return m_verbose; }
    void sendMessage(const Dcp::Message &message);

protected slots:
    void printInfo(const QString &infoString);
    void printError(const QString &errorString);

    void dcpError(Dcp::Client::Error error);
    void dcpStateChanged(Dcp::Client::State state);
    void dcpMessageReceived();

    void recorderFrameFinished(ulong id, int status);
    void recorderStarted();
    void recorderStopped();

    void streamerThreadStarted();
    void streamerThreadFinished();

    void writerFrameFinished(tPvFrame *frame);
    void writerThreadStarted();
    void writerThreadFinished();

private:
    Q_DISABLE_COPY(SjcServer)
    QTextStream cout;
    Recorder * const m_recorder;
    ImageStreamer * const m_imageStreamer;
    QThread * const m_imageStreamerThread;
    ImageWriter * const m_imageWriter;
    QThread * const m_imageWriterThread;
    Dcp::Client * const m_dcp;
    Dcp::CommandParser m_command;
    QString m_serverName;
    quint16 m_serverPort;
    QByteArray m_deviceName;
    ulong m_cameraId;
    int m_numBuffers;
    QString m_configFileName;
    QList<CamAttr> m_camAttrList;
    bool m_verbose;
};

#endif // SJCAM_SJCSERVER_H
