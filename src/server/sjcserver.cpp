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

#include "sjcserver.h"
#include "recorder.h"
#include "imagestreamer.h"
#include "pvutils.h"
#include "version.h"
#include <QtCore/QtCore>

SjcServer::SjcServer(const CmdLineOptions &opts, QObject *parent)
    : QObject(parent),
      cout(stdout, QIODevice::WriteOnly),
      m_recorder(new Recorder),
      m_imageStreamer(new ImageStreamer),
      m_imageStreamerThread(new QThread),
      m_dcp(new Dcp::Client),
      m_serverName("localhost"),
      m_serverPort(2001),
      m_deviceName("sjcam"),
      m_cameraId(0),
      m_configFileName(opts.configFileName),
      m_verbose(false)
{
    PvInitialize();

    m_dcp->setAutoReconnect(true);
    connect(m_dcp, SIGNAL(error(Dcp::Client::Error)),
                   SLOT(dcpError(Dcp::Client::Error)));
    connect(m_dcp, SIGNAL(stateChanged(Dcp::Client::State)),
                   SLOT(dcpStateChanged(Dcp::Client::State)));
    connect(m_dcp, SIGNAL(messageReceived()), SLOT(dcpMessageReceived()));

    connect(m_recorder, SIGNAL(frameFinished(ulong,int)),
                        SLOT(recorderFrameFinished(ulong,int)));
    connect(m_recorder, SIGNAL(info(QString)), SLOT(recorderInfo(QString)));
    connect(m_recorder, SIGNAL(error(QString)), SLOT(recorderError(QString)));
    connect(m_recorder, SIGNAL(started()), SLOT(recorderStarted()));
    connect(m_recorder, SIGNAL(finished()), SLOT(recorderStopped()));

    connect(m_imageStreamer, SIGNAL(frameFinished(tPvFrame*)),
                             SLOT(streamerFrameFinished(tPvFrame*)));
    connect(m_imageStreamer, SIGNAL(info(QString)), SLOT(streamerInfo(QString)));
    connect(m_imageStreamer, SIGNAL(error(QString)), SLOT(streamerError(QString)));
    connect(m_imageStreamerThread, SIGNAL(started()), SLOT(streamerThreadStarted()));
    connect(m_imageStreamerThread, SIGNAL(finished()), SLOT(streamerThreadFinished()));

    if (!m_configFileName.isEmpty())
        loadConfigFile();
    if (!opts.serverName.isEmpty())
        m_serverName = opts.serverName;
    if (opts.serverPort != 0)
        m_serverPort = 0;
    if (!opts.deviceName.isEmpty())
        m_deviceName = opts.deviceName;
    if (opts.cameraId != 0)
        m_cameraId = opts.cameraId;
    if (opts.verbose != -1)
        m_verbose = bool(opts.verbose);

    m_imageStreamerThread->start();
    m_imageStreamer->moveToThread(m_imageStreamerThread);
}

SjcServer::~SjcServer()
{
    m_dcp->disconnectFromServer();
    m_dcp->waitForDisconnected();

    m_recorder->stop();
    m_recorder->wait();
    m_recorder->closeCamera();

    m_imageStreamerThread->quit();
    m_imageStreamerThread->wait();

    delete m_dcp;
    delete m_recorder;
    delete m_imageStreamer;
    delete m_imageStreamerThread;

    PvUnInitialize();
}

bool SjcServer::openCamera()
{
    //return m_recorder->openCamera(m_cameraId);

    // test
    bool ok = m_recorder->openCamera(m_cameraId);
    if (ok) {
        m_recorder->setAttribute("FrameStartTriggerMode", "FixedRate");
        m_recorder->setAttribute("FrameRate", 5);
        m_recorder->setAttribute("ExposureValue", 150000);
        m_recorder->setAttribute("PixelFormat", "Mono16");
        QTimer::singleShot(1000, m_recorder, SLOT(start()));
    }
    return ok;
}

void SjcServer::connectToDcpServer()
{
    m_dcp->connectToServer(m_serverName, m_serverPort, m_deviceName);
}

