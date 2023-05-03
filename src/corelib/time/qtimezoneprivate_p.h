// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QTIMEZONEPRIVATE_P_H
#define QTIMEZONEPRIVATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qlist.h"
#include "qtimezone.h"
#include "private/qlocale_p.h"

#if QT_CONFIG(icu)
#include <unicode/ucal.h>
#endif

#ifdef Q_OS_DARWIN
Q_FORWARD_DECLARE_OBJC_CLASS(NSTimeZone);
#endif // Q_OS_DARWIN

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif // Q_OS_WIN

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QTimeZonePrivate : public QSharedData
{
public:
    //Version of QTimeZone::OffsetData struct using msecs for efficiency
    struct Data {
        QString abbreviation;
        qint64 atMSecsSinceEpoch;
        int offsetFromUtc;
        int standardTimeOffset;
        int daylightTimeOffset;
    };
    typedef QList<Data> DataList;

    // Create null time zone
    QTimeZonePrivate();
    QTimeZonePrivate(const QTimeZonePrivate &other);
    virtual ~QTimeZonePrivate();

    virtual QTimeZonePrivate *clone() const;

    bool operator==(const QTimeZonePrivate &other) const;
    bool operator!=(const QTimeZonePrivate &other) const;

    bool isValid() const;

    QByteArray id() const;
    virtual QLocale::Territory territory() const;
    virtual QString comment() const;

    virtual QString displayName(qint64 atMSecsSinceEpoch,
                                QTimeZone::NameType nameType,
                                const QLocale &locale) const;
    virtual QString displayName(QTimeZone::TimeType timeType,
                                QTimeZone::NameType nameType,
                                const QLocale &locale) const;
    virtual QString abbreviation(qint64 atMSecsSinceEpoch) const;

    virtual int offsetFromUtc(qint64 atMSecsSinceEpoch) const;
    virtual int standardTimeOffset(qint64 atMSecsSinceEpoch) const;
    virtual int daylightTimeOffset(qint64 atMSecsSinceEpoch) const;

    virtual bool hasDaylightTime() const;
    virtual bool isDaylightTime(qint64 atMSecsSinceEpoch) const;

    virtual Data data(qint64 forMSecsSinceEpoch) const;
    Data dataForLocalTime(qint64 forLocalMSecs, int hint) const;

    virtual bool hasTransitions() const;
    virtual Data nextTransition(qint64 afterMSecsSinceEpoch) const;
    virtual Data previousTransition(qint64 beforeMSecsSinceEpoch) const;
    DataList transitions(qint64 fromMSecsSinceEpoch, qint64 toMSecsSinceEpoch) const;

    virtual QByteArray systemTimeZoneId() const;

    virtual bool isTimeZoneIdAvailable(const QByteArray &ianaId) const;
    virtual QList<QByteArray> availableTimeZoneIds() const;
    virtual QList<QByteArray> availableTimeZoneIds(QLocale::Territory territory) const;
    virtual QList<QByteArray> availableTimeZoneIds(int utcOffset) const;

    virtual void serialize(QDataStream &ds) const;

    // Static Utility Methods
    [[nodiscard]] static constexpr qint64 maxMSecs()
    { return (std::numeric_limits<qint64>::max)(); }
    [[nodiscard]] static constexpr qint64 minMSecs()
    { return (std::numeric_limits<qint64>::min)() + 1; }
    [[nodiscard]] static constexpr qint64 invalidMSecs()
    { return (std::numeric_limits<qint64>::min)(); }
    [[nodiscard]] static constexpr qint64 invalidSeconds()
    { return (std::numeric_limits<int>::min)(); }
    static Data invalidData();
    static QTimeZone::OffsetData invalidOffsetData();
    static QTimeZone::OffsetData toOffsetData(const Data &data);
    static bool isValidId(const QByteArray &ianaId);
    static QString isoOffsetFormat(int offsetFromUtc,
                                   QTimeZone::NameType mode = QTimeZone::OffsetName);

    static QByteArray ianaIdToWindowsId(const QByteArray &ianaId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                                QLocale::Territory territory);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId,
                                                 QLocale::Territory territory);

    // returns "UTC" QString and QByteArray
    [[nodiscard]] static inline QString utcQString()
    {
        return QStringLiteral("UTC");
    }

    [[nodiscard]] static inline QByteArray utcQByteArray()
    {
        return QByteArrayLiteral("UTC");
    }

