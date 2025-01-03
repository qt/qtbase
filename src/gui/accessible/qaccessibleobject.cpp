// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessibleobject.h"

#if QT_CONFIG(accessibility)

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
    for (int i = 0; i < tlw.size(); ++i) {
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
    return topLevelObjects().size();
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
    if (index >= 0 && index < tlo.size())
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

#endif // QT_CONFIG(accessibility)
