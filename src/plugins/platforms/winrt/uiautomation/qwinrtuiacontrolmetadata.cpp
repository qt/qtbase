/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwinrtuiacontrolmetadata.h"
#include "qwinrtuiautils.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

using namespace QWinRTUiAutomation;

QWinRTUiaControlMetadata::QWinRTUiaControlMetadata()
{
}

QWinRTUiaControlMetadata::QWinRTUiaControlMetadata(QAccessible::Id id)
{
    update(id);
}

void QWinRTUiaControlMetadata::update(QAccessible::Id id)
{
    if (QAccessibleInterface *accessible = accessibleForId(id)) {
        m_automationId = generateAutomationId(accessible);
        m_className = generateClassName(accessible);
        m_controlName = generateControlName(accessible);
        m_role = generateRole(accessible);
        m_state = accessible->state();
        m_accelerator = accessible->text(QAccessible::Accelerator);
        m_access = accessible->text(QAccessible::Accelerator);
        m_help = accessible->text(QAccessible::Help);
        m_description = accessible->text(QAccessible::Description);
        m_value = accessible->text(QAccessible::Value);
        m_boundingRect = accessible->rect();
        updateValueData(accessible);
        updateTableData(accessible);
        updateTextData(accessible);
    }
}

QString QWinRTUiaControlMetadata::generateControlName(QAccessibleInterface *accessible)
{
    const bool clientTopLevel = (accessible->role() == QAccessible::Client)
        && accessible->parent() && (accessible->parent()->role() == QAccessible::Application);

    QString name = accessible->text(QAccessible::Name);
    if (name.isEmpty() && clientTopLevel)
        name = QCoreApplication::applicationName();
    return name;
}

QString QWinRTUiaControlMetadata::generateClassName(QAccessibleInterface *accessible)
{
    QString name;

    if (QObject *obj = accessible->object())
        name = QLatin1String(obj->metaObject()->className());
    return name;
}

// Generates an ID based on the name of the controls and their parents.
QString QWinRTUiaControlMetadata::generateAutomationId(QAccessibleInterface *accessible)
{
    QString autid;
    QObject *obj = accessible->object();
    while (obj) {
        QString name = obj->objectName();
        if (name.isEmpty()) {
            autid = QStringLiteral("");
            break;
        }
        if (!autid.isEmpty())
            autid.prepend(QLatin1Char('.'));
        autid.prepend(name);
        obj = obj->parent();
    }
    return autid;
}

QAccessible::Role QWinRTUiaControlMetadata::generateRole(QAccessibleInterface *accessible)
{
    const bool clientTopLevel = (accessible->role() == QAccessible::Client)
        && accessible->parent() && (accessible->parent()->role() == QAccessible::Application);

    if (clientTopLevel) {
        // Reports a top-level widget as a window.
        return QAccessible::Window;
    } else {
        return accessible->role();
    }
}

void QWinRTUiaControlMetadata::updateValueData(QAccessibleInterface *accessible)
{
    if (QAccessibleValueInterface *valueInterface = accessible->valueInterface()) {
        m_minimumValue = valueInterface->minimumValue().toDouble();
        m_maximumValue = valueInterface->maximumValue().toDouble();
        m_currentValue = valueInterface->currentValue().toDouble();
        m_minimumStepSize = valueInterface->minimumStepSize().toDouble();
    } else {
        m_minimumValue = 0.0;
        m_maximumValue = 0.0;
        m_currentValue = 0.0;
        m_minimumStepSize = 0.0;
    }
}

void QWinRTUiaControlMetadata::updateTableData(QAccessibleInterface *accessible)
{
    if (QAccessibleTableInterface *tableInterface = accessible->tableInterface()) {
        m_rowIndex = 0;
        m_columnIndex = 0;
        m_rowCount = tableInterface->rowCount();
        m_columnCount = tableInterface->columnCount();
    } else if (QAccessibleTableCellInterface *tableCellInterface = accessible->tableCellInterface()) {
        m_rowIndex = tableCellInterface->rowIndex();
        m_columnIndex = tableCellInterface->columnIndex();
        m_rowCount = tableCellInterface->rowExtent();
        m_columnCount = tableCellInterface->columnExtent();
    } else {
        m_rowIndex = 0;
        m_columnIndex = 0;
        m_rowCount = 0;
        m_columnCount = 0;
    }
}

void QWinRTUiaControlMetadata::updateTextData(QAccessibleInterface *accessible)
{
    if (QAccessibleTextInterface *textInterface = accessible->textInterface()) {
        m_cursorPosition = textInterface->cursorPosition();
        m_text = textInterface->text(0, textInterface->characterCount());
    } else {
        m_cursorPosition = 0;
        m_text = QStringLiteral("");
    }
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
