/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#include "qguiapplication.h"

#include <private/qobject_p.h>
#include "qkeymapper_p.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

/*!
    \class QKeyMapper
    \since 4.2
    \internal

    \sa QObject
*/

/*!
    Constructs a new key mapper.
*/
QKeyMapper::QKeyMapper()
    : QObject(*new QKeyMapperPrivate, nullptr)
{
}

/*!
    Destroys the key mapper.
*/
QKeyMapper::~QKeyMapper()
{
}

QList<int> QKeyMapper::possibleKeys(QKeyEvent *e)
{
    QList<int> result;

    if (!e->nativeScanCode()) {
        if (e->key() && (e->key() != Qt::Key_unknown))
            result << int(e->key() + e->modifiers());
        else if (!e->text().isEmpty())
            result << int(e->text().at(0).unicode() + e->modifiers());
        return result;
    }

    return instance()->d_func()->possibleKeys(e);
}

extern bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event); // in qapplication_*.cpp
void QKeyMapper::changeKeyboard()
{
    instance()->d_func()->clearMappings();

    // ## TODO: Support KeyboardLayoutChange on QPA
#if 0
    // inform all toplevel widgets of the change
    QEvent e(QEvent::KeyboardLayoutChange);
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; i < list.size(); ++i) {
        QWidget *w = list.at(i);
        qt_sendSpontaneousEvent(w, &e);
    }
#endif
}

Q_GLOBAL_STATIC(QKeyMapper, keymapper)
/*!
    Returns the pointer to the single instance of QKeyMapper in the application.
    If none yet exists, the function ensures that one is created.
*/
QKeyMapper *QKeyMapper::instance()
{
    return keymapper();
}

QKeyMapperPrivate *qt_keymapper_private()
{
    return QKeyMapper::instance()->d_func();
}

QKeyMapperPrivate::QKeyMapperPrivate()
{
    keyboardInputLocale = QLocale::system();
    keyboardInputDirection = keyboardInputLocale.textDirection();
}

QKeyMapperPrivate::~QKeyMapperPrivate()
{
    // clearMappings();
}

void QKeyMapperPrivate::clearMappings()
{
}

QList<int> QKeyMapperPrivate::possibleKeys(QKeyEvent *e)
{
    QList<int> result = QGuiApplicationPrivate::platformIntegration()->possibleKeys(e);
    if (!result.isEmpty())
        return result;

    if (e->key() && (e->key() != Qt::Key_unknown))
        result << int(e->key() + e->modifiers());
    else if (!e->text().isEmpty())
        result << int(e->text().at(0).unicode() + e->modifiers());
    return result;
}

QT_END_NAMESPACE
