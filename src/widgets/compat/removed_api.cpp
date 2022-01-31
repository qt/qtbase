/****************************************************************************
**
** Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt.io/licensing/
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

#define QT_WIDGETS_BUILD_REMOVED_API

#include "qtwidgetsglobal.h"

QT_USE_NAMESPACE

#if QT_WIDGETS_REMOVED_SINCE(6, 3)

#include "qmenu.h"

QAction *QMenu::addAction(const QString &text)
{
    return QWidget::addAction(text);
}

QAction *QMenu::addAction(const QIcon &icon, const QString &text)
{
    return QWidget::addAction(icon, text);
}

#if !QT_CONFIG(shortcut)
// the overloads taking QKeySequence as a trailing argument are deprecated, not removed,
// so remained in qmenu.cpp
QAction *QMenu::addAction(const QString &text, const QObject *receiver, const char* member)
{
    return QWidget::addAction(text, receiver, member);
}

QAction *QMenu::addAction(const QIcon &icon, const QString &text,
                          const QObject *receiver, const char* member)
{
    return QWidget::addAction(icon, text, receiver, member);
}
#endif

#include "qtoolbar.h"

QAction *QToolBar::addAction(const QString &text)
{
    return QWidget::addAction(text);
}

QAction *QToolBar::addAction(const QIcon &icon, const QString &text)
{
    return QWidget::addAction(icon, text);
}

QAction *QToolBar::addAction(const QString &text,
                             const QObject *receiver, const char* member)
{
    return QWidget::addAction(text, receiver, member);
}

QAction *QToolBar::addAction(const QIcon &icon, const QString &text,
                             const QObject *receiver, const char* member)
{
    return QWidget::addAction(icon, text, receiver, member);
}

#include "qmenubar.h"

QAction *QMenuBar::addAction(const QString &text)
{
    return QWidget::addAction(text);
}

QAction *QMenuBar::addAction(const QString &text, const QObject *receiver, const char* member)
{
    return QWidget::addAction(text, receiver, member);
}

// #include <qotherheader.h>
// // implement removed functions from qotherheader.h

#endif // QT_WIDGETS_REMOVED_SINCE(6, 3)
