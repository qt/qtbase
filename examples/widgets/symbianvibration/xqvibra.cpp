#include "xqvibra.h"
#include "xqvibra_p.h"

/*!
    \class XQVibra

    \brief The XQVibra class is used to control the device's vibra. The XQVibra
    class provides also information of vibration setting in the user profile.

    Example:
    \code
    XQVibra *vibra = new XQVibra(this);
    QPushButton *startButton = new QPushButton(this);
    QPushButton *stopButton = new QPushButton(this);
    connect(startButton, SIGNAL(clicked()), vibra, SLOT(start()));
    connect(stopButton, SIGNAL(clicked()), vibra, SLOT(stop()));
    \endcode
*/

/*! \var XQVibra::InfiniteDuration
    With this constant vibration can be set to work indefinitely (Note! Depends on the HW)
*/
/*! \var XQVibra::MaxIntensity
    Maximum intensity as percentages
*/
/*! \var XQVibra::MinIntensity
    Minumum intensity as percentages
*/

/*!
    Constructs a XQVibra object with the given parent.
    Call error() to get a value of XQVibra::Error that indicates which error occurred during initialisation if any.
    \sa start(), setIntensity(), error()
*/
XQVibra::XQVibra(QObject *parent)
    : QObject(parent), d(new XQVibraPrivate(this))
{
}

/*!
    Destroys the XQVibra object.
*/
XQVibra::~XQVibra()
{
    delete d;
}

/*!
    \enum XQVibra::Error

    This enum defines the possible errors for a XQVibra object.
*/
/*! \var XQVibra::Error XQVibra::NoError
    No error occured.
*/
/*! \var XQVibra::Error XQVibra::OutOfMemoryError
    Not enough memory.
*/
/*! \var XQVibra::Error XQVibra::ArgumentError
    Duration is invalid.
*/
/*! \var XQVibra::Error XQVibra::VibraInUseError
    Vibra is already in used by other client.
*/
/*! \var XQVibra::Error XQVibra::HardwareError
    There is a hardware error.
*/
/*! \var XQVibra::Error XQVibra::TimeOutError
    Timeout occurred in controlling vibra.
*/
/*! \var XQVibra::Error XQVibra::VibraLockedError
    Vibra is locked down because too much continuous use or explicitly blocked by
    for example some vibration sensitive accessory.
*/
/*! \var XQVibra::Error XQVibra::AccessDeniedError
    Vibration setting in the user profile is not set.
*/
/*! \var XQVibra::Error XQVibra::UnknownError
    Unknown error.
*/

/*!
    \enum XQVibra::Status

    This enum defines the possible statuses of the vibration
*/
/*! \var XQVibra::Status XQVibra::StatusNotAllowed
    Vibra is set off in the user profile or status is unknow
*/
/*! \var XQVibra::Status XQVibra::StatusOff
    Vibration is non-active
*/
/*! \var XQVibra::Status XQVibra::StatusOn
    Vibration is active
*/

/*!
    Starts vibrating. If duration hasn't been set the vibration continues
    indefinitely unless stopped with stop() function. Calling the start while vibration
    is active the active vibration is interrupted and the new vibration starts immediately.

    \param duration Specifies duration how long vibration should last
    \return If false is returned, an error has occurred. Call error() to get a value of
    XQVibra::Error that indicates which error occurred
    \sa stop(), setIntensity(), error()
*/
bool XQVibra::start(int duration)
{
    return d->start(duration);
}

/*!
    Interrupts the device vibration immediately.

    \return If false is returned, an error has occurred. Call error() to get a value of
    XQVibra::Error that indicates which error occurred
    \sa start(), setIntensity(), error()
*/
bool XQVibra::stop()
{
    return d->stop();
}

/*!
    Sets the intensity of the vibration. Allowed values for the intensity are
    between -100 and 100. 0 means no vibrating. NOTE: The device might have
    hardware-imposed limits on supported vibra intensity values, so actual
    effect might vary between different hardware.

    \param intensity Intensity of the vibra in decimals
    \return If false is returned, an error has occurred. Call error() to get a value of
    XQVibra::Error that indicates which error occurred
    \sa error()
*/
bool XQVibra::setIntensity(int intensity)
{
    return d->setIntensity(intensity);
}

/*!
    Returns the current status of the vibration. This function can be used to check has vibration
    allowed in the user profile.

    \return current status
    \sa statusChanged()
*/
XQVibra::Status XQVibra::currentStatus() const
{
    return d->currentStatus();
}

/*!
    Returns the type of error that occurred if the latest function call failed; otherwise returns NoError
    \return Error code
*/
XQVibra::Error XQVibra::error() const
{
    return d->error();
}

/*!
    \fn void XQVibra::statusChanged(Status status)

    This signal is emitted when the there is a change of the vibration status.

    \param status a vibration status
    \sa currentStatus()
*/

// End of file
