/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2014 Drew Parsons <dparsons@emerall.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QSet>
#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

QT_BEGIN_NAMESPACE

/*
    Private

    Android implementation
*/

// Create the system default time zone
QAndroidTimeZonePrivate::QAndroidTimeZonePrivate()
    : QTimeZonePrivate()
{
    // Keep in sync with systemTimeZoneId():
    androidTimeZone = QJNIObjectPrivate::callStaticObjectMethod(
        "java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
    m_id = androidTimeZone.callObjectMethod("getID", "()Ljava/lang/String;").toString().toUtf8();
}

// Create a named time zone
QAndroidTimeZonePrivate::QAndroidTimeZonePrivate(const QByteArray &ianaId)
    : QTimeZonePrivate()
{
    init(ianaId);
}

QAndroidTimeZonePrivate::QAndroidTimeZonePrivate(const QAndroidTimeZonePrivate &other)
    : QTimeZonePrivate(other)
{
    androidTimeZone = other.androidTimeZone;
    m_id = other.id();
}

QAndroidTimeZonePrivate::~QAndroidTimeZonePrivate()
{
}

static QJNIObjectPrivate getDisplayName(QJNIObjectPrivate zone, jint style, jboolean dst,
                                        const QLocale &locale)
{
    QJNIObjectPrivate jlanguage
        = QJNIObjectPrivate::fromString(QLocale::languageToString(locale.language()));
    QJNIObjectPrivate jcountry
        = QJNIObjectPrivate::fromString(QLocale::countryToString(locale.country()));
    QJNIObjectPrivate
        jvariant = QJNIObjectPrivate::fromString(QLocale::scriptToString(locale.script()));
    QJNIObjectPrivate jlocale("java.util.Locale",
                              "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                              static_cast<jstring>(jlanguage.object()),
                              static_cast<jstring>(jcountry.object()),
                              static_cast<jstring>(jvariant.object()));

    return zone.callObjectMethod("getDisplayName",
                                 "(ZILjava/util/Locale;)Ljava/lang/String;",
                                 dst, style, jlocale.object());
}

void QAndroidTimeZonePrivate::init(const QByteArray &ianaId)
{
    const QString iana = QString::fromUtf8(ianaId);
    androidTimeZone = QJNIObjectPrivate::callStaticObjectMethod(
        "java.util.TimeZone", "getTimeZone", "(Ljava/lang/String;)Ljava/util/TimeZone;",
        static_cast<jstring>(QJNIObjectPrivate::fromString(iana).object()));

    // The ID or display name of the zone we've got, if it looks like what we asked for:
    const auto match = [iana](const QJNIObjectPrivate &jname) -> QByteArray {
        const QString name = jname.toString();
        if (iana.compare(name, Qt::CaseInsensitive))
            return name.toUtf8();

        return QByteArray();
    };

    // Painfully, JNI gives us back a default zone object if it doesn't
    // recognize the name; so check for whether ianaId is a recognized name of
    // the zone object we got and ignore the zone if not.
    // Try checking ianaId against getID(), getDisplayName():
    m_id = match(androidTimeZone.callObjectMethod("getID", "()Ljava/lang/String;"));
    for (int style = 1; m_id.isEmpty() && style >= 0; --style) {
        for (int dst = 1; m_id.isEmpty() && dst >= 0; --dst) {
            for (int pick = 2; m_id.isEmpty() && pick >= 0; --pick) {
                QLocale locale = (pick == 0 ? QLocale::system()
                                  : pick == 1 ? QLocale() : QLocale::c());
                m_id = match(getDisplayName(androidTimeZone, style, jboolean(dst), locale));
            }
        }
    }
}

QAndroidTimeZonePrivate *QAndroidTimeZonePrivate::clone() const
{
    return new QAndroidTimeZonePrivate(*this);
}


QString QAndroidTimeZonePrivate::displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                                             const QLocale &locale) const
{
    QString name;

    if (androidTimeZone.isValid()) {
        jboolean daylightTime = (timeType == QTimeZone::DaylightTime);  // treat QTimeZone::GenericTime as QTimeZone::StandardTime

        // treat all NameTypes as java TimeZone style LONG (value 1), except of course QTimeZone::ShortName which is style SHORT (value 0);
        jint style = (nameType == QTimeZone::ShortName ? 0 : 1);

        name = getDisplayName(androidTimeZone, style, daylightTime, locale).toString();
    }

    return name;
}

