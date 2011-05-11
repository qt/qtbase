
#include "mcevibrator.h"

#include <QStringList>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusMessage>

#include <mce/dbus-names.h>

const char MceVibrator::defaultMceFilePath[] = "/etc/mce/mce.ini";

//! [5]
static void checkError(QDBusMessage &msg)
{
    if (msg.type() == QDBusMessage::ErrorMessage)
        qDebug() << msg.errorName() << msg.errorMessage();
}
//! [5]

//! [0]
MceVibrator::MceVibrator(QObject *parent) :
    QObject(parent),
    mceInterface(MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF,
                   QDBusConnection::systemBus())
{
    QDBusMessage reply = mceInterface.call(MCE_ENABLE_VIBRATOR);
    checkError(reply);
}
//! [0]

//! [3]
MceVibrator::~MceVibrator()
{
    deactivate(lastPatternName);
    QDBusMessage reply = mceInterface.call(MCE_DISABLE_VIBRATOR);
    checkError(reply);
}
//! [3]

//! [1]
void MceVibrator::vibrate(const QString &patternName)
{
    deactivate(lastPatternName);
    lastPatternName = patternName;
    QDBusMessage reply = mceInterface.call(MCE_ACTIVATE_VIBRATOR_PATTERN, patternName);
    checkError(reply);
}
//! [1]

//! [2]
void MceVibrator::deactivate(const QString &patternName)
{
    if (!patternName.isNull()) {
        QDBusMessage reply = mceInterface.call(MCE_DEACTIVATE_VIBRATOR_PATTERN, patternName);
        checkError(reply);
    }
}
//! [2]

//! [4]
QStringList MceVibrator::parsePatternNames(QTextStream &stream)
{
    QStringList result;
    QString line;

    do {
        line = stream.readLine();
        if (line.startsWith(QLatin1String("VibratorPatterns="))) {
            QString values = line.section('=', 1);
            result = values.split(';');
            break;
        }
    } while (!line.isNull());

    return result;
}
//! [4]

