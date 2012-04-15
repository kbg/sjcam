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

#ifndef SJCAM_SJCCLIENT_H
#define SJCAM_SJCCLIENT_H

#include "cmdlineopts.h"
#include <dcpclient/dcpclient.h>
#include <QtGui/QMainWindow>
#include <QtCore/QTextStream>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtNetwork/QTcpSocket>

class QTimer;
class CameraDock;
class RecordingDock;
class HistogramDock;

namespace CamSys {
    class Image;
    class ImageWidget;
    class ImageScrollArea;
}

namespace Ui {
    class SjcClient;
}

struct RequestItem
{
    RequestItem() {}
    explicit RequestItem(const QByteArray &identifier_)
        : identifier(identifier_) { timer.start(); }
    QByteArray identifier;
    QElapsedTimer timer;
};

class SjcClient : public QMainWindow
{
    Q_OBJECT

public:
    explicit SjcClient(const CmdLineOpts &opts, QWidget *parent = 0);
    ~SjcClient();

public slots:
    void connectToServer();
    void disconnectFromServer();

protected:
    void loadSettings();
    void saveSettings();
    void closeEvent(QCloseEvent *event);
    void sendMessage(const Dcp::Message &msg);
    Dcp::Message sendMessage(const QByteArray &data);
    void sendRequest(const QByteArray &data);

protected slots:
    void dcpError(Dcp::Client::Error error);
    void dcpStateChanged(Dcp::Client::State state);
    void dcpConnected();
    void dcpDisconnected();
    void dcpMessageReceived();

    void openButtonClicked(bool checked);
    void captureButtonClicked(bool checked);
    void exposureChanged(double ms);
    void frameRateChanged(double hz);

    void writeFrames(int count, int stepping);

    void socketError(QAbstractSocket::SocketError error);
    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketConnected();
    void socketDisconnected();
    void socketReadyRead();

private slots:
    void histDock_colorSpreadChanged(double minColorValue, double maxColorValue);
    void requestTimer_timeout();
    void on_actionConnect_triggered(bool checked);
    void on_actionAbout_triggered();

private:
    Ui::SjcClient *ui;
    QTextStream cout;
    QTcpSocket *m_socket;
    Dcp::Client * const m_dcp;
    Dcp::ReplyParser m_reply;
    Dcp::CommandParser m_command;
    CamSys::ImageScrollArea *m_scrollArea;
    CamSys::ImageWidget *m_imageWidget;
    CamSys::Image *m_image;
    CameraDock *m_cameraDock;
    RecordingDock *m_recordingDock;
    HistogramDock *m_histogramDock;
    bool m_sjcamAlive;
    QTimer *m_requestTimer;
    int m_requestTimeout;
    QMap<quint32, RequestItem> m_requestMap;
    QString m_serverName;
    quint16 m_serverPort;
    QByteArray m_deviceName;
    QByteArray m_sjcamName;
    QString m_streamingServerName;
    quint16 m_streamingServerPort;
    QString m_configFileName;
    bool m_verbose;
};

#endif // SJCAM_SJCCLIENT_H
