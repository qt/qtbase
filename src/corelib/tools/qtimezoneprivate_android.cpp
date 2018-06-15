/****************************************************************************
**
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
    // start with system time zone
    androidTimeZone = QJNIObjectPrivate::callStaticObjectMethod("java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
    init("UTC");
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


void QAndroidTimeZonePrivate::init(const QByteArray &ianaId)
{
    QJNIObjectPrivate jo_ianaId = QJNIObjectPrivate::fromString( QString::fromUtf8(ianaId) );
    androidTimeZone = QJNIObjectPrivate::callStaticObjectMethod( "java.util.TimeZone", "getTimeZone", "(Ljava/lang/String;)Ljava/util/TimeZone;",  static_cast<jstring>(jo_ianaId.object()) );

    // Painfully, JNI gives us back a default zone object if it doesn't
    // recognize the name; so check for whether ianaId is a recognized name of
    // the zone object we got and ignore the zone if not.
    // Try checking ianaId against getID(), getDisplayName():
    QJNIObjectPrivate jname = androidTimeZone.callObjectMethod("getID", "()Ljava/lang/String;");
    bool found = (jname.toString().toUtf8() == ianaId);
    for (int style = 1; !found && style-- > 0;) {
        for (int dst = 1; !found && dst-- > 0;) {
            jname = androidTimeZone.callObjectMethod("getDisplayName", "(ZI;)Ljava/lang/String;",
                                                     bool(dst), style);
            found = (jname.toString().toUtf8() == ianaId);
        }
    }

    if (!found)
        m_id.clear();
    else if (ianaId.isEmpty())
        m_id = systemTimeZoneId();
    else
        m_id = ianaId;
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

        QJNIObjectPrivate jlanguage = QJNIObjectPrivate::fromString(QLocale::languageToString(locale.language()));
        QJNIObjectPrivate jcountry = QJNIObjectPrivate::fromString(QLocale::countryToString(locale.country()));
        QJNIObjectPrivate jvariant = QJNIObjectPrivate::fromString(QLocale::scriptToString(locale.script()));
        QJNIObjectPrivate jlocale("java.util.Locale", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", static_cast<jstring>(jlanguage.object()), static_cast<jstring>(jcountry.object()), static_cast<jstring>(jvariant.object()));

        QJNIObjectPrivate jname = androidTimeZone.callObjectMethod("getDisplayName", "(ZILjava/util/Locale;)Ljava/lang/String;", daylightTime, style, jlocale.object());

        name = jname.toString();
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
    QJNIObjectPrivate androidSystemTimeZone = QJNIObjectPrivate::callStaticObjectMethod("java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
    QJNIObjectPrivate systemTZIdAndroid = androidSystemTimeZone.callObjectMethod<jstring>("getID");
    QByteArray systemTZid = systemTZIdAndroid.toString().toUtf8();

    return systemTZid;
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
