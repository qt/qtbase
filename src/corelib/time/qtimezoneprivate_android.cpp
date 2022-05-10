// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Drew Parsons <dparsons@emerall.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QSet>

QT_BEGIN_NAMESPACE

/*
    Private

    Android implementation

    Note that a QJniObject manages a global reference, so it serves as an
    owning smart-pointer, ensuring an object doesn't get garbage-collected
    before we're done with it.
*/

// Create the system default time zone
QAndroidTimeZonePrivate::QAndroidTimeZonePrivate()
    : QTimeZonePrivate()
{
    // Keep in sync with systemTimeZoneId():
    androidTimeZone = QJniObject::callStaticObjectMethod(
        "java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
    const QJniObject id = androidTimeZone.callObjectMethod("getID", "()Ljava/lang/String;");
    m_id = id.toString().toUtf8();
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

static QString getDisplayName(QJniObject zone, jint style, jboolean dst,
                                        const QLocale &locale)
{
    QJniObject jbcpTag = QJniObject::fromString(locale.bcp47Name());
    QJniObject jlocale = QJniObject::callStaticObjectMethod(
                "java/util/Locale", "forLanguageTag", "(Ljava/lang/String;)Ljava/util/Locale;",
                jbcpTag.object<jstring>());

    return zone.callObjectMethod("getDisplayName",
                                 "(ZILjava/util/Locale;)Ljava/lang/String;",
                                 dst, style, jlocale.object()).toString();
}

void QAndroidTimeZonePrivate::init(const QByteArray &ianaId)
{
    const QString iana = QString::fromUtf8(ianaId);
    androidTimeZone = QJniObject::callStaticObjectMethod(
        "java.util.TimeZone", "getTimeZone", "(Ljava/lang/String;)Ljava/util/TimeZone;",
        QJniObject::fromString(iana).object<jstring>());

    // The ID or display name of the zone we've got, if it looks like what we asked for:
    const auto match = [iana](const QString &name) -> QByteArray {
        if (iana.compare(name, Qt::CaseInsensitive) == 0)
            return name.toUtf8();

        return QByteArray();
    };

    // Painfully, JNI gives us back a default zone object if it doesn't
    // recognize the name; so check for whether ianaId is a recognized name of
    // the zone object we got and ignore the zone if not.
    // Try checking ianaId against getID(), getDisplayName():
    m_id = match(androidTimeZone.callObjectMethod("getID", "()Ljava/lang/String;").toString());
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

        name = getDisplayName(androidTimeZone, style, daylightTime, locale);
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
        /* note: the Java function only tests for future DST transitions, not past */
        return androidTimeZone.callMethod<jboolean>("useDaylightTime" );
    else
        return false;
}

bool QAndroidTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    if ( androidTimeZone.isValid() ) {
        QJniObject jDate( "java/util/Date", "(J)V", static_cast<jlong>(atMSecsSinceEpoch) );
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

// java.util.TimeZone does not directly provide transitions,
// so don't over-ride QTZP's base implementation of transition methods.

QByteArray QAndroidTimeZonePrivate::systemTimeZoneId() const
{
    // Keep in sync with default constructor:
    QJniObject androidSystemTimeZone = QJniObject::callStaticObjectMethod(
        "java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
    const QJniObject id = androidSystemTimeZone.callObjectMethod<jstring>("getID");
    return id.toString().toUtf8();
}

QList<QByteArray> QAndroidTimeZonePrivate::availableTimeZoneIds() const
{
    QList<QByteArray> availableTimeZoneIdList;
    QJniObject androidAvailableIdList = QJniObject::callStaticObjectMethod("java.util.TimeZone", "getAvailableIDs", "()[Ljava/lang/String;");

    QJniEnvironment jniEnv;
    int androidTZcount = jniEnv->GetArrayLength(androidAvailableIdList.object<jarray>());

    // need separate jobject and QJniObject here so that we can delete (DeleteLocalRef) the reference to the jobject
    // (or else the JNI reference table fills after 512 entries from GetObjectArrayElement)
    jobject androidTZobject;
    QJniObject androidTZ;
    for (int i = 0; i < androidTZcount; i++) {
        androidTZobject = jniEnv->GetObjectArrayElement(androidAvailableIdList.object<jobjectArray>(), i);
        androidTZ = androidTZobject;
        availableTimeZoneIdList.append(androidTZ.toString().toUtf8());
        jniEnv->DeleteLocalRef(androidTZobject);
    }

    return availableTimeZoneIdList;
}

QT_END_NAMESPACE
