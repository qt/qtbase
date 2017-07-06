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

#include <QtGui/private/qshadergenerator_p.h>

namespace
{
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

    QShaderGraph createGraph()
    {
        const auto openGLES2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
        const auto openGL3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0);

        auto graph = QShaderGraph();

        auto worldPosition = createNode({
            createPort(QShaderNodePort::Output, "value")
        });
        worldPosition.setParameter("name", "worldPosition");
        worldPosition.addRule(openGLES2, QShaderNode::Rule("highp vec3 $value = $name;",
                                                           QByteArrayList() << "varying highp vec3 $name;"));
        worldPosition.addRule(openGL3, QShaderNode::Rule("vec3 $value = $name;",
                                                         QByteArrayList() << "in vec3 $name;"));

        auto texture = createNode({
            createPort(QShaderNodePort::Output, "texture")
        });
        texture.addRule(openGLES2, QShaderNode::Rule("sampler2D $texture = texture;",
                                                     QByteArrayList() << "uniform sampler2D texture;"));
        texture.addRule(openGL3, QShaderNode::Rule("sampler2D $texture = texture;",
                                                   QByteArrayList() << "uniform sampler2D texture;"));

        auto texCoord = createNode({
            createPort(QShaderNodePort::Output, "texCoord")
        });
        texCoord.addRule(openGLES2, QShaderNode::Rule("highp vec2 $texCoord = texCoord;",
                                                      QByteArrayList() << "varying highp vec2 texCoord;"));
        texCoord.addRule(openGL3, QShaderNode::Rule("vec2 $texCoord = texCoord;",
                                                    QByteArrayList() << "in vec2 texCoord;"));

        auto lightIntensity = createNode({
            createPort(QShaderNodePort::Output, "lightIntensity")
        });
        lightIntensity.addRule(openGLES2, QShaderNode::Rule("highp float $lightIntensity = lightIntensity;",
                                                            QByteArrayList() << "uniform highp float lightIntensity;"));
        lightIntensity.addRule(openGL3, QShaderNode::Rule("float $lightIntensity = lightIntensity;",
                                                          QByteArrayList() << "uniform float lightIntensity;"));

        auto exposure = createNode({
            createPort(QShaderNodePort::Output, "exposure")
        });
        exposure.addRule(openGLES2, QShaderNode::Rule("highp float $exposure = exposure;",
                                                      QByteArrayList() << "uniform highp float exposure;"));
        exposure.addRule(openGL3, QShaderNode::Rule("float $exposure = exposure;",
                                                    QByteArrayList() << "uniform float exposure;"));

        auto fragColor = createNode({
            createPort(QShaderNodePort::Input, "fragColor")
        });
        fragColor.addRule(openGLES2, QShaderNode::Rule("gl_fragColor = $fragColor;"));
        fragColor.addRule(openGL3, QShaderNode::Rule("fragColor = $fragColor;",
                                                     QByteArrayList() << "out vec4 fragColor;"));

        auto sampleTexture = createNode({
            createPort(QShaderNodePort::Input, "sampler"),
            createPort(QShaderNodePort::Input, "coord"),
            createPort(QShaderNodePort::Output, "color")
        });
        sampleTexture.addRule(openGLES2, QShaderNode::Rule("highp vec4 $color = texture2D($sampler, $coord);"));
        sampleTexture.addRule(openGL3, QShaderNode::Rule("vec4 $color = texture2D($sampler, $coord);"));

