/*
 * Copyright (c) 2012 Kolja Glogowski
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

#include <QtCore/QtCore>

//#define CLIENTSTARTER_VERBOSE_OUTPUT

#ifdef Q_WS_WIN
    static const char *clientName = "sjcclient.exe";
#else
    static const char *clientName = "sjcclient";
#endif

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QDir appDirectory = QDir(app.applicationDirPath());
    QString appBaseName = QFileInfo(app.applicationFilePath()).baseName();
    QString configFileName = appBaseName + ".ini";
    QString clientFilePath = appDirectory.absoluteFilePath(clientName);

    QStringList args = app.arguments();
    args.removeFirst();
    args << "-c" << appDirectory.absoluteFilePath(configFileName);

#ifdef CLIENTSTARTER_VERBOSE_OUTPUT
    QTextStream cout(stdout, QIODevice::WriteOnly);
    cout << "Starting: \"" << clientFilePath << " " << args.join(" ") << "\"."
         << endl;
#endif

    return QProcess::execute(clientFilePath, args);
}
