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

#ifndef QWINDOWSCURSOR_H
#define QWINDOWSCURSOR_H

#include "qtwindows_additional.h"

#include <QtGui/QPlatformCursor>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QHash>

QT_BEGIN_NAMESPACE

class QWindowsWindowCursorData;

class QWindowsWindowCursor
{
public:
    explicit QWindowsWindowCursor(const QCursor &c);
    ~QWindowsWindowCursor();
    QWindowsWindowCursor(const QWindowsWindowCursor &c);
    QWindowsWindowCursor &operator=(const QWindowsWindowCursor &c);

    QCursor cursor() const;
    HCURSOR handle() const;

private:
    QSharedDataPointer<QWindowsWindowCursorData> m_data;
};

class QWindowsCursor : public QPlatformCursor
{
public:
    QWindowsCursor() {}

    virtual void changeCursor(QCursor * widgetCursor, QWindow * widget);
    virtual QPoint pos() const { return mousePosition(); }
    virtual void setPos(const QPoint &pos);

    static HCURSOR createPixmapCursor(const QPixmap &pixmap, int hotX, int hotY);
    static HCURSOR createSystemCursor(const QCursor &c);
    static QPoint mousePosition();

    QWindowsWindowCursor standardWindowCursor(Qt::CursorShape s = Qt::ArrowCursor);

private:
    typedef QHash<Qt::CursorShape, QWindowsWindowCursor> StandardCursorCache;

    StandardCursorCache m_standardCursorCache;
};

QT_END_NAMESPACE

#endif // QWINDOWSCURSOR_H
