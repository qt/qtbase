// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblewidget.h"

#if QT_CONFIG(accessibility)

#include "qapplication.h"
#if QT_CONFIG(groupbox)
#include "qgroupbox.h"
#endif
#if QT_CONFIG(label)
#include "qlabel.h"
#endif
#if QT_CONFIG(tooltip)
#include "qtooltip.h"
#endif
#if QT_CONFIG(whatsthis)
#include "qwhatsthis.h"
#endif
#include "qwidget.h"
#include "qdebug.h"
#include <qmath.h>
#if QT_CONFIG(rubberband)
#include <QRubberBand>
#endif
#include <QFocusFrame>
#if QT_CONFIG(menu)
#include <QMenu>
#endif
#include <QtGui/private/qaccessiblehelper_p.h>
#include <QtWidgets/private/qwidget_p.h>

#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QWidgetList _q_ac_childWidgets(const QWidget *widget);

static QString buddyString(const QWidget *widget)
{
    if (!widget)
        return QString();
    QWidget *parent = widget->parentWidget();
    if (!parent)
        return QString();
#if QT_CONFIG(shortcut) && QT_CONFIG(label)
    for (QObject *o : parent->children()) {
        QLabel *label = qobject_cast<QLabel*>(o);
        if (label && label->buddy() == widget)
            return label->text();
    }
#endif

#if QT_CONFIG(groupbox)
    QGroupBox *groupbox = qobject_cast<QGroupBox*>(parent);
    if (groupbox)
        return groupbox->title();
#endif

    return QString();
}

QString qt_accHotKey(const QString &text)
{
#ifndef QT_NO_SHORTCUT
    return QKeySequence::mnemonic(text).toString(QKeySequence::NativeText);
#else
    Q_UNUSED(text);
#endif

    return QString();
}

// ### inherit QAccessibleObjectPrivate
class QAccessibleWidgetPrivate
{
public:
    QAccessibleWidgetPrivate()
        :role(QAccessible::Client)
    {}

    QAccessible::Role role;
    QString name;
    QStringList primarySignals;
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
    \a role is an optional parameter that sets the object's role property.
*/
QAccessibleWidget::QAccessibleWidget(QWidget *w, QAccessible::Role role)
: QAccessibleObject(w)
{
    Q_ASSERT(widget());
    d = new QAccessibleWidgetPrivate();
    d->role = role;
}

/*!
    Creates a QAccessibleWidget object for widget \a w.
    \a role and \a name are optional parameters that set the object's
    role and name properties.
*/
QAccessibleWidget::QAccessibleWidget(QWidget *w, QAccessible::Role role, const QString &name)
    : QAccessibleWidget(w, role)
{
    d->name = name;
}

/*! \reimp */
bool QAccessibleWidget::isValid() const
{
    if (!object() || static_cast<QWidget *>(object())->d_func()->data.in_destructor)
        return false;
    return QAccessibleObject::isValid();
}

/*! \reimp */
QWindow *QAccessibleWidget::window() const
{
    const QWidget *w = widget();
    Q_ASSERT(w);
    QWindow *result = w->windowHandle();
    if (!result) {
        if (const QWidget *nativeParent = w->nativeParentWidget())
            result = nativeParent->windowHandle();
    }
    return result;
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
    QWidget *w = widget();
    if (!w || w->isWindow() || !w->parentWidget())
        return qApp;
    return w->parent();
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

/*!
    Registers \a signal as a controlling signal.

    An object is a Controller to any other object connected to a
    controlling signal.
*/
void QAccessibleWidget::addControllingSignal(const QString &signal)
{
    QByteArray s = QMetaObject::normalizedSignature(signal.toLatin1());
    if (Q_UNLIKELY(object()->metaObject()->indexOfSignal(s) < 0))
        qWarning("Signal %s unknown in %s", s.constData(), object()->metaObject()->className());
    d->primarySignals << QLatin1StringView(s);
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
QList<std::pair<QAccessibleInterface *, QAccessible::Relation>>
QAccessibleWidget::relations(QAccessible::Relation match /*= QAccessible::AllRelations*/) const
{
    QList<std::pair<QAccessibleInterface *, QAccessible::Relation>> rels;
    if (match & QAccessible::Label) {
        const QAccessible::Relation rel = QAccessible::Label;
        if (QWidget *parent = widget()->parentWidget()) {
#if QT_CONFIG(shortcut) && QT_CONFIG(label)
            // first check for all siblings that are labels to us
            // ideally we would go through all objects and check, but that
            // will be too expensive
            const QList<QWidget*> kids = _q_ac_childWidgets(parent);
            for (QWidget *kid : kids) {
                if (QLabel *labelSibling = qobject_cast<QLabel*>(kid)) {
                    if (labelSibling->buddy() == widget()) {
                        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(labelSibling);
                        rels.emplace_back(iface, rel);
                    }
                }
            }
#endif
#if QT_CONFIG(groupbox)
            QGroupBox *groupbox = qobject_cast<QGroupBox*>(parent);
            if (groupbox && !groupbox->title().isEmpty()) {
                QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(groupbox);
                rels.emplace_back(iface, rel);
            }
#endif
        }
    }

    if (match & QAccessible::Controlled) {
        QObjectList allReceivers;
        QObject *connectionObject = object();
        for (int sig = 0; sig < d->primarySignals.size(); ++sig) {
            const QObjectList receivers = connectionObject->d_func()->receiverList(d->primarySignals.at(sig).toLatin1());
            allReceivers += receivers;
        }

        allReceivers.removeAll(object());  //### The object might connect to itself internally

        for (int i = 0; i < allReceivers.size(); ++i) {
            const QAccessible::Relation rel = QAccessible::Controlled;
            QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(allReceivers.at(i));
            if (iface)
                rels.emplace_back(iface, rel);
        }
    }

    return rels;
}

/*! \reimp */
QAccessibleInterface *QAccessibleWidget::parent() const
{
    return QAccessible::queryAccessibleInterface(parentObject());
}

/*! \reimp */
QAccessibleInterface *QAccessibleWidget::child(int index) const
{
    Q_ASSERT(widget());
    QWidgetList childList = _q_ac_childWidgets(widget());
    if (index >= 0 && index < childList.size())
        return QAccessible::queryAccessibleInterface(childList.at(index));
    return nullptr;
}

/*! \reimp */
QAccessibleInterface *QAccessibleWidget::focusChild() const
{
    if (widget()->hasFocus())
        return QAccessible::queryAccessibleInterface(object());

    QWidget *fw = widget()->focusWidget();
    if (!fw)
        return nullptr;

    if (isAncestor(widget(), fw)) {
        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(fw);
        if (!iface || iface == this || !iface->focusChild())
            return iface;
        return iface->focusChild();
    }
    return nullptr;
}

/*! \reimp */
int QAccessibleWidget::childCount() const
{
    QWidgetList cl = _q_ac_childWidgets(widget());
    return cl.size();
}

/*! \reimp */
int QAccessibleWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;
    QWidgetList cl = _q_ac_childWidgets(widget());
    return cl.indexOf(qobject_cast<QWidget *>(child->object()));
}

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
            if (QWindow *window = widget()->windowHandle()) {
                if (QPlatformWindow *platformWindow = window->handle())
                    str = platformWindow->windowTitle();
            }
        } else {
            str = qt_accStripAmp(buddyString(widget()));
        }
        break;
    case QAccessible::Description:
        str = widget()->accessibleDescription();
#if QT_CONFIG(tooltip)
        if (str.isEmpty())
            str = widget()->toolTip();
#endif
        break;
    case QAccessible::Identifier:
        str = widget()->accessibleIdentifier();
        break;
    case QAccessible::Help:
#if QT_CONFIG(whatsthis)
        str = widget()->whatsThis();
#endif
        break;
    case QAccessible::Accelerator:
        str = qt_accHotKey(buddyString(widget()));
        break;
    case QAccessible::Value:
        break;
    default:
        break;
    }
    return str;
}

