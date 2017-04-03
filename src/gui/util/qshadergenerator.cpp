/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
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

#include "qshadergenerator_p.h"

QT_BEGIN_NAMESPACE

QByteArray QShaderGenerator::createShaderCode() const
{
    auto code = QByteArrayList();

    if (format.isValid()) {
        const auto isGLES = format.api() == QShaderFormat::OpenGLES;
        const auto major = format.version().majorVersion();
        const auto minor = format.version().minorVersion();

        const auto version = major == 2 && isGLES ? 100
                           : major == 3 && isGLES ? 300
                           : major == 2 ? 100 + 10 * (minor + 1)
                           : major == 3 && minor <= 2 ? 100 + 10 * (minor + 3)
                           : major * 100 + minor * 10;

        const auto profile = isGLES ? QByteArrayLiteral(" es")
                           : version >= 150 && format.api() == QShaderFormat::OpenGLCoreProfile ? QByteArrayLiteral(" core")
                           : version >= 150 && format.api() == QShaderFormat::OpenGLCompatibilityProfile ? QByteArrayLiteral(" compatibility")
                           : QByteArray();

        code << (QByteArrayLiteral("#version ") + QByteArray::number(version) + profile);
        code << QByteArray();
    }

    for (const auto &node : graph.nodes()) {
        for (const auto &snippet : node.rule(format).headerSnippets) {
            code << snippet;
        }
    }

    code << QByteArray();
    code << QByteArrayLiteral("void main()");
    code << QByteArrayLiteral("{");

    for (const auto &statement : graph.createStatements()) {
        const auto node = statement.node;
        auto line = node.rule(format).substitution;
        for (const auto &port : node.ports()) {
            const auto portName = port.name;
            const auto portDirection = port.direction;
            const auto isInput = port.direction == QShaderNodePort::Input;

            const auto portIndex = statement.portIndex(portDirection, portName);
            const auto variableIndex = isInput ? statement.inputs.at(portIndex)
                                               : statement.outputs.at(portIndex);
            if (variableIndex < 0)
                continue;

            const auto placeholder = QByteArray(QByteArrayLiteral("$") + portName.toUtf8());
            const auto variable = QByteArray(QByteArrayLiteral("v") + QByteArray::number(variableIndex));
            line.replace(placeholder, variable);
        }
        code << QByteArrayLiteral("    ") + line;
    }

    code << QByteArrayLiteral("}");
    code << QByteArray();
    return code.join('\n');
}

QT_END_NAMESPACE
