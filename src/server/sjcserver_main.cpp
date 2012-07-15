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
#include "cmdlineopts.h"
#include "version.h"
#include "pvutils.h"
#include "recorder.h"
#include <QtCore/QtCore>
#include <csignal>

static void exitHandler(int param) {
    Q_UNUSED(param);
    QCoreApplication::exit(0);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QFileInfo(app.arguments()[0]).fileName());

    // register types which are used as slot arguments
    qRegisterMetaType<tPvFrame *>("tPvFrame *");
    qRegisterMetaType<CameraInfo>("CameraInfo");

    // use custom signal handler for SIGINT and SIGTERM to perform a clean
    // shutdown on CTRL+C or 'kill -15'
    std::signal(SIGINT, exitHandler);
    std::signal(SIGTERM, exitHandler);

    CmdLineOpts opts;
    if (!opts.parse() || opts.help)
        return opts.help ? 0 : 1;

    QTextStream cout(stdout, QIODevice::WriteOnly);

    if (opts.version) {
        cout << "SjcServer version " << SJCAM_VERSION_STRING << "\n"
             << SJCAM_COPYRIGHT_STRING << "\n" << endl;
        return 0;
    }

    SjcServer server(opts);

    if (opts.list || opts.info)
    {
        cout << "PvApi Version: " << PvVersionString() << endl;
        cout << "Searching for cameras..." << endl;
        CameraInfoList cameraList = availablePvCameras();

        if (cameraList.isEmpty()) {
            cout << "Error: No cameras found." << endl;
            return 1;
        }

        if (opts.list) {
            uint maxNameSize = 0;
            foreach (const tPvCameraInfoEx &info, cameraList)
                maxNameSize = qMax(qstrlen(info.CameraName), maxNameSize);
            cout << "\nAvailable Cameras:\n";
            foreach (const tPvCameraInfoEx &info, cameraList)
                cout << qSetFieldWidth(maxNameSize + 4)
                     << info.CameraName << qSetFieldWidth(0) << " - "
                     << info.SerialNumber << " - "
                     << "UniqueId: " << info.UniqueId << "\n";
        }

        if (opts.info) {
            int i = 0;
            foreach (const tPvCameraInfoEx &info, cameraList) {
                if (opts.cameraId != 0 && opts.cameraId != info.UniqueId)
                    continue;
                cout << "\nCamera " << i++ << ":"
                     << "\n    UniqueId .......... " << info.UniqueId
                     << "\n    CameraName ........ " << info.CameraName
                     << "\n    ModelName ......... " << info.ModelName
                     << "\n    SerialNumber ...... " << info.SerialNumber
                     << "\n    FirmwareVersion ... " << info.FirmwareVersion
                     << "\n    PermittedAccess ... "
                            << permittedAccessString(info.PermittedAccess)
                     << "\n    InterfaceType ..... "
                            << interfaceTypeString(info.InterfaceType)
                     << "\n    InterfaceId ....... " << info.InterfaceId
                     << endl;
            }
        }

        cout << endl;
        return 0;
    }

    QTimer::singleShot(0, &server, SLOT(connectToDcpServer()));
    if (server.openCamera())
        QTimer::singleShot(0, &server, SLOT(startCapturing()));

    return app.exec();
}