protected:
    QByteArray m_id;
};
Q_DECLARE_TYPEINFO(QTimeZonePrivate::Data, Q_RELOCATABLE_TYPE);

template<> QTimeZonePrivate *QSharedDataPointer<QTimeZonePrivate>::clone();

class Q_AUTOTEST_EXPORT QUtcTimeZonePrivate final : public QTimeZonePrivate
{
public:
    // Create default UTC time zone
    QUtcTimeZonePrivate();
    // Create named time zone
    QUtcTimeZonePrivate(const QByteArray &utcId);
    // Create offset from UTC
    QUtcTimeZonePrivate(qint32 offsetSeconds);
    // Create custom offset from UTC
    QUtcTimeZonePrivate(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                        const QString &abbreviation, QLocale::Territory territory,
                        const QString &comment);
    QUtcTimeZonePrivate(const QUtcTimeZonePrivate &other);
    virtual ~QUtcTimeZonePrivate();

    // Fall-back for UTC[+-]\d+(:\d+){,2} IDs.
    static qint64 offsetFromUtcString(const QByteArray &id);

    QUtcTimeZonePrivate *clone() const override;

    Data data(qint64 forMSecsSinceEpoch) const override;

    QLocale::Territory territory() const override;
    QString comment() const override;

    QString displayName(QTimeZone::TimeType timeType,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;
    QList<QByteArray> availableTimeZoneIds(QLocale::Territory country) const override;
    QList<QByteArray> availableTimeZoneIds(int utcOffset) const override;

    void serialize(QDataStream &ds) const override;

private:
    void init(const QByteArray &zoneId);
    void init(const QByteArray &zoneId, int offsetSeconds, const QString &name,
              const QString &abbreviation, QLocale::Territory territory,
              const QString &comment);

    QString m_name;
    QString m_abbreviation;
    QString m_comment;
    QLocale::Territory m_territory;
    int m_offsetFromUtc;
};

#if QT_CONFIG(icu)
class Q_AUTOTEST_EXPORT QIcuTimeZonePrivate final : public QTimeZonePrivate
{
public:
    // Create default time zone
    QIcuTimeZonePrivate();
    // Create named time zone
    QIcuTimeZonePrivate(const QByteArray &ianaId);
    QIcuTimeZonePrivate(const QIcuTimeZonePrivate &other);
    ~QIcuTimeZonePrivate();

    QIcuTimeZonePrivate *clone() const override;

    using QTimeZonePrivate::displayName;
    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    QList<QByteArray> availableTimeZoneIds() const override;
    QList<QByteArray> availableTimeZoneIds(QLocale::Territory territory) const override;
    QList<QByteArray> availableTimeZoneIds(int offsetFromUtc) const override;

private:
    void init(const QByteArray &ianaId);

    UCalendar *m_ucal;
};
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN) && !defined(Q_OS_ANDROID)
struct QTzTransitionTime
{
    qint64 atMSecsSinceEpoch;
    quint8 ruleIndex;
};
Q_DECLARE_TYPEINFO(QTzTransitionTime, Q_PRIMITIVE_TYPE);
struct QTzTransitionRule
{
    int stdOffset;
    int dstOffset;
    quint8 abbreviationIndex;
};
Q_DECLARE_TYPEINFO(QTzTransitionRule, Q_PRIMITIVE_TYPE);
constexpr inline bool operator==(const QTzTransitionRule &lhs, const QTzTransitionRule &rhs) noexcept
{ return lhs.stdOffset == rhs.stdOffset && lhs.dstOffset == rhs.dstOffset && lhs.abbreviationIndex == rhs.abbreviationIndex; }
constexpr inline bool operator!=(const QTzTransitionRule &lhs, const QTzTransitionRule &rhs) noexcept
{ return !operator==(lhs, rhs); }

// These are stored separately from QTzTimeZonePrivate so that they can be
// cached, avoiding the need to re-parse them from disk constantly.
struct QTzTimeZoneCacheEntry
{
    QList<QTzTransitionTime> m_tranTimes;
    QList<QTzTransitionRule> m_tranRules;
    QList<QByteArray> m_abbreviations;
    QByteArray m_posixRule;
    QTzTransitionRule m_preZoneRule;
    bool m_hasDst;
};

