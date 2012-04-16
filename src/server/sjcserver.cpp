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
#include "imagewriter.h"
#include "pvutils.h"
#include "version.h"
#include <QtCore/QtCore>
#include <QtNetwork/QHostAddress>

SjcServer::SjcServer(const CmdLineOpts &opts, QObject *parent)
    : QObject(parent),
      cout(stdout, QIODevice::WriteOnly),
      m_recorder(new Recorder),
      m_imageStreamer(new ImageStreamer),
      m_imageStreamerThread(new QThread),
      m_imageWriter(new ImageWriter),
      m_imageWriterThread(new QThread),
      m_dcp(new Dcp::Client),
      m_clientTimeout(30000),
      m_updateClientMapTimer(new QTimer),
      m_serverName("localhost"),
      m_serverPort(2001),
      m_deviceName("sjcam"),
      m_cameraId(0),
      m_numBuffers(10),
      m_streamingPort(0),
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
    connect(m_recorder, SIGNAL(info(QString)), SLOT(printInfo(QString)));
    connect(m_recorder, SIGNAL(error(QString)), SLOT(printError(QString)));
    connect(m_recorder, SIGNAL(started()), SLOT(recorderStarted()));
    connect(m_recorder, SIGNAL(finished()), SLOT(recorderStopped()));

    connect(m_imageStreamer, SIGNAL(info(QString)), SLOT(printInfo(QString)));
    connect(m_imageStreamer, SIGNAL(error(QString)), SLOT(printError(QString)));
    connect(m_imageStreamer, SIGNAL(connectionListChanged(QStringList)),
                             SLOT(streamerConnectionListChanged(QStringList)));
    connect(m_imageStreamerThread, SIGNAL(started()),
                                   SLOT(streamerThreadStarted()));
    connect(m_imageStreamerThread, SIGNAL(finished()),
                                   SLOT(streamerThreadFinished()));

    connect(m_imageWriter, SIGNAL(frameWritten(int,int)),
                           SLOT(writerFrameWritten(int,int)));
    connect(m_imageWriter, SIGNAL(frameFinished(tPvFrame*)),
                           SLOT(writerFrameFinished(tPvFrame*)));
    connect(m_imageWriter, SIGNAL(info(QString)), SLOT(printInfo(QString)));
    connect(m_imageWriter, SIGNAL(error(QString)), SLOT(printError(QString)));
    connect(m_imageWriterThread, SIGNAL(started()),
                                 SLOT(writerThreadStarted()));
    connect(m_imageWriterThread, SIGNAL(finished()),
                                 SLOT(writerThreadFinished()));

    connect(m_imageStreamer, SIGNAL(frameFinished(tPvFrame*)),
            m_imageWriter, SLOT(processFrame(tPvFrame*)));

    connect(m_updateClientMapTimer, SIGNAL(timeout()), SLOT(updateClientMap()));
    m_updateClientMapTimer->start(m_clientTimeout / 3);

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

    m_recorder->setNumBuffers(m_numBuffers);

    if (m_imageStreamer->listen(m_streamingPort)) {
        m_imageStreamerThread->start();
        m_imageStreamer->moveToThread(m_imageStreamerThread);
    }
    m_streamingPort = m_imageStreamer->serverPort();

    m_imageWriter->setFileNamePrefix(m_outputFileNamePrefix);
    m_imageWriter->setDirectory(m_outputDirectory);
    m_imageWriter->setDeviceName(m_deviceName);
    m_imageWriter->setTelescopeName(m_telescopeName);
    m_imageWriterThread->start();
    m_imageWriter->moveToThread(m_imageWriterThread);
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

    m_imageWriterThread->quit();
    m_imageWriterThread->wait();

    delete m_dcp;
    delete m_recorder;
    delete m_imageStreamer;
    delete m_imageStreamerThread;
    delete m_imageWriter;
    delete m_imageWriterThread;
    delete m_updateClientMapTimer;

    PvUnInitialize();
}

bool SjcServer::openCamera()
{
    if (!m_recorder->openCamera(m_cameraId))
        return false;

    // default attribute settings
    m_recorder->setAttribute("FrameStartTriggerMode", "FixedRate");
    m_recorder->setAttribute("FrameRate", 10);
    m_recorder->setAttribute("ExposureValue", 10000);
    m_recorder->setAttribute("PixelFormat", "Mono16");

    // config file attribute settings
    foreach (const NamedValue &attr, m_camAttrList)
        m_recorder->setAttribute(attr.name, attr.value);

    if (verbose())
        cout << "\n" << m_recorder->cameraInfoString() << "\n" << endl;

    CameraInfo cameraInfo = m_recorder->cameraInfo();
    QMetaObject::invokeMethod(m_imageWriter, "setCameraInfo",
                              Q_ARG(CameraInfo, cameraInfo));
    return true;
}