        auto lightFunction = createNode({
            createPort(QShaderNodePort::Input, "baseColor"),
            createPort(QShaderNodePort::Input, "position"),
            createPort(QShaderNodePort::Input, "lightIntensity"),
            createPort(QShaderNodePort::Output, "outputColor")
        });
        lightFunction.addRule(openGLES2, QShaderNode::Rule("highp vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                           QByteArrayList() << "#pragma include es2/lightmodel.frag.inc"));
        lightFunction.addRule(openGL3, QShaderNode::Rule("vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                         QByteArrayList() << "#pragma include gl3/lightmodel.frag.inc"));

        auto exposureFunction = createNode({
            createPort(QShaderNodePort::Input, "inputColor"),
            createPort(QShaderNodePort::Input, "exposure"),
            createPort(QShaderNodePort::Output, "outputColor")
        });
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
        graph.addEdge(createEdge(lightIntensity.uuid(), "lightIntensity", lightFunction.uuid(), "lightIntensity"));

        graph.addEdge(createEdge(lightFunction.uuid(), "outputColor", exposureFunction.uuid(), "inputColor"));
        graph.addEdge(createEdge(exposure.uuid(), "exposure", exposureFunction.uuid(), "exposure"));

        graph.addEdge(createEdge(exposureFunction.uuid(), "outputColor", fragColor.uuid(), "fragColor"));

        return graph;
    }
}

class tst_QShaderGenerator : public QObject
{
    Q_OBJECT
private slots:
    void shouldHaveDefaultState();
    void shouldGenerateShaderCode_data();
    void shouldGenerateShaderCode();
    void shouldGenerateVersionCommands_data();
    void shouldGenerateVersionCommands();
};

void tst_QShaderGenerator::shouldHaveDefaultState()
{
    // GIVEN
    auto generator = QShaderGenerator();

    // THEN
    QVERIFY(generator.graph.nodes().isEmpty());
    QVERIFY(generator.graph.edges().isEmpty());
    QVERIFY(!generator.format.isValid());
}

void tst_QShaderGenerator::shouldGenerateShaderCode_data()
{
    QTest::addColumn<QShaderGraph>("graph");
    QTest::addColumn<QShaderFormat>("format");
    QTest::addColumn<QByteArray>("expectedCode");

    const auto graph = createGraph();

    const auto openGLES2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
    const auto openGL3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0);
    const auto openGL32 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 2);
    const auto openGL4 = createFormat(QShaderFormat::OpenGLCoreProfile, 4, 0);

    const auto versionGLES2 = QByteArrayList() << "#version 100 es" << "";
    const auto versionGL3 = QByteArrayList() << "#version 130" << "";
    const auto versionGL32 = QByteArrayList() << "#version 150 core" << "";
    const auto versionGL4 = QByteArrayList() << "#version 400 core" << "";

    const auto es2Code = QByteArrayList() << "varying highp vec3 worldPosition;"
                                          << "uniform sampler2D texture;"
                                          << "varying highp vec2 texCoord;"
                                          << "uniform highp float lightIntensity;"
                                          << "uniform highp float exposure;"
                                          << "#pragma include es2/lightmodel.frag.inc"
                                          << ""
                                          << "void main()"
                                          << "{"
                                          << "    highp vec2 v2 = texCoord;"
                                          << "    sampler2D v1 = texture;"
                                          << "    highp float v3 = lightIntensity;"
                                          << "    highp vec4 v5 = texture2D(v1, v2);"
                                          << "    highp vec3 v0 = worldPosition;"
                                          << "    highp float v4 = exposure;"
                                          << "    highp vec4 v6 = lightModel(v5, v0, v3);"
                                          << "    highp vec4 v7 = v6 * pow(2.0, v4);"
                                          << "    gl_fragColor = v7;"
                                          << "}"
                                          << "";

    const auto gl3Code = QByteArrayList() << "in vec3 worldPosition;"
                                          << "uniform sampler2D texture;"
                                          << "in vec2 texCoord;"
                                          << "uniform float lightIntensity;"
                                          << "uniform float exposure;"
                                          << "out vec4 fragColor;"
                                          << "#pragma include gl3/lightmodel.frag.inc"
                                          << ""
                                          << "void main()"
                                          << "{"
                                          << "    vec2 v2 = texCoord;"
                                          << "    sampler2D v1 = texture;"
                                          << "    float v3 = lightIntensity;"
                                          << "    vec4 v5 = texture2D(v1, v2);"
                                          << "    vec3 v0 = worldPosition;"
                                          << "    float v4 = exposure;"
                                          << "    vec4 v6 = lightModel(v5, v0, v3);"
                                          << "    vec4 v7 = v6 * pow(2.0, v4);"
                                          << "    fragColor = v7;"
                                          << "}"
                                          << "";

    QTest::newRow("EmptyGraphAndFormat") << QShaderGraph() << QShaderFormat() << QByteArrayLiteral("\nvoid main()\n{\n}\n");
    QTest::newRow("LightExposureGraphAndES2") << graph << openGLES2 << (versionGLES2 + es2Code).join('\n');
    QTest::newRow("LightExposureGraphAndGL3") << graph << openGL3 << (versionGL3 + gl3Code).join('\n');
    QTest::newRow("LightExposureGraphAndGL32") << graph << openGL32 << (versionGL32 + gl3Code).join('\n');
    QTest::newRow("LightExposureGraphAndGL4") << graph << openGL4 << (versionGL4 + gl3Code).join('\n');
}

void tst_QShaderGenerator::shouldGenerateShaderCode()
{
    // GIVEN
    QFETCH(QShaderGraph, graph);
    QFETCH(QShaderFormat, format);

    auto generator = QShaderGenerator();
    generator.graph = graph;
    generator.format = format;

    // WHEN
    const auto code = generator.createShaderCode();

    // THEN
    QFETCH(QByteArray, expectedCode);
    QCOMPARE(code, expectedCode);
}

void tst_QShaderGenerator::shouldGenerateVersionCommands_data()
{
    QTest::addColumn<QShaderFormat>("format");
    QTest::addColumn<QByteArray>("version");

    QTest::newRow("GLES2") << createFormat(QShaderFormat::OpenGLES, 2, 0) << QByteArrayLiteral("#version 100 es");
    QTest::newRow("GLES3") << createFormat(QShaderFormat::OpenGLES, 3, 0) << QByteArrayLiteral("#version 300 es");

    QTest::newRow("GL20") << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0) << QByteArrayLiteral("#version 110");
    QTest::newRow("GL21") << createFormat(QShaderFormat::OpenGLNoProfile, 2, 1) << QByteArrayLiteral("#version 120");
    QTest::newRow("GL30") << createFormat(QShaderFormat::OpenGLNoProfile, 3, 0) << QByteArrayLiteral("#version 130");
    QTest::newRow("GL31") << createFormat(QShaderFormat::OpenGLNoProfile, 3, 1) << QByteArrayLiteral("#version 140");
    QTest::newRow("GL32") << createFormat(QShaderFormat::OpenGLNoProfile, 3, 2) << QByteArrayLiteral("#version 150");
    QTest::newRow("GL33") << createFormat(QShaderFormat::OpenGLNoProfile, 3, 3) << QByteArrayLiteral("#version 330");
    QTest::newRow("GL40") << createFormat(QShaderFormat::OpenGLNoProfile, 4, 0) << QByteArrayLiteral("#version 400");
    QTest::newRow("GL41") << createFormat(QShaderFormat::OpenGLNoProfile, 4, 1) << QByteArrayLiteral("#version 410");
    QTest::newRow("GL42") << createFormat(QShaderFormat::OpenGLNoProfile, 4, 2) << QByteArrayLiteral("#version 420");
    QTest::newRow("GL43") << createFormat(QShaderFormat::OpenGLNoProfile, 4, 3) << QByteArrayLiteral("#version 430");

