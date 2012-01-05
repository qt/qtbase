/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qaccessiblewidget.h"

#ifndef QT_NO_ACCESSIBILITY

#include "qaction.h"
#include "qapplication.h"
#include "qgroupbox.h"
#include "qlabel.h"
#include "qtooltip.h"
#include "qwhatsthis.h"
#include "qwidget.h"
#include "qdebug.h"
#include <qmath.h>
#include <QRubberBand>
#include <QFocusFrame>
#include <QMenu>

QT_BEGIN_NAMESPACE

static QList<QWidget*> childWidgets(const QWidget *widget)
{
    QList<QObject*> list = widget->children();
    QList<QWidget*> widgets;
    for (int i = 0; i < list.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(list.at(i));
        if (w && !w->isWindow() 
            && !qobject_cast<QFocusFrame*>(w)
#if !defined(QT_NO_MENU)
            && !qobject_cast<QMenu*>(w)
#endif
            && w->objectName() != QLatin1String("qt_rubberband"))
            widgets.append(w);
    }
    return widgets;
}

static QString buddyString(const QWidget *widget)
{
    if (!widget)
        return QString();
    QWidget *parent = widget->parentWidget();
    if (!parent)
        return QString();
#ifndef QT_NO_SHORTCUT
    QObjectList ol = parent->children();
    for (int i = 0; i < ol.size(); ++i) {
        QLabel *label = qobject_cast<QLabel*>(ol.at(i));
        if (label && label->buddy() == widget)
            return label->text();
    }
#endif

#ifndef QT_NO_GROUPBOX
    QGroupBox *groupbox = qobject_cast<QGroupBox*>(parent);
    if (groupbox)
        return groupbox->title();
#endif

    return QString();
}

QString Q_WIDGETS_EXPORT qt_accStripAmp(const QString &text)
{
    return QString(text).remove(QLatin1Char('&'));
}

QString Q_WIDGETS_EXPORT qt_accHotKey(const QString &text)
{
#ifndef QT_NO_SHORTCUT
    if (text.isEmpty())
        return text;

    int fa = 0;
    QChar ac;
    while ((fa = text.indexOf(QLatin1Char('&'), fa)) != -1) {
        ++fa;
        if (fa < text.length()) {
            // ignore "&&"
            if (text.at(fa) == QLatin1Char('&')) {
                ++fa;
                continue;
            } else {
                ac = text.at(fa);
                break;
            }
        }
    }
    if (ac.isNull())
        return QString();
    return (QString)QKeySequence(Qt::ALT) + ac.toUpper();
#else
    Q_UNUSED(text);
    return QString();
#endif
}

class QAccessibleWidgetPrivate
{
public:
    QAccessibleWidgetPrivate()
        :role(QAccessible::Client)
    {}

    QAccessible::Role role;
    QString name;
    QString description;
    QString value;
    QString help;
    QString accelerator;
    QStringList primarySignals;
    const QAccessibleInterface *asking;
};

/*!
    \class QAccessibleWidget
    \brief The QAccessibleWidget class implements the QAccessibleInterface for QWidgets.

    \ingroup accessibility
    \inmodule QtWidgets

    This class is part of \l {Accessibility for QWidget Applications}.

    This class is convenient to use as a base class for custom
    implementations of QAccessibleInterfaces that provide information
    about widget objects.

    The class provides functions to retrieve the parentObject() (the
    widget's parent widget), and the associated widget(). Controlling
    signals can be added with addControllingSignal(), and setters are
    provided for various aspects of the interface implementation, for
    example setValue(), setDescription(), setAccelerator(), and
    setHelp().

    \sa QAccessible, QAccessibleObject
*/

/*!
    Creates a QAccessibleWidget object for widget \a w.
    \a role and \a name are optional parameters that set the object's
    role and name properties.
*/
QAccessibleWidget::QAccessibleWidget(QWidget *w, QAccessible::Role role, const QString &name)
: QAccessibleObject(w)
{
    Q_ASSERT(widget());
    d = new QAccessibleWidgetPrivate();
    d->role = role;
    d->name = name;
    d->asking = 0;
}

QWindow *QAccessibleWidget::window() const
{
    return widget()->windowHandle();
}

/*!
    Destroys this object.
*/
QAccessibleWidget::~QAccessibleWidget()
{
    delete d;
}

/*!
    Returns the associated widget.
*/
QWidget *QAccessibleWidget::widget() const
{
    return qobject_cast<QWidget*>(object());
}