class Q_AUTOTEST_EXPORT QTzTimeZonePrivate final : public QTimeZonePrivate
{
    QTzTimeZonePrivate(const QTzTimeZonePrivate &) = default;
public:
    // Create default time zone
    QTzTimeZonePrivate();
    // Create named time zone
    QTzTimeZonePrivate(const QByteArray &ianaId);
    ~QTzTimeZonePrivate();

    QTzTimeZonePrivate *clone() const override;

    QLocale::Territory territory() const override;
    QString comment() const override;

    QString displayName(qint64 atMSecsSinceEpoch,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString displayName(QTimeZone::TimeType timeType,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;
    QList<QByteArray> availableTimeZoneIds(QLocale::Territory territory) const override;

private:
    static QByteArray staticSystemTimeZoneId();
    QList<QTimeZonePrivate::Data> getPosixTransitions(qint64 msNear) const;

    Data dataForTzTransition(QTzTransitionTime tran) const;
    Data dataFromRule(QTzTransitionRule rule, qint64 msecsSinceEpoch) const;
#if QT_CONFIG(icu)
# ifdef __cpp_lib_is_final
    static_assert(std::is_final<QIcuTimeZonePrivate>::value,
                  "if QIcuTimeZonePrivate isn't final, we may need to specialize "
                  "QExplicitlySharedDataPointer::clone() to call QTimeZonePrivate::clone()");
# endif
    mutable QExplicitlySharedDataPointer<const QIcuTimeZonePrivate> m_icu;
#endif
    QTzTimeZoneCacheEntry cached_data;
    const QList<QTzTransitionTime> &tranCache() const { return cached_data.m_tranTimes; }
};
#endif // Q_OS_UNIX

#ifdef Q_OS_MAC
class Q_AUTOTEST_EXPORT QMacTimeZonePrivate final : public QTimeZonePrivate
{
public:
    // Create default time zone
    QMacTimeZonePrivate();
    // Create named time zone
    QMacTimeZonePrivate(const QByteArray &ianaId);
    QMacTimeZonePrivate(const QMacTimeZonePrivate &other);
    ~QMacTimeZonePrivate();

    QMacTimeZonePrivate *clone() const override;

    QString comment() const override;

    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    QList<QByteArray> availableTimeZoneIds() const override;

    NSTimeZone *nsTimeZone() const;

private:
    void init(const QByteArray &zoneId);

    NSTimeZone *m_nstz;
};
#endif // Q_OS_MAC

#ifdef Q_OS_WIN
class Q_AUTOTEST_EXPORT QWinTimeZonePrivate final : public QTimeZonePrivate
{
public:
    struct QWinTransitionRule {
        int startYear;
        int standardTimeBias;
        int daylightTimeBias;
        SYSTEMTIME standardTimeRule;
        SYSTEMTIME daylightTimeRule;
    };

    // Create default time zone
    QWinTimeZonePrivate();
    // Create named time zone
    QWinTimeZonePrivate(const QByteArray &ianaId);
    QWinTimeZonePrivate(const QWinTimeZonePrivate &other);
    ~QWinTimeZonePrivate();

    QWinTimeZonePrivate *clone() const override;

    QString comment() const override;

    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    QList<QByteArray> availableTimeZoneIds() const override;

    // For use within implementation's TransitionTimePair:
    QTimeZonePrivate::Data ruleToData(const QWinTransitionRule &rule, qint64 atMSecsSinceEpoch,
                                      QTimeZone::TimeType type, bool fakeDst = false) const;
private:
    void init(const QByteArray &ianaId);

    QByteArray m_windowsId;
    QString m_displayName;
    QString m_standardName;
    QString m_daylightName;
    QList<QWinTransitionRule> m_tranRules;
};
#endif // Q_OS_WIN

#ifdef Q_OS_ANDROID
class QAndroidTimeZonePrivate final : public QTimeZonePrivate
{
public:
    // Create default time zone
    QAndroidTimeZonePrivate();
    // Create named time zone
    QAndroidTimeZonePrivate(const QByteArray &ianaId);
    QAndroidTimeZonePrivate(const QAndroidTimeZonePrivate &other);
    ~QAndroidTimeZonePrivate();

    QAndroidTimeZonePrivate *clone() const override;

    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    Data data(qint64 forMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    QList<QByteArray> availableTimeZoneIds() const override;

private:
    void init(const QByteArray &zoneId);

    QJniObject androidTimeZone;

};
#endif // Q_OS_ANDROID

QT_END_NAMESPACE

#endif // QTIMEZONEPRIVATE_P_H
