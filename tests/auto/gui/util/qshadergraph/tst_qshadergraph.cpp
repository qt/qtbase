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

    QShaderNode createNode(const QVector<QShaderNodePort> &ports, const QStringList &layers = QStringList())
    {
        auto node = QShaderNode();
        node.setUuid(QUuid::createUuid());
        node.setLayers(layers);
        for (const auto &port : ports)
            node.addPort(port);
        return node;
    }

    QShaderGraph::Edge createEdge(const QUuid &sourceUuid, const QString &sourceName,
                                  const QUuid &targetUuid, const QString &targetName,
                                  const QStringList &layers = QStringList())
    {
        auto edge = QShaderGraph::Edge();
        edge.sourceNodeUuid = sourceUuid;
        edge.sourcePortName = sourceName;
        edge.targetNodeUuid = targetUuid;
        edge.targetPortName = targetName;
        edge.layers = layers;
        return edge;
    }

    QShaderGraph::Statement createStatement(const QShaderNode &node,
                                            const QVector<int> &inputs = QVector<int>(),
                                            const QVector<int> &outputs = QVector<int>())
    {
        auto statement = QShaderGraph::Statement();
        statement.node = node;
        statement.inputs = inputs;
        statement.outputs = outputs;
        return statement;
    }

    void debugStatement(const QString &prefix, const QShaderGraph::Statement &statement)
    {
        qDebug() << prefix << statement.inputs << statement.uuid().toString() << statement.outputs;
    }

    void dumpStatementsIfNeeded(const QVector<QShaderGraph::Statement> &statements, const QVector<QShaderGraph::Statement> &expected)
    {
        if (statements != expected) {
            for (int i = 0; i < qMax(statements.size(), expected.size()); i++) {
                qDebug() << "----" << i << "----";
                if (i < statements.size())
                    debugStatement("A:", statements.at(i));
                if (i < expected.size())
                    debugStatement("E:", expected.at(i));
                qDebug() << "-----------";
            }
        }
    }
}

