// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qaccessiblebridgeutils_p.h"
#include <QtCore/qmath.h>

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

}   //namespace

QT_END_NAMESPACE
