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

#include "qshadernode_p.h"

QT_BEGIN_NAMESPACE

QShaderNode::Type QShaderNode::type() const noexcept
{
    int inputCount = 0;
    int outputCount = 0;
    for (const auto &port : qAsConst(m_ports)) {
        switch (port.direction) {
        case QShaderNodePort::Input:
            inputCount++;
            break;
        case QShaderNodePort::Output:
            outputCount++;
            break;
        }
    }

    return (inputCount == 0 && outputCount == 0) ? Invalid
         : (inputCount > 0 && outputCount == 0) ? Output
         : (inputCount == 0 && outputCount > 0) ? Input
         : Function;
}

QUuid QShaderNode::uuid() const noexcept
{
    return m_uuid;
}

void QShaderNode::setUuid(const QUuid &uuid) noexcept
{
    m_uuid = uuid;
}

QStringList QShaderNode::layers() const noexcept
{
    return m_layers;
}

void QShaderNode::setLayers(const QStringList &layers) noexcept
{
    m_layers = layers;
}

QVector<QShaderNodePort> QShaderNode::ports() const noexcept
{
    return m_ports;
}

void QShaderNode::addPort(const QShaderNodePort &port)
{
    removePort(port);
    m_ports.append(port);
}

void QShaderNode::removePort(const QShaderNodePort &port)
{
    const auto it = std::find_if(m_ports.begin(), m_ports.end(),
                                 [port](const QShaderNodePort &p) {
                                    return p.name == port.name;
                                 });
    if (it != m_ports.end())
        m_ports.erase(it);
}

QStringList QShaderNode::parameterNames() const
{
    return m_parameters.keys();
}

QVariant QShaderNode::parameter(const QString &name) const
{
    return m_parameters.value(name);
}

void QShaderNode::setParameter(const QString &name, const QVariant &value)
{
    m_parameters.insert(name, value);
}

void QShaderNode::clearParameter(const QString &name)
{
    m_parameters.remove(name);
}

void QShaderNode::addRule(const QShaderFormat &format, const QShaderNode::Rule &rule)
{
    removeRule(format);
    m_rules << qMakePair(format, rule);
}

void QShaderNode::removeRule(const QShaderFormat &format)
{
    const auto it = std::find_if(m_rules.begin(), m_rules.end(),
                                 [format](const QPair<QShaderFormat, Rule> &entry) {
                                     return entry.first == format;
                                 });
    if (it != m_rules.end())
        m_rules.erase(it);
}

QVector<QShaderFormat> QShaderNode::availableFormats() const
{
    auto res = QVector<QShaderFormat>();
    std::transform(m_rules.cbegin(), m_rules.cend(),
                   std::back_inserter(res),
                   [](const QPair<QShaderFormat, Rule> &entry) { return entry.first; });
    return res;
}

QShaderNode::Rule QShaderNode::rule(const QShaderFormat &format) const
{
    const auto it = std::find_if(m_rules.crbegin(), m_rules.crend(),
                                 [format](const QPair<QShaderFormat, Rule> &entry) {
        return format.supports(entry.first);
    });
    return it != m_rules.crend() ? it->second : Rule();
}

QShaderNode::Rule::Rule(const QByteArray &subs, const QByteArrayList &snippets) noexcept
    : substitution(subs),
      headerSnippets(snippets)
{
}

bool operator==(const QShaderNode::Rule &lhs, const QShaderNode::Rule &rhs) noexcept
{
    return lhs.substitution == rhs.substitution
        && lhs.headerSnippets == rhs.headerSnippets;
}

QT_END_NAMESPACE
