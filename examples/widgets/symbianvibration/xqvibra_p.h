#ifndef XQVIBRA_P_H
#define XQVIBRA_P_H

// INCLUDES
#include "xqvibra.h"
#include <HWRMVibra.h>
#include <QTimer>

// CLASS DECLARATION
class XQVibraPrivate: public CBase, public MHWRMVibraObserver
{

public:
    XQVibraPrivate(XQVibra *vibra);
    ~XQVibraPrivate();

    bool start(int aDuration = XQVibra::InfiniteDuration);
    bool stop();
    bool setIntensity(int aIntensity);
    XQVibra::Status currentStatus() const;
    XQVibra::Error error() const;

private: // From MHWRMVibraObserver
    void VibraModeChanged(CHWRMVibra::TVibraModeState aStatus);
    void VibraStatusChanged(CHWRMVibra::TVibraStatus aStatus);

private:
    XQVibra *q;
    XQVibra::Status iStatus;
    CHWRMVibra *iVibra;
    QTimer iTimer;
    int iDuration;
    int iIntensity;
    int iError;
};

#endif /*XQVIBRA_P_H*/

// End of file