QString QAndroidTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    if ( isDaylightTime( atMSecsSinceEpoch ) )
        return displayName(QTimeZone::DaylightTime, QTimeZone::ShortName, QLocale() );
    else
        return displayName(QTimeZone::StandardTime, QTimeZone::ShortName, QLocale() );
}

int QAndroidTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    // offsetFromUtc( ) is invoked when androidTimeZone may not yet be set at startup,
    // so a validity test is needed here
    if ( androidTimeZone.isValid() )
        // the java method getOffset() returns milliseconds, but QTimeZone returns seconds
        return androidTimeZone.callMethod<jint>( "getOffset", "(J)I", static_cast<jlong>(atMSecsSinceEpoch) ) / 1000;
    else
        return 0;
}

int QAndroidTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    // the java method does not use a reference time
    Q_UNUSED( atMSecsSinceEpoch );
    if ( androidTimeZone.isValid() )
        // the java method getRawOffset() returns milliseconds, but QTimeZone returns seconds
        return androidTimeZone.callMethod<jint>( "getRawOffset" ) / 1000;
    else
        return 0;
}

int QAndroidTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return ( offsetFromUtc(atMSecsSinceEpoch) - standardTimeOffset(atMSecsSinceEpoch) );
}

bool QAndroidTimeZonePrivate::hasDaylightTime() const
{
    if ( androidTimeZone.isValid() )
        /* note: the Java function only tests for future DST transtions, not past */
        return androidTimeZone.callMethod<jboolean>("useDaylightTime" );
    else
        return false;
}

bool QAndroidTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    if ( androidTimeZone.isValid() ) {
        QJNIObjectPrivate jDate( "java/util/Date", "(J)V", static_cast<jlong>(atMSecsSinceEpoch) );
        return androidTimeZone.callMethod<jboolean>("inDaylightTime", "(Ljava/util/Date;)Z", jDate.object() );
    }
    else
        return false;
}

QTimeZonePrivate::Data QAndroidTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    if (androidTimeZone.isValid()) {
        Data data;
        data.atMSecsSinceEpoch = forMSecsSinceEpoch;
        data.standardTimeOffset = standardTimeOffset(forMSecsSinceEpoch);
        data.offsetFromUtc = offsetFromUtc(forMSecsSinceEpoch);
        data.daylightTimeOffset = data.offsetFromUtc - data.standardTimeOffset;
        data.abbreviation = abbreviation(forMSecsSinceEpoch);
        return data;
    } else {
        return invalidData();
    }
}

bool QAndroidTimeZonePrivate::hasTransitions() const
{
    // java.util.TimeZone does not directly provide transitions
    return false;
}

QTimeZonePrivate::Data QAndroidTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    // transitions not available on Android, so return an invalid data object
    Q_UNUSED( afterMSecsSinceEpoch );
    return invalidData();
}

QTimeZonePrivate::Data QAndroidTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // transitions not available on Android, so return an invalid data object
    Q_UNUSED( beforeMSecsSinceEpoch );
    return invalidData();
}

QByteArray QAndroidTimeZonePrivate::systemTimeZoneId() const
{
    // Keep in sync with default constructor:
    QJNIObjectPrivate androidSystemTimeZone = QJNIObjectPrivate::callStaticObjectMethod(
        "java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
    return androidSystemTimeZone.callObjectMethod<jstring>("getID").toString().toUtf8();
}

QList<QByteArray> QAndroidTimeZonePrivate::availableTimeZoneIds() const
{
    QList<QByteArray> availableTimeZoneIdList;
    QJNIObjectPrivate androidAvailableIdList = QJNIObjectPrivate::callStaticObjectMethod("java.util.TimeZone", "getAvailableIDs", "()[Ljava/lang/String;");

    QJNIEnvironmentPrivate jniEnv;
    int androidTZcount = jniEnv->GetArrayLength( static_cast<jarray>(androidAvailableIdList.object()) );

    // need separate jobject and QAndroidJniObject here so that we can delete (DeleteLocalRef) the reference to the jobject
    // (or else the JNI reference table fills after 512 entries from GetObjectArrayElement)
    jobject androidTZobject;
    QJNIObjectPrivate androidTZ;
    for (int i=0; i<androidTZcount; i++ ) {
        androidTZobject = jniEnv->GetObjectArrayElement( static_cast<jobjectArray>( androidAvailableIdList.object() ), i );
        androidTZ = androidTZobject;
        availableTimeZoneIdList.append( androidTZ.toString().toUtf8() );
        jniEnv->DeleteLocalRef(androidTZobject);
    }

    return availableTimeZoneIdList;
}

QT_END_NAMESPACE