class tst_QShaderGraph : public QObject
{
    Q_OBJECT
private slots:
    void shouldHaveEdgeDefaultState();
    void shouldTestEdgesEquality_data();
    void shouldTestEdgesEquality();
    void shouldManipulateStatementMembers();
    void shouldTestStatementsEquality_data();
    void shouldTestStatementsEquality();
    void shouldFindIndexFromPortNameInStatements_data();
    void shouldFindIndexFromPortNameInStatements();
    void shouldManageNodeList();
    void shouldManageEdgeList();
    void shouldSerializeGraphForCodeGeneration();
    void shouldHandleUnboundPortsDuringGraphSerialization();
    void shouldSurviveCyclesDuringGraphSerialization();
    void shouldDealWithEdgesJumpingOverLayers();
    void shouldGenerateDifferentStatementsDependingOnActiveLayers();
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

void tst_QShaderGraph::shouldManipulateStatementMembers()
{
    // GIVEN
    auto statement = QShaderGraph::Statement();

    // THEN (default state)
    QVERIFY(statement.inputs.isEmpty());
    QVERIFY(statement.outputs.isEmpty());
    QVERIFY(statement.node.uuid().isNull());
    QVERIFY(statement.uuid().isNull());

    // WHEN
    const auto node = createNode({});
    statement.node = node;

    // THEN
    QCOMPARE(statement.uuid(), node.uuid());

    // WHEN
    statement.node = QShaderNode();

    // THEN
    QVERIFY(statement.uuid().isNull());
}

void tst_QShaderGraph::shouldTestStatementsEquality_data()
{
    QTest::addColumn<QShaderGraph::Statement>("left");
    QTest::addColumn<QShaderGraph::Statement>("right");
    QTest::addColumn<bool>("expected");

    const auto node1 = createNode({});
    const auto node2 = createNode({});

    QTest::newRow("EqualNodes") << createStatement(node1, {1, 2}, {3, 4})
                                << createStatement(node1, {1, 2}, {3, 4})
                                << true;
    QTest::newRow("EqualInvalids") << createStatement(QShaderNode(), {1, 2}, {3, 4})
                                   << createStatement(QShaderNode(), {1, 2}, {3, 4})
                                   << true;
    QTest::newRow("Nodes") << createStatement(node1, {1, 2}, {3, 4})
                           << createStatement(node2, {1, 2}, {3, 4})
                           << false;
    QTest::newRow("Inputs") << createStatement(node1, {1, 2}, {3, 4})
                            << createStatement(node1, {1, 2, 0}, {3, 4})
                            << false;
    QTest::newRow("Outputs") << createStatement(node1, {1, 2}, {3, 4})
                             << createStatement(node1, {1, 2}, {3, 0, 4})
                             << false;
}

void tst_QShaderGraph::shouldTestStatementsEquality()
{
    // GIVEN
    QFETCH(QShaderGraph::Statement, left);
    QFETCH(QShaderGraph::Statement, right);

    // WHEN
    const auto equal = (left == right);
    const auto notEqual = (left != right);

    // THEN
    QFETCH(bool, expected);
    QCOMPARE(equal, expected);
    QCOMPARE(notEqual, !expected);
}

void tst_QShaderGraph::shouldFindIndexFromPortNameInStatements_data()
{
    QTest::addColumn<QShaderGraph::Statement>("statement");
    QTest::addColumn<QString>("portName");
    QTest::addColumn<int>("expectedInputIndex");
    QTest::addColumn<int>("expectedOutputIndex");

    const auto inputNodeStatement = createStatement(createNode({
        createPort(QShaderNodePort::Output, "input")
    }));
    const auto outputNodeStatement = createStatement(createNode({
        createPort(QShaderNodePort::Input, "output")
    }));
    const auto functionNodeStatement = createStatement(createNode({
        createPort(QShaderNodePort::Input, "input1"),
        createPort(QShaderNodePort::Output, "output1"),
        createPort(QShaderNodePort::Input, "input2"),
        createPort(QShaderNodePort::Output, "output2"),
        createPort(QShaderNodePort::Output, "output3"),
        createPort(QShaderNodePort::Input, "input3")
    }));

    QTest::newRow("Invalid") << QShaderGraph::Statement() << "foo" << -1 << -1;
    QTest::newRow("InputNodeWrongName") << inputNodeStatement << "foo" << -1 << -1;
    QTest::newRow("InputNodeExistingName") << inputNodeStatement << "input" << -1 << 0;
    QTest::newRow("OutputNodeWrongName") << outputNodeStatement << "foo" << -1 << -1;
    QTest::newRow("OutputNodeExistingName") << outputNodeStatement << "output" << 0 << -1;
    QTest::newRow("FunctionNodeWrongName") << functionNodeStatement << "foo" << -1 << -1;
    QTest::newRow("FunctionNodeInput1") << functionNodeStatement << "input1" << 0 << -1;
    QTest::newRow("FunctionNodeOutput1") << functionNodeStatement << "output1" << -1 << 0;
    QTest::newRow("FunctionNodeInput2") << functionNodeStatement << "input2" << 1 << -1;
    QTest::newRow("FunctionNodeOutput2") << functionNodeStatement << "output2" << -1 << 1;
    QTest::newRow("FunctionNodeInput3") << functionNodeStatement << "input3" << 2 << -1;
    QTest::newRow("FunctionNodeOutput3") << functionNodeStatement << "output3" << -1 << 2;
}

void tst_QShaderGraph::shouldFindIndexFromPortNameInStatements()
{
    // GIVEN
    QFETCH(QShaderGraph::Statement, statement);
    QFETCH(QString, portName);
    QFETCH(int, expectedInputIndex);
    QFETCH(int, expectedOutputIndex);

    // WHEN
    const auto inputIndex = statement.portIndex(QShaderNodePort::Input, portName);
    const auto outputIndex = statement.portIndex(QShaderNodePort::Output, portName);

    // THEN
    QCOMPARE(inputIndex, expectedInputIndex);
    QCOMPARE(outputIndex, expectedOutputIndex);
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

void tst_QShaderGraph::shouldSerializeGraphForCodeGeneration()
{
    // GIVEN
    const auto input1 = createNode({
        createPort(QShaderNodePort::Output, "input1Value")
    });
    const auto input2 = createNode({
        createPort(QShaderNodePort::Output, "input2Value")
    });
    const auto output1 = createNode({
        createPort(QShaderNodePort::Input, "output1Value")
    });
    const auto output2 = createNode({
        createPort(QShaderNodePort::Input, "output2Value")
    });
    const auto function1 = createNode({
        createPort(QShaderNodePort::Input, "function1Input"),
        createPort(QShaderNodePort::Output, "function1Output")
    });
    const auto function2 = createNode({
        createPort(QShaderNodePort::Input, "function2Input1"),
        createPort(QShaderNodePort::Input, "function2Input2"),
        createPort(QShaderNodePort::Output, "function2Output")
    });
    const auto function3 = createNode({
        createPort(QShaderNodePort::Input, "function3Input1"),
        createPort(QShaderNodePort::Input, "function3Input2"),
        createPort(QShaderNodePort::Output, "function3Output1"),
        createPort(QShaderNodePort::Output, "function3Output2")
    });

    const auto graph = [=] {
        auto res = QShaderGraph();
        res.addNode(input1);
        res.addNode(input2);
        res.addNode(output1);
        res.addNode(output2);
        res.addNode(function1);
        res.addNode(function2);
        res.addNode(function3);
        res.addEdge(createEdge(input1.uuid(), "input1Value", function1.uuid(), "function1Input"));
        res.addEdge(createEdge(input1.uuid(), "input1Value", function2.uuid(), "function2Input1"));
        res.addEdge(createEdge(input2.uuid(), "input2Value", function2.uuid(), "function2Input2"));
        res.addEdge(createEdge(function1.uuid(), "function1Output", function3.uuid(), "function3Input1"));
        res.addEdge(createEdge(function2.uuid(), "function2Output", function3.uuid(), "function3Input2"));
        res.addEdge(createEdge(function3.uuid(), "function3Output1", output1.uuid(), "output1Value"));
        res.addEdge(createEdge(function3.uuid(), "function3Output2", output2.uuid(), "output2Value"));
        return res;
    }();

    // WHEN
    const auto statements = graph.createStatements();

    // THEN
    const auto expected = QVector<QShaderGraph::Statement>()
            << createStatement(input2, {}, {1})
            << createStatement(input1, {}, {0})
            << createStatement(function2, {0, 1}, {3})
            << createStatement(function1, {0}, {2})
            << createStatement(function3, {2, 3}, {4, 5})
            << createStatement(output2, {5}, {})
            << createStatement(output1, {4}, {});
    dumpStatementsIfNeeded(statements, expected);
    QCOMPARE(statements, expected);
}

void tst_QShaderGraph::shouldHandleUnboundPortsDuringGraphSerialization()
{
    // GIVEN
    const auto input = createNode({
        createPort(QShaderNodePort::Output, "input")
    });
    const auto unboundInput = createNode({
        createPort(QShaderNodePort::Output, "unbound")
    });
    const auto output = createNode({
        createPort(QShaderNodePort::Input, "output")
    });
    const auto unboundOutput = createNode({
        createPort(QShaderNodePort::Input, "unbound")
    });
    const auto function = createNode({
        createPort(QShaderNodePort::Input, "functionInput1"),
        createPort(QShaderNodePort::Input, "functionInput2"),
        createPort(QShaderNodePort::Input, "functionInput3"),
        createPort(QShaderNodePort::Output, "functionOutput1"),
        createPort(QShaderNodePort::Output, "functionOutput2"),
        createPort(QShaderNodePort::Output, "functionOutput3")
    });

    const auto graph = [=] {
        auto res = QShaderGraph();
        res.addNode(input);
        res.addNode(unboundInput);
        res.addNode(output);
        res.addNode(unboundOutput);
        res.addNode(function);
        res.addEdge(createEdge(input.uuid(), "input", function.uuid(), "functionInput2"));
        res.addEdge(createEdge(function.uuid(), "functionOutput2", output.uuid(), "output"));
        return res;
    }();

    // WHEN
    const auto statements = graph.createStatements();

    // THEN
    // Note that no edge leads to the unbound input
    const auto expected = QVector<QShaderGraph::Statement>()
            << createStatement(input, {}, {0})
            << createStatement(function, {-1, 0, -1}, {2, 3, 4})
            << createStatement(unboundOutput, {-1}, {})
            << createStatement(output, {3}, {});
    dumpStatementsIfNeeded(statements, expected);
    QCOMPARE(statements, expected);
}

void tst_QShaderGraph::shouldSurviveCyclesDuringGraphSerialization()
{
    // GIVEN
    const auto input = createNode({
        createPort(QShaderNodePort::Output, "input")
    });
    const auto output = createNode({
        createPort(QShaderNodePort::Input, "output")
    });
    const auto function1 = createNode({
        createPort(QShaderNodePort::Input, "function1Input1"),
        createPort(QShaderNodePort::Input, "function1Input2"),
        createPort(QShaderNodePort::Output, "function1Output")
    });
    const auto function2 = createNode({
        createPort(QShaderNodePort::Input, "function2Input"),
        createPort(QShaderNodePort::Output, "function2Output")
    });
    const auto function3 = createNode({
        createPort(QShaderNodePort::Input, "function3Input"),
        createPort(QShaderNodePort::Output, "function3Output")
    });

    const auto graph = [=] {
        auto res = QShaderGraph();
        res.addNode(input);
        res.addNode(output);
        res.addNode(function1);
        res.addNode(function2);
        res.addNode(function3);
        res.addEdge(createEdge(input.uuid(), "input", function1.uuid(), "function1Input1"));
        res.addEdge(createEdge(function1.uuid(), "function1Output", function2.uuid(), "function2Input"));
        res.addEdge(createEdge(function2.uuid(), "function2Output", function3.uuid(), "function3Input"));
        res.addEdge(createEdge(function3.uuid(), "function3Output", function1.uuid(), "function1Input2"));
        res.addEdge(createEdge(function2.uuid(), "function2Output", output.uuid(), "output"));
        return res;
    }();

    // WHEN
    const auto statements = graph.createStatements();

    // THEN
    // Obviously will lead to a compile failure later on since it cuts everything beyond the cycle
    const auto expected = QVector<QShaderGraph::Statement>()
            << createStatement(output, {2}, {});
    dumpStatementsIfNeeded(statements, expected);
    QCOMPARE(statements, expected);
}

void tst_QShaderGraph::shouldDealWithEdgesJumpingOverLayers()
{
    // GIVEN
    const auto worldPosition = createNode({
        createPort(QShaderNodePort::Output, "worldPosition")
    });
    const auto texture = createNode({
        createPort(QShaderNodePort::Output, "texture")
    });
    const auto texCoord = createNode({
        createPort(QShaderNodePort::Output, "texCoord")
    });
    const auto lightIntensity = createNode({
        createPort(QShaderNodePort::Output, "lightIntensity")
    });
    const auto exposure = createNode({
        createPort(QShaderNodePort::Output, "exposure")
    });
    const auto fragColor = createNode({
        createPort(QShaderNodePort::Input, "fragColor")
    });
    const auto sampleTexture = createNode({
        createPort(QShaderNodePort::Input, "sampler"),
        createPort(QShaderNodePort::Input, "coord"),
        createPort(QShaderNodePort::Output, "color")
    });
    const auto lightFunction = createNode({
        createPort(QShaderNodePort::Input, "baseColor"),
        createPort(QShaderNodePort::Input, "position"),
        createPort(QShaderNodePort::Input, "lightIntensity"),
        createPort(QShaderNodePort::Output, "outputColor")
    });
    const auto exposureFunction = createNode({
        createPort(QShaderNodePort::Input, "inputColor"),
        createPort(QShaderNodePort::Input, "exposure"),
        createPort(QShaderNodePort::Output, "outputColor")
    });

    const auto graph = [=] {
        auto res = QShaderGraph();

        res.addNode(worldPosition);
        res.addNode(texture);
        res.addNode(texCoord);
        res.addNode(lightIntensity);
        res.addNode(exposure);
        res.addNode(fragColor);
        res.addNode(sampleTexture);
        res.addNode(lightFunction);
        res.addNode(exposureFunction);

        res.addEdge(createEdge(texture.uuid(), "texture", sampleTexture.uuid(), "sampler"));
        res.addEdge(createEdge(texCoord.uuid(), "texCoord", sampleTexture.uuid(), "coord"));

        res.addEdge(createEdge(worldPosition.uuid(), "worldPosition", lightFunction.uuid(), "position"));
        res.addEdge(createEdge(sampleTexture.uuid(), "color", lightFunction.uuid(), "baseColor"));
        res.addEdge(createEdge(lightIntensity.uuid(), "lightIntensity", lightFunction.uuid(), "lightIntensity"));

        res.addEdge(createEdge(lightFunction.uuid(), "outputColor", exposureFunction.uuid(), "inputColor"));
        res.addEdge(createEdge(exposure.uuid(), "exposure", exposureFunction.uuid(), "exposure"));

        res.addEdge(createEdge(exposureFunction.uuid(), "outputColor", fragColor.uuid(), "fragColor"));

        return res;
    }();

    // WHEN
    const auto statements = graph.createStatements();

    // THEN
    const auto expected = QVector<QShaderGraph::Statement>()
            << createStatement(texCoord, {}, {2})
            << createStatement(texture, {}, {1})
            << createStatement(lightIntensity, {}, {3})
            << createStatement(sampleTexture, {1, 2}, {5})
            << createStatement(worldPosition, {}, {0})
            << createStatement(exposure, {}, {4})
            << createStatement(lightFunction, {5, 0, 3}, {6})
            << createStatement(exposureFunction, {6, 4}, {7})
            << createStatement(fragColor, {7}, {});
    dumpStatementsIfNeeded(statements, expected);
    QCOMPARE(statements, expected);
}

void tst_QShaderGraph::shouldGenerateDifferentStatementsDependingOnActiveLayers()
{
    // GIVEN
    const auto texCoord = createNode({
        createPort(QShaderNodePort::Output, "texCoord")
    }, {
        "diffuseTexture",
        "normalTexture"
    });
    const auto diffuseUniform = createNode({
        createPort(QShaderNodePort::Output, "color")
    }, {"diffuseUniform"});
    const auto diffuseTexture = createNode({
        createPort(QShaderNodePort::Input, "coord"),
        createPort(QShaderNodePort::Output, "color")
    }, {"diffuseTexture"});
    const auto normalUniform = createNode({
        createPort(QShaderNodePort::Output, "normal")
    }, {"normalUniform"});
    const auto normalTexture = createNode({
        createPort(QShaderNodePort::Input, "coord"),
        createPort(QShaderNodePort::Output, "normal")
    }, {"normalTexture"});
    const auto lightFunction = createNode({
        createPort(QShaderNodePort::Input, "color"),
        createPort(QShaderNodePort::Input, "normal"),
        createPort(QShaderNodePort::Output, "output")
    });
    const auto fragColor = createNode({
        createPort(QShaderNodePort::Input, "fragColor")
    });

    const auto graph = [=] {
        auto res = QShaderGraph();

        res.addNode(texCoord);
        res.addNode(diffuseUniform);
        res.addNode(diffuseTexture);
        res.addNode(normalUniform);
        res.addNode(normalTexture);
        res.addNode(lightFunction);
        res.addNode(fragColor);

        res.addEdge(createEdge(diffuseUniform.uuid(), "color", lightFunction.uuid(), "color", {"diffuseUniform"}));
        res.addEdge(createEdge(texCoord.uuid(), "texCoord", diffuseTexture.uuid(), "coord", {"diffuseTexture"}));
        res.addEdge(createEdge(diffuseTexture.uuid(), "color", lightFunction.uuid(), "color", {"diffuseTexture"}));

        res.addEdge(createEdge(normalUniform.uuid(), "normal", lightFunction.uuid(), "normal", {"normalUniform"}));
        res.addEdge(createEdge(texCoord.uuid(), "texCoord", normalTexture.uuid(), "coord", {"normalTexture"}));
        res.addEdge(createEdge(normalTexture.uuid(), "normal", lightFunction.uuid(), "normal", {"normalTexture"}));

        res.addEdge(createEdge(lightFunction.uuid(), "output", fragColor.uuid(), "fragColor"));

        return res;
    }();

    {
        // WHEN
        const auto statements = graph.createStatements({"diffuseUniform", "normalUniform"});

        // THEN
        const auto expected = QVector<QShaderGraph::Statement>()
                << createStatement(normalUniform, {}, {1})
                << createStatement(diffuseUniform, {}, {0})
                << createStatement(lightFunction, {0, 1}, {2})
                << createStatement(fragColor, {2}, {});
        dumpStatementsIfNeeded(statements, expected);
        QCOMPARE(statements, expected);
    }

    {
        // WHEN
        const auto statements = graph.createStatements({"diffuseUniform", "normalTexture"});

        // THEN
        const auto expected = QVector<QShaderGraph::Statement>()
                << createStatement(texCoord, {}, {0})
                << createStatement(normalTexture, {0}, {2})
                << createStatement(diffuseUniform, {}, {1})
                << createStatement(lightFunction, {1, 2}, {3})
                << createStatement(fragColor, {3}, {});
        dumpStatementsIfNeeded(statements, expected);
        QCOMPARE(statements, expected);
    }

    {
        // WHEN
        const auto statements = graph.createStatements({"diffuseTexture", "normalUniform"});

        // THEN
        const auto expected = QVector<QShaderGraph::Statement>()
                << createStatement(texCoord, {}, {0})
                << createStatement(normalUniform, {}, {2})
                << createStatement(diffuseTexture, {0}, {1})
                << createStatement(lightFunction, {1, 2}, {3})
                << createStatement(fragColor, {3}, {});
        dumpStatementsIfNeeded(statements, expected);
        QCOMPARE(statements, expected);
    }

    {
        // WHEN
        const auto statements = graph.createStatements({"diffuseTexture", "normalTexture"});

        // THEN
        const auto expected = QVector<QShaderGraph::Statement>()
                << createStatement(texCoord, {}, {0})
                << createStatement(normalTexture, {0}, {2})
                << createStatement(diffuseTexture, {0}, {1})
                << createStatement(lightFunction, {1, 2}, {3})
                << createStatement(fragColor, {3}, {});
        dumpStatementsIfNeeded(statements, expected);
        QCOMPARE(statements, expected);
    }
}

QTEST_MAIN(tst_QShaderGraph)

#include "tst_qshadergraph.moc"
