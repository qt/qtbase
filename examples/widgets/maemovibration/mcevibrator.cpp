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

