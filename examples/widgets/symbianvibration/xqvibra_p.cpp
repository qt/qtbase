/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "xqvibra_p.h"

const int KDefaultIntensity = 0xFF;

XQVibraPrivate::XQVibraPrivate(XQVibra *vibra)
    : q(vibra), iStatus(XQVibra::StatusOff), iDuration(XQVibra::InfiniteDuration), iIntensity(KDefaultIntensity)

{
    TRAP(iError, iVibra = CHWRMVibra::NewL();)
    QObject::connect(&iTimer, SIGNAL(timeout()), q, SLOT(stop()));
}

XQVibraPrivate::~XQVibraPrivate()
{
    delete iVibra;
}

bool XQVibraPrivate::start(int aDuration)
{
    iDuration = aDuration;
    TRAP(iError,
        if (iIntensity == KDefaultIntensity) {
            iVibra->StartVibraL(XQVibra::InfiniteDuration);
        } else {
            iVibra->StopVibraL();
            iVibra->StartVibraL(XQVibra::InfiniteDuration, iIntensity);
        }

        if (aDuration != XQVibra::InfiniteDuration) {
            iTimer.start(aDuration);
        } else {
            iTimer.stop();
        }

        if (iStatus != XQVibra::StatusOn) {
            iStatus = XQVibra::StatusOn;
            emit q->statusChanged(iStatus);
        }
    )
    return (iError == KErrNone);
}

bool XQVibraPrivate::stop()
{
    TRAP(iError,
        if (iVibra->VibraStatus() == CHWRMVibra::EVibraStatusOn) {
            iVibra->StopVibraL();
            if (iTimer.isActive()) {
                iTimer.stop();
            }
        }

        iStatus = XQVibra::StatusOff;
        emit q->statusChanged(iStatus);
    )
    return (iError == KErrNone);
}

void XQVibraPrivate::VibraModeChanged(CHWRMVibra::TVibraModeState /*aStatus*/)
{
    // Implementation isn't needed here because this information isn't used in the public side of the extension
}

void XQVibraPrivate::VibraStatusChanged(CHWRMVibra::TVibraStatus aStatus)
{
    if (aStatus == CHWRMVibra::EVibraStatusUnknown ||
            aStatus == CHWRMVibra::EVibraStatusNotAllowed) {
        iStatus = XQVibra::StatusNotAllowed;
        emit q->statusChanged(iStatus);
    }

    if (iDuration ==  XQVibra::InfiniteDuration) {
        if (iStatus != XQVibra::StatusOff) {
            iStatus = XQVibra::StatusOff;
            emit q->statusChanged(iStatus);
        }
    }
}

bool XQVibraPrivate::setIntensity(int aIntensity)
{
    TRAP(iError,
        if (aIntensity >= KHWRMVibraMinIntensity && aIntensity <= KHWRMVibraMaxIntensity) {
            iIntensity = aIntensity;
            if (iIntensity == 0 && iStatus == XQVibra::StatusOn) {
                iVibra->StopVibraL();
            } else if (iStatus == XQVibra::StatusOn) {
                iVibra->StopVibraL();
                iVibra->StartVibraL(XQVibra::InfiniteDuration, iIntensity);
            }
        } else {
            User::Leave(KErrArgument);
        }
    )
    return (iError == KErrNone);
}

XQVibra::Status XQVibraPrivate::currentStatus() const
{
    if (iVibra->VibraStatus() == CHWRMVibra::EVibraStatusUnknown ||
            iVibra->VibraStatus() == CHWRMVibra::EVibraStatusNotAllowed) {
        return XQVibra::StatusNotAllowed;
    }
    return iStatus;
}

XQVibra::Error XQVibraPrivate::error() const
{
    switch (iError) {
    case KErrNone:
        return XQVibra::NoError;
    case KErrNoMemory:
        return XQVibra::OutOfMemoryError;
    case KErrArgument:
        return XQVibra::ArgumentError;
    case KErrInUse:
        return XQVibra::VibraInUseError;
    case KErrGeneral:
        return XQVibra::HardwareError;
    case KErrTimedOut:
        return XQVibra::TimeOutError;
    case KErrLocked:
        return XQVibra::VibraLockedError;
    case KErrAccessDenied:
        return XQVibra::AccessDeniedError;
    default:
        return XQVibra::UnknownError;
    }
}

// End of file
