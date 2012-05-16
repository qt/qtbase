/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the tools applications of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  separator.cpp
*/

#include "separator.h"
#include "tr.h"

QT_BEGIN_NAMESPACE

QString separator(int index, int count)
{
    if (index == count - 1)
        return tr(".", "terminator");
    if (count == 2)
        return tr(" and ", "separator when N = 2");
    if (index == 0)
        return tr(", ", "first separator when N > 2");
    if (index < count - 2)
        return tr(", ", "general separator when N > 2");
    return tr(", and ", "last separator when N > 2");
}

QString comma(int index, int count)
{
    if (index == count - 1)
        return QString();
    if (count == 2)
        return tr(" and ", "separator when N = 2");
    if (index == 0)
        return tr(", ", "first separator when N > 2");
    if (index < count - 2)
        return tr(", ", "general separator when N > 2");
    return tr(", and ", "last separator when N > 2");
}

QT_END_NAMESPACE
