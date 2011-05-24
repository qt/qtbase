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
