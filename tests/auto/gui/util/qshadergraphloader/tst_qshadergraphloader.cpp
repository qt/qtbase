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

#include <QtCore/qbuffer.h>

#include <QtGui/private/qshadergraphloader_p.h>
#include <QtGui/private/qshaderlanguage_p.h>

using QBufferPointer = QSharedPointer<QBuffer>;
Q_DECLARE_METATYPE(QBufferPointer);

using PrototypeHash = QHash<QString, QShaderNode>;
Q_DECLARE_METATYPE(PrototypeHash);

namespace
{
    QBufferPointer createBuffer(const QByteArray &data, QIODevice::OpenMode openMode = QIODevice::ReadOnly)
    {
        auto buffer = QBufferPointer::create();
        buffer->setData(data);
        if (openMode != QIODevice::NotOpen)
            buffer->open(openMode);
        return buffer;
    }

    QShaderFormat createFormat(QShaderFormat::Api api, int majorVersion, int minorVersion)
    {
        auto format = QShaderFormat();
        format.setApi(api);
        format.setVersion(QVersionNumber(majorVersion, minorVersion));
        return format;
    }

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

    QShaderGraph createGraph()
    {
        const auto openGLES2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
        const auto openGL3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0);

        auto graph = QShaderGraph();

        auto worldPosition = createNode({
            createPort(QShaderNodePort::Output, "value")
        });
        worldPosition.setUuid(QUuid("{00000000-0000-0000-0000-000000000001}"));
        worldPosition.setParameter("name", "worldPosition");
        worldPosition.setParameter("qualifier", QVariant::fromValue<QShaderLanguage::StorageQualifier>(QShaderLanguage::Input));
        worldPosition.setParameter("type", QVariant::fromValue<QShaderLanguage::VariableType>(QShaderLanguage::Vec3));
        worldPosition.addRule(openGLES2, QShaderNode::Rule("highp $type $value = $name;",
                                                           QByteArrayList() << "$qualifier highp $type $name;"));
        worldPosition.addRule(openGL3, QShaderNode::Rule("$type $value = $name;",
                                                         QByteArrayList() << "$qualifier $type $name;"));

        auto texture = createNode({
            createPort(QShaderNodePort::Output, "texture")
        });
        texture.setUuid(QUuid("{00000000-0000-0000-0000-000000000002}"));
        texture.addRule(openGLES2, QShaderNode::Rule("sampler2D $texture = texture;",
                                                     QByteArrayList() << "uniform sampler2D texture;"));
        texture.addRule(openGL3, QShaderNode::Rule("sampler2D $texture = texture;",
                                                   QByteArrayList() << "uniform sampler2D texture;"));

        auto texCoord = createNode({
            createPort(QShaderNodePort::Output, "texCoord")
        });
        texCoord.setUuid(QUuid("{00000000-0000-0000-0000-000000000003}"));
        texCoord.addRule(openGLES2, QShaderNode::Rule("highp vec2 $texCoord = texCoord;",
                                                      QByteArrayList() << "varying highp vec2 texCoord;"));
        texCoord.addRule(openGL3, QShaderNode::Rule("vec2 $texCoord = texCoord;",
                                                    QByteArrayList() << "in vec2 texCoord;"));

        auto lightIntensity = createNode({
            createPort(QShaderNodePort::Output, "value")
        });
        lightIntensity.setUuid(QUuid("{00000000-0000-0000-0000-000000000004}"));
        lightIntensity.setParameter("name", "defaultName");
        lightIntensity.setParameter("qualifier", QVariant::fromValue<QShaderLanguage::StorageQualifier>(QShaderLanguage::Uniform));
        lightIntensity.setParameter("type", QVariant::fromValue<QShaderLanguage::VariableType>(QShaderLanguage::Float));
        lightIntensity.addRule(openGLES2, QShaderNode::Rule("highp $type $value = $name;",
                                                            QByteArrayList() << "$qualifier highp $type $name;"));
        lightIntensity.addRule(openGL3, QShaderNode::Rule("$type $value = $name;",
                                                          QByteArrayList() << "$qualifier $type $name;"));