bool SjcServer::closeCamera()
{
    stopCapturing();
    return m_recorder->closeCamera();
}

void SjcServer::startCapturing()
{
    if (!m_recorder->isRunning())
        m_recorder->start();
}

void SjcServer::stopCapturing()
{
    if (m_recorder->isRunning()) {
        m_recorder->stop();
        m_recorder->wait();
    }
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

    // DCP Section
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

    // Camera Section
    settings.beginGroup("Camera");

    uint cameraId = settings.value("UniqueId").toUInt(&ok);
    if (ok) m_cameraId = cameraId;

    int numBuffers = settings.value("NumBuffers").toInt(&ok);
    if (ok) m_numBuffers = numBuffers;

    settings.endGroup();

    // CamAttr Section
    settings.beginGroup("CamAttr");
    m_camAttrList.clear();
    foreach (QString key, settings.allKeys()) {
        NamedValue attr(key.toAscii(), settings.value(key));
        if (!attr.value.toString().isEmpty())
            m_camAttrList.append(attr);
    }
    settings.endGroup();

    // Streaming Section
    settings.beginGroup("Streaming");
    uint streamingPort = settings.value("ServerPort").toUInt(&ok);
    if (ok && streamingPort <= 65535)
        m_streamingPort = quint16(streamingPort);
    settings.endGroup();

    // Recording Section
    settings.beginGroup("Recording");
    m_outputFileNamePrefix = settings.value("FileNamePrefix").toString();
    if (m_outputFileNamePrefix.isEmpty())
        m_outputFileNamePrefix = m_deviceName;
    m_outputDirectory = settings.value("Directory").toString();
    m_telescopeName = settings.value("TelescopeName").toByteArray();
    settings.endGroup();
}

void SjcServer::sendMessage(const Dcp::Message &message)
{
    if (verbose())
        cout << message << endl;
    m_dcp->sendMessage(message);
}

void SjcServer::sendNotification(const QByteArray &data)
{
    foreach (const QByteArray &deviceName, m_clientMap.keys()) {
        if (verbose())
            cout << m_dcp->sendMessage(deviceName, data) << endl;
        else
            m_dcp->sendMessage(deviceName, data);
    }
}

void SjcServer::addClient(const QByteArray &deviceName)
{
    if (!m_clientMap.contains(deviceName)) {
        QElapsedTimer timer;
        timer.start();
        m_clientMap[deviceName] = timer;
        cout << "Added client '" << deviceName
             << "' to the notification list." << endl;
    }
}

void SjcServer::removeClient(const QByteArray &deviceName)
{
    if (m_clientMap.contains(deviceName)) {
        m_clientMap.remove(deviceName);
        cout << "Removed client '" << deviceName
             << "' from the notification list." << endl;
    }
}

void SjcServer::updateClientMap()
{
    QMutableMapIterator<QByteArray, QElapsedTimer> iter(m_clientMap);
    while (iter.hasNext()) {
        iter.next();
        if (iter.value().hasExpired(m_clientTimeout)) {
            cout << "Removed client '" << iter.key()
                 << "' from the notification list (timeout)." << endl;
            iter.remove();
        }
    }
}

void SjcServer::printInfo(const QString &infoString)
{
    cout << infoString << endl;
}

void SjcServer::printError(const QString &errorString)
{
    cout << "Error: " << errorString << endl;
}

