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
#include "cameradock.h"
#include "recordingdock.h"
#include "histogramdock.h"
#include "version.h"
#include <sjcdata.h>
#include <dcpclient/dcpclient.h>
#include <QtCore/QtCore>
#include <QtNetwork/QHostAddress>

SjcClient::SjcClient(const CmdLineOpts &opts, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::SjcClient),
      cout(stdout, QIODevice::WriteOnly),
      m_socket(new QTcpSocket),
      m_dcp(new Dcp::Client),
      m_cameraDock(new CameraDock),
      m_recordingDock(new RecordingDock),
      m_histogramDock(new HistogramDock),
      m_sjcamAlive(false),
      m_requestTimer(new QTimer),
      m_requestTimeout(10000),
      m_serverPort(0),
      m_sjcamName("sjcam"),
      m_streamingServerPort(0),
      m_verbose(false)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/camera-sj.svg"));

    addDockWidget(Qt::RightDockWidgetArea, m_histogramDock);
    addDockWidget(Qt::RightDockWidgetArea, m_cameraDock);
    addDockWidget(Qt::RightDockWidgetArea, m_recordingDock);

    connect(m_dcp, SIGNAL(error(Dcp::Client::Error)),
                   SLOT(dcpError(Dcp::Client::Error)));
    connect(m_dcp, SIGNAL(stateChanged(Dcp::Client::State)),
                   SLOT(dcpStateChanged(Dcp::Client::State)));
    connect(m_dcp, SIGNAL(connected()), SLOT(dcpConnected()));
    connect(m_dcp, SIGNAL(disconnected()), SLOT(dcpDisconnected()));
    connect(m_dcp, SIGNAL(messageReceived()), SLOT(dcpMessageReceived()));

    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            SLOT(socketError(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    connect(m_socket, SIGNAL(connected()), SLOT(socketConnected()));
    connect(m_socket, SIGNAL(disconnected()), SLOT(socketDisconnected()));
    connect(m_socket, SIGNAL(readyRead()), SLOT(socketReadyRead()));

    connect(m_cameraDock, SIGNAL(openButtonClicked(bool)), SLOT(openButtonClicked(bool)));
    connect(m_cameraDock, SIGNAL(captureButtonClicked(bool)), SLOT(captureButtonClicked(bool)));
    connect(m_cameraDock, SIGNAL(exposureTimeChanged(double)), SLOT(exposureChanged(double)));
    connect(m_cameraDock, SIGNAL(frameRateChanged(double)), SLOT(frameRateChanged(double)));

    connect(m_recordingDock, SIGNAL(writeFrames(int,int)), SLOT(writeFrames(int,int)));
    connect(m_requestTimer, SIGNAL(timeout()), SLOT(requestTimer_timeout()));

    m_configFileName = opts.configFileName;
    loadSettings();

    if (!opts.serverName.isEmpty())
        m_serverName = opts.serverName;
    if (opts.serverPort != 0)
        m_serverPort = opts.serverPort;
    if (!opts.deviceName.isEmpty())
        m_deviceName = opts.deviceName;
    if (opts.verbose != -1)
        m_verbose = (opts.verbose == 0) ? false : true;

    QTimer::singleShot(0, ui->actionConnect, SLOT(trigger()));
}

SjcClient::~SjcClient()
{
    delete m_requestTimer;
    delete m_dcp;
    delete m_socket;
    delete m_cameraDock;
    delete m_recordingDock;
    delete m_histogramDock;
    delete ui;
}

void SjcClient::connectToServer()
{
    m_dcp->connectToServer(m_serverName, m_serverPort, m_deviceName);
}

void SjcClient::disconnectFromServer()
{
    sendMessage("set notify false");
    m_dcp->waitForMessagesWritten(1000);
    m_dcp->disconnectFromServer();
}

void SjcClient::loadSettings()
{
    bool ok;
    QScopedPointer<QSettings> settings(!m_configFileName.isEmpty() ?
        new QSettings(m_configFileName, QSettings::IniFormat) :
        new QSettings(QSettings::IniFormat, QSettings::UserScope,
                      "Kis", "sjcclient"));

    // Dcp Settings
    settings->beginGroup("Dcp");
    m_serverName = settings->value("ServerName", "localhost").toString();

    uint serverPort = settings->value("ServerPort").toUInt(&ok);
    if (ok && serverPort <= 65535)
        m_serverPort = quint16(serverPort);

    m_deviceName = settings->value("DeviceName").toByteArray();
    if (m_deviceName.isEmpty()) {
        QString code = QDateTime::currentDateTimeUtc().toString("hhmmss");
        m_deviceName = "sjcclient" + code.toAscii();
    }
    QByteArray sjcamName = settings->value("SjcamName").toByteArray();
    if (!sjcamName.isEmpty())
        m_sjcamName = sjcamName;
    settings->endGroup();

    // Streaming Settings
    settings->beginGroup("Streaming");
    m_streamingServerName = settings->value("ServerName").toString();

    serverPort = settings->value("ServerPort", 0).toUInt(&ok);
    m_streamingServerPort = (ok && serverPort <= 65535) ?
                quint16(serverPort) : 0;
    settings->endGroup();

    // User Interface Settings
    settings->beginGroup("UserInterface");
    restoreGeometry(settings->value("WindowGeometry").toByteArray());
    restoreState(settings->value("WindowState").toByteArray());
    if (settings->contains("Verbose"))
        m_verbose = settings->value("Verbose").toBool();
    settings->endGroup();
}

void SjcClient::saveSettings()
{
    QScopedPointer<QSettings> settings(!m_configFileName.isEmpty() ?
        new QSettings(m_configFileName, QSettings::IniFormat) :
        new QSettings(QSettings::IniFormat, QSettings::UserScope,
                      "Kis", "sjcclient"));

    // Dcp Settings will not be modified
    // Streaming Settings will not be modified

    // User Interface Settings
    settings->beginGroup("UserInterface");
    settings->setValue("WindowGeometry", saveGeometry());
    settings->setValue("WindowState", saveState());
    settings->endGroup();
}

void SjcClient::closeEvent(QCloseEvent *event)
{
    saveSettings();
    disconnectFromServer();
    QMainWindow::closeEvent(event);
}

void SjcClient::sendMessage(const Dcp::Message &msg)
{
    if (m_verbose)
        cout << msg << endl;
    m_dcp->sendMessage(msg);
}

Dcp::Message SjcClient::sendMessage(const QByteArray &data)
{
    Dcp::Message msg = m_dcp->sendMessage(m_sjcamName, data);
    if (m_verbose)
        cout << msg << endl;
    return msg;
}

void SjcClient::sendRequest(const QByteArray &identifier)
{
    Dcp::Message msg = m_dcp->sendMessage(m_sjcamName, "get " + identifier);
    if (m_verbose)
        cout << msg << endl;
    m_requestMap[msg.snr()] = RequestItem(identifier);
}

void SjcClient::dcpError(Dcp::Client::Error error)
{
    Q_UNUSED(error);
    if (m_verbose)
        cout << "DCP Error: " << m_dcp->errorString() << "." << endl;
}

void SjcClient::dcpStateChanged(Dcp::Client::State state)
{
    // debug message output only
    if (!m_verbose)
        return;

    switch (state)
    {
    case Dcp::Client::HostLookupState:
        cout << "Connecting to DCP server [" << m_dcp->serverName() << ":"
             << m_dcp->serverPort() << "]..." << endl;
        break;
    case Dcp::Client::ConnectedState:
        cout << "Connected to DCP server [" << m_dcp->deviceName()
             << "@" << m_dcp->serverName() << ":" << m_dcp->serverPort()
             << "]." << endl;
        break;
    case Dcp::Client::UnconnectedState:
        cout << "Disconnected from DCP server." << endl;
        break;
    default:
        break;
    }
}

void SjcClient::dcpConnected()
{
    ui->actionConnect->setChecked(true);

    // ask for camera state, streaming server address and enable notifications
    sendRequest("camerastate");
    sendRequest("streaminghost");
    sendMessage("set notify true");
    m_requestTimer->start(m_requestTimeout);
}

void SjcClient::dcpDisconnected()
{
    ui->actionConnect->setChecked(false);
    m_requestTimer->stop();
    m_requestMap.clear();
    m_sjcamAlive = false;
}

void SjcClient::dcpMessageReceived()
{
    Dcp::Message msg = m_dcp->readMessage();
    if (m_verbose)
        cout << msg << endl;

    // handle reply messages
    if (msg.isReply())
    {
        // ignore replies that we cannot parse
        if (!m_reply.parse(msg))
            return;

        // ignore ack replies, or replies with no argument or errcode != 0
        if (m_reply.isAckReply() || !m_reply.hasArguments() ||
                m_reply.errorCode() != 0)
            return;

        // ignore replies we did not ask for
        if (!m_requestMap.contains(msg.snr()))
            return;

        // finally handle the requested reply
        QByteArray identifier = m_requestMap.take(msg.snr()).identifier;
        QList<QByteArray> args = m_reply.arguments();

        if (identifier == "notify")
        {
            // if notification is disabled, enable it and request settings
            if (args[0] != "false")
                return;
            sendMessage("set notify true");
            sendRequest("camerastate");
            sendRequest("streaminghost");
        }
        else if (identifier == "camerastate")
        {
            CameraDock::CameraState state;
            if (args[0] == "closed")
                state = CameraDock::ClosedState;
            else if (args[0] == "opened")
                state = CameraDock::OpenedState;
            else if (args[0] == "capturing")
                state = CameraDock::CapturingState;
            else
                state = CameraDock::UnknownState;

            if (state == CameraDock::OpenedState ||
                    state == CameraDock::CapturingState) {
                sendRequest("camerainfo");
                sendRequest("exposure");
                sendRequest("framerate");
            }
            else
                setWindowTitle(tr("Slit Jaw Camera"));

            m_cameraDock->setCameraState(state);
        }
        else if (identifier == "camerainfo")
        {
            if (args.size() != 5)
                return;
            m_cameraDock->setCameraName(args[0]);
            m_cameraDock->setCameraId(args[1]);
            m_cameraDock->setCameraSensor(args[2] + "x" + args[3] + "@" +
                                          args[4]);
            setWindowTitle(args[0] + tr(" - Slit Jaw Camera"));
        }
        else if (identifier == "exposure")
        {
            bool ok;
            uint value = args[0].toUInt(&ok);
            if (ok) m_cameraDock->setExposureTime(value / 1000.0);
        }
        else if (identifier == "framerate")
        {
            bool ok;
            double value = args[0].toDouble(&ok);
            if (ok) m_cameraDock->setFrameRate(value);
        }
        else if (identifier == "streaminghost")
        {
            if (args.size() != 2)
                return;
            if (m_streamingServerName.isEmpty() || m_streamingServerPort == 0)
            {
                if (m_streamingServerName.isEmpty())
                    m_streamingServerName = args[0];

                if (m_streamingServerPort == 0) {
                    bool ok;
                    ushort value = args[1].toUShort(&ok);
                    if (ok) m_streamingServerPort = value;
                }

                if (m_socket->state() != QAbstractSocket::UnconnectedState) {
                    m_socket->disconnectFromHost();
                    m_socket->waitForDisconnected();
                }
                m_socket->connectToHost(m_streamingServerName, m_streamingServerPort);
            }
        }
        return;
    }

    // handle commands
    if (!m_command.parse(msg)) {
        sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
        return;
    }

    const Dcp::CommandParser::CmdType cmdType = m_command.cmdType();
    const QByteArray identifier = m_command.identifier();
    if (cmdType == Dcp::CommandParser::SetCmd)
    {
        // set nop
        //     returns: FIN
        if (identifier == "nop")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            sendMessage(msg.replyMessage());
            return;
        }

        // set camerastate ( closed | opened | capturing )
        //     returns: FIN
        if (identifier == "camerastate")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            const QByteArray arg = m_command.arguments()[0];
            CameraDock::CameraState state;
            if (arg == "closed")
                state = CameraDock::ClosedState;
            else if (arg == "opened")
                state = CameraDock::OpenedState;
            else if (arg == "capturing")
                state = CameraDock::CapturingState;
            else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            m_cameraDock->setCameraState(state);
            sendMessage(msg.replyMessage());
            if (state == CameraDock::OpenedState ||
                    state == CameraDock::CapturingState) {
                sendRequest("camerainfo");
                sendRequest("exposure");
                sendRequest("framerate");
            }
            else
                setWindowTitle(tr("Slit Jaw Camera"));

            return;
        }

        // set exposure <usecs>
        //     returns: FIN
        if (identifier == "exposure")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool ok;
            const QByteArray arg = m_command.arguments()[0];
            uint value = arg.toUInt(&ok);
            if (ok) {
                sendMessage(msg.ackMessage());
                m_cameraDock->setExposureTime(value / 1000.0);
                sendMessage(msg.replyMessage());
            } else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
            }
            return;
        }

        // set framerate <Hz>
        //     returns: FIN
        if (identifier == "framerate")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool ok;
            const QByteArray arg = m_command.arguments()[0];
            double value = arg.toDouble(&ok);
            if (ok) {
                sendMessage(msg.ackMessage());
                m_cameraDock->setFrameRate(value);
                sendMessage(msg.replyMessage());
            } else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
            }
            return;
        }

        // set framewritten <number> <total>
        //     returns: FIN
        if (identifier == "framewritten")
        {
            if (m_command.numArguments() != 2) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool ok1, ok2;
            QList<QByteArray> args = m_command.arguments();
            int n = args[0].toInt(&ok1);
            int total = args[1].toInt(&ok2);

            if (ok1 && ok2) {
                sendMessage(msg.ackMessage());

                m_recordingDock->setFramesWritten(n, total);

                // xx

                sendMessage(msg.replyMessage());
            } else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
            }
            return;
        }
    }
    else if (cmdType == Dcp::CommandParser::GetCmd)
    {
        // get version
        //     returns: <client version>
        if (identifier == "version")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            sendMessage(msg.replyMessage(SJCAM_VERSION_STRING));
            return;
        }
    }

    // if we get this far the message is not a valid command
    sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
}

