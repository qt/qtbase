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

#include <QtCore/qmetaobject.h>
#include <QtGui/private/qshadergenerator_p.h>
#include <QtGui/private/qshaderlanguage_p.h>

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
    void shouldProcessLanguageQualifierAndTypeEnums_data();
    void shouldProcessLanguageQualifierAndTypeEnums();
    void shouldGenerateDifferentCodeDependingOnActiveLayers();
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

    const auto versionGLES2 = QByteArrayList() << "#version 100" << "";
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

    QTest::newRow("GLES2") << createFormat(QShaderFormat::OpenGLES, 2, 0) << QByteArrayLiteral("#version 100");
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


namespace {
    QString toGlsl(QShaderLanguage::StorageQualifier qualifier, const QShaderFormat &format)
    {
        if (format.version().majorVersion() <= 2) {
            // Note we're assuming fragment shader only here, it'd be different
            // values for vertex shader, will need to be fixed properly at some
            // point but isn't necessary yet (this problem already exists in past
            // commits anyway)
            switch (qualifier) {
            case QShaderLanguage::Const:
                return "const";
            case QShaderLanguage::Input:
                return "varying";
            case QShaderLanguage::BuiltIn:
                return "//";
            case QShaderLanguage::Output:
                return ""; // Although fragment shaders for <=2 only have fixed outputs
            case QShaderLanguage::Uniform:
                return "uniform";
            }
        } else {
            switch (qualifier) {
            case QShaderLanguage::Const:
                return "const";
            case QShaderLanguage::Input:
                return "in";
            case QShaderLanguage::BuiltIn:
                return "//";
            case QShaderLanguage::Output:
                return "out";
            case QShaderLanguage::Uniform:
                return "uniform";
            }
        }

        Q_UNREACHABLE();
    }

