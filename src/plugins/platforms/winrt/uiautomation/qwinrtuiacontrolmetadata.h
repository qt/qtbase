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

#ifndef QWINRTUIACONTROLMETADATA_H
#define QWINRTUIACONTROLMETADATA_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include <QString>
#include <QtGui/QAccessible>
#include <QtGui/QAccessibleInterface>

QT_BEGIN_NAMESPACE

// Cacheable control metadata
class QWinRTUiaControlMetadata
{
public:
    QWinRTUiaControlMetadata();
    QWinRTUiaControlMetadata(QAccessible::Id id);
    void update(QAccessible::Id id);
    QString automationId() const { return m_automationId; }
    QString className() const { return m_className; }
    QString controlName() const { return m_controlName; }
    QString accelerator() const { return m_accelerator; }
    QString access() const { return m_access; }
    QString help() const {  return m_help; }
    QString description() const { return m_description; }
    QString value() const { return m_value; }
    QString text() const { return m_text; }
    QAccessible::Role role() const { return m_role; }
    QAccessible::State state() const { return m_state; }
    QRect boundingRect() const { return m_boundingRect; }
    double minimumValue() const { return m_minimumValue; }
    double maximumValue() const { return m_maximumValue; }
    double currentValue() const { return m_currentValue; }
    double minimumStepSize() const { return m_minimumStepSize; }
    int rowIndex() const { return m_rowIndex; }
    int columnIndex() const { return m_columnIndex; }
    int rowCount() const { return m_rowCount; }
    int columnCount() const { return m_columnCount; }
    int characterCount() const { return m_text.length(); }
    int cursorPosition() const { return m_cursorPosition; }

private:
    QString generateControlName(QAccessibleInterface *accessible);
    QString generateClassName(QAccessibleInterface *accessible);
    QString generateAutomationId(QAccessibleInterface *accessible);
    QAccessible::Role generateRole(QAccessibleInterface *accessible);
    void updateValueData(QAccessibleInterface *accessible);
    void updateTableData(QAccessibleInterface *accessible);
    void updateTextData(QAccessibleInterface *accessible);
    QString m_automationId;
    QString m_className;
    QString m_controlName;
    QString m_accelerator;
    QString m_access;
    QString m_help;
    QString m_description;
    QString m_value;
    QString m_text;
    QAccessible::Role m_role = QAccessible::NoRole;
    QAccessible::State m_state;
    QRect m_boundingRect;
    double m_minimumValue = 0.0;
    double m_maximumValue = 0.0;
    double m_currentValue = 0.0;
    double m_minimumStepSize = 0.0;
    int m_rowIndex = 0;
    int m_columnIndex = 0;
    int m_rowCount = 0;
    int m_columnCount = 0;
    int m_cursorPosition = 0;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINRTUIACONTROLMETADATA_H
