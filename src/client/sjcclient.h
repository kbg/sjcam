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

#include <dcpclient/client.h>
#include <QtGui/QMainWindow>
#include <QtCore/QTextStream>
#include <QtNetwork/QTcpSocket>

class QTcpSocket;

namespace Ui {
    class SjcClient;
}

class SjcClient : public QMainWindow
{
    Q_OBJECT

public:
    explicit SjcClient(QWidget *parent = 0);
    ~SjcClient();

protected slots:
    void dcpError(Dcp::Client::Error error);
    void dcpStateChanged(Dcp::Client::State state);
    void dcpMessageReceived();

    void socketError(QAbstractSocket::SocketError error);
    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketReadyRead();

private:
    Ui::SjcClient *ui;
    QTextStream cout;
    QTcpSocket *m_socket;
    Dcp::Client * const m_dcp;

};

#endif // SJCAM_SJCCLIENT_H
