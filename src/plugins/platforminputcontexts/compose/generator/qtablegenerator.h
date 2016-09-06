/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QTABLEGENERATOR_H
#define QTABLEGENERATOR_H

#include <QtCore/QVector>
#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QString>

#include <algorithm>

static Q_CONSTEXPR int QT_KEYSEQUENCE_MAX_LEN = 6;

//#define DEBUG_GENERATOR

/* Whenever QComposeTableElement gets modified supportedCacheVersion
 from qtablegenerator.cpp must be bumped. */
struct QComposeTableElement {
    uint keys[QT_KEYSEQUENCE_MAX_LEN];
    uint value;
#ifdef DEBUG_GENERATOR
    QString comment;
#endif
};

#ifndef DEBUG_GENERATOR
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QComposeTableElement, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE
#endif

struct ByKeys
{
    using uint_array = uint[QT_KEYSEQUENCE_MAX_LEN];
    using result_type = bool;

    bool operator()(const uint_array &lhs, const uint_array &rhs) const Q_DECL_NOTHROW
    {
        return std::lexicographical_compare(lhs, lhs + QT_KEYSEQUENCE_MAX_LEN,
                                            rhs, rhs + QT_KEYSEQUENCE_MAX_LEN);
    }

    bool operator()(const uint_array &lhs, const QComposeTableElement &rhs) const Q_DECL_NOTHROW
    {
        return operator()(lhs, rhs.keys);
    }

    bool operator()(const QComposeTableElement &lhs, const uint_array &rhs) const Q_DECL_NOTHROW
    {
        return operator()(lhs.keys, rhs);
    }

    bool operator()(const QComposeTableElement &lhs, const QComposeTableElement &rhs) const Q_DECL_NOTHROW
    {
        return operator()(lhs.keys, rhs.keys);
    }
};

class TableGenerator
{

public:
    enum TableState
    {
        UnsupportedLocale,
        EmptyTable,
        UnknownSystemComposeDir,
        MissingComposeFile,
        NoErrors
    };

    TableGenerator();
    ~TableGenerator();

    void parseComposeFile(QFile *composeFile);
    void printComposeTable() const;
    void orderComposeTable();

    QVector<QComposeTableElement> composeTable() const;
    TableState tableState() const { return m_state; }

protected:
    bool processFile(const QString &composeFileName);
    void parseKeySequence(char *line);
    void parseIncludeInstruction(QString line);

    QString findComposeFile();
    bool findSystemComposeDir();
    QString systemComposeDir();
    QString composeTableForLocale();

    ushort keysymToUtf8(quint32 sym);

    QString readLocaleMappings(const QByteArray &locale);
    QByteArray readLocaleAliases(const QByteArray &locale);
    void initPossibleLocations();
    bool cleanState() const { return m_state == NoErrors; }
    QString locale() const;

private:
      QVector<QComposeTableElement> m_composeTable;
      TableState m_state;
      QString m_systemComposeDir;
      QList<QString> m_possibleLocations;
};

#endif // QTABLEGENERATOR_H