        auto exposure = createNode({
            createPort(QShaderNodePort::Output, "exposure")
        });
        exposure.setUuid(QUuid("{00000000-0000-0000-0000-000000000005}"));
        exposure.addRule(openGLES2, QShaderNode::Rule("highp float $exposure = exposure;",
                                                      QByteArrayList() << "uniform highp float exposure;"));
        exposure.addRule(openGL3, QShaderNode::Rule("float $exposure = exposure;",
                                                    QByteArrayList() << "uniform float exposure;"));

        auto fragColor = createNode({
            createPort(QShaderNodePort::Input, "fragColor")
        });
        fragColor.setUuid(QUuid("{00000000-0000-0000-0000-000000000006}"));
        fragColor.addRule(openGLES2, QShaderNode::Rule("gl_fragColor = $fragColor;"));
        fragColor.addRule(openGL3, QShaderNode::Rule("fragColor = $fragColor;",
                                                     QByteArrayList() << "out vec4 fragColor;"));

        auto sampleTexture = createNode({
            createPort(QShaderNodePort::Input, "sampler"),
            createPort(QShaderNodePort::Input, "coord"),
            createPort(QShaderNodePort::Output, "color")
        });
        sampleTexture.setUuid(QUuid("{00000000-0000-0000-0000-000000000007}"));
        sampleTexture.addRule(openGLES2, QShaderNode::Rule("highp vec4 $color = texture2D($sampler, $coord);"));
        sampleTexture.addRule(openGL3, QShaderNode::Rule("vec4 $color = texture2D($sampler, $coord);"));

