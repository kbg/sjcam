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

#include "sjcclient.h"
#include "ui_sjcclient.h"
#include <dcpclient/dcpclient.h>
#include <QtCore/QtCore>

SjcClient::SjcClient(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::SjcClient),
      cout(stdout, QIODevice::WriteOnly),
      m_socket(new QTcpSocket),
      m_dcp(new Dcp::Client)
{
    ui->setupUi(this);

    connect(m_dcp, SIGNAL(error(Dcp::Client::Error)),
                   SLOT(dcpError(Dcp::Client::Error)));
    connect(m_dcp, SIGNAL(stateChanged(Dcp::Client::State)),
                   SLOT(dcpStateChanged(Dcp::Client::State)));
    connect(m_dcp, SIGNAL(messageReceived()), SLOT(dcpMessageReceived()));

    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(socketError(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    connect(m_socket, SIGNAL(readyRead()), SLOT(socketReadyRead()));

    m_dcp->connectToServer("localhost", 2001, "sjcclient");
    m_socket->connectToHost("localhost", 4711);
}

SjcClient::~SjcClient()
{
    delete m_socket;
    delete m_dcp;
    delete ui;
}

void SjcClient::dcpError(Dcp::Client::Error error)
{
    Q_UNUSED(error);
    cout << "DCP Error: " << m_dcp->errorString() << "." << endl;
}

void SjcClient::dcpStateChanged(Dcp::Client::State state)
{
    switch (state)
    {
    case Dcp::Client::HostLookupState:
        cout << "Connecting to DCP server [" << m_dcp->serverName() << ":"
             << m_dcp->serverPort() << "]..." << endl;
        break;
    case Dcp::Client::ConnectedState:
        cout << "Connected to DCP server [" << m_dcp->deviceName() << "]."
             << endl;
        break;
    case Dcp::Client::UnconnectedState:
        cout << "Disconnected from DCP server." << endl;
        break;
    default:
        break;
    }
}

void SjcClient::dcpMessageReceived()
{
    Dcp::Message msg = m_dcp->readMessage();
    cout << msg << endl;
}

void SjcClient::socketError(QAbstractSocket::SocketError error)
{
    cout << "Socket Error: " << m_socket->errorString() << "." << endl;
}

void SjcClient::socketStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectedState) {
        cout << "Connected to streaming server." << endl;
        m_socket->write("gimmisome");
    }
    else if (state == QAbstractSocket::UnconnectedState) {
        cout << "Disconnected from streaming server." << endl;
    }
}

void SjcClient::socketReadyRead()
{
    QByteArray buf = m_socket->peek(4);

    quint32 size;
    QDataStream is(&buf, QIODevice::ReadOnly);
    is >> size;

    if (m_socket->bytesAvailable() < size)
        return;

    QByteArray jpeg;
    is.setDevice(m_socket);
    is >> size >> jpeg;

    QImage image = QImage::fromData(jpeg, "jpeg");
    ui->labelImage->setPixmap(QPixmap::fromImage(image));

    m_socket->write("gimmimore");
}
