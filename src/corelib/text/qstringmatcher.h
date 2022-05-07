/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2019 Mail.ru Group.
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
******************************************************************************/

#ifndef QSTRINGMATCHER_H
#define QSTRINGMATCHER_H

#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>

QT_BEGIN_NAMESPACE


class QStringMatcherPrivate;

class Q_CORE_EXPORT QStringMatcher
{
    void updateSkipTable();
public:
    QStringMatcher() = default;
    explicit QStringMatcher(const QString &pattern,
                   Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QStringMatcher(const QChar *uc, qsizetype len,
                   Qt::CaseSensitivity cs = Qt::CaseSensitive)
        : QStringMatcher(QStringView(uc, len), cs)
    {}
    QStringMatcher(QStringView pattern,
                   Qt::CaseSensitivity cs = Qt::CaseSensitive);
    QStringMatcher(const QStringMatcher &other);
    ~QStringMatcher();

    QStringMatcher &operator=(const QStringMatcher &other);

    void setPattern(const QString &pattern);
    void setCaseSensitivity(Qt::CaseSensitivity cs);

    qsizetype indexIn(const QString &str, qsizetype from = 0) const
    { return indexIn(QStringView(str), from); }
    qsizetype indexIn(const QChar *str, qsizetype length, qsizetype from = 0) const
    { return indexIn(QStringView(str, length), from); }
    qsizetype indexIn(QStringView str, qsizetype from = 0) const;
    QString pattern() const;
    inline Qt::CaseSensitivity caseSensitivity() const { return q_cs; }

private:
    QStringMatcherPrivate *d_ptr = nullptr;
    Qt::CaseSensitivity q_cs = Qt::CaseSensitive;
    QString q_pattern;
    QStringView q_sv;
    uchar q_skiptable[256] = {};
};

QT_END_NAMESPACE

#endif // QSTRINGMATCHER_H
