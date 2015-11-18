/****************************************************************************
**
** Copyright (C) 2014 Drew Parsons <dparsons@emerall.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    if (ianaId.isEmpty())
        m_id = systemTimeZoneId();
    else
        m_id = ianaId;
}

QTimeZonePrivate *QAndroidTimeZonePrivate::clone()
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

// Since Android does not provide an API to access transitions,
// dataForLocalTime needs to be reimplemented without direct use of transitions
QTimeZonePrivate::Data QAndroidTimeZonePrivate::dataForLocalTime(qint64 forLocalMSecs) const
{
    if (!androidTimeZone.isValid()) {
        return invalidData();
    } else {
        qint64 UTCepochMSecs;

        // compare the UTC time with standard offset against normal DST offset of one hour
        qint64 standardUTCMSecs(forLocalMSecs - (standardTimeOffset(forLocalMSecs) * 1000));
        qint64 daylightUTCMsecs;

        // Check if daylight-saving time applies,
        // checking also for DST boundaries
        if (isDaylightTime(standardUTCMSecs)) {
            // If DST does apply, then standardUTCMSecs will be an hour or so ahead of the real epoch time
            // so check that time
            daylightUTCMsecs = standardUTCMSecs - daylightTimeOffset(standardUTCMSecs)*1000;
            if (isDaylightTime(daylightUTCMsecs)) {
                // DST confirmed
                UTCepochMSecs = daylightUTCMsecs;
            } else {
                // DST has just finished
                UTCepochMSecs = standardUTCMSecs;
            }
        } else {
            // Standard time indicated, but check for a false negative.
            // Would a standard one-hour DST offset indicate DST?
            daylightUTCMsecs = standardUTCMSecs - 3600000; // 3600000 MSECS_PER_HOUR
            if (isDaylightTime(daylightUTCMsecs)) {
                // DST may have just started,
                // but double check against timezone's own DST offset
                // (don't necessarily assume a one-hour offset)
                daylightUTCMsecs = standardUTCMSecs - daylightTimeOffset(daylightUTCMsecs)*1000;
                if (isDaylightTime(daylightUTCMsecs)) {
                    // DST confirmed
                    UTCepochMSecs = daylightUTCMsecs;
                } else {
                    // false positive, apply standard time after all
                    UTCepochMSecs = standardUTCMSecs;
                }
            } else {
                // confirmed standard time
                UTCepochMSecs = standardUTCMSecs;
            }
        }

        return data(UTCepochMSecs);
    }
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