void SjcServer::loadConfigFile()
{
    QFileInfo fileInfo(m_configFileName);
    if (!fileInfo.isFile()) {
        cout << "Warning: Cannot find config file \"" << m_configFileName
             << "\"." << endl;
        return;
    }

    if (!fileInfo.isReadable()) {
        cout << "Warning: Cannot read config file \"" << m_configFileName
             << "\"." << endl;
        return;
    }

    bool ok;
    QSettings settings(m_configFileName, QSettings::IniFormat);

    settings.beginGroup("Dcp");
    QString serverName = settings.value("ServerName").toString();
    if (!serverName.isEmpty())
        m_serverName = serverName;

    uint serverPort = settings.value("ServerPort").toUInt(&ok);
    if (ok && serverPort <= 65535)
        m_serverPort = quint16(serverPort);

    QByteArray deviceName = settings.value("DeviceName").toByteArray();
    if (!deviceName.isEmpty())
        m_deviceName = deviceName;
    settings.endGroup();

    settings.beginGroup("Camera");
    uint cameraId = settings.value("UniqueId").toUInt(&ok);
    if (ok) m_cameraId = cameraId;
    settings.endGroup();
}

void SjcServer::sendMessage(const Dcp::Message &message)
{
    if (verbose())
        cout << message << endl;
    m_dcp->sendMessage(message);
}

void SjcServer::dcpError(Dcp::Client::Error error)
{
    cout << "DCP Error: " << m_dcp->errorString() << "." << endl;
}

void SjcServer::dcpStateChanged(Dcp::Client::State state)
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

void SjcServer::dcpMessageReceived()
{
    Dcp::Message msg = m_dcp->readMessage();

    if (verbose())
        cout << msg << endl;

    // ignore reply messages
    if (msg.isReply())
        return;

    if (!m_command.parse(msg)) {
        sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
        return;
    }

    Dcp::CommandParser::CmdType cmdType = m_command.cmdType();
    QByteArray identifier = m_command.identifier();
    if (cmdType == Dcp::CommandParser::SetCmd)
    {
        // set nop
        if (identifier == "nop")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            if (!m_recorder->isRunning())
                m_recorder->start();
            else
                m_recorder->stop();
            sendMessage(msg.replyMessage());
            return;
        }

        // set attr name value
        if (identifier == "pvattr")
        {
            if (m_command.numArguments() != 2) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            QList<QByteArray> args = m_command.arguments();
            if (m_recorder->setAttribute(args[0], args[1]))
                sendMessage(msg.replyMessage());
            else
                sendMessage(msg.replyMessage(QByteArray(), 1));
            return;
        }
    }
    else if (cmdType == Dcp::CommandParser::GetCmd)
    {
        // get attr name
        if (identifier == "pvattr")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            QVariant value;
            QList<QByteArray> args = m_command.arguments();
            if (m_recorder->getAttribute(args[0], &value))
                sendMessage(msg.replyMessage(value.toByteArray()));
            else
                sendMessage(msg.replyMessage(QByteArray(), 1));
            return;
        }

        // get version
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

        // get pvversion
        if (identifier == "pvversion")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            sendMessage(msg.replyMessage(PvVersionString().toAscii()));
            return;
        }
    }

    // if we get this far the message is not a valid command
    sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
}

void SjcServer::recorderFrameFinished(ulong id, int status)
{
    switch (status)
    {
    case ePvErrSuccess:
        cout << ".";
        break;
    case ePvErrCancelled:
        cout << "C";
        break;
    case ePvErrDataLost:
        cout << "L";
        break;
    case ePvErrDataMissing:
        cout << "M";
        break;
    default:
        cout << "?";
    }
    cout.flush();

    tPvFrame *frame = m_recorder->readFinishedFrame();
    QMetaObject::invokeMethod(m_imageStreamer, "processFrame",
                              Q_ARG(tPvFrame *, frame));
}

void SjcServer::recorderInfo(const QString &infoString)
{
    cout << infoString << endl;
}

void SjcServer::recorderError(const QString &errorString)
{
    cout << "Error: " << errorString << endl;
}

void SjcServer::recorderStarted()
{
    cout << "Capture thread started." << endl;
}

void SjcServer::recorderStopped()
{
    cout << "Capture thread stopped." << endl;
}

void SjcServer::streamerFrameFinished(tPvFrame *frame)
{
    m_recorder->enqueueFrame(frame);
}

void SjcServer::streamerInfo(const QString &infoString)
{
    cout << infoString << endl;
}

void SjcServer::streamerError(const QString &errorString)
{
    cout << "Error:" << errorString << endl;
}

void SjcServer::streamerThreadStarted()
{
    cout << "Streaming server started." << endl;
}

void SjcServer::streamerThreadFinished()
{
    cout << "Streaming server stopped." << endl;
}