void SjcServer::dcpError(Dcp::Client::Error error)
{
    Q_UNUSED(error);
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
        cout << "Connected to DCP server [" << m_dcp->deviceName()
             << "@" << m_dcp->serverName() << ":" << m_dcp->serverPort()
             << "]." << endl;
        if (verbose())
            cout << "Local IP address for DCP connection: "
                 << m_dcp->localAddress().toString() << endl;
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

    // update client map if the sender requested notifications
    if (m_clientMap.contains(msg.source()))
        m_clientMap[msg.source()].restart();

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

        // set notify ( true | false )
        //     returns: FIN
        if (identifier == "notify")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            QByteArray arg = m_command.arguments()[0];
            if (arg == "true") {
                sendMessage(msg.ackMessage());
                addClient(msg.source());
                sendMessage(msg.replyMessage());
            }
            else if (arg == "false") {
                sendMessage(msg.ackMessage());
                removeClient(msg.source());
                sendMessage(msg.replyMessage());
            }
            else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            return;
        }

        // set camera ( open | close )
        //     errcodes: 1 -> cannot open/close camera
        if (identifier == "camera")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool open;
            QByteArray arg = m_command.arguments()[0];
            if (arg == "open")
                open = true;
            else if (arg == "close")
                open = false;
            else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            if (open && m_recorder->isCameraOpen()) {
                sendMessage(msg.ackMessage(Dcp::AckWrongModeError));
                return;
            }
            sendMessage(msg.ackMessage());

            bool ok = open ? openCamera() : closeCamera();
            sendMessage(msg.replyMessage(QByteArray(), ok ? 0 : 1));

            // send notifications
            if (!m_clientMap.isEmpty())
            {
                QByteArray arg;
                if (m_recorder->isCameraOpen())
                    arg = m_recorder->isRunning() ? "capturing" : "opened";
                else
                    arg = "closed";
                sendNotification("set camerastate " + arg);

                if (arg != "closed") {
                    bool ok;
                    QVariant value;
                    if (m_recorder->getAttribute("ExposureValue", &value)) {
                        arg = QByteArray::number(value.toUInt(&ok));
                        if (ok) sendNotification("set exposure " + arg);
                    }
                    if (m_recorder->getAttribute("FrameRate", &value)) {
                        arg = QByteArray::number(value.toFloat(&ok), 'f', 3);
                        if (ok) sendNotification("set framerate " + arg);
                    }
                }
            }
            return;
        }

        // set capturing ( start | stop )
        //     errcodes: 1 -> cannot start/stop capturing
        if (identifier == "capturing")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool start;
            QByteArray arg = m_command.arguments()[0];
            if (arg == "start")
                start = true;
            else if (arg == "stop")
                start = false;
            else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            // wrong mode, when trying to start capturing while the camera
            // is not opened yet or the capture thread is already running
            if (start && (!m_recorder->isCameraOpen() ||
                           m_recorder->isRunning())) {
                sendMessage(msg.ackMessage(Dcp::AckWrongModeError));
                return;
            }
            sendMessage(msg.ackMessage());

            if (start)
                m_recorder->start();
            else if (m_recorder->isRunning())
                m_recorder->stop();

            // Note: errorcode 1 cannot be returned without waiting for the
            // Recorder::started() signal; for now we always return errcode 0.
            sendMessage(msg.replyMessage());

            // Notification messages will be sent from the recorderStarted()
            // and recorderStopped() slots
            return;
        }

        // set exposure <usecs>
        //     returns: FIN
        //     errorcodes: 1 -> cannot set exposure value
        if (identifier == "exposure")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool ok;
            quint32 value = m_command.arguments()[0].toUInt(&ok);
            if (!ok) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            if (m_recorder->setAttribute("ExposureValue", value))
                sendMessage(msg.replyMessage());
            else {
                sendMessage(msg.replyMessage(QByteArray(), 1));
                QVariant cameraValue;
                m_recorder->getAttribute("ExposureValue", &cameraValue);
                value = cameraValue.toUInt(&ok);
                if (!ok)
                    return; // don't send notification if value isn't valid
            }
            sendNotification("set exposure " + QByteArray::number(value));
            return;
        }

        // set framerate <Hz>
        //     returns: FIN
        //     errorcodes: 1 -> cannot set framerate value
        if (identifier == "framerate")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool ok;
            float value = m_command.arguments()[0].toFloat(&ok);
            if (!ok || value < 0) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            if (m_recorder->setAttribute("FrameRate", value))
                sendMessage(msg.replyMessage());
            else {
                sendMessage(msg.replyMessage(QByteArray(), 1));
                QVariant cameraValue;
                m_recorder->getAttribute("FrameRate", &cameraValue);
                value = cameraValue.toFloat(&ok);
                if (!ok)
                    return; // don't send notification if value isn't valid
            }
            sendNotification("set framerate " + QByteArray::number(
                                 value, 'f', 3));
            return;
        }

        // set roi <left> <top> <width> <height>
        //     returns: FIN
        //     errorcodes: 1 -> cannot set roi values
        if (identifier == "roi")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // set binningx <pixels>
        //     returns: FIN
        //     errorcodes: 1 -> cannot set x-binning value
        //     note: this affects roi and maximagesize
        if (identifier == "binningx")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // set binningy <pixels>
        //     returns: FIN
        //     errorcodes: 1 -> cannot set y-binning value
        //     note: this affects roi and maximagesize
        if (identifier == "binningy")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // set binning <xbin> <ybin>
        //     returns: FIN
        //     errcodes: 1 -> cannot set x-binning value
        //               2 -> cannot set y-binning value
        //               3 -> cannot set any binning values
        if (identifier == "binning")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // set writeframes <count> [<stepping>]
        //     returns: FIN
        if (identifier == "writeframes")
        {
            QList<QByteArray> args = m_command.arguments();
            if (args.size() < 1 || args.size() > 2) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool ok;
            int count = args[0].toInt(&ok);
            if (!ok || count < 0) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            int stepping = 1;
            if (args.size() == 2) {
                stepping = args[1].toInt(&ok);
                if (!ok || stepping < 1) {
                    sendMessage(msg.ackMessage(Dcp::AckParameterError));
                    return;
                }
            }

            sendMessage(msg.ackMessage());
            QMetaObject::invokeMethod(m_imageWriter, "writeNextFrames",
                    Q_ARG(int, count), Q_ARG(int, stepping));
            sendMessage(msg.replyMessage());
            return;
        }

        // set verbose ( true | false )
        //     note: for debugging
        if (identifier == "verbose")
        {
            if (m_command.numArguments() != 1) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }

            bool verbose;
            QByteArray arg = m_command.arguments()[0];
            if (arg == "true" || arg == "1")
                verbose = true;
            else if (arg == "false" || arg == "0")
                verbose = false;
            else {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            m_verbose = verbose;
            sendMessage(msg.replyMessage());
            return;
        }

        // set pvattr <name> [<value>]
        //     note: for debugging only!
        if (identifier == "pvattr")
        {
            QList<QByteArray> args = m_command.arguments();
            if (args.size() < 1 || args.size() > 2) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            int errcode = 0;
            QVariant value = (args.size() == 2 ? args[1] : QVariant());
            if (!m_recorder->setAttribute(args[0], value))
                errcode = 1;
            sendMessage(msg.replyMessage(QByteArray(), errcode));
            return;
        }
    }
    else if (cmdType == Dcp::CommandParser::GetCmd)
    {
        // get notify
        //     returns: ( true | false )
        if (identifier == "notify")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            if (m_clientMap.contains(msg.source()))
                sendMessage(msg.replyMessage("true"));
            else
                sendMessage(msg.replyMessage("false"));
            return;
        }

        // get camerastate
        //     returns: ( closed | opened | capturing )
        if (identifier == "camerastate")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            QByteArray state;
            if (m_recorder->isCameraOpen())
                state = m_recorder->isRunning() ? "capturing" : "opened";
            else
                state = "closed";
            sendMessage(msg.replyMessage(state));
            return;
        }

        // get exposure
        //     returns: <usecs>
        //     errorcodes: 1 -> cannot get exposure value
        if (identifier == "exposure")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            QVariant value;
            if (!m_recorder->getAttribute("ExposureValue", &value) ||
                    !value.canConvert(QVariant::UInt)) {
                sendMessage(msg.replyMessage(QByteArray(), 1));
                return;
            }
            sendMessage(msg.replyMessage(QByteArray::number(value.toUInt())));
            return;
        }

        // get exposure_range
        //     returns: <min> <max>
        //     errorcodes: 1 -> cannot get range values
        if (identifier == "exposure_range")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // get framerate
        //     returns: <Hz>
        //     errorcodes: 1 -> cannot get framerate value
        if (identifier == "framerate")
        {
            // not implemented yet
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());

            QVariant value;
            if (!m_recorder->getAttribute("FrameRate", &value) ||
                    !value.canConvert(QVariant::Double)) {
                sendMessage(msg.replyMessage(QByteArray(), 1));
                return;
            }
            sendMessage(msg.replyMessage(
                            QByteArray::number(value.toFloat(), 'f', 3)));
            return;
        }

        // get framerate_range
        //     returns: <min> <max>
        //     errorcodes: 1 -> cannot get range values
        if (identifier == "framerate_range")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // get roi
        //     returns: <left> <top> <width> <height>
        //     errorcodes: 1 -> cannot get roi values
        if (identifier == "roi")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // get maximagesize
        //     returns: <width> <height>
        //     errorcodes: 1 -> cannot get image size
        if (identifier == "maximagesize")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // get binning
        //     returns: <xbin> <ybin>
        //     errcodes: 1 -> cannot get x-binning value
        //               2 -> cannot get y-binning value
        //               3 -> cannot get any binning values
        if (identifier == "binning")
        {
            // not implemented yet
            sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
            return;
        }

        // get streaminghost
        //     returns: <address> <port>
        if (identifier == "streaminghost")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            QByteArray address = m_dcp->localAddress().toString().toAscii();
            QByteArray port = QByteArray::number(m_streamingPort);
            sendMessage(msg.replyMessage(address + " " + port));
            return;
        }

        // get camerainfo
        //     returns: <camera name> <unique id> <width> <height> <bitdepth>
        if (identifier == "camerainfo")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            if (!m_recorder->isCameraOpen()) {
                sendMessage(msg.ackMessage(Dcp::AckWrongModeError));
                return;
            }

            sendMessage(msg.ackMessage());
            CameraInfo info = m_recorder->cameraInfo();
            QByteArray reply = m_deviceName + " " +
                    QByteArray::number(uint(info.pvCameraInfo.UniqueId)) + " " +
                    QByteArray::number(info.sensorWidth) + " " +
                    QByteArray::number(info.sensorHeight) + " " +
                    QByteArray::number(info.sensorBits);
            sendMessage(msg.replyMessage(reply));
            return;
        }

        // get version
        //     returns: <server version>
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
        //     returns: <pvapi version>
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

        // get verbose
        //     returns: ( true | false )
        //     note: for debugging
        if (identifier == "verbose")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            sendMessage(msg.replyMessage(verbose() ? "true" : "false"));
            return;
        }

        // get clients
        //     returns: <client list>
        //     note: for debugging
        if (identifier == "clients")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            QList<QByteArray> clientList = m_clientMap.keys();
            QByteArray clients;
            if (!clientList.isEmpty())
                clients = clientList.takeFirst();
            foreach (const QByteArray &client, clientList)
                clients += " " + client;
            sendMessage(msg.replyMessage(clients));
            return;
        }

        // get connections
        //     returns: <stream socket connections>
        //     note: for debugging
        if (identifier == "connections")
        {
            if (m_command.hasArguments()) {
                sendMessage(msg.ackMessage(Dcp::AckParameterError));
                return;
            }
            sendMessage(msg.ackMessage());
            QString connections = m_streamConnectionList.join(" ");
            sendMessage(msg.replyMessage(connections.toAscii()));
            return;
        }

        // get pvattr <name>
        //     note: for debugging only!
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
    }

    // if we get this far the message is not a valid command
    sendMessage(msg.ackMessage(Dcp::AckUnknownCommandError));
}

