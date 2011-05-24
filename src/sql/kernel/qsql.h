/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSQL_H
#define QSQL_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Sql)

namespace QSql
{
    enum Location
    {
        BeforeFirstRow = -1,
        AfterLastRow = -2
#ifdef QT3_SUPPORT
        , BeforeFirst = BeforeFirstRow,
        AfterLast = AfterLastRow
#endif
    };

    enum ParamTypeFlag
    {
        In = 0x00000001,
        Out = 0x00000002,
        InOut = In | Out,
        Binary = 0x00000004
    };
    Q_DECLARE_FLAGS(ParamType, ParamTypeFlag)

    enum TableType
    {
        Tables = 0x01,
        SystemTables = 0x02,
        Views = 0x04,
        AllTables = 0xff
    };

    enum NumericalPrecisionPolicy
    {
        LowPrecisionInt32    = 0x01,
        LowPrecisionInt64    = 0x02,
        LowPrecisionDouble   = 0x04,

        HighPrecision        = 0
    };

#ifdef QT3_SUPPORT
    enum Op {
        None = -1,
        Insert = 0,
        Update = 1,
        Delete = 2
    };

    enum Confirm {
        Cancel = -1,
        No = 0,
        Yes = 1
    };
#endif
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QSql::ParamType)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSQL_H
