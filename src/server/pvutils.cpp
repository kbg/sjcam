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

#include "pvutils.h"

#include <ctime>
#include <cerrno>

#ifdef _WIN32
#include <windows.h>
int pvmsleep(unsigned int ms)
{
    Sleep(DWORD(ms));
    return 0;
}
#else
int pvmsleep(unsigned int ms)
{
    time_t s = static_cast<time_t>(ms / 1000);
    long ns = (ms - 1000 * s) * 1000000L;

    timespec t, r;
    t.tv_sec = s;
    t.tv_nsec = ns;

    while (nanosleep(&t, &r) == -1)
    {
        if (errno == EINTR)
            t = r;
        else
            return -1;
    }

    return 0;
}
#endif

qint64 PvFrameTimestamp(tPvFrame *frame, uint tsFreq, double timeScale)
{
    Q_ASSERT(frame);
    Q_ASSERT(tsFreq > 0);
    double timestamp = frame->TimestampHi * 4294967295.0 + frame->TimestampLo;
    timestamp *= timeScale / tsFreq;
    return qRound64(timestamp);
}

QString PvVersionString()
{
    ulong major, minor;
    PvVersion(&major, &minor);
    return QString("%1.%2").arg(major).arg(minor);
}

static const int PvErrorCodeCount = 23;

static const char *PvErrorCodeStrList[PvErrorCodeCount] = {
    "ePvErrSuccess",
    "ePvErrCameraFault",
    "ePvErrInternalFault",
    "ePvErrBadHandle",
    "ePvErrBadParameter",
    "ePvErrBadSequence",
    "ePvErrNotFound",
    "ePvErrAccessDenied",
    "ePvErrUnplugged",
    "ePvErrInvalidSetup",
    "ePvErrResources",
    "ePvErrBandwidth",
    "ePvErrQueueFull",
    "ePvErrBufferTooSmall",
    "ePvErrCancelled",
    "ePvErrDataLost",
    "ePvErrDataMissing",
    "ePvErrTimeout",
    "ePvErrOutOfRange",
    "ePvErrWrongType",
    "ePvErrForbidden",
    "ePvErrUnavailable",
    "ePvErrFirewall"
};

static const char *PvErrorMessageList[PvErrorCodeCount] = {
    "No error",
    "Unexpected camera fault",
    "Unexpected fault in PvApi or driver",
    "Camera handle is invalid",
    "Bad parameter to API call",
    "Sequence of API calls is incorrect",
    "Camera or attribute not found",
    "Camera cannot be opened in the specified mode",
    "Camera was unplugged",
    "Setup is invalid (an attribute is invalid)",
    "System/network resources or memory not available",
    "1394 bandwidth not available",
    "Too many frames on queue",
    "Frame buffer is too small",
    "Frame cancelled by user",
    "The data for the frame was lost",
    "Some data in the frame is missing",
    "Timeout during wait",
    "Attribute value is out of the expected range",
    "Attribute is not this type (wrong access function)",
    "Attribute write forbidden at this time",
    "Attribute is not available at this time",
    "A firewall is blocking the traffic"
};

QString PvErrorCodeString(tPvErr errorCode)
{
    if (errorCode >= 0 && errorCode < PvErrorCodeCount)
        return QString(PvErrorCodeStrList[errorCode]);
    else
        return QString();
}

QString PvErrorMessage(tPvErr errorCode)
{
    if (errorCode >= 0 && errorCode < PvErrorCodeCount)
        return QString(PvErrorMessageList[errorCode]);
    else
        return QString("Unkown error");
}

QString formatErrorMessage(const QString &errorString, tPvErr errorCode)
{
    return QString("%1 PvApi: %2. [%3]")
            .arg(errorString)
            .arg(PvErrorMessage(errorCode))
            .arg(PvErrorCodeString(errorCode));
}

CameraInfoList availablePvCameras(uint timeout)
{
    // try to find a camera for timeout milliseconds
    uint numLoops = timeout / 100;
    ulong camCount = 0;
    for (uint i = 0; i < numLoops && camCount < 1; ++i) {
        camCount = PvCameraCount();
        pvmsleep(100);
    }

    if (camCount < 1)
        return CameraInfoList();

    tPvCameraInfoEx *camInfos = new tPvCameraInfoEx[camCount];
    ulong n = PvCameraListEx(camInfos, camCount, 0, sizeof(tPvCameraInfoEx));

    CameraInfoList result;
    for (ulong i = 0; i < n; ++i)
        result.append(*(camInfos + i));

    delete [] camInfos;
    return result;
}

QString permittedAccessString(ulong permittedAccess)
{
    if ((permittedAccess & ePvAccessMaster) != 0)
        return QString("Master");
    else if ((permittedAccess & ePvAccessMonitor) != 0)
        return QString("Monitor");
    return QString("None");
}

QString interfaceTypeString(tPvInterface interfaceType)
{
    if (interfaceType == ePvInterfaceEthernet)
        return QString("GigE");
    else if (interfaceType == ePvInterfaceFirewire)
        return QString("Firewire");
    return QString("Unknown");
}

tPvFrame * allocPvFrame(ulong bufferSize)
{
    tPvFrame *frame = new tPvFrame;
    qMemSet(frame, 0, sizeof(tPvFrame));
    frame->ImageBuffer = new uchar[bufferSize];
    frame->ImageBufferSize = bufferSize;
    frame->AncillaryBuffer = new uchar[48]; // Needs firmware 1.42; the buffer
    frame->AncillaryBufferSize = 48;        // size is 48 bytes (PvApi 1.26)
    return frame;
}

void freePvFrame(tPvFrame *frame)
{
    if (frame) {
        delete [] reinterpret_cast<uchar *>(frame->ImageBuffer);
        delete [] reinterpret_cast<uchar *>(frame->AncillaryBuffer);
        delete frame;
    }
}
