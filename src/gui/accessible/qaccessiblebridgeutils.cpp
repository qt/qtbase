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
