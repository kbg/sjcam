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
#include "recordingdock.h"
#include "histogramdock.h"
#include "version.h"
#include <sjcdata.h>
#include "CamSys/ImageScrollArea.h"
#include "CamSys/ImageWidget.h"
#include "CamSys/Image.h"
#include "CamSys/ColorTable.h"
#include <dcpclient/dcpclient.h>
#include <QtCore/QtCore>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtNetwork/QHostAddress>

SjcClient::SjcClient(const CmdLineOpts &opts, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::SjcClient),
      cout(stdout, QIODevice::WriteOnly),
      m_socket(new QTcpSocket),
      m_dcp(new Dcp::Client),
      m_scrollArea(new CamSys::ImageScrollArea),
      m_imageWidget(new CamSys::ImageWidget),
      m_image(new CamSys::Image),
      m_cameraDock(new CameraDock),
      m_recordingDock(new RecordingDock),
      m_histogramDock(new HistogramDock),
      m_comboColorTables(new QComboBox),
      m_labelImagePos(new QLabel),
      m_labelDcpStatus(new QLabel),
      m_labelStreamStatus(new QLabel),
      m_labelCameraStatus(new QLabel),
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

    m_comboColorTables->addItems(QStringList()
            << "Gray" << "Hot" << "Alien" << "Cool" << "Flame"
            << "Rgb" << "Rainbow");
    connect(m_comboColorTables, SIGNAL(activated(QString)),
                                SLOT(selectColorTable(QString)));
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget(m_comboColorTables);

    ui->statusbar->addWidget(m_labelImagePos);
    ui->statusbar->addPermanentWidget(m_labelDcpStatus);
    ui->statusbar->addPermanentWidget(m_labelStreamStatus);
    ui->statusbar->addPermanentWidget(m_labelCameraStatus);
    updateStatusBarDcp(Dcp::Client::UnconnectedState);
    updateStatusBarStream(QAbstractSocket::UnconnectedState);
    updateStatusBarCamera(CameraDock::UnknownState);

    addDockWidget(Qt::RightDockWidgetArea, m_histogramDock);
    addDockWidget(Qt::RightDockWidgetArea, m_cameraDock);
    addDockWidget(Qt::RightDockWidgetArea, m_recordingDock);

    m_histogramDock->setColorRange(0, 4095);
    m_imageWidget->setColorRange(0, 4095);
    m_imageWidget->setMouseTracking(true);
    //m_imageWidget->setCursor(Qt::CrossCursor);
    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_scrollArea->setWidget(m_imageWidget);
    setCentralWidget(m_scrollArea);
    m_scrollArea->setFocus();

    connect(m_imageWidget, SIGNAL(mouseMovedTo(QPoint)),
            SLOT(imageWidget_mouseMovedTo(QPoint)));
    connect(m_imageWidget, SIGNAL(mouseLeft()), SLOT(imageWidget_mouseLeft()));

    connect(m_histogramDock, SIGNAL(colorSpreadChanged(double,double)),
            SLOT(histDock_colorSpreadChanged(double,double)));

    connect(ui->actionZoomIn, SIGNAL(triggered()), m_scrollArea, SLOT(zoomIn()));
    connect(ui->actionZoomOut, SIGNAL(triggered()), m_scrollArea, SLOT(zoomOut()));
    connect(ui->actionZoomNormal, SIGNAL(triggered()), m_scrollArea, SLOT(zoomNormalSize()));
    connect(ui->actionZoomBestFit, SIGNAL(triggered()), m_scrollArea, SLOT(zoomBestFit()));

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
    connect(m_cameraDock, SIGNAL(triggerModeChanged(QByteArray)), SLOT(triggerModeChanged(QByteArray)));

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
    delete m_image;
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
    m_socket->disconnectFromHost();
}

