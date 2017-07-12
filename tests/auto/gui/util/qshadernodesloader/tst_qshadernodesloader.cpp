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

#include <QtGui/private/qshadernodesloader_p.h>
#include <QtGui/private/qshaderlanguage_p.h>

using QBufferPointer = QSharedPointer<QBuffer>;
Q_DECLARE_METATYPE(QBufferPointer);

using NodeHash = QHash<QString, QShaderNode>;
Q_DECLARE_METATYPE(NodeHash);

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

    QShaderFormat createFormat(QShaderFormat::Api api, int majorVersion, int minorVersion,
                               const QStringList &extensions = QStringList(),
                               const QString &vendor = QString())
    {
        auto format = QShaderFormat();
        format.setApi(api);
        format.setVersion(QVersionNumber(majorVersion, minorVersion));
        format.setExtensions(extensions);
        format.setVendor(vendor);
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
        for (const auto &port : ports)
            node.addPort(port);
        return node;
    }
}

class tst_QShaderNodesLoader : public QObject
{
    Q_OBJECT
private slots:
    void shouldManipulateLoaderMembers();
    void shouldLoadFromJsonStream_data();
    void shouldLoadFromJsonStream();
};

void tst_QShaderNodesLoader::shouldManipulateLoaderMembers()
{
    // GIVEN
    auto loader = QShaderNodesLoader();

    // THEN (default state)
    QCOMPARE(loader.status(), QShaderNodesLoader::Null);
    QVERIFY(!loader.device());
    QVERIFY(loader.nodes().isEmpty());

    // WHEN
    auto device1 = createBuffer(QByteArray("..........."), QIODevice::NotOpen);
    loader.setDevice(device1.data());

    // THEN
    QCOMPARE(loader.status(), QShaderNodesLoader::Error);
    QCOMPARE(loader.device(), device1.data());
    QVERIFY(loader.nodes().isEmpty());

    // WHEN
    auto device2 = createBuffer(QByteArray("..........."), QIODevice::ReadOnly);
    loader.setDevice(device2.data());

    // THEN
    QCOMPARE(loader.status(), QShaderNodesLoader::Waiting);
    QCOMPARE(loader.device(), device2.data());
    QVERIFY(loader.nodes().isEmpty());
}

