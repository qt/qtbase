/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSACCESSIBILITY_H
#define QWINDOWSACCESSIBILITY_H

#include "../qtwindowsglobal.h"
#include "../qwindowscontext.h"
#include <QtGui/QPlatformAccessibility>

#include <oleacc.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

class QWindowsAccessibility : public QPlatformAccessibility
{
public:
    QWindowsAccessibility();
    static bool handleAccessibleObjectFromWindowRequest(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
    virtual void notifyAccessibilityUpdate(QAccessibleEvent *event);
    /*
    virtual void setRootObject(QObject *o);
    virtual void initialize();
    virtual void cleanup();
    */
    static IAccessible *wrap(QAccessibleInterface *acc);
    static QWindow *windowHelper(const QAccessibleInterface *iface);

    static QPair<QObject*, int> getCachedObject(int entryId);
};

QT_END_NAMESPACE
QT_END_HEADER

#endif // QWINDOWSACCESSIBILITY_H