        auto lightFunction = createNode({
            createPort(QShaderNodePort::Input, "baseColor"),
            createPort(QShaderNodePort::Input, "position"),
            createPort(QShaderNodePort::Input, "lightIntensity"),
            createPort(QShaderNodePort::Output, "outputColor")
        });
        lightFunction.setUuid(QUuid("{00000000-0000-0000-0000-000000000008}"));
        lightFunction.addRule(openGLES2, QShaderNode::Rule("highp vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                           QByteArrayList() << "#pragma include es2/lightmodel.frag.inc"));
        lightFunction.addRule(openGL3, QShaderNode::Rule("vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                         QByteArrayList() << "#pragma include gl3/lightmodel.frag.inc"));

        auto exposureFunction = createNode({
            createPort(QShaderNodePort::Input, "inputColor"),
            createPort(QShaderNodePort::Input, "exposure"),
            createPort(QShaderNodePort::Output, "outputColor")
        });
        exposureFunction.setUuid(QUuid("{00000000-0000-0000-0000-000000000009}"));
        exposureFunction.addRule(openGLES2, QShaderNode::Rule("highp vec4 $outputColor = $inputColor * pow(2.0, $exposure);"));
        exposureFunction.addRule(openGL3, QShaderNode::Rule("vec4 $outputColor = $inputColor * pow(2.0, $exposure);"));

        graph.addNode(worldPosition);
        graph.addNode(texture);
        graph.addNode(texCoord);
        graph.addNode(lightIntensity);
        graph.addNode(exposure);
        graph.addNode(fragColor);
        graph.addNode(sampleTexture);
        graph.addNode(lightFunction);
        graph.addNode(exposureFunction);

        graph.addEdge(createEdge(texture.uuid(), "texture", sampleTexture.uuid(), "sampler"));
        graph.addEdge(createEdge(texCoord.uuid(), "texCoord", sampleTexture.uuid(), "coord"));

        graph.addEdge(createEdge(worldPosition.uuid(), "value", lightFunction.uuid(), "position"));
        graph.addEdge(createEdge(sampleTexture.uuid(), "color", lightFunction.uuid(), "baseColor"));
        graph.addEdge(createEdge(lightIntensity.uuid(), "value", lightFunction.uuid(), "lightIntensity"));

        graph.addEdge(createEdge(lightFunction.uuid(), "outputColor", exposureFunction.uuid(), "inputColor"));
        graph.addEdge(createEdge(exposure.uuid(), "exposure", exposureFunction.uuid(), "exposure"));

        graph.addEdge(createEdge(exposureFunction.uuid(), "outputColor", fragColor.uuid(), "fragColor"));

        return graph;
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

class tst_QShaderGraphLoader : public QObject
{
    Q_OBJECT
private slots:
    void shouldManipulateLoaderMembers();
    void shouldLoadFromJsonStream_data();
    void shouldLoadFromJsonStream();
};

void tst_QShaderGraphLoader::shouldManipulateLoaderMembers()
{
    // GIVEN
    auto loader = QShaderGraphLoader();

    // THEN (default state)
    QCOMPARE(loader.status(), QShaderGraphLoader::Null);
    QVERIFY(!loader.device());
    QVERIFY(loader.graph().nodes().isEmpty());
    QVERIFY(loader.graph().edges().isEmpty());
    QVERIFY(loader.prototypes().isEmpty());

    // WHEN
    auto device1 = createBuffer(QByteArray("..........."), QIODevice::NotOpen);
    loader.setDevice(device1.data());

    // THEN
    QCOMPARE(loader.status(), QShaderGraphLoader::Error);
    QCOMPARE(loader.device(), device1.data());
    QVERIFY(loader.graph().nodes().isEmpty());
    QVERIFY(loader.graph().edges().isEmpty());

    // WHEN
    auto device2 = createBuffer(QByteArray("..........."), QIODevice::ReadOnly);
    loader.setDevice(device2.data());

    // THEN
    QCOMPARE(loader.status(), QShaderGraphLoader::Waiting);
    QCOMPARE(loader.device(), device2.data());
    QVERIFY(loader.graph().nodes().isEmpty());
    QVERIFY(loader.graph().edges().isEmpty());


    // WHEN
    const auto prototypes = [this]{
        auto res = QHash<QString, QShaderNode>();
        res.insert("foo", createNode({}));
        return res;
    }();
    loader.setPrototypes(prototypes);

    // THEN
    QCOMPARE(loader.prototypes().size(), prototypes.size());
    QVERIFY(loader.prototypes().contains("foo"));
    QCOMPARE(loader.prototypes().value("foo").uuid(), prototypes.value("foo").uuid());
}

void tst_QShaderGraphLoader::shouldLoadFromJsonStream_data()
{
    QTest::addColumn<QBufferPointer>("device");
    QTest::addColumn<PrototypeHash>("prototypes");
    QTest::addColumn<QShaderGraph>("graph");
    QTest::addColumn<QShaderGraphLoader::Status>("status");

    QTest::newRow("empty") << createBuffer("", QIODevice::ReadOnly) << PrototypeHash()
                           << QShaderGraph() << QShaderGraphLoader::Error;

    const auto smallJson = "{"
                           "    \"nodes\": ["
                           "        {"
                           "            \"uuid\": \"{00000000-0000-0000-0000-000000000001}\","
                           "            \"type\": \"MyInput\","
                           "            \"layers\": [\"foo\", \"bar\"]"
                           "        },"
                           "        {"
                           "            \"uuid\": \"{00000000-0000-0000-0000-000000000002}\","
                           "            \"type\": \"MyOutput\""
                           "        },"
                           "        {"
                           "            \"uuid\": \"{00000000-0000-0000-0000-000000000003}\","
                           "            \"type\": \"MyFunction\""
                           "        }"
                           "    ],"
                           "    \"edges\": ["
                           "        {"
                           "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000001}\","
                           "            \"sourcePort\": \"input\","
                           "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000003}\","
                           "            \"targetPort\": \"functionInput\","
                           "            \"layers\": [\"bar\", \"baz\"]"
                           "        },"
                           "        {"
                           "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000003}\","
                           "            \"sourcePort\": \"functionOutput\","
                           "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000002}\","
                           "            \"targetPort\": \"output\""
                           "        }"
                           "    ]"
                           "}";

    const auto smallProtos = [this]{
        auto protos = PrototypeHash();

        auto input = createNode({
            createPort(QShaderNodePort::Output, "input")
        });
        protos.insert("MyInput", input);

        auto output = createNode({
            createPort(QShaderNodePort::Input, "output")
        });
        protos.insert("MyOutput", output);

        auto function = createNode({
            createPort(QShaderNodePort::Input, "functionInput"),
            createPort(QShaderNodePort::Output, "functionOutput")
        });
        protos.insert("MyFunction", function);
        return protos;
    }();

    const auto smallGraph = [this]{
        auto graph = QShaderGraph();

        auto input = createNode({
            createPort(QShaderNodePort::Output, "input")
        }, {"foo", "bar"});
        input.setUuid(QUuid("{00000000-0000-0000-0000-000000000001}"));
        auto output = createNode({
            createPort(QShaderNodePort::Input, "output")
        });
        output.setUuid(QUuid("{00000000-0000-0000-0000-000000000002}"));
        auto function = createNode({
            createPort(QShaderNodePort::Input, "functionInput"),
            createPort(QShaderNodePort::Output, "functionOutput")
        });
        function.setUuid(QUuid("{00000000-0000-0000-0000-000000000003}"));

        graph.addNode(input);
        graph.addNode(output);
        graph.addNode(function);
        graph.addEdge(createEdge(input.uuid(), "input", function.uuid(), "functionInput", {"bar", "baz"}));
        graph.addEdge(createEdge(function.uuid(), "functionOutput", output.uuid(), "output"));

        return graph;
    }();

    QTest::newRow("TwoNodesOneEdge") << createBuffer(smallJson) << smallProtos << smallGraph << QShaderGraphLoader::Ready;
    QTest::newRow("NotOpen") << createBuffer(smallJson, QIODevice::NotOpen) << smallProtos << QShaderGraph() << QShaderGraphLoader::Error;
    QTest::newRow("NoPrototype") << createBuffer(smallJson) << PrototypeHash() << QShaderGraph() << QShaderGraphLoader::Error;

    const auto complexJson = "{"
                             "    \"nodes\": ["
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000001}\","
                             "            \"type\": \"inputValue\","
                             "            \"parameters\": {"
                             "                \"name\": \"worldPosition\","
                             "                \"qualifier\": {"
                             "                    \"type\": \"QShaderLanguage::StorageQualifier\","
                             "                    \"value\": \"QShaderLanguage::Input\""
                             "                },"
                             "                \"type\": {"
                             "                    \"type\": \"QShaderLanguage::VariableType\","
                             "                    \"value\": \"QShaderLanguage::Vec3\""
                             "                }"
                             "            }"
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000002}\","
                             "            \"type\": \"texture\""
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000003}\","
                             "            \"type\": \"texCoord\""
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000004}\","
                             "            \"type\": \"inputValue\""
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000005}\","
                             "            \"type\": \"exposure\""
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000006}\","
                             "            \"type\": \"fragColor\""
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000007}\","
                             "            \"type\": \"sampleTexture\""
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000008}\","
                             "            \"type\": \"lightModel\""
                             "        },"
                             "        {"
                             "            \"uuid\": \"{00000000-0000-0000-0000-000000000009}\","
                             "            \"type\": \"exposureFunction\""
                             "        }"
                             "    ],"
                             "    \"edges\": ["
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000002}\","
                             "            \"sourcePort\": \"texture\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000007}\","
                             "            \"targetPort\": \"sampler\""
                             "        },"
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000003}\","
                             "            \"sourcePort\": \"texCoord\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000007}\","
                             "            \"targetPort\": \"coord\""
                             "        },"
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000001}\","
                             "            \"sourcePort\": \"value\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000008}\","
                             "            \"targetPort\": \"position\""
                             "        },"
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000007}\","
                             "            \"sourcePort\": \"color\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000008}\","
                             "            \"targetPort\": \"baseColor\""
                             "        },"
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000004}\","
                             "            \"sourcePort\": \"value\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000008}\","
                             "            \"targetPort\": \"lightIntensity\""
                             "        },"
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000008}\","
                             "            \"sourcePort\": \"outputColor\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000009}\","
                             "            \"targetPort\": \"inputColor\""
                             "        },"
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000005}\","
                             "            \"sourcePort\": \"exposure\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000009}\","
                             "            \"targetPort\": \"exposure\""
                             "        },"
                             "        {"
                             "            \"sourceUuid\": \"{00000000-0000-0000-0000-000000000009}\","
                             "            \"sourcePort\": \"outputColor\","
                             "            \"targetUuid\": \"{00000000-0000-0000-0000-000000000006}\","
                             "            \"targetPort\": \"fragColor\""
                             "        }"
                             "    ]"
                             "}";

    const auto complexProtos = [this]{
        const auto openGLES2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
        const auto openGL3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0);

        auto protos = PrototypeHash();

        auto inputValue = createNode({
            createPort(QShaderNodePort::Output, "value")
        });
        inputValue.setParameter("name", "defaultName");
        inputValue.setParameter("qualifier", QVariant::fromValue<QShaderLanguage::StorageQualifier>(QShaderLanguage::Uniform));
        inputValue.setParameter("type", QVariant::fromValue<QShaderLanguage::VariableType>(QShaderLanguage::Float));
        inputValue.addRule(openGLES2, QShaderNode::Rule("highp $type $value = $name;",
                                                        QByteArrayList() << "$qualifier highp $type $name;"));
        inputValue.addRule(openGL3, QShaderNode::Rule("$type $value = $name;",
                                                      QByteArrayList() << "$qualifier $type $name;"));
        protos.insert("inputValue", inputValue);

        auto texture = createNode({
            createPort(QShaderNodePort::Output, "texture")
        });
        texture.addRule(openGLES2, QShaderNode::Rule("sampler2D $texture = texture;",
                                                     QByteArrayList() << "uniform sampler2D texture;"));
        texture.addRule(openGL3, QShaderNode::Rule("sampler2D $texture = texture;",
                                                   QByteArrayList() << "uniform sampler2D texture;"));
        protos.insert("texture", texture);

        auto texCoord = createNode({
            createPort(QShaderNodePort::Output, "texCoord")
        });
        texCoord.addRule(openGLES2, QShaderNode::Rule("highp vec2 $texCoord = texCoord;",
                                                      QByteArrayList() << "varying highp vec2 texCoord;"));
        texCoord.addRule(openGL3, QShaderNode::Rule("vec2 $texCoord = texCoord;",
                                                    QByteArrayList() << "in vec2 texCoord;"));
        protos.insert("texCoord", texCoord);

        auto exposure = createNode({
            createPort(QShaderNodePort::Output, "exposure")
        });
        exposure.addRule(openGLES2, QShaderNode::Rule("highp float $exposure = exposure;",
                                                      QByteArrayList() << "uniform highp float exposure;"));
        exposure.addRule(openGL3, QShaderNode::Rule("float $exposure = exposure;",
                                                    QByteArrayList() << "uniform float exposure;"));
        protos.insert("exposure", exposure);

        auto fragColor = createNode({
            createPort(QShaderNodePort::Input, "fragColor")
        });
        fragColor.addRule(openGLES2, QShaderNode::Rule("gl_fragColor = $fragColor;"));
        fragColor.addRule(openGL3, QShaderNode::Rule("fragColor = $fragColor;",
                                                     QByteArrayList() << "out vec4 fragColor;"));
        protos.insert("fragColor", fragColor);

        auto sampleTexture = createNode({
            createPort(QShaderNodePort::Input, "sampler"),
            createPort(QShaderNodePort::Input, "coord"),
            createPort(QShaderNodePort::Output, "color")
        });
        sampleTexture.addRule(openGLES2, QShaderNode::Rule("highp vec4 $color = texture2D($sampler, $coord);"));
        sampleTexture.addRule(openGL3, QShaderNode::Rule("vec4 $color = texture2D($sampler, $coord);"));
        protos.insert("sampleTexture", sampleTexture);

        auto lightModel = createNode({
            createPort(QShaderNodePort::Input, "baseColor"),
            createPort(QShaderNodePort::Input, "position"),
            createPort(QShaderNodePort::Input, "lightIntensity"),
            createPort(QShaderNodePort::Output, "outputColor")
        });
        lightModel.addRule(openGLES2, QShaderNode::Rule("highp vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                        QByteArrayList() << "#pragma include es2/lightmodel.frag.inc"));
        lightModel.addRule(openGL3, QShaderNode::Rule("vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                      QByteArrayList() << "#pragma include gl3/lightmodel.frag.inc"));
        protos.insert("lightModel", lightModel);

        auto exposureFunction = createNode({
            createPort(QShaderNodePort::Input, "inputColor"),
            createPort(QShaderNodePort::Input, "exposure"),
            createPort(QShaderNodePort::Output, "outputColor")
        });
        exposureFunction.addRule(openGLES2, QShaderNode::Rule("highp vec4 $outputColor = $inputColor * pow(2.0, $exposure);"));
        exposureFunction.addRule(openGL3, QShaderNode::Rule("vec4 $outputColor = $inputColor * pow(2.0, $exposure);"));
        protos.insert("exposureFunction", exposureFunction);

        return protos;
    }();

    const auto complexGraph = createGraph();

    QTest::newRow("ComplexGraph") << createBuffer(complexJson) << complexProtos << complexGraph << QShaderGraphLoader::Ready;
}

void tst_QShaderGraphLoader::shouldLoadFromJsonStream()
{
    // GIVEN
    QFETCH(QBufferPointer, device);
    QFETCH(PrototypeHash, prototypes);

    auto loader = QShaderGraphLoader();

    // WHEN
    loader.setPrototypes(prototypes);
    loader.setDevice(device.data());
    loader.load();

    // THEN
    QFETCH(QShaderGraphLoader::Status, status);
    QCOMPARE(loader.status(), status);

    QFETCH(QShaderGraph, graph);
    const auto statements = loader.graph().createStatements({"foo", "bar", "baz"});
    const auto expected = graph.createStatements({"foo", "bar", "baz"});
    dumpStatementsIfNeeded(statements, expected);
    QCOMPARE(statements, expected);

    const auto sortedParameters = [](const QShaderNode &node) {
        auto res = node.parameterNames();
        res.sort();
        return res;
    };

    for (int i = 0; i < statements.size(); i++) {
        const auto actualNode = statements.at(i).node;
        const auto expectedNode = expected.at(i).node;

        QCOMPARE(actualNode.layers(), expectedNode.layers());
        QCOMPARE(actualNode.ports(), expectedNode.ports());
        QCOMPARE(sortedParameters(actualNode), sortedParameters(expectedNode));
        for (const auto &name : expectedNode.parameterNames()) {
            QCOMPARE(actualNode.parameter(name), expectedNode.parameter(name));
        }
        QCOMPARE(actualNode.availableFormats(), expectedNode.availableFormats());
        for (const auto &format : expectedNode.availableFormats()) {
            QCOMPARE(actualNode.rule(format), expectedNode.rule(format));
        }
    }
}

QTEST_MAIN(tst_QShaderGraphLoader)

#include "tst_qshadergraphloader.moc"
