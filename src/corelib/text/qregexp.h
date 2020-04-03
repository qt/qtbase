/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QREGEXP_H
#define QREGEXP_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_REGEXP

#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE


struct QRegExpPrivate;
class QStringList;
class QRegExp;

Q_CORE_EXPORT size_t qHash(const QRegExp &key, size_t seed = 0) noexcept;

class Q_CORE_EXPORT QRegExp
{
public:
    enum PatternSyntax {
        RegExp,
        Wildcard,
        FixedString,
        RegExp2,
        WildcardUnix,
        W3CXmlSchema11 };
    enum CaretMode { CaretAtZero, CaretAtOffset, CaretWontMatch };

    QRegExp();
    explicit QRegExp(const QString &pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive,
                     PatternSyntax syntax = RegExp);
    QRegExp(const QRegExp &rx);
    ~QRegExp();
    QRegExp &operator=(const QRegExp &rx);
    QRegExp &operator=(QRegExp &&other) noexcept { swap(other); return *this; }
    void swap(QRegExp &other) noexcept { qSwap(priv, other.priv); }

    bool operator==(const QRegExp &rx) const;
    inline bool operator!=(const QRegExp &rx) const { return !operator==(rx); }

    bool isEmpty() const;
    bool isValid() const;
    QString pattern() const;
    void setPattern(const QString &pattern);
    Qt::CaseSensitivity caseSensitivity() const;
    void setCaseSensitivity(Qt::CaseSensitivity cs);
    PatternSyntax patternSyntax() const;
    void setPatternSyntax(PatternSyntax syntax);

    bool isMinimal() const;
    void setMinimal(bool minimal);

    bool exactMatch(const QString &str) const;

    operator QVariant() const;

    int indexIn(const QString &str, int offset = 0, CaretMode caretMode = CaretAtZero) const;
    int lastIndexIn(const QString &str, int offset = -1, CaretMode caretMode = CaretAtZero) const;
    int matchedLength() const;
#ifndef QT_NO_REGEXP_CAPTURE
    int captureCount() const;
    QStringList capturedTexts() const;
    QStringList capturedTexts();
    QString cap(int nth = 0) const;
    QString cap(int nth = 0);
    int pos(int nth = 0) const;
    int pos(int nth = 0);
    QString errorString() const;
    QString errorString();
#endif

    QString replaceIn(const QString &str, const QString &after) const;
    QString removeIn(const QString &str) const
    { return replaceIn(str, QString()); }
    bool containedIn(const QString &str) const
    { return indexIn(str) != -1; }
    int countIn(const QString &str) const;

    QStringList splitString(const QString &str, Qt::SplitBehavior behavior = Qt::KeepEmptyParts) const;

    int indexIn(const QStringList &list, int from) const;
    int lastIndexIn(const QStringList &list, int from) const;
    QStringList replaceIn(const QStringList &stringList, const QString &after) const;
    QStringList filterList(const QStringList &stringList) const;

    static QString escape(const QString &str);

    friend Q_CORE_EXPORT size_t qHash(const QRegExp &key, size_t seed) noexcept;

private:
    QRegExpPrivate *priv;
};

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &out, const QRegExp &regExp);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &in, QRegExp &regExp);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QRegExp &);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QRegExp)

#endif // QT_NO_REGEXP

#endif // QREGEXP_H