    QString toGlsl(QShaderLanguage::VariableType type)
    {
        switch (type) {
        case QShaderLanguage::Bool:
            return "bool";
        case QShaderLanguage::Int:
            return "int";
        case QShaderLanguage::Uint:
            return "uint";
        case QShaderLanguage::Float:
            return "float";
        case QShaderLanguage::Double:
            return "double";
        case QShaderLanguage::Vec2:
            return "vec2";
        case QShaderLanguage::Vec3:
            return "vec3";
        case QShaderLanguage::Vec4:
            return "vec4";
        case QShaderLanguage::DVec2:
            return "dvec2";
        case QShaderLanguage::DVec3:
            return "dvec3";
        case QShaderLanguage::DVec4:
            return "dvec4";
        case QShaderLanguage::BVec2:
            return "bvec2";
        case QShaderLanguage::BVec3:
            return "bvec3";
        case QShaderLanguage::BVec4:
            return "bvec4";
        case QShaderLanguage::IVec2:
            return "ivec2";
        case QShaderLanguage::IVec3:
            return "ivec3";
        case QShaderLanguage::IVec4:
            return "ivec4";
        case QShaderLanguage::UVec2:
            return "uvec2";
        case QShaderLanguage::UVec3:
            return "uvec3";
        case QShaderLanguage::UVec4:
            return "uvec4";
        case QShaderLanguage::Mat2:
            return "mat2";
        case QShaderLanguage::Mat3:
            return "mat3";
        case QShaderLanguage::Mat4:
            return "mat4";
        case QShaderLanguage::Mat2x2:
            return "mat2x2";
        case QShaderLanguage::Mat2x3:
            return "mat2x3";
        case QShaderLanguage::Mat2x4:
            return "mat2x4";
        case QShaderLanguage::Mat3x2:
            return "mat3x2";
        case QShaderLanguage::Mat3x3:
            return "mat3x3";
        case QShaderLanguage::Mat3x4:
            return "mat3x4";
        case QShaderLanguage::Mat4x2:
            return "mat4x2";
        case QShaderLanguage::Mat4x3:
            return "mat4x3";
        case QShaderLanguage::Mat4x4:
            return "mat4x4";
        case QShaderLanguage::DMat2:
            return "dmat2";
        case QShaderLanguage::DMat3:
            return "dmat3";
        case QShaderLanguage::DMat4:
            return "dmat4";
        case QShaderLanguage::DMat2x2:
            return "dmat2x2";
        case QShaderLanguage::DMat2x3:
            return "dmat2x3";
        case QShaderLanguage::DMat2x4:
            return "dmat2x4";
        case QShaderLanguage::DMat3x2:
            return "dmat3x2";
        case QShaderLanguage::DMat3x3:
            return "dmat3x3";
        case QShaderLanguage::DMat3x4:
            return "dmat3x4";
        case QShaderLanguage::DMat4x2:
            return "dmat4x2";
        case QShaderLanguage::DMat4x3:
            return "dmat4x3";
        case QShaderLanguage::DMat4x4:
            return "dmat4x4";
        case QShaderLanguage::Sampler1D:
            return "sampler1D";
        case QShaderLanguage::Sampler2D:
            return "sampler2D";
        case QShaderLanguage::Sampler3D:
            return "sampler3D";
        case QShaderLanguage::SamplerCube:
            return "samplerCube";
        case QShaderLanguage::Sampler2DRect:
            return "sampler2DRect";
        case QShaderLanguage::Sampler2DMs:
            return "sampler2DMS";
        case QShaderLanguage::SamplerBuffer:
            return "samplerBuffer";
        case QShaderLanguage::Sampler1DArray:
            return "sampler1DArray";
        case QShaderLanguage::Sampler2DArray:
            return "sampler2DArray";
        case QShaderLanguage::Sampler2DMsArray:
            return "sampler2DMSArray";
        case QShaderLanguage::SamplerCubeArray:
            return "samplerCubeArray";
        case QShaderLanguage::Sampler1DShadow:
            return "sampler1DShadow";
        case QShaderLanguage::Sampler2DShadow:
            return "sampler2DShadow";
        case QShaderLanguage::Sampler2DRectShadow:
            return "sampler2DRectShadow";
        case QShaderLanguage::Sampler1DArrayShadow:
            return "sampler1DArrayShadow";
        case QShaderLanguage::Sampler2DArrayShadow:
            return "sample2DArrayShadow";
        case QShaderLanguage::SamplerCubeShadow:
            return "samplerCubeShadow";
        case QShaderLanguage::SamplerCubeArrayShadow:
            return "samplerCubeArrayShadow";
        case QShaderLanguage::ISampler1D:
            return "isampler1D";
        case QShaderLanguage::ISampler2D:
            return "isampler2D";
        case QShaderLanguage::ISampler3D:
            return "isampler3D";
        case QShaderLanguage::ISamplerCube:
            return "isamplerCube";
        case QShaderLanguage::ISampler2DRect:
            return "isampler2DRect";
        case QShaderLanguage::ISampler2DMs:
            return "isampler2DMS";
        case QShaderLanguage::ISamplerBuffer:
            return "isamplerBuffer";
        case QShaderLanguage::ISampler1DArray:
            return "isampler1DArray";
        case QShaderLanguage::ISampler2DArray:
            return "isampler2DArray";
        case QShaderLanguage::ISampler2DMsArray:
            return "isampler2DMSArray";
        case QShaderLanguage::ISamplerCubeArray:
            return "isamplerCubeArray";
        case QShaderLanguage::USampler1D:
            return "usampler1D";
        case QShaderLanguage::USampler2D:
            return "usampler2D";
        case QShaderLanguage::USampler3D:
            return "usampler3D";
        case QShaderLanguage::USamplerCube:
            return "usamplerCube";
        case QShaderLanguage::USampler2DRect:
            return "usampler2DRect";
        case QShaderLanguage::USampler2DMs:
            return "usampler2DMS";
        case QShaderLanguage::USamplerBuffer:
            return "usamplerBuffer";
        case QShaderLanguage::USampler1DArray:
            return "usampler1DArray";
        case QShaderLanguage::USampler2DArray:
            return "usampler2DArray";
        case QShaderLanguage::USampler2DMsArray:
            return "usampler2DMSArray";
        case QShaderLanguage::USamplerCubeArray:
            return "usamplerCubeArray";
        }

        Q_UNREACHABLE();
    }
}