/*!
    Returns the associated widget's parent object, which is either the
    parent widget, or qApp for top-level widgets.
*/
QObject *QAccessibleWidget::parentObject() const
{
    QObject *parent = object()->parent();
    if (!parent)
        parent = qApp;
    return parent;
}

/*! \reimp */
QRect QAccessibleWidget::rect() const
{
    QWidget *w = widget();
    if (!w->isVisible())
        return QRect();
    QPoint wpos = w->mapToGlobal(QPoint(0, 0));

    return QRect(wpos.x(), wpos.y(), w->width(), w->height());
}

QT_BEGIN_INCLUDE_NAMESPACE
#include <private/qobject_p.h>
QT_END_INCLUDE_NAMESPACE

class QACConnectionObject : public QObject
{
    Q_DECLARE_PRIVATE(QObject)
public:
    inline bool isSender(const QObject *receiver, const char *signal) const
    { return d_func()->isSender(receiver, signal); }
    inline QObjectList receiverList(const char *signal) const
    { return d_func()->receiverList(signal); }
    inline QObjectList senderList() const
    { return d_func()->senderList(); }
};

/*!
    Registers \a signal as a controlling signal.

    An object is a Controller to any other object connected to a
    controlling signal.
*/
void QAccessibleWidget::addControllingSignal(const QString &signal)
{
    QByteArray s = QMetaObject::normalizedSignature(signal.toAscii());
    if (object()->metaObject()->indexOfSignal(s) < 0)
        qWarning("Signal %s unknown in %s", s.constData(), object()->metaObject()->className());
    d->primarySignals << QLatin1String(s);
}

/*!
    Sets the value of this interface implementation to \a value.

    The default implementation of text() returns the set value for
    the Value text.

    Note that the object wrapped by this interface is not modified.
*/
void QAccessibleWidget::setValue(const QString &value)
{
    d->value = value;
}

/*!
    Sets the description of this interface implementation to \a desc.

    The default implementation of text() returns the set value for
    the Description text.

    Note that the object wrapped by this interface is not modified.
*/
void QAccessibleWidget::setDescription(const QString &desc)
{
    d->description = desc;
}

/*!
    Sets the help of this interface implementation to \a help.

    The default implementation of text() returns the set value for
    the Help text.

    Note that the object wrapped by this interface is not modified.
*/
void QAccessibleWidget::setHelp(const QString &help)
{
    d->help = help;
}

/*!
    Sets the accelerator of this interface implementation to \a accel.

    The default implementation of text() returns the set value for
    the Accelerator text.

    Note that the object wrapped by this interface is not modified.
*/
void QAccessibleWidget::setAccelerator(const QString &accel)
{
    d->accelerator = accel;
}

static inline bool isAncestor(const QObject *obj, const QObject *child)
{
    while (child) {
        if (child == obj)
            return true;
        child = child->parent();
    }
    return false;
}


/*! \reimp */
QAccessible::Relation QAccessibleWidget::relationTo(const QAccessibleInterface *other) const
{
    QAccessible::Relation relation = QAccessible::Unrelated;
    if (d->asking == this) // recursive call
        return relation;

    QObject *o = other ? other->object() : 0;
    if (!o)
        return relation;

    QWidget *focus = widget()->focusWidget();
    if (object() == focus && isAncestor(o, focus))
        relation |= QAccessible::FocusChild;

    QACConnectionObject *connectionObject = (QACConnectionObject*)object();
    for (int sig = 0; sig < d->primarySignals.count(); ++sig) {
        if (connectionObject->isSender(o, d->primarySignals.at(sig).toAscii())) {
            relation |= QAccessible::Controller;
            break;
        }
    }
    // test for passive relationships.
    // d->asking protects from endless recursion.
    d->asking = this;
    int inverse = other->relationTo(this);
    d->asking = 0;

    if (inverse & QAccessible::Controller)
        relation |= QAccessible::Controlled;
    if (inverse & QAccessible::Label)
        relation |= QAccessible::Labelled;

    if(o == object()) {
        return relation | QAccessible::Self;
    }

    QObject *parent = object()->parent();
    if (o->parent() == parent) {
        QAccessibleInterface *sibIface = QAccessible::queryAccessibleInterface(o);
        Q_ASSERT(sibIface);
        QRect wg = rect();
        QRect sg = sibIface->rect();
        if (wg.intersects(sg)) {
            QAccessibleInterface *pIface = 0;
            pIface = sibIface->parent();
            if (pIface && !(sibIface->state().invisible | state().invisible)) {
                int wi = pIface->indexOfChild(this);
                int si = pIface->indexOfChild(sibIface);

                if (wi > si)
                    relation |= QAccessible::Covers;
                else
                    relation |= QAccessible::Covered;
            }
            delete pIface;
        }
        delete sibIface;

        return relation;
    }

    return relation;
}

