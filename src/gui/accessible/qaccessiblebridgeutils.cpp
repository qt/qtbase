// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qaccessiblebridgeutils_p.h"
#include <QtCore/qmath.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

namespace QAccessibleBridgeUtils {

static bool performAction(QAccessibleInterface *iface, const QString &actionName)
{
    if (QAccessibleActionInterface *actionIface = iface->actionInterface()) {
        if (actionIface->actionNames().contains(actionName)) {
            actionIface->doAction(actionName);
            return true;
        }
    }
    return false;
}

QStringList effectiveActionNames(QAccessibleInterface *iface)
{
    QStringList actions;
    if (QAccessibleActionInterface *actionIface = iface->actionInterface())
        actions = actionIface->actionNames();

    if (iface->valueInterface()) {
        if (!actions.contains(QAccessibleActionInterface::increaseAction()))
            actions << QAccessibleActionInterface::increaseAction();
        if (!actions.contains(QAccessibleActionInterface::decreaseAction()))
            actions << QAccessibleActionInterface::decreaseAction();
    }
    return actions;
}

bool performEffectiveAction(QAccessibleInterface *iface, const QString &actionName)
{
    if (!iface)
        return false;
    if (performAction(iface, actionName))
        return true;
    if (actionName != QAccessibleActionInterface::increaseAction()
        && actionName != QAccessibleActionInterface::decreaseAction())
        return false;

    QAccessibleValueInterface *valueIface = iface->valueInterface();
    if (!valueIface)
        return false;
    bool success;
    const QVariant currentVariant = valueIface->currentValue();
    double stepSize = valueIface->minimumStepSize().toDouble(&success);
    if (!success || qFuzzyIsNull(stepSize)) {
        const double min = valueIface->minimumValue().toDouble(&success);
        if (!success)
            return false;
        const double max = valueIface->maximumValue().toDouble(&success);
        if (!success)
            return false;
        stepSize = (max - min) / 10;  // this is pretty arbitrary, we just need to provide something
        const int typ = currentVariant.userType();
        if (typ != QMetaType::Float && typ != QMetaType::Double) {
            // currentValue is an integer. Round it up to ensure stepping in case it was below 1
            stepSize = qCeil(stepSize);
        }
    }
    const double current = currentVariant.toDouble(&success);
    if (!success)
        return false;
    if (actionName == QAccessibleActionInterface::decreaseAction())
        stepSize = -stepSize;
    valueIface->setCurrentValue(current + stepSize);
    return true;
}

QString accessibleId(QAccessibleInterface *accessible) {
    QString result;
    if (!accessible)
        return result;
    result = accessible->text(QAccessible::Identifier);
    if (!result.isEmpty())
        return result;
    while (accessible) {
        if (!result.isEmpty())
            result.prepend(u'.');
        if (auto obj = accessible->object()) {
            const QString name = obj->objectName();
            if (!name.isEmpty())
                result.prepend(name);
            else
                result.prepend(QString::fromUtf8(obj->metaObject()->className()));
        }
        accessible = accessible->parent();
    }
    return result;
}

/*
    Returns the window whose native handle is responsible for \a iface.

    Not every interface implements window(), so we walk up the ancestors until
    one does, as the documentation asks of the backend. The first ancestor to
    answer gives the closest window, since a window's root interface sits on this
    chain and reports its own window.

    A caller that already knows the window of an ancestor passes the ancestor as
    \a ancestorInterface and its window as \a ancestorWindow. The walk then stops
    at that ancestor and answers with \a ancestorWindow. The full walk ends up at
    the same window, so the two arguments only skip the part the caller has
    already walked.

    The two arguments also let us reject an answer that can not be true. An
    interface below \a ancestorInterface can not live in a window that contains
    \a ancestorWindow. A bridge that substituted the native view of such a window
    would hand back a view that contains \a ancestorWindow's own view, and the
    native accessibility tree would loop. An interface that answers window() with
    the top level instead of the window containing it gives such an answer, so we
    discard it and keep walking.

    We do however accept a window that is not a descendant of \a ancestorWindow. A
    QWindowContainer inside a child QWindow hands its contained window to the top
    level, and the two windows are then siblings. Move the container further down
    the QWindow hierarchy and \a ancestorWindow becomes a descendant of a sibling
    of the window we return.

    Callers that have specific demands on the accepted window hierarchy beyond
    the loop detection must handle those cases at the call site.
*/
QWindow *windowFor(const QAccessibleInterface *iface,
                   const QAccessibleInterface *ancestorInterface,
                   QWindow *ancestorWindow)
{
    Q_ASSERT(bool(ancestorInterface) == bool(ancestorWindow));

    for (const auto *candidate = iface; candidate && candidate != ancestorInterface;
         candidate = candidate->parent()) {
        QWindow *window = candidate->window();
        if (!window)
            continue;

        // We found a window before reaching ancestorInterface. It can only
        // represent the interface if it does not contain ancestorWindow,
        // as otherwise the native accessibility tree would loop.
        if (ancestorWindow && window->isAncestorOf(ancestorWindow, QWindow::ExcludeTransients))
            continue;

        return window;
    }

    return ancestorWindow;
}

}   //namespace

QT_END_NAMESPACE
