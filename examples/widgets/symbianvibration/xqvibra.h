#ifndef XQVIBRA_H
#define XQVIBRA_H

// INCLUDES
#include <QObject>

// FORWARD DECLARATIONS
class XQVibraPrivate;

// CLASS DECLARATION
//! [0]
class XQVibra : public QObject
{
    Q_OBJECT

public:
    static const int InfiniteDuration = 0;
    static const int MaxIntensity = 100;
    static const int MinIntensity = -100;

    enum Error {
        NoError = 0,
        OutOfMemoryError,
        ArgumentError,
        VibraInUseError,
        HardwareError,
        TimeOutError,
        VibraLockedError,
        AccessDeniedError,
        UnknownError = -1
    };

    enum Status {
        StatusNotAllowed = 0,
        StatusOff,
        StatusOn
    };

    XQVibra(QObject *parent = 0);
    ~XQVibra();

    XQVibra::Status currentStatus() const;
    XQVibra::Error error() const;

Q_SIGNALS:
    void statusChanged(XQVibra::Status status);

public Q_SLOTS:
    bool start(int duration = InfiniteDuration);
    bool stop();
    bool setIntensity(int intensity);

private:
    friend class XQVibraPrivate;
    XQVibraPrivate *d;
};
//! [0]

#endif // XQVIBRA_H

// End of file