QAccessibleInterface *QAccessibleWidget::parent() const
{
    QObject *parentWidget= widget()->parentWidget();
    if (!parentWidget)
        parentWidget = qApp;
    return QAccessible::queryAccessibleInterface(parentWidget);
}

QAccessibleInterface *QAccessibleWidget::child(int index) const
{
    QWidgetList childList = childWidgets(widget());
    if (index >= 0 && index < childList.size())
        return QAccessible::queryAccessibleInterface(childList.at(index));
    return 0;
}

/*! \reimp */
int QAccessibleWidget::navigate(QAccessible::RelationFlag relation, int entry,
                                QAccessibleInterface **target) const
{
    if (!target)
        return -1;

    *target = 0;
    QObject *targetObject = 0;

    switch (relation) {
    case QAccessible::Covers:
        if (entry > 0) {
            QAccessibleInterface *pIface = QAccessible::queryAccessibleInterface(parentObject());
            if (!pIface)
                return -1;

            QRect r = rect();
            int sibCount = pIface->childCount();
            QAccessibleInterface *sibling = 0;
            for (int i = pIface->indexOfChild(this) + 1; i <= sibCount && entry; ++i) {
                sibling = pIface->child(i - 1);
                if (!sibling || (sibling->state().invisible)) {
                    delete sibling;
                    sibling = 0;
                    continue;
                }
                if (sibling->rect().intersects(r))
                    --entry;
                if (!entry)
                    break;
                delete sibling;
                sibling = 0;
            }
            delete pIface;
            *target = sibling;
            if (*target)
                return 0;
        }
        break;
    case QAccessible::Covered:
        if (entry > 0) {
            QAccessibleInterface *pIface = QAccessible::queryAccessibleInterface(parentObject());
            if (!pIface)
                return -1;

            QRect r = rect();
            int index = pIface->indexOfChild(this);
            QAccessibleInterface *sibling = 0;
            for (int i = 1; i < index && entry; ++i) {
                sibling = pIface->child(i - 1);
                Q_ASSERT(sibling);
                if (!sibling || (sibling->state().invisible)) {
                    delete sibling;
                    sibling = 0;
                    continue;
                }
                if (sibling->rect().intersects(r))
                    --entry;
                if (!entry)
                    break;
                delete sibling;
                sibling = 0;
            }
            delete pIface;
            *target = sibling;
            if (*target)
                return 0;
        }
        break;

    // Logical
    case QAccessible::FocusChild:
        {
            if (widget()->hasFocus()) {
                targetObject = object();
                break;
            }

            QWidget *fw = widget()->focusWidget();
            if (!fw)
                return -1;

            if (isAncestor(widget(), fw) || fw == widget())
                targetObject = fw;
            /* ###
            QWidget *parent = fw;
            while (parent && !targetObject) {
                parent = parent->parentWidget();
                if (parent == widget())
                    targetObject = fw;
            }
            */
        }
        break;
    case QAccessible::Label:
        if (entry > 0) {
            QAccessibleInterface *pIface = QAccessible::queryAccessibleInterface(parentObject());
            if (!pIface)
                return -1;

            // first check for all siblings that are labels to us
            // ideally we would go through all objects and check, but that
            // will be too expensive
            int sibCount = pIface->childCount();
            QAccessibleInterface *candidate = 0;
            for (int i = 0; i < sibCount && entry; ++i) {
                candidate = pIface->child(i);
                Q_ASSERT(candidate);
                if (candidate->relationTo(this) & QAccessible::Label)
                    --entry;
                if (!entry)
                    break;

                delete candidate;
                candidate = 0;
            }
            if (!candidate) {
                if (pIface->relationTo(this) & QAccessible::Label)
                    --entry;
                if (!entry)
                    candidate = pIface;
            }
            if (pIface != candidate)
                delete pIface;

            *target = candidate;
            if (*target)
                return 0;
        }
        break;
    case QAccessible::Labelled: // only implemented in subclasses
        break;
    case QAccessible::Controller:
        if (entry > 0) {
            // check all senders we are connected to,
            // and figure out which one are controllers to us
            QACConnectionObject *connectionObject = (QACConnectionObject*)object();
            QObjectList allSenders = connectionObject->senderList();
            QObjectList senders;
            for (int s = 0; s < allSenders.size(); ++s) {
                QObject *sender = allSenders.at(s);
                QAccessibleInterface *candidate = QAccessible::queryAccessibleInterface(sender);
                if (!candidate)
                    continue;
                if (candidate->relationTo(this) & QAccessible::Controller)
                    senders << sender;
                delete candidate;
            }
            if (entry <= senders.size())
                targetObject = senders.at(entry-1);
        }
        break;
    case QAccessible::Controlled:
        if (entry > 0) {
            QObjectList allReceivers;
            QACConnectionObject *connectionObject = (QACConnectionObject*)object();
            for (int sig = 0; sig < d->primarySignals.count(); ++sig) {
                QObjectList receivers = connectionObject->receiverList(d->primarySignals.at(sig).toAscii());
                allReceivers += receivers;
            }
            if (entry <= allReceivers.size())
                targetObject = allReceivers.at(entry-1);
        }
        break;
    default:
        break;
    }

    *target = QAccessible::queryAccessibleInterface(targetObject);
    return *target ? 0 : -1;
}