void tst_QShaderNodesLoader::shouldLoadFromJsonStream_data()
{
    QTest::addColumn<QBufferPointer>("device");
    QTest::addColumn<NodeHash>("nodes");
    QTest::addColumn<QShaderNodesLoader::Status>("status");

    QTest::newRow("empty") << createBuffer("", QIODevice::ReadOnly) << NodeHash() << QShaderNodesLoader::Error;

    const auto smallJson = "{"
                           "    \"inputValue\": {"
                           "        \"outputs\": ["
                           "            \"value\""
                           "        ],"
                           "        \"parameters\": {"
                           "            \"name\": \"defaultName\","
                           "            \"qualifier\": {"
                           "                \"type\": \"QShaderLanguage::StorageQualifier\","
                           "                \"value\": \"QShaderLanguage::Uniform\""
                           "            },"
                           "            \"type\": {"
                           "                \"type\": \"QShaderLanguage::VariableType\","
                           "                \"value\": \"QShaderLanguage::Vec3\""
                           "            },"
                           "            \"defaultValue\": {"
                           "                \"type\": \"float\","
                           "                \"value\": \"1.25\""
                           "            }"
                           "        },"
                           "        \"rules\": ["
                           "            {"
                           "                \"format\": {"
                           "                    \"api\": \"OpenGLES\","
                           "                    \"major\": 2,"
                           "                    \"minor\": 0"
                           "                },"
                           "                \"substitution\": \"highp vec3 $value = $name;\","
                           "                \"headerSnippets\": [ \"varying highp vec3 $name;\" ]"
                           "            },"
                           "            {"
                           "                \"format\": {"
                           "                    \"api\": \"OpenGLCompatibilityProfile\","
                           "                    \"major\": 2,"
                           "                    \"minor\": 1"
                           "                },"
                           "                \"substitution\": \"vec3 $value = $name;\","
                           "                \"headerSnippets\": [ \"in vec3 $name;\" ]"
                           "            }"
                           "        ]"
                           "    },"
                           "    \"fragColor\": {"
                           "        \"inputs\": ["
                           "            \"fragColor\""
                           "        ],"
                           "        \"rules\": ["
                           "            {"
                           "                \"format\": {"
                           "                    \"api\": \"OpenGLES\","
                           "                    \"major\": 2,"
                           "                    \"minor\": 0"
                           "                },"
                           "                \"substitution\": \"gl_fragColor = $fragColor;\""
                           "            },"
                           "            {"
                           "                \"format\": {"
                           "                    \"api\": \"OpenGLNoProfile\","
                           "                    \"major\": 4,"
                           "                    \"minor\": 0"
                           "                },"
                           "                \"substitution\": \"fragColor = $fragColor;\","
                           "                \"headerSnippets\": [ \"out vec4 fragColor;\" ]"
                           "            }"
                           "        ]"
                           "    },"
                           "    \"lightModel\": {"
                           "        \"inputs\": ["
                           "            \"baseColor\","
                           "            \"position\","
                           "            \"lightIntensity\""
                           "        ],"
                           "        \"outputs\": ["
                           "            \"outputColor\""
                           "        ],"
                           "        \"rules\": ["
                           "            {"
                           "                \"format\": {"
                           "                    \"api\": \"OpenGLES\","
                           "                    \"major\": 2,"
                           "                    \"minor\": 0,"
                           "                    \"extensions\": [ \"ext1\", \"ext2\" ],"
                           "                    \"vendor\": \"kdab\""
                           "                },"
                           "                \"substitution\": \"highp vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);\","
                           "                \"headerSnippets\": [ \"#pragma include es2/lightmodel.frag.inc\" ]"
                           "            },"
                           "            {"
                           "                \"format\": {"
                           "                    \"api\": \"OpenGLCoreProfile\","
                           "                    \"major\": 3,"
                           "                    \"minor\": 3"
                           "                },"
                           "                \"substitution\": \"vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);\","
                           "                \"headerSnippets\": [ \"#pragma include gl3/lightmodel.frag.inc\" ]"
                           "            }"
                           "        ]"
                           "    }"
                           "}";

    const auto smallProtos = [this]{
        const auto openGLES2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
        const auto openGLES2Extended = createFormat(QShaderFormat::OpenGLES, 2, 0, {"ext1", "ext2"}, "kdab");
        const auto openGL2 = createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 1);
        const auto openGL3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 3);
        const auto openGL4 = createFormat(QShaderFormat::OpenGLNoProfile, 4, 0);

        auto protos = NodeHash();

        auto inputValue = createNode({
            createPort(QShaderNodePort::Output, "value")
        });
        inputValue.setParameter("name", "defaultName");
        inputValue.setParameter("qualifier", QVariant::fromValue<QShaderLanguage::StorageQualifier>(QShaderLanguage::Uniform));
        inputValue.setParameter("type", QVariant::fromValue<QShaderLanguage::VariableType>(QShaderLanguage::Vec3));
        inputValue.setParameter("defaultValue", QVariant(1.25f));
        inputValue.addRule(openGLES2, QShaderNode::Rule("highp vec3 $value = $name;",
                                                        QByteArrayList() << "varying highp vec3 $name;"));
        inputValue.addRule(openGL2, QShaderNode::Rule("vec3 $value = $name;",
                                                      QByteArrayList() << "in vec3 $name;"));
        protos.insert("inputValue", inputValue);

        auto fragColor = createNode({
            createPort(QShaderNodePort::Input, "fragColor")
        });
        fragColor.addRule(openGLES2, QShaderNode::Rule("gl_fragColor = $fragColor;"));
        fragColor.addRule(openGL4, QShaderNode::Rule("fragColor = $fragColor;",
                                                     QByteArrayList() << "out vec4 fragColor;"));
        protos.insert(QStringLiteral("fragColor"), fragColor);

        auto lightModel = createNode({
            createPort(QShaderNodePort::Input, "baseColor"),
            createPort(QShaderNodePort::Input, "position"),
            createPort(QShaderNodePort::Input, "lightIntensity"),
            createPort(QShaderNodePort::Output, "outputColor")
        });
        lightModel.addRule(openGLES2Extended, QShaderNode::Rule("highp vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                                QByteArrayList() << "#pragma include es2/lightmodel.frag.inc"));
        lightModel.addRule(openGL3, QShaderNode::Rule("vec4 $outputColor = lightModel($baseColor, $position, $lightIntensity);",
                                                      QByteArrayList() << "#pragma include gl3/lightmodel.frag.inc"));
        protos.insert("lightModel", lightModel);

        return protos;
    }();

    QTest::newRow("NotOpen") << createBuffer(smallJson, QIODevice::NotOpen) << NodeHash() << QShaderNodesLoader::Error;
    QTest::newRow("CorrectJSON") << createBuffer(smallJson) << smallProtos << QShaderNodesLoader::Ready;
}

void tst_QShaderNodesLoader::shouldLoadFromJsonStream()
{
    // GIVEN
    QFETCH(QBufferPointer, device);

    auto loader = QShaderNodesLoader();

    // WHEN
    loader.setDevice(device.data());
    loader.load();

    // THEN
    QFETCH(QShaderNodesLoader::Status, status);
    QCOMPARE(loader.status(), status);

    QFETCH(NodeHash, nodes);
    const auto sortedKeys = [](const NodeHash &nodes) {
        auto res = nodes.keys();
        res.sort();
        return res;
    };
    const auto sortedParameters = [](const QShaderNode &node) {
        auto res = node.parameterNames();
        res.sort();
        return res;
    };
    QCOMPARE(sortedKeys(loader.nodes()), sortedKeys(nodes));
    for (const auto &key : nodes.keys()) {
        const auto actual = loader.nodes().value(key);
        const auto expected = nodes.value(key);

        QVERIFY(actual.uuid().isNull());
        QCOMPARE(actual.ports(), expected.ports());
        QCOMPARE(sortedParameters(actual), sortedParameters(expected));
        for (const auto &name : expected.parameterNames()) {
            QCOMPARE(actual.parameter(name), expected.parameter(name));
        }
        QCOMPARE(actual.availableFormats(), expected.availableFormats());
        for (const auto &format : expected.availableFormats()) {
            QCOMPARE(actual.rule(format), expected.rule(format));
        }
    }
}

QTEST_MAIN(tst_QShaderNodesLoader)

#include "tst_qshadernodesloader.moc"
