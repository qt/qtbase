/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QTABLEGENERATOR_H
#define QTABLEGENERATOR_H

#include <QtCore/QVector>
#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QString>

#define QT_KEYSEQUENCE_MAX_LEN 6

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

class Compare
{
public:
    bool operator () (const QComposeTableElement &lhs, const uint rhs[QT_KEYSEQUENCE_MAX_LEN])
    {
        for (size_t i = 0; i < QT_KEYSEQUENCE_MAX_LEN; i++) {
            if (lhs.keys[i] != rhs[i])
                return (lhs.keys[i] < rhs[i]);
        }
        return false;
    }

    bool operator () (const QComposeTableElement &lhs, const QComposeTableElement &rhs)
    {
        for (size_t i = 0; i < QT_KEYSEQUENCE_MAX_LEN; i++) {
            if (lhs.keys[i] != rhs.keys[i])
                return (lhs.keys[i] < rhs.keys[i]);
        }
        return false;
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