/*! \reimp */
QStringList QAccessibleWidget::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        if (widget()->focusPolicy() != Qt::NoFocus)
            names << setFocusAction();
        if (widget()->contextMenuPolicy() == Qt::ActionsContextMenu && widget()->actions().size() > 0)
            names << showMenuAction();
    }
    return names;
}

/*! \reimp */
void QAccessibleWidget::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;

    if (actionName == setFocusAction()) {
        if (widget()->isWindow())
            widget()->activateWindow();
        widget()->setFocus();
    } else if (actionName == showMenuAction()) {
        QContextMenuEvent e(QContextMenuEvent::Other,
            QPoint(), widget()->mapToGlobal(QPoint()),
            QGuiApplication::keyboardModifiers());
        QCoreApplication::sendEvent(widget(), &e);
    }
}

/*! \reimp */
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
    if (w->focusPolicy() != Qt::NoFocus)
        state.focusable = true;
    if (w->hasFocus())
        state.focused = true;
    if (!w->isEnabled())
        state.disabled = true;
    if (w->isWindow()) {
        if (w->windowFlags() & Qt::WindowSystemMenuHint)
            state.movable = true;
        if (w->minimumSize() != w->maximumSize())
            state.sizeable = true;
        if (w->isActiveWindow())
            state.active = true;
    }

    return state;
}

/*! \reimp */
QColor QAccessibleWidget::foregroundColor() const
{
    return widget()->palette().color(widget()->foregroundRole());
}

/*! \reimp */
QColor QAccessibleWidget::backgroundColor() const
{
    return widget()->palette().color(widget()->backgroundRole());
}

/*! \reimp */
void *QAccessibleWidget::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ActionInterface)
       return static_cast<QAccessibleActionInterface*>(this);
    return nullptr;
}

// QAccessibleWidgetV2 implementation

QAccessibleWidgetV2::QAccessibleWidgetV2(QWidget *object, QAccessible::Role role,
                                         const QString &name)
    : QAccessibleWidget(object, role, name)
{
}

QAccessibleWidgetV2::QAccessibleWidgetV2(QWidget *object, QAccessible::Role role)
    : QAccessibleWidget(object, role)
{
}

QAccessibleWidgetV2::~QAccessibleWidgetV2() = default;

/*! \reimp */
void *QAccessibleWidgetV2::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::AttributesInterface)
        return static_cast<QAccessibleAttributesInterface *>(this);
    return QAccessibleWidget::interface_cast(t);
}

/*! \reimp */
QList<QAccessible::Attribute> QAccessibleWidgetV2::attributeKeys() const
{
    return { QAccessible::Attribute::Locale };
}

/*! \reimp */
QVariant QAccessibleWidgetV2::attributeValue(QAccessible::Attribute key) const
{
    if (key == QAccessible::Attribute::Locale)
        return QVariant::fromValue(widget()->locale());

    return QVariant();
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
