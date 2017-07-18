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


namespace
{
    QVector<QShaderNode> copyOutputNodes(const QVector<QShaderNode> &nodes)
    {
        auto res = QVector<QShaderNode>();
        std::copy_if(nodes.cbegin(), nodes.cend(),
                     std::back_inserter(res),
                     [] (const QShaderNode &node) {
                         return node.type() == QShaderNode::Output;
                     });
        return res;
    }

    QVector<QShaderGraph::Edge> incomingEdges(const QVector<QShaderGraph::Edge> &edges, const QUuid &uuid)
    {
        auto res = QVector<QShaderGraph::Edge>();
        std::copy_if(edges.cbegin(), edges.cend(),
                     std::back_inserter(res),
                     [uuid] (const QShaderGraph::Edge &edge) {
                         return edge.sourceNodeUuid == uuid;
                     });
        return res;
    }

    QVector<QShaderGraph::Edge> outgoingEdges(const QVector<QShaderGraph::Edge> &edges, const QUuid &uuid)
    {
        auto res = QVector<QShaderGraph::Edge>();
        std::copy_if(edges.cbegin(), edges.cend(),
                     std::back_inserter(res),
                     [uuid] (const QShaderGraph::Edge &edge) {
                         return edge.targetNodeUuid == uuid;
                     });
        return res;
    }

    QShaderGraph::Statement nodeToStatement(const QShaderNode &node, int &nextVarId)
    {
        auto statement = QShaderGraph::Statement();
        statement.node = node;

        const auto ports = node.ports();
        for (const auto &port : ports) {
            if (port.direction == QShaderNodePort::Input) {
                statement.inputs.append(-1);
            } else {
                statement.outputs.append(nextVarId);
                nextVarId++;
            }
        }
        return statement;
    }

    QShaderGraph::Statement completeStatement(const QHash<QUuid, QShaderGraph::Statement> &idHash,
                                              const QVector<QShaderGraph::Edge> edges,
                                              const QUuid &uuid)
    {
        auto targetStatement = idHash.value(uuid);
        for (const auto &edge : edges) {
            if (edge.targetNodeUuid != uuid)
                continue;

            const auto sourceStatement = idHash.value(edge.sourceNodeUuid);
            const auto sourcePortIndex = sourceStatement.portIndex(QShaderNodePort::Output, edge.sourcePortName);
            const auto targetPortIndex = targetStatement.portIndex(QShaderNodePort::Input, edge.targetPortName);

            if (sourcePortIndex < 0 || targetPortIndex < 0)
                continue;

            const auto &sourceOutputs = sourceStatement.outputs;
            auto &targetInputs = targetStatement.inputs;
            targetInputs[targetPortIndex] = sourceOutputs[sourcePortIndex];
        }
        return targetStatement;
    }
}

QUuid QShaderGraph::Statement::uuid() const Q_DECL_NOTHROW
{
    return node.uuid();
}

int QShaderGraph::Statement::portIndex(QShaderNodePort::Direction direction, const QString &portName) const Q_DECL_NOTHROW
{
    const auto ports = node.ports();
    int index = 0;
    for (const auto &port : ports) {
        if (port.name == portName && port.direction == direction)
            return index;
        else if (port.direction == direction)
            index++;
    }
    return -1;
}

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

QVector<QShaderGraph::Statement> QShaderGraph::createStatements(const QStringList &enabledLayers) const
{
    const auto intersectsEnabledLayers = [enabledLayers] (const QStringList &layers) {
        return layers.isEmpty()
            || std::any_of(layers.cbegin(), layers.cend(),
                           [enabledLayers] (const QString &s) { return enabledLayers.contains(s); });
    };

    const auto enabledNodes = [this, intersectsEnabledLayers] {
        auto res = QVector<QShaderNode>();
        std::copy_if(m_nodes.cbegin(), m_nodes.cend(),
                     std::back_inserter(res),
                     [intersectsEnabledLayers] (const QShaderNode &node) {
                         return intersectsEnabledLayers(node.layers());
                     });
        return res;
    }();

    const auto enabledEdges = [this, intersectsEnabledLayers] {
        auto res = QVector<Edge>();
        std::copy_if(m_edges.cbegin(), m_edges.cend(),
                     std::back_inserter(res),
                     [intersectsEnabledLayers] (const Edge &edge) {
                         return intersectsEnabledLayers(edge.layers);
                     });
        return res;
    }();

    const auto idHash = [enabledNodes] {
        auto nextVarId = 0;
        auto res = QHash<QUuid, Statement>();
        for (const auto &node : enabledNodes)
            res.insert(node.uuid(), nodeToStatement(node, nextVarId));
        return res;
    }();

    auto result = QVector<Statement>();
    auto currentEdges = enabledEdges;
    auto currentUuids = [enabledNodes] {
        const auto inputs = copyOutputNodes(enabledNodes);
        auto res = QVector<QUuid>();
        std::transform(inputs.cbegin(), inputs.cend(),
                       std::back_inserter(res),
                       [](const QShaderNode &node) { return node.uuid(); });
        return res;
    }();

    // Implements Kahn's algorithm to flatten the graph
    // https://en.wikipedia.org/wiki/Topological_sorting#Kahn.27s_algorithm
    //
    // We implement it with a small twist though, we follow the edges backward
    // because we want to track the dependencies from the output nodes and not the
    // input nodes
    while (!currentUuids.isEmpty()) {
        const auto uuid = currentUuids.takeFirst();
        result.append(completeStatement(idHash, enabledEdges, uuid));

        const auto outgoing = outgoingEdges(currentEdges, uuid);
        for (const auto &outgoingEdge : outgoing) {
            currentEdges.removeAll(outgoingEdge);
            const QUuid nextUuid = outgoingEdge.sourceNodeUuid;
            const auto incoming = incomingEdges(currentEdges, nextUuid);
            if (incoming.isEmpty()) {
                currentUuids.append(nextUuid);
            }
        }
    }

    std::reverse(result.begin(), result.end());
    return result;
}

bool operator==(const QShaderGraph::Edge &lhs, const QShaderGraph::Edge &rhs) Q_DECL_NOTHROW
{
    return lhs.sourceNodeUuid == rhs.sourceNodeUuid
        && lhs.sourcePortName == rhs.sourcePortName
        && lhs.targetNodeUuid == rhs.targetNodeUuid
        && lhs.targetPortName == rhs.targetPortName;
}

bool operator==(const QShaderGraph::Statement &lhs, const QShaderGraph::Statement &rhs) Q_DECL_NOTHROW
{
    return lhs.inputs == rhs.inputs
        && lhs.outputs == rhs.outputs
        && lhs.node.uuid() == rhs.node.uuid();
}

QT_END_NAMESPACE
