/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <QtGui/private/qshadergraph_p.h>

namespace
{
    QShaderNodePort createPort(QShaderNodePort::Direction portDirection, const QString &portName)
    {
        auto port = QShaderNodePort();
        port.direction = portDirection;
        port.name = portName;
        return port;
    }

    QShaderNode createNode(const QVector<QShaderNodePort> &ports)
    {
        auto node = QShaderNode();
        node.setUuid(QUuid::createUuid());
        for (const auto &port : ports)
            node.addPort(port);
        return node;
    }

    QShaderGraph::Edge createEdge(const QUuid &sourceUuid, const QString &sourceName,
                                  const QUuid &targetUuid, const QString &targetName)
    {
        auto edge = QShaderGraph::Edge();
        edge.sourceNodeUuid = sourceUuid;
        edge.sourcePortName = sourceName;
        edge.targetNodeUuid = targetUuid;
        edge.targetPortName = targetName;
        return edge;
    }
}

class tst_QShaderGraph : public QObject
{
    Q_OBJECT
private slots:
    void shouldHaveEdgeDefaultState();
    void shouldTestEdgesEquality_data();
    void shouldTestEdgesEquality();
    void shouldManageNodeList();
    void shouldManageEdgeList();
};

void tst_QShaderGraph::shouldHaveEdgeDefaultState()
{
    // GIVEN
    auto edge = QShaderGraph::Edge();

    // THEN
    QVERIFY(edge.sourceNodeUuid.isNull());
    QVERIFY(edge.sourcePortName.isEmpty());
    QVERIFY(edge.targetNodeUuid.isNull());
    QVERIFY(edge.targetPortName.isEmpty());
}

void tst_QShaderGraph::shouldTestEdgesEquality_data()
{
    QTest::addColumn<QShaderGraph::Edge>("left");
    QTest::addColumn<QShaderGraph::Edge>("right");
    QTest::addColumn<bool>("expected");

    const auto sourceUuid1 = QUuid::createUuid();
    const auto sourceUuid2 = QUuid::createUuid();
    const auto targetUuid1 = QUuid::createUuid();
    const auto targetUuid2 = QUuid::createUuid();

    QTest::newRow("Equals") << createEdge(sourceUuid1, "foo", targetUuid1, "bar")
                            << createEdge(sourceUuid1, "foo", targetUuid1, "bar")
                            << true;
    QTest::newRow("SourceUuid") << createEdge(sourceUuid1, "foo", targetUuid1, "bar")
                                << createEdge(sourceUuid2, "foo", targetUuid1, "bar")
                                << false;
    QTest::newRow("SourceName") << createEdge(sourceUuid1, "foo", targetUuid1, "bar")
                                << createEdge(sourceUuid1, "bleh", targetUuid1, "bar")
                                << false;
    QTest::newRow("TargetUuid") << createEdge(sourceUuid1, "foo", targetUuid1, "bar")
                                << createEdge(sourceUuid1, "foo", targetUuid2, "bar")
                                << false;
    QTest::newRow("TargetName") << createEdge(sourceUuid1, "foo", targetUuid1, "bar")
                                << createEdge(sourceUuid1, "foo", targetUuid1, "bleh")
                                << false;
}

void tst_QShaderGraph::shouldTestEdgesEquality()
{
    // GIVEN
    QFETCH(QShaderGraph::Edge, left);
    QFETCH(QShaderGraph::Edge, right);

    // WHEN
    const auto equal = (left == right);
    const auto notEqual = (left != right);

    // THEN
    QFETCH(bool, expected);
    QCOMPARE(equal, expected);
    QCOMPARE(notEqual, !expected);
}