void SjcClient::selectColorTable(const QString &name)
{
    // select combo box entry
    m_comboColorTables->setCurrentIndex(m_comboColorTables->findText(name));

    if (name == QString("Gray"))
        m_imageWidget->setColorTable(CamSys::ColorTable::grayTable());
    else if (name == QString("Red"))
        m_imageWidget->setColorTable(CamSys::ColorTable::redTable());
    else if (name == QString("Green"))
        m_imageWidget->setColorTable(CamSys::ColorTable::greenTable());
    else if (name == QString("Blue"))
        m_imageWidget->setColorTable(CamSys::ColorTable::blueTable());
    else if (name == QString("Hot"))
        m_imageWidget->setColorTable(CamSys::ColorTable::hotTable());
    else if (name == QString("Alien"))
        m_imageWidget->setColorTable(CamSys::ColorTable::alienTable());
    else if (name == QString("Cool"))
        m_imageWidget->setColorTable(CamSys::ColorTable::coolTable());
    else if (name == QString("Rgb"))
        m_imageWidget->setColorTable(CamSys::ColorTable::rgbTable());
    else if (name == QString("Rainbow"))
        m_imageWidget->setColorTable(CamSys::ColorTable::rainbowTable());
    else if (name == QString("Flame"))
        m_imageWidget->setColorTable(CamSys::ColorTable::flameTable());

    // update color bar
    m_histogramDock->setColorTable(m_imageWidget->colorTable());
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
    m_imageWidget->setMarkerSize(settings->value("MarkerSize", 5).toInt());
    m_imageWidget->setMarkerInnerColor(
                settings->value("MarkerInnerColor", "black").toString());
    m_imageWidget->setMarkerOuterColor(
                settings->value("MarkerOuterColor", "white").toString());
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

void SjcClient::updateStatusBarImagePos(const QPoint &pos)
{
    QString s;
    if (pos.x() >= 0 || pos.y() >= 0)
        s = tr("%1, %2").arg(pos.x()).arg(pos.y());
    m_labelImagePos->setText(s);
}

void SjcClient::updateStatusBarDcp(Dcp::Client::State state)
{
    QString s = tr("&nbsp;DCP: ");
    if (state == Dcp::Client::ConnectingState)
        s += tr("<font color=red>Connecting</font>");
    else if (state == Dcp::Client::ConnectedState)
        s += tr("<font color=blue>Connected</font>");
    else if (state == Dcp::Client::UnconnectedState)
        s += tr("<font color=red>Disconnected</font>");
    else
        return;
    m_labelDcpStatus->setText(s + "&nbsp;");
}

void SjcClient::updateStatusBarStream(QAbstractSocket::SocketState state)
{
    QString s = tr("&nbsp;Stream: ");
    if (state == QAbstractSocket::ConnectingState)
        s += tr("<font color=red>Connecting</font>");
    else if (state == QAbstractSocket::ConnectedState)
        s += tr("<font color=blue>Connected</font>");
    else if (state == QAbstractSocket::UnconnectedState)
        s += tr("<font color=red>Disconnected</font>");
    else
        return;
    m_labelStreamStatus->setText(s + "&nbsp;");
}

void SjcClient::updateStatusBarCamera(CameraDock::CameraState state)
{
    QString s = tr("&nbsp;Camera: ");
    if (state == CameraDock::UnknownState)
        s += tr("<font color=red>Unknown</font>");
    else if (state == CameraDock::ClosedState)
        s += tr("<font color=red>Closed</font>");
    else if (state == CameraDock::OpenedState)
        s += tr("<font color=green>Opened</font>");
    else if (state == CameraDock::CapturingState)
        s += tr("<font color=blue>Capturing</font>");
    else
        return;
    m_labelCameraStatus->setText(s + "&nbsp;");
}

void SjcClient::dcpError(Dcp::Client::Error error)
{
    Q_UNUSED(error);
    if (m_verbose)
        cout << "DCP Error: " << m_dcp->errorString() << "." << endl;
}

void SjcClient::dcpStateChanged(Dcp::Client::State state)
{
    updateStatusBarDcp(state);

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
    m_socket->disconnectFromHost();
    m_cameraDock->setCameraState(CameraDock::UnknownState);
    updateStatusBarCamera(CameraDock::UnknownState);
    setWindowTitle(tr("Slit Jaw Camera"));
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
                sendRequest("triggermode");
                sendRequest("exposure");
                sendRequest("framerate");
                sendRequest("marker");
            }
            else {
                setWindowTitle(tr("Slit Jaw Camera"));
                m_recordingDock->setFramesWritten(0, 0);
                m_image->clear();
                m_imageWidget->setImage(m_image);
                m_histogramDock->setImage(m_image);
            }

            m_cameraDock->setCameraState(state);
            updateStatusBarCamera(state);
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
        else if (identifier == "triggermode")
        {
            m_cameraDock->setTriggerMode(args[0]);
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
        else if (identifier == "marker")
        {
            if (args.size() != 3)
                return;

            bool ok1, ok2, ok3;
            bool enabled;
            if (args[0] == "true") {
                ok1 = true;
                enabled = true;
            } else if (args[0] == "false") {
                ok1 = true;
                enabled = false;
            } else {
                ok1 = false;
            }
            QPointF pos(args[1].toDouble(&ok2), args[2].toDouble(&ok3));

            if (ok1 && ok2 && ok3) {
                m_imageWidget->setMarkerEnabled(enabled);
                m_imageWidget->setMarkerPos(pos);
            }
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
            }
            if (m_socket->state() != QAbstractSocket::UnconnectedState) {
                m_socket->disconnectFromHost();
                if (m_socket->state() != QAbstractSocket::UnconnectedState)
                    m_socket->waitForDisconnected();
            }
            m_socket->connectToHost(m_streamingServerName, m_streamingServerPort);
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
            updateStatusBarCamera(state);
            sendMessage(msg.replyMessage());
            if (state == CameraDock::OpenedState ||
                    state == CameraDock::CapturingState) {
                sendRequest("camerainfo");
                sendRequest("exposure");
                sendRequest("framerate");
            }
            else {
                setWindowTitle(tr("Slit Jaw Camera"));
                m_recordingDock->setFramesWritten(0, 0);
                m_image->clear();
                m_imageWidget->setImage(m_image);
                m_histogramDock->setImage(m_image);
            }

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

        // set triggermode <mode>
        //     returns: FIN
        if (identifier == "triggermode")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            QByteArray value = m_command.arguments()[0];
            sendMessage(msg.ackMessage());
            m_cameraDock->setTriggerMode(value);
            sendMessage(msg.replyMessage());
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

        // set marker ( true | false ) <xpos> <ypos>
        //     returns: FIN
        if (identifier == "marker")
        {
            if (m_command.numArguments() != 3) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool ok1, ok2, ok3;
            QList<QByteArray> args = m_command.arguments();

            bool enabled;
            if (args[0] == "true") {
                ok1 = true;
                enabled = true;
            } else if (args[0] == "false") {
                ok1 = true;
                enabled = false;
            } else {
                ok1 = false;
            }
            QPointF pos(args[1].toDouble(&ok2), args[2].toDouble(&ok3));

            if (ok1 && ok2 && ok3) {
                sendMessage(msg.ackMessage());
                m_imageWidget->setMarkerEnabled(enabled);
                m_imageWidget->setMarkerPos(pos);
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

void SjcClient::triggerModeChanged(const QByteArray &triggerMode)
{
    sendMessage("set triggermode " + triggerMode);
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
    updateStatusBarStream(state);

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
    m_image->clear();
    m_imageWidget->setImage(m_image);
    m_histogramDock->setImage(m_image);
    updateStatusBarImagePos(QPoint(-1, -1));
}

void SjcClient::socketReadyRead()
{
    QByteArray buf = m_socket->peek(4);

    quint32 size;
    QDataStream is(&buf, QIODevice::ReadOnly);
    is.setVersion(QDataStream::Qt_4_7);
    is >> size;

    // check if enough data is available to read the size variable and the
    // following QByteArray (4 + 4 + size)
    if (m_socket->bytesAvailable() < size + 8)
        return;

    QByteArray jpeg;
    is.setDevice(m_socket);
    is >> size >> jpeg;

    QImage qimage = QImage::fromData(jpeg, "jpeg");
    int width = qimage.width();
    int height = qimage.height();

    bool sizeChanged = false;
    if (width != m_image->width() || height != m_image->height()) {
        if (width != 0 && height != 0)
            m_image->reset(width, height, CamSys::Image::Uint16, 12);
        else
            m_image->clear();
        sizeChanged = true;
    }

    if (!m_image->isNull()) {
        for (int i = 0; i < height; ++i) {
            const uchar *srcLine = qimage.scanLine(i);
            quint16 *destLine = m_image->scanLine<quint16>(i);
            for (int j = 0; j < width; ++j) {
                destLine[j] = quint16(srcLine[j]) * 16;
            }
        }
    }

    m_imageWidget->setColorRange(m_histogramDock->minColorValue(),
                                 m_histogramDock->maxColorValue());
    m_imageWidget->setImage(m_image);
    if (sizeChanged) {
        m_scrollArea->zoomBestFit();
        QPoint pos = m_imageWidget->mapFromGlobal(QCursor::pos());
        updateStatusBarImagePos(m_imageWidget->mapToImage(pos));
    }
    m_histogramDock->setImage(m_image);

    m_socket->write("moreplease");
}

void SjcClient::histDock_colorSpreadChanged(double minColorValue,
                                            double maxColorValue)
{
    m_imageWidget->setColorRange(minColorValue, maxColorValue);
    m_imageWidget->setImage(m_image);
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

void SjcClient::imageWidget_mouseMovedTo(const QPoint &pos)
{
    updateStatusBarImagePos(m_imageWidget->mapToImage(pos));
}

void SjcClient::imageWidget_mouseLeft()
{
    updateStatusBarImagePos(QPoint(-1,-1));
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
    QString aboutText = tr(
                "<h2>SjcClient %1</h2>" \
                "<p><b>Library Versions:</b><br>" \
                "&nbsp;&nbsp;DcpClient %2<br>" \
                "&nbsp;&nbsp;Qt %3</p>" \
                "<p>%4</p>"
            )
            .arg(SJCAM_VERSION_STRING)
            .arg(Dcp::versionString())
            .arg(qVersion())
            .arg(QString(SJCAM_COPYRIGHT_STRING)
                 .replace("\n", "<br>")
                 .replace(" fuer ", " f&uuml;r ")
        );
    QMessageBox::about(this, tr("About SjcClient"), aboutText);
}
