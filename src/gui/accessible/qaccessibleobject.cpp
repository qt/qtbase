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

#include "qaccessibleobject.h"

#ifndef QT_NO_ACCESSIBILITY

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>

#include "qpointer.h"
#include "qmetaobject.h"

QT_BEGIN_NAMESPACE

class QAccessibleObjectPrivate
{
public:
    QPointer<QObject> object;
};

/*!
    \class QAccessibleObject
    \brief The QAccessibleObject class implements parts of the
    QAccessibleInterface for QObjects.

    \ingroup accessibility
    \inmodule QtGui

    This class is part of \l {Accessibility for QWidget Applications}.

    This class is mainly provided for convenience. All subclasses of
    the QAccessibleInterface that provide implementations of non-widget objects
    should use this class as their base class.

    \sa QAccessible, QAccessibleWidget
*/

/*!
    Creates a QAccessibleObject for \a object.
*/
QAccessibleObject::QAccessibleObject(QObject *object)
{
    d = new QAccessibleObjectPrivate;
    d->object = object;
}

/*!
    Destroys the QAccessibleObject.

    This only happens when a call to release() decrements the internal
    reference counter to zero.
*/
QAccessibleObject::~QAccessibleObject()
{
    delete d;
}

/*!
    \reimp
*/
QObject *QAccessibleObject::object() const
{
    return d->object;
}

/*!
    \reimp
*/
bool QAccessibleObject::isValid() const
{
    return !d->object.isNull();
}

/*! \reimp */
QRect QAccessibleObject::rect() const
{
    return QRect();
}

/*! \reimp */
void QAccessibleObject::setText(QAccessible::Text, const QString &)
{
}

/*! \reimp */
QAccessibleInterface *QAccessibleObject::childAt(int x, int y) const
{
    for (int i = 0; i < childCount(); ++i) {
        QAccessibleInterface *childIface = child(i);
        Q_ASSERT(childIface);
        if (childIface->isValid() && childIface->rect().contains(x,y))
            return childIface;
    }
    return nullptr;
}

/*!
    \class QAccessibleApplication
    \brief The QAccessibleApplication class implements the QAccessibleInterface for QApplication.

    \internal

    \ingroup accessibility
*/

/*!
    Creates a QAccessibleApplication for the QApplication object referenced by qApp.
*/
QAccessibleApplication::QAccessibleApplication()
: QAccessibleObject(qApp)
{
}

QWindow *QAccessibleApplication::window() const
{
    // an application can have several windows, and AFAIK we don't need
    // to notify about changes on the application.
    return nullptr;
}

// all toplevel windows except popups and the desktop
static QObjectList topLevelObjects()
{
    QObjectList list;
    const QWindowList tlw(QGuiApplication::topLevelWindows());
    for (int i = 0; i < tlw.count(); ++i) {
        QWindow *w = tlw.at(i);
        if (w->type() != Qt::Popup && w->type() != Qt::Desktop) {
            if (QAccessibleInterface *root = w->accessibleRoot()) {
                if (root->object())
                    list.append(root->object());
            }
        }
    }

    return list;
}

/*! \reimp */
int QAccessibleApplication::childCount() const
{
    return topLevelObjects().count();
}

/*! \reimp */
int QAccessibleApplication::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;
    const QObjectList tlw(topLevelObjects());
    return tlw.indexOf(child->object());
}

QAccessibleInterface *QAccessibleApplication::parent() const
{
    return nullptr;
}

QAccessibleInterface *QAccessibleApplication::child(int index) const
{
    const QObjectList tlo(topLevelObjects());
    if (index >= 0 && index < tlo.count())
        return QAccessible::queryAccessibleInterface(tlo.at(index));
    return nullptr;
}


/*! \reimp */
QAccessibleInterface *QAccessibleApplication::focusChild() const
{
    if (QWindow *window = QGuiApplication::focusWindow())
        return window->accessibleRoot();
    return nullptr;
}

/*! \reimp */
QString QAccessibleApplication::text(QAccessible::Text t) const
{
    switch (t) {
    case QAccessible::Name:
        return QGuiApplication::applicationName();
    case QAccessible::Description:
        return QGuiApplication::applicationFilePath();
    default:
        break;
    }
    return QString();
}

/*! \reimp */
QAccessible::Role QAccessibleApplication::role() const
{
    return QAccessible::Application;
}

/*! \reimp */
QAccessible::State QAccessibleApplication::state() const
{
    return QAccessible::State();
}


QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