void tst_QShaderGenerator::shouldProcessLanguageQualifierAndTypeEnums_data()
{
    QTest::addColumn<QShaderGraph>("graph");
    QTest::addColumn<QShaderFormat>("format");
    QTest::addColumn<QByteArray>("expectedCode");

    const auto es2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
    const auto es3 = createFormat(QShaderFormat::OpenGLES, 3, 0);
    const auto gl2 = createFormat(QShaderFormat::OpenGLNoProfile, 2, 0);
    const auto gl3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0);
    const auto gl4 = createFormat(QShaderFormat::OpenGLCoreProfile, 4, 0);

    const auto qualifierEnum = QMetaEnum::fromType<QShaderLanguage::StorageQualifier>();
    const auto typeEnum = QMetaEnum::fromType<QShaderLanguage::VariableType>();

    for (int qualifierIndex = 0; qualifierIndex < qualifierEnum.keyCount(); qualifierIndex++) {
        const auto qualifierName = qualifierEnum.key(qualifierIndex);
        const auto qualifierValue = static_cast<QShaderLanguage::StorageQualifier>(qualifierEnum.value(qualifierIndex));

        for (int typeIndex = 0; typeIndex < typeEnum.keyCount(); typeIndex++) {
            const auto typeName = typeEnum.key(typeIndex);
            const auto typeValue = static_cast<QShaderLanguage::VariableType>(typeEnum.value(typeIndex));

            auto graph = QShaderGraph();

            auto worldPosition = createNode({
                createPort(QShaderNodePort::Output, "value")
            });
            worldPosition.setParameter("name", "worldPosition");
            worldPosition.setParameter("qualifier", QVariant::fromValue<QShaderLanguage::StorageQualifier>(qualifierValue));
            worldPosition.setParameter("type", QVariant::fromValue<QShaderLanguage::VariableType>(typeValue));
            worldPosition.addRule(es2, QShaderNode::Rule("highp $type $value = $name;",
                                                         QByteArrayList() << "$qualifier highp $type $name;"));
            worldPosition.addRule(gl2, QShaderNode::Rule("$type $value = $name;",
                                                         QByteArrayList() << "$qualifier $type $name;"));
            worldPosition.addRule(gl3, QShaderNode::Rule("$type $value = $name;",
                                                         QByteArrayList() << "$qualifier $type $name;"));

            auto fragColor = createNode({
                createPort(QShaderNodePort::Input, "fragColor")
            });
            fragColor.addRule(es2, QShaderNode::Rule("gl_fragColor = $fragColor;"));
            fragColor.addRule(gl2, QShaderNode::Rule("gl_fragColor = $fragColor;"));
            fragColor.addRule(gl3, QShaderNode::Rule("fragColor = $fragColor;",
                                                     QByteArrayList() << "out vec4 fragColor;"));

            graph.addNode(worldPosition);
            graph.addNode(fragColor);

            graph.addEdge(createEdge(worldPosition.uuid(), "value", fragColor.uuid(), "fragColor"));

            const auto gl2Code = (QByteArrayList() << "#version 110"
                                                   << ""
                                                   << QStringLiteral("%1 %2 worldPosition;").arg(toGlsl(qualifierValue, gl2))
                                                                                            .arg(toGlsl(typeValue))
                                                                                            .toUtf8()
                                                   << ""
                                                   << "void main()"
                                                   << "{"
                                                   << QStringLiteral("    %1 v0 = worldPosition;").arg(toGlsl(typeValue)).toUtf8()
                                                   << "    gl_fragColor = v0;"
                                                   << "}"
                                                   << "").join("\n");
            const auto gl3Code = (QByteArrayList() << "#version 130"
                                                   << ""
                                                   << QStringLiteral("%1 %2 worldPosition;").arg(toGlsl(qualifierValue, gl3))
                                                                                            .arg(toGlsl(typeValue))
                                                                                            .toUtf8()
                                                   << "out vec4 fragColor;"
                                                   << ""
                                                   << "void main()"
                                                   << "{"
                                                   << QStringLiteral("    %1 v0 = worldPosition;").arg(toGlsl(typeValue)).toUtf8()
                                                   << "    fragColor = v0;"
                                                   << "}"
                                                   << "").join("\n");
            const auto gl4Code = (QByteArrayList() << "#version 400 core"
                                                   << ""
                                                   << QStringLiteral("%1 %2 worldPosition;").arg(toGlsl(qualifierValue, gl4))
                                                                                            .arg(toGlsl(typeValue))
                                                                                            .toUtf8()
                                                   << "out vec4 fragColor;"
                                                   << ""
                                                   << "void main()"
                                                   << "{"
                                                   << QStringLiteral("    %1 v0 = worldPosition;").arg(toGlsl(typeValue)).toUtf8()
                                                   << "    fragColor = v0;"
                                                   << "}"
                                                   << "").join("\n");
            const auto es2Code = (QByteArrayList() << "#version 100"
                                                   << ""
                                                   << QStringLiteral("%1 highp %2 worldPosition;").arg(toGlsl(qualifierValue, es2))
                                                                                                  .arg(toGlsl(typeValue))
                                                                                                  .toUtf8()
                                                   << ""
                                                   << "void main()"
                                                   << "{"
                                                   << QStringLiteral("    highp %1 v0 = worldPosition;").arg(toGlsl(typeValue)).toUtf8()
                                                   << "    gl_fragColor = v0;"
                                                   << "}"
                                                   << "").join("\n");
            const auto es3Code = (QByteArrayList() << "#version 300 es"
                                                   << ""
                                                   << QStringLiteral("%1 highp %2 worldPosition;").arg(toGlsl(qualifierValue, es3))
                                                                                                  .arg(toGlsl(typeValue))
                                                                                                  .toUtf8()
                                                   << ""
                                                   << "void main()"
                                                   << "{"
                                                   << QStringLiteral("    highp %1 v0 = worldPosition;").arg(toGlsl(typeValue)).toUtf8()
                                                   << "    gl_fragColor = v0;"
                                                   << "}"
                                                   << "").join("\n");

            QTest::addRow("%s %s ES2", qualifierName, typeName) << graph << es2 << es2Code;
            QTest::addRow("%s %s ES3", qualifierName, typeName) << graph << es3 << es3Code;
            QTest::addRow("%s %s GL2", qualifierName, typeName) << graph << gl2 << gl2Code;
            QTest::addRow("%s %s GL3", qualifierName, typeName) << graph << gl3 << gl3Code;
            QTest::addRow("%s %s GL4", qualifierName, typeName) << graph << gl4 << gl4Code;
        }
    }
}

