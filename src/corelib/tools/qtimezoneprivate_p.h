/****************************************************************************
**
** Copyright (C) 2013 John Layt <jlayt@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


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

#include "qtimezone.h"
#include "qlocale_p.h"

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QTimeZonePrivate : public QSharedData
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
    typedef QVector<Data> DataList;

    // Create null time zone
    QTimeZonePrivate();
    QTimeZonePrivate(const QTimeZonePrivate &other);
    virtual ~QTimeZonePrivate();

    virtual QTimeZonePrivate *clone();

    bool operator==(const QTimeZonePrivate &other) const;
    bool operator!=(const QTimeZonePrivate &other) const;

    bool isValid() const;

    QByteArray id() const;
    virtual QLocale::Country country() const;
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

    virtual bool hasTransitions() const;
    virtual Data nextTransition(qint64 afterMSecsSinceEpoch) const;
    virtual Data previousTransition(qint64 beforeMSecsSinceEpoch) const;
    DataList transitions(qint64 fromMSecsSinceEpoch, qint64 toMSecsSinceEpoch) const;

    virtual QByteArray systemTimeZoneId() const;

    virtual QSet<QByteArray> availableTimeZoneIds() const;
    virtual QSet<QByteArray> availableTimeZoneIds(QLocale::Country country) const;
    virtual QSet<QByteArray> availableTimeZoneIds(int utcOffset) const;

    virtual void serialize(QDataStream &ds) const;

    // Static Utility Methods
    static inline qint64 maxMSecs() { return std::numeric_limits<qint64>::max(); }
    static inline qint64 minMSecs() { return std::numeric_limits<qint64>::min() + 1; }
    static inline qint64 invalidMSecs() { return std::numeric_limits<qint64>::min(); }
    static inline qint64 invalidSeconds() { return std::numeric_limits<int>::min(); }
    static Data invalidData();
    static QTimeZone::OffsetData invalidOffsetData();
    static QTimeZone::OffsetData toOffsetData(const Data &data);
    static bool isValidId(const QByteArray &olsenId);
    static QString isoOffsetFormat(int offsetFromUtc);

    static QByteArray olsenIdToWindowsId(const QByteArray &olsenId);
    static QByteArray windowsIdToDefaultOlsenId(const QByteArray &windowsId);
    static QByteArray windowsIdToDefaultOlsenId(const QByteArray &windowsId,
                                                QLocale::Country country);
    static QList<QByteArray> windowsIdToOlsenIds(const QByteArray &windowsId);
    static QList<QByteArray> windowsIdToOlsenIds(const QByteArray &windowsId,
                                                 QLocale::Country country);

protected:
    QByteArray m_id;
};

template<> QTimeZonePrivate *QSharedDataPointer<QTimeZonePrivate>::clone();

class Q_AUTOTEST_EXPORT QUtcTimeZonePrivate Q_DECL_FINAL : public QTimeZonePrivate
{
public:
    // Create default UTC time zone
    QUtcTimeZonePrivate();
    // Create named time zone
    QUtcTimeZonePrivate(const QByteArray &utcId);
    // Create offset from UTC
    QUtcTimeZonePrivate(int offsetSeconds);
    // Create custom offset from UTC
    QUtcTimeZonePrivate(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                        const QString &abbreviation, QLocale::Country country,
                        const QString &comment);
    QUtcTimeZonePrivate(const QUtcTimeZonePrivate &other);
    virtual ~QUtcTimeZonePrivate();

    QTimeZonePrivate *clone();

    QLocale::Country country() const Q_DECL_OVERRIDE;
    QString comment() const Q_DECL_OVERRIDE;

    QString displayName(QTimeZone::TimeType timeType,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const Q_DECL_OVERRIDE;
    QString abbreviation(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    int standardTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    QByteArray systemTimeZoneId() const Q_DECL_OVERRIDE;

    QSet<QByteArray> availableTimeZoneIds() const Q_DECL_OVERRIDE;
    QSet<QByteArray> availableTimeZoneIds(QLocale::Country country) const Q_DECL_OVERRIDE;
    QSet<QByteArray> availableTimeZoneIds(int utcOffset) const Q_DECL_OVERRIDE;

    void serialize(QDataStream &ds) const Q_DECL_OVERRIDE;

private:
    void init(const QByteArray &zoneId);
    void init(const QByteArray &zoneId, int offsetSeconds, const QString &name,
              const QString &abbreviation, QLocale::Country country,
              const QString &comment);

    int m_offsetFromUtc;
    QString m_name;
    QString m_abbreviation;
    QLocale::Country m_country;
    QString m_comment;
};

QT_END_NAMESPACE

#endif // QTIMEZONEPRIVATE_P_H
