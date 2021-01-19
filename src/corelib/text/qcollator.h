/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCOLLATOR_H
#define QCOLLATOR_H

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qlocale.h>

QT_BEGIN_NAMESPACE

class QCollatorPrivate;
class QCollatorSortKeyPrivate;

class Q_CORE_EXPORT QCollatorSortKey
{
    friend class QCollator;
public:
    QCollatorSortKey(const QCollatorSortKey &other);
    ~QCollatorSortKey();
    QCollatorSortKey &operator=(const QCollatorSortKey &other);
    inline QCollatorSortKey &operator=(QCollatorSortKey &&other) noexcept
    { swap(other); return *this; }
    void swap(QCollatorSortKey &other) noexcept
    { d.swap(other.d); }

    int compare(const QCollatorSortKey &key) const;

protected:
    QCollatorSortKey(QCollatorSortKeyPrivate*);

    QExplicitlySharedDataPointer<QCollatorSortKeyPrivate> d;

private:
    QCollatorSortKey();
};

inline bool operator<(const QCollatorSortKey &lhs, const QCollatorSortKey &rhs)
{
    return lhs.compare(rhs) < 0;
}

class Q_CORE_EXPORT QCollator
{
public:
    QCollator();
    explicit QCollator(const QLocale &locale);
    QCollator(const QCollator &);
    ~QCollator();
    QCollator &operator=(const QCollator &);
    QCollator(QCollator &&other) noexcept
        : d(other.d) { other.d = nullptr; }
    QCollator &operator=(QCollator &&other) noexcept
    { swap(other); return *this; }

    void swap(QCollator &other) noexcept
    { qSwap(d, other.d); }

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity cs);

    void setNumericMode(bool on);
    bool numericMode() const;

    void setIgnorePunctuation(bool on);
    bool ignorePunctuation() const;

#if QT_STRINGVIEW_LEVEL < 2
    int compare(const QString &s1, const QString &s2) const;
    int compare(const QStringRef &s1, const QStringRef &s2) const;
    int compare(const QChar *s1, int len1, const QChar *s2, int len2) const;

    bool operator()(const QString &s1, const QString &s2) const
    { return compare(s1, s2) < 0; }
#endif
    int compare(QStringView s1, QStringView s2) const;

    bool operator()(QStringView s1, QStringView s2) const
    { return compare(s1, s2) < 0; }

    QCollatorSortKey sortKey(const QString &string) const;

private:
    QCollatorPrivate *d;

    void detach();
};

Q_DECLARE_SHARED(QCollatorSortKey)
Q_DECLARE_SHARED(QCollator)

QT_END_NAMESPACE

#endif // QCOLLATOR_P_H