void SjcServer::recorderFrameFinished(ulong id, int status)
{
    if (verbose())
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
    }

    tPvFrame *frame = m_recorder->readFinishedFrame();
    QMetaObject::invokeMethod(m_imageStreamer, "processFrame",
                              Q_ARG(tPvFrame *, frame));
}


void SjcServer::recorderStarted()
{
    cout << "Capturing started." << endl;
    sendNotification("set camerastate capturing");
}

void SjcServer::recorderStopped()
{
    cout << "Capturing stopped." << endl;
    QByteArray state = (m_recorder->isCameraOpen()) ? "opened" : "closed";
    sendNotification("set camerastate " + state);
}

void SjcServer::streamerConnectionListChanged(const QStringList &connections)
{
    m_streamConnectionList = connections;
}

void SjcServer::streamerThreadStarted()
{
    if (verbose())
        cout << "Streaming server started [" << m_streamingPort << "]."
             << endl;
}

void SjcServer::streamerThreadFinished()
{
    if (verbose())
        cout << "Streaming server stopped." << endl;
    m_streamConnectionList.clear();
}

void SjcServer::writerFrameWritten(int n, int total)
{
    sendNotification("set framewritten " + QByteArray::number(n) + " " +
                     QByteArray::number(total));
}

void SjcServer::writerFrameFinished(tPvFrame *frame)
{
    m_recorder->enqueueFrame(frame);
}

void SjcServer::writerThreadStarted()
{
    if (verbose())
        cout << "Writer thread started." << endl;
}

void SjcServer::writerThreadFinished()
{
    if (verbose())
        cout << "Writer thread stopped." << endl;
}
