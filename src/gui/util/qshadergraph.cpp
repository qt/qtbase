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

#include "qshadergraph_p.h"

QT_BEGIN_NAMESPACE

void QShaderGraph::addNode(const QShaderNode &node)
{
    removeNode(node);
    m_nodes.append(node);
}

void QShaderGraph::removeNode(const QShaderNode &node)
{
    const auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
                                 [node] (const QShaderNode &n) { return n.uuid() == node.uuid(); });
    if (it != m_nodes.end())
        m_nodes.erase(it);
}

QVector<QShaderNode> QShaderGraph::nodes() const Q_DECL_NOTHROW
{
    return m_nodes;
}

void QShaderGraph::addEdge(const QShaderGraph::Edge &edge)
{
    if (m_edges.contains(edge))
        return;
    m_edges.append(edge);
}

void QShaderGraph::removeEdge(const QShaderGraph::Edge &edge)
{
    m_edges.removeAll(edge);
}

QVector<QShaderGraph::Edge> QShaderGraph::edges() const Q_DECL_NOTHROW
{
    return m_edges;
}

bool operator==(const QShaderGraph::Edge &lhs, const QShaderGraph::Edge &rhs) Q_DECL_NOTHROW
{
    return lhs.sourceNodeUuid == rhs.sourceNodeUuid
        && lhs.sourcePortName == rhs.sourcePortName
        && lhs.targetNodeUuid == rhs.targetNodeUuid
        && lhs.targetPortName == rhs.targetPortName;
}

QT_END_NAMESPACE
