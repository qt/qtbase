/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

    QList<QByteArray> actionList() const;
};

QList<QByteArray> QAccessibleObjectPrivate::actionList() const
{
    QList<QByteArray> actionList;

    if (!object)
        return actionList;

    const QMetaObject *mo = object->metaObject();
    Q_ASSERT(mo);

    QByteArray defaultAction = QMetaObject::normalizedSignature(
        mo->classInfo(mo->indexOfClassInfo("DefaultSlot")).value());

    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod member = mo->method(i);
        if (member.methodType() != QMetaMethod::Slot && member.access() != QMetaMethod::Public)
            continue;

        if (!qstrcmp(member.tag(), "QACCESSIBLE_SLOT")) {
            if (member.signature() == defaultAction)
                actionList.prepend(defaultAction);
            else
                actionList << member.signature();
        }
    }

    return actionList;
}

/*!
    \class QAccessibleObject
    \brief The QAccessibleObject class implements parts of the
    QAccessibleInterface for QObjects.

    \ingroup accessibility

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
#ifndef QT_NO_DEBUG
    if (!d->object)
        qWarning("QAccessibleInterface is invalid. Crash pending...");
#endif
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
QRect QAccessibleObject::rect(int) const
{
    return QRect();
}

/*! \reimp */
void QAccessibleObject::setText(Text, int, const QString &)
{
}

/*! \reimp */
int QAccessibleObject::userActionCount(int) const
{
    return 0;
}

/*! \reimp */
bool QAccessibleObject::doAction(int, int, const QVariantList &)
{
    return false;
}

static const char * const action_text[][5] =
{
    // Name, Description, Value, Help, Accelerator
    { "Press", "", "", "", "Space" },
    { "SetFocus", "Passes focus to this widget", "", "", "" },
    { "Increase", "", "", "", "" },
    { "Decrease", "", "", "", "" },
    { "Accept", "", "", "", "" },
    { "Cancel", "", "", "", "" },
    { "Select", "", "", "", "" },
    { "ClearSelection", "", "", "", "" },
    { "RemoveSelection", "", "", "", "" },
    { "ExtendSelection", "", "", "", "" },
    { "AddToSelection", "", "", "", "" }
};

/*! \reimp */
QString QAccessibleObject::actionText(int action, Text t, int child) const
{
    if (child || action > FirstStandardAction || action < LastStandardAction || t > Accelerator)
        return QString();

    return QString::fromLatin1(action_text[-(action - FirstStandardAction)][t]);
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
    return 0;
}

// all toplevel windows except popups and the desktop
static QObjectList topLevelObjects()
{
    QObjectList list;
    const QWindowList tlw(QGuiApplication::topLevelWindows());
    for (int i = 0; i < tlw.count(); ++i) {
        QWindow *w = tlw.at(i);
        if (w->windowType() != Qt::Popup && w->windowType() != Qt::Desktop) {
            if (QAccessibleInterface *root = w->accessibleRoot()) {
                if (root->object())
                    list.append(w->accessibleRoot()->object());
                delete root;
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
    const QObjectList tlw(topLevelObjects());
    int index = tlw.indexOf(child->object());
    if (index != -1)
        ++index;
    return index;
}

/*! \reimp */
int QAccessibleApplication::childAt(int x, int y) const
{
    for (int i = 0; i < childCount(); ++i) {
        QAccessibleInterface *childIface = child(i);
        QRect geom = childIface->rect();
        if (geom.contains(x,y))
            return i+1;
        delete childIface;
    }
    return rect().contains(x,y) ? 0 : -1;
}

/*! \reimp */
QAccessible::Relation QAccessibleApplication::relationTo(int child, const
        QAccessibleInterface *other, int otherChild) const
{
    QObject *o = other ? other->object() : 0;
    if (!o)
        return Unrelated;

    if(o == object()) {
        if (child && !otherChild)
            return Child;
        if (!child && otherChild)
            return Ancestor;
        if (!child && !otherChild)
            return Self;
    }

    return Unrelated;
}

QAccessibleInterface *QAccessibleApplication::parent() const
{
    return 0;
}

QAccessibleInterface *QAccessibleApplication::child(int index) const
{
    Q_ASSERT(index >= 0);
    const QObjectList tlo(topLevelObjects());
    if (index >= 0 && index < tlo.count())
        return QAccessible::queryAccessibleInterface(tlo.at(index));
    return 0;
}

/*! \reimp */
int QAccessibleApplication::navigate(RelationFlag relation, int entry,
                                     QAccessibleInterface **target) const
{
    if (!target)
        return -1;

    *target = 0;
    QObject *targetObject = 0;

    switch (relation) {
    case Self:
        targetObject = object();
        break;
    case FocusChild:
        targetObject = QGuiApplication::activeWindow();
        break;
    case Ancestor:
        *target = parent();
        return 0;
    default:
        break;
    }
    *target = QAccessible::queryAccessibleInterface(targetObject);
    return *target ? 0 : -1;
}

/*! \reimp */
QString QAccessibleApplication::text(Text t, int) const
{
    switch (t) {
    case Name:
        return QGuiApplication::applicationName();
    case Description:
        return QGuiApplication::applicationFilePath();
    default:
        break;
    }
    return QString();
}

/*! \reimp */
QAccessible::Role QAccessibleApplication::role(int) const
{
    return Application;
}

/*! \reimp */
QAccessible::State QAccessibleApplication::state(int) const
{
    return QGuiApplication::activeWindow() ? Focused : Normal;
}

/*! \reimp */
int QAccessibleApplication::userActionCount(int) const
{
    return 1;
}

/*! \reimp */
bool QAccessibleApplication::doAction(int action, int child, const QVariantList &param)
{
    //###Move to IA2 action interface at some point to get rid of the ambiguity.
    /*  //### what is action == 0 and action == 1 ?????
    if (action == 0 || action == 1) {
        QWindow *w = 0;
        w = QGuiApplication::activeWindow();
        if (!w)
            w = topLevelWindows().at(0);
        if (!w)
            return false;
        w->requestActivateWindow();
        return true;
    }
    */
    return QAccessibleObject::doAction(action, child, param);
}

/*! \reimp */
QString QAccessibleApplication::actionText(int action, Text text, int child) const
{
    QString str;
    if ((action == 0 || action == 1) && !child) switch (text) {
    case Name:
        return QGuiApplication::tr("Activate");
    case Description:
        return QGuiApplication::tr("Activates the program's main window");
    default:
        break;
    }
    return QAccessibleObject::actionText(action, text, child);
}

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
