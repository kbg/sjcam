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

#ifndef SJCAM_PVUTILS_H
#define SJCAM_PVUTILS_H

#include <QtCore/QList>
#include <QtCore/QString>
#include <PvApi.h>

// The sleep() / usleep() functions don't work under Linux with the SIGALRM
// spamming Prosilica API.
int msleep(unsigned int ms);

QString PvVersionString();
QString PvErrorCodeString(tPvErr errorCode);
QString PvErrorMessage(tPvErr errorCode);
QString formatErrorMessage(const QString &errorString, tPvErr errorCode);

typedef QList<tPvCameraInfoEx> CameraInfoList;
CameraInfoList availablePvCameras(uint timeout = 3000);

QString permittedAccessString(ulong permittedAccess);
QString interfaceTypeString(tPvInterface interfaceType);

tPvFrame * allocPvFrame(ulong bufferSize);
void freePvFrame(tPvFrame *frame);

#endif // SJCAM_PVUTILS_H
