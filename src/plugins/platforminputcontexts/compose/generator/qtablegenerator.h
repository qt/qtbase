/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QTABLEGENERATOR_H
#define QTABLEGENERATOR_H

#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QString>

#define QT_KEYSEQUENCE_MAX_LEN 6

struct QComposeTableElement {
    uint keys[QT_KEYSEQUENCE_MAX_LEN];
    uint value;
    QString comment;
};

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

    QList<QComposeTableElement> composeTable() const;
    TableState tableState() const { return m_state; }

protected:
    bool processFile(QString composeFileName);
    void parseKeySequence(QString line);
    void parseIncludeInstruction(QString line);

    void findComposeFile();
    bool findSystemComposeDir();
    QString systemComposeDir();

    ushort keysymToUtf8(quint32 sym);
    quint32 stringToKeysym(QString keysymName);

    void readLocaleMappings();
    void initPossibleLocations();
    bool cleanState() const { return ((m_state & NoErrors) == NoErrors); }
    QString locale() const;

private:
      QList<QComposeTableElement> m_composeTable;
      QMap<QString, QString> m_localeToTable;
      TableState m_state;
      QString m_systemComposeDir;
      QList<QString> m_possibleLocations;
};

#endif // QTABLEGENERATOR_H
