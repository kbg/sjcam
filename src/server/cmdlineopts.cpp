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

#include "cmdlineopts.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>

CmdLineOpts::CmdLineOpts()
    : serverName(QString()),
      serverPort(0),
      deviceName(QByteArray()),
      cameraId(0),
      configFileName(QString()),
      verbose(-1),
      list(false),
      info(false),
      version(false),
      help(false)
{
}

bool CmdLineOpts::parse()
{
    QTextStream cout(stdout, QIODevice::WriteOnly);

    QString appName = qApp->applicationName();
    QStringList arguments = qApp->arguments();

    // handle --help first and return if help is requested
    if (arguments.contains("-h") || arguments.contains("--help") ||
            arguments.contains("-help"))
    {
        help = true;
        printHelp();
        return true;
    }

    QListIterator<QString> iter(arguments);
    iter.next(); // skip executable name

    while (iter.hasNext())
    {
        QString arg = iter.next();

        if (arg == "-s") {
            if (!iter.hasNext()) {
                printReqArg("-s");
                return false;
            }
            serverName = iter.next();
        }
        else if (arg == "-p") {
            if (!iter.hasNext()) {
                printReqArg("-p");
                return false;
            }
            bool ok;
            ushort value = iter.next().toUShort(&ok);
            if (!ok) {
                cout << appName << ": argument of option `-p' must be "
                     << "an integer between 1 and 65535.\n" << moreInfo()
                     << endl;
                return false;
            }
            serverPort = quint16(value);
        }
        else if (arg == "-n") {
            if (!iter.hasNext()) {
                printReqArg("-n");
                return false;
            }
            deviceName = iter.next().toAscii();
        }
        else if (arg == "-u") {
            if (!iter.hasNext()) {
                printReqArg("-u");
                return false;
            }
            bool ok;
            ulong value = iter.next().toULong(&ok);
            if (!ok) {
                cout << appName << ": argument of option `-u' must be "
                     << "an unsigned integer.\n" << moreInfo()
                     << endl;
                return false;
            }
            cameraId = value;
        }
        else if (arg == "-c") {
            if (!iter.hasNext()) {
                printReqArg("-c");
                return false;
            }
            configFileName = iter.next();
        }
        else if (arg == "-v") {
            verbose = 1;
        }
        else if (arg == "--list") {
            list = true;
        }
        else if (arg == "--info") {
            info = true;
        }
        else if (arg == "--version") {
            version = true;
        }
        else if (arg.startsWith('-')) {
            cout << appName << ": unknown option `" << arg << "'.\n"
                 << moreInfo() << endl;
            return false;
        }
        else {
            cout << appName
                 << ": invalid command line argument.\n"
                 << moreInfo() << endl;
            return false;
        }
    }

    return true;
}

void CmdLineOpts::printHelp()
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    cout << "Usage: " << qApp->applicationName() << " [options]\n"
         << "\nOptions:"
         << "\n  -s name     DCP server name [localhost]"
         << "\n  -p port     DCP server port [2001]"
         << "\n  -n device   DCP device name [sjcam]"
         << "\n  -u id       Select camera by its unique ID"
         << "\n  -c file     Load configuration from config file"
         << "\n  -v          Verbose text output"
         << "\n  --list      List available cameras and quit"
         << "\n  --info      Show camera informations and quit"
         << "\n  --version   Show program version and quit"
         << "\n  -h, --help  Show this help message and quit"
         << "\n" << endl;
}

void CmdLineOpts::printReqArg(const QString &optionName)
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    cout << qApp->applicationName() << ": option `" << optionName
         << "' requires an argument.\n" << moreInfo() << endl;
}

QString CmdLineOpts::moreInfo()
{
    return QString("Try `%1 --help' for more information.")
            .arg(qApp->applicationName());
}