    QTest::newRow("GL20core") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0) << QByteArrayLiteral("#version 110");
    QTest::newRow("GL21core") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 1) << QByteArrayLiteral("#version 120");
    QTest::newRow("GL30core") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0) << QByteArrayLiteral("#version 130");
    QTest::newRow("GL31core") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 1) << QByteArrayLiteral("#version 140");
    QTest::newRow("GL32core") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 2) << QByteArrayLiteral("#version 150 core");
    QTest::newRow("GL33core") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 3) << QByteArrayLiteral("#version 330 core");
    QTest::newRow("GL40core") << createFormat(QShaderFormat::OpenGLCoreProfile, 4, 0) << QByteArrayLiteral("#version 400 core");
    QTest::newRow("GL41core") << createFormat(QShaderFormat::OpenGLCoreProfile, 4, 1) << QByteArrayLiteral("#version 410 core");
    QTest::newRow("GL42core") << createFormat(QShaderFormat::OpenGLCoreProfile, 4, 2) << QByteArrayLiteral("#version 420 core");
    QTest::newRow("GL43core") << createFormat(QShaderFormat::OpenGLCoreProfile, 4, 3) << QByteArrayLiteral("#version 430 core");

    QTest::newRow("GL20compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0) << QByteArrayLiteral("#version 110");
    QTest::newRow("GL21compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 1) << QByteArrayLiteral("#version 120");
    QTest::newRow("GL30compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 3, 0) << QByteArrayLiteral("#version 130");
    QTest::newRow("GL31compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 3, 1) << QByteArrayLiteral("#version 140");
    QTest::newRow("GL32compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 3, 2) << QByteArrayLiteral("#version 150 compatibility");
    QTest::newRow("GL33compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 3, 3) << QByteArrayLiteral("#version 330 compatibility");
    QTest::newRow("GL40compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 4, 0) << QByteArrayLiteral("#version 400 compatibility");
    QTest::newRow("GL41compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 4, 1) << QByteArrayLiteral("#version 410 compatibility");
    QTest::newRow("GL42compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 4, 2) << QByteArrayLiteral("#version 420 compatibility");
    QTest::newRow("GL43compatibility") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 4, 3) << QByteArrayLiteral("#version 430 compatibility");
}

void tst_QShaderGenerator::shouldGenerateVersionCommands()
{
    // GIVEN
    QFETCH(QShaderFormat, format);

    auto generator = QShaderGenerator();
    generator.format = format;

    // WHEN
    const auto code = generator.createShaderCode();

    // THEN
    QFETCH(QByteArray, version);
    const auto expectedCode = (QByteArrayList() << version
                                                << ""
                                                << ""
                                                << "void main()"
                                                << "{"
                                                << "}"
                                                << "").join('\n');
    QCOMPARE(code, expectedCode);
}

QTEST_MAIN(tst_QShaderGenerator)

#include "tst_qshadergenerator.moc"