/*! \reimp */
int QAccessibleWidget::childCount() const
{
    QWidgetList cl = childWidgets(widget());
    return cl.size();
}

/*! \reimp */
int QAccessibleWidget::indexOfChild(const QAccessibleInterface *child) const
{
    QWidgetList cl = childWidgets(widget());
    int index = cl.indexOf(qobject_cast<QWidget *>(child->object()));
    if (index != -1)
        ++index;
    return index;
}

// from qwidget.cpp
extern QString qt_setWindowTitle_helperHelper(const QString &, const QWidget*);

/*! \reimp */
QString QAccessibleWidget::text(QAccessible::Text t) const
{
    QString str;

    switch (t) {
    case QAccessible::Name:
        if (!d->name.isEmpty()) {
            str = d->name;
        } else if (!widget()->accessibleName().isEmpty()) {
            str = widget()->accessibleName();
        } else if (widget()->isWindow()) {
            if (widget()->isMinimized())
                str = qt_setWindowTitle_helperHelper(widget()->windowIconText(), widget());
            else
                str = qt_setWindowTitle_helperHelper(widget()->windowTitle(), widget());
        } else {
            str = qt_accStripAmp(buddyString(widget()));
        }
        break;
    case QAccessible::Description:
        if (!d->description.isEmpty())
            str = d->description;
        else if (!widget()->accessibleDescription().isEmpty())
            str = widget()->accessibleDescription();
#ifndef QT_NO_TOOLTIP
        else
            str = widget()->toolTip();
#endif
        break;
    case QAccessible::Help:
        if (!d->help.isEmpty())
            str = d->help;
#ifndef QT_NO_WHATSTHIS
        else
            str = widget()->whatsThis();
#endif
        break;
    case QAccessible::Accelerator:
        if (!d->accelerator.isEmpty())
            str = d->accelerator;
        else
            str = qt_accHotKey(buddyString(widget()));
        break;
    case QAccessible::Value:
        str = d->value;
        break;
    default:
        break;
    }
    return str;
}

QStringList QAccessibleWidget::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        if (widget()->focusPolicy() != Qt::NoFocus)
            names << setFocusAction();
    }
    return names;
}

void QAccessibleWidget::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;

    if (actionName == setFocusAction()) {
        if (widget()->isWindow())
            widget()->activateWindow();
        widget()->setFocus();
    }
}

QStringList QAccessibleWidget::keyBindingsForAction(const QString & /* actionName */) const
{
    return QStringList();
}

/*! \reimp */
QAccessible::Role QAccessibleWidget::role() const
{
    return d->role;
}

/*! \reimp */
QAccessible::State QAccessibleWidget::state() const
{
    QAccessible::State state;

    QWidget *w = widget();
    if (w->testAttribute(Qt::WA_WState_Visible) == false)
        state.invisible = true;
    if (w->focusPolicy() != Qt::NoFocus && w->isActiveWindow())
        state.focusable = true;
    if (w->hasFocus())
        state.focused = true;
    if (!w->isEnabled())
        state.unavailable = true;
    if (w->isWindow()) {
        if (w->windowFlags() & Qt::WindowSystemMenuHint)
            state.movable = true;
        if (w->minimumSize() != w->maximumSize())
            state.sizeable = true;
    }

    return state;
}

QColor QAccessibleWidget::foregroundColor() const
{
    return widget()->palette().color(widget()->foregroundRole());
}

QColor QAccessibleWidget::backgroundColor() const
{
    return widget()->palette().color(widget()->backgroundRole());
}

void *QAccessibleWidget::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ActionInterface)
       return static_cast<QAccessibleActionInterface*>(this);
    return 0;
}

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