void tst_QShaderGraph::shouldManageNodeList()
{
    // GIVEN
    const auto node1 = createNode({createPort(QShaderNodePort::Output, "node1")});
    const auto node2 = createNode({createPort(QShaderNodePort::Output, "node2")});

    auto graph = QShaderGraph();

    // THEN (default state)
    QVERIFY(graph.nodes().isEmpty());

    // WHEN
    graph.addNode(node1);

    // THEN
    QCOMPARE(graph.nodes().size(), 1);
    QCOMPARE(graph.nodes().at(0).uuid(), node1.uuid());
    QCOMPARE(graph.nodes().at(0).ports().at(0).name, node1.ports().at(0).name);

    // WHEN
    graph.addNode(node2);

    // THEN
    QCOMPARE(graph.nodes().size(), 2);
    QCOMPARE(graph.nodes().at(0).uuid(), node1.uuid());
    QCOMPARE(graph.nodes().at(0).ports().at(0).name, node1.ports().at(0).name);
    QCOMPARE(graph.nodes().at(1).uuid(), node2.uuid());
    QCOMPARE(graph.nodes().at(1).ports().at(0).name, node2.ports().at(0).name);


    // WHEN
    graph.removeNode(node2);

    // THEN
    QCOMPARE(graph.nodes().size(), 1);
    QCOMPARE(graph.nodes().at(0).uuid(), node1.uuid());
    QCOMPARE(graph.nodes().at(0).ports().at(0).name, node1.ports().at(0).name);

    // WHEN
    graph.addNode(node2);

    // THEN
    QCOMPARE(graph.nodes().size(), 2);
    QCOMPARE(graph.nodes().at(0).uuid(), node1.uuid());
    QCOMPARE(graph.nodes().at(0).ports().at(0).name, node1.ports().at(0).name);
    QCOMPARE(graph.nodes().at(1).uuid(), node2.uuid());
    QCOMPARE(graph.nodes().at(1).ports().at(0).name, node2.ports().at(0).name);

    // WHEN
    const auto node1bis = [node1] {
        auto res = node1;
        auto port = res.ports().at(0);
        port.name = QStringLiteral("node1bis");
        res.addPort(port);
        return res;
    }();
    graph.addNode(node1bis);

    // THEN
    QCOMPARE(graph.nodes().size(), 2);
    QCOMPARE(graph.nodes().at(0).uuid(), node2.uuid());
    QCOMPARE(graph.nodes().at(0).ports().at(0).name, node2.ports().at(0).name);
    QCOMPARE(graph.nodes().at(1).uuid(), node1bis.uuid());
    QCOMPARE(graph.nodes().at(1).ports().at(0).name, node1bis.ports().at(0).name);
}

void tst_QShaderGraph::shouldManageEdgeList()
{
    // GIVEN
    const auto edge1 = createEdge(QUuid::createUuid(), "foo", QUuid::createUuid(), "bar");
    const auto edge2 = createEdge(QUuid::createUuid(), "baz", QUuid::createUuid(), "boo");

    auto graph = QShaderGraph();

    // THEN (default state)
    QVERIFY(graph.edges().isEmpty());

    // WHEN
    graph.addEdge(edge1);

    // THEN
    QCOMPARE(graph.edges().size(), 1);
    QCOMPARE(graph.edges().at(0), edge1);

    // WHEN
    graph.addEdge(edge2);

    // THEN
    QCOMPARE(graph.edges().size(), 2);
    QCOMPARE(graph.edges().at(0), edge1);
    QCOMPARE(graph.edges().at(1), edge2);


    // WHEN
    graph.removeEdge(edge2);

    // THEN
    QCOMPARE(graph.edges().size(), 1);
    QCOMPARE(graph.edges().at(0), edge1);

    // WHEN
    graph.addEdge(edge2);

    // THEN
    QCOMPARE(graph.edges().size(), 2);
    QCOMPARE(graph.edges().at(0), edge1);
    QCOMPARE(graph.edges().at(1), edge2);

    // WHEN
    graph.addEdge(edge1);

    // THEN
    QCOMPARE(graph.edges().size(), 2);
    QCOMPARE(graph.edges().at(0), edge1);
    QCOMPARE(graph.edges().at(1), edge2);
}

QTEST_MAIN(tst_QShaderGraph)

#include "tst_qshadergraph.moc"