void SjcClient::openButtonClicked(bool checked)
{
    QByteArray state = checked ? "open" : "close";
    sendMessage("set camera " + state);
}

void SjcClient::captureButtonClicked(bool checked)
{
    QByteArray state = checked ? "start" : "stop";
    sendMessage("set capturing " + state);
}

void SjcClient::exposureChanged(double ms)
{
    uint value = uint(qRound64(ms * 1000));
    sendMessage("set exposure " + QByteArray::number(value));
}

void SjcClient::frameRateChanged(double hz)
{
    sendMessage("set framerate " + QByteArray::number(hz, 'f', 3));
}

void SjcClient::writeFrames(int count, int stepping)
{
    if (count != 0)
        sendMessage("set writeframes " + QByteArray::number(count) + " " +
                    QByteArray::number(stepping));
    else
        sendMessage("set writeframes 0");
}

void SjcClient::socketError(QAbstractSocket::SocketError error)
{
    cout << "Socket Error: " << m_socket->errorString() << "." << endl;
}

void SjcClient::socketStateChanged(QAbstractSocket::SocketState state)
{
    // debug message output only
    if (!m_verbose)
        return;

    switch (state)
    {
    case QAbstractSocket::HostLookupState:
        cout << "Connecting to streaming server [" << m_socket->peerName() << ":"
             << m_socket->peerPort() << "]..." << endl;
        break;
    case QAbstractSocket::ConnectedState:
        cout << "Connected to streaming server [" << m_socket->peerName()
             << ":" << m_socket->peerPort() << "]." << endl;
        break;
    case QAbstractSocket::UnconnectedState:
        cout << "Disconnected from streaming server." << endl;
        break;
    default:
        break;
    }
}

void SjcClient::socketConnected()
{
    // request first image
    m_socket->write("gimmisome");
}

void SjcClient::socketDisconnected()
{
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
    ui->labelImage->resize(image.size());
    ui->labelImage->setPixmap(QPixmap::fromImage(image));

    m_socket->write("gimmimore");
}

void SjcClient::requestTimer_timeout()
{
    sendRequest("notify");
    QMutableMapIterator<quint32, RequestItem> iter(m_requestMap);
    while (iter.hasNext()) {
        iter.next();
        if (iter.value().timer.hasExpired(m_requestTimeout))
            iter.remove();
    }
}

void SjcClient::on_actionConnect_triggered(bool checked)
{
    if (checked)
        connectToServer();
    else
        disconnectFromServer();
}

void SjcClient::on_actionAbout_triggered()
{

}