void tst_QShaderGenerator::shouldProcessLanguageQualifierAndTypeEnums()
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

void tst_QShaderGenerator::shouldGenerateDifferentCodeDependingOnActiveLayers()
{
    // GIVEN
    const auto gl4 = createFormat(QShaderFormat::OpenGLCoreProfile, 4, 0);

    auto texCoord = createNode({
        createPort(QShaderNodePort::Output, "texCoord")
    }, {
        "diffuseTexture",
        "normalTexture"
    });
    texCoord.addRule(gl4, QShaderNode::Rule("vec2 $texCoord = texCoord;",
                                            QByteArrayList() << "in vec2 texCoord;"));
    auto diffuseUniform = createNode({
        createPort(QShaderNodePort::Output, "color")
    }, {"diffuseUniform"});
    diffuseUniform.addRule(gl4, QShaderNode::Rule("vec4 $color = diffuseUniform;",
                                                  QByteArrayList() << "uniform vec4 diffuseUniform;"));
    auto diffuseTexture = createNode({
        createPort(QShaderNodePort::Input, "coord"),
        createPort(QShaderNodePort::Output, "color")
    }, {"diffuseTexture"});
    diffuseTexture.addRule(gl4, QShaderNode::Rule("vec4 $color = texture2D(diffuseTexture, $coord);",
                                                 QByteArrayList() << "uniform sampler2D diffuseTexture;"));
    auto normalUniform = createNode({
        createPort(QShaderNodePort::Output, "normal")
    }, {"normalUniform"});
    normalUniform.addRule(gl4, QShaderNode::Rule("vec3 $normal = normalUniform;",
                                                 QByteArrayList() << "uniform vec3 normalUniform;"));
    auto normalTexture = createNode({
        createPort(QShaderNodePort::Input, "coord"),
        createPort(QShaderNodePort::Output, "normal")
    }, {"normalTexture"});
    normalTexture.addRule(gl4, QShaderNode::Rule("vec3 $normal = texture2D(normalTexture, $coord).rgb;",
                                                 QByteArrayList() << "uniform sampler2D normalTexture;"));
    auto lightFunction = createNode({
        createPort(QShaderNodePort::Input, "color"),
        createPort(QShaderNodePort::Input, "normal"),
        createPort(QShaderNodePort::Output, "output")
    });
    lightFunction.addRule(gl4, QShaderNode::Rule("vec4 $output = lightModel($color, $normal);",
                                                 QByteArrayList() << "#pragma include gl4/lightmodel.frag.inc"));
    auto fragColor = createNode({
        createPort(QShaderNodePort::Input, "fragColor")
    });
    fragColor.addRule(gl4, QShaderNode::Rule("fragColor = $fragColor;",
                                             QByteArrayList() << "out vec4 fragColor;"));

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

    auto generator = QShaderGenerator();
    generator.graph = graph;
    generator.format = gl4;

    {
        // WHEN
        const auto code = generator.createShaderCode({"diffuseUniform", "normalUniform"});

        // THEN
        const auto expected = QByteArrayList()
                << "#version 400 core"
                << ""
                << "uniform vec4 diffuseUniform;"
                << "uniform vec3 normalUniform;"
                << "#pragma include gl4/lightmodel.frag.inc"
                << "out vec4 fragColor;"
                << ""
                << "void main()"
                << "{"
                << "    vec3 v1 = normalUniform;"
                << "    vec4 v0 = diffuseUniform;"
                << "    vec4 v2 = lightModel(v0, v1);"
                << "    fragColor = v2;"
                << "}"
                << "";
        QCOMPARE(code, expected.join("\n"));
    }

    {
        // WHEN
        const auto code = generator.createShaderCode({"diffuseUniform", "normalTexture"});

        // THEN
        const auto expected = QByteArrayList()
                << "#version 400 core"
                << ""
                << "in vec2 texCoord;"
                << "uniform vec4 diffuseUniform;"
                << "uniform sampler2D normalTexture;"
                << "#pragma include gl4/lightmodel.frag.inc"
                << "out vec4 fragColor;"
                << ""
                << "void main()"
                << "{"
                << "    vec2 v0 = texCoord;"
                << "    vec3 v2 = texture2D(normalTexture, v0).rgb;"
                << "    vec4 v1 = diffuseUniform;"
                << "    vec4 v3 = lightModel(v1, v2);"
                << "    fragColor = v3;"
                << "}"
                << "";
        QCOMPARE(code, expected.join("\n"));
    }

    {
        // WHEN
        const auto code = generator.createShaderCode({"diffuseTexture", "normalUniform"});

        // THEN
        const auto expected = QByteArrayList()
                << "#version 400 core"
                << ""
                << "in vec2 texCoord;"
                << "uniform sampler2D diffuseTexture;"
                << "uniform vec3 normalUniform;"
                << "#pragma include gl4/lightmodel.frag.inc"
                << "out vec4 fragColor;"
                << ""
                << "void main()"
                << "{"
                << "    vec2 v0 = texCoord;"
                << "    vec3 v2 = normalUniform;"
                << "    vec4 v1 = texture2D(diffuseTexture, v0);"
                << "    vec4 v3 = lightModel(v1, v2);"
                << "    fragColor = v3;"
                << "}"
                << "";
        QCOMPARE(code, expected.join("\n"));
    }

    {
        // WHEN
        const auto code = generator.createShaderCode({"diffuseTexture", "normalTexture"});

        // THEN
        const auto expected = QByteArrayList()
                << "#version 400 core"
                << ""
                << "in vec2 texCoord;"
                << "uniform sampler2D diffuseTexture;"
                << "uniform sampler2D normalTexture;"
                << "#pragma include gl4/lightmodel.frag.inc"
                << "out vec4 fragColor;"
                << ""
                << "void main()"
                << "{"
                << "    vec2 v0 = texCoord;"
                << "    vec3 v2 = texture2D(normalTexture, v0).rgb;"
                << "    vec4 v1 = texture2D(diffuseTexture, v0);"
                << "    vec4 v3 = lightModel(v1, v2);"
                << "    fragColor = v3;"
                << "}"
                << "";
        QCOMPARE(code, expected.join("\n"));
    }
}

QTEST_MAIN(tst_QShaderGenerator)

#include "tst_qshadergenerator.moc"
