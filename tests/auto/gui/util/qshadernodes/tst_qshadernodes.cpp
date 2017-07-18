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

#include <QtGui/private/qshaderformat_p.h>
#include <QtGui/private/qshadernode_p.h>
#include <QtGui/private/qshadernodeport_p.h>

namespace
{
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

    QShaderNodePort createPort(QShaderNodePort::Direction direction, const QString &name)
    {
        auto port = QShaderNodePort();
        port.direction = direction;
        port.name = name;
        return port;
    }
}

class tst_QShaderNodes : public QObject
{
    Q_OBJECT
private slots:
    void shouldManipulateFormatMembers();
    void shouldVerifyFormatsEquality_data();
    void shouldVerifyFormatsEquality();
    void shouldVerifyFormatsCompatibilities_data();
    void shouldVerifyFormatsCompatibilities();

    void shouldHaveDefaultPortState();
    void shouldVerifyPortsEquality_data();
    void shouldVerifyPortsEquality();

    void shouldManipulateNodeMembers();
    void shouldHandleNodeRulesSupportAndOrder();
};

void tst_QShaderNodes::shouldManipulateFormatMembers()
{
    // GIVEN
    auto format = QShaderFormat();

    // THEN (default state)
    QCOMPARE(format.api(), QShaderFormat::NoApi);
    QCOMPARE(format.version().majorVersion(), 0);
    QCOMPARE(format.version().minorVersion(), 0);
    QCOMPARE(format.extensions(), QStringList());
    QCOMPARE(format.vendor(), QString());
    QVERIFY(!format.isValid());

    // WHEN
    format.setApi(QShaderFormat::OpenGLES);

    // THEN
    QCOMPARE(format.api(), QShaderFormat::OpenGLES);
    QCOMPARE(format.version().majorVersion(), 0);
    QCOMPARE(format.version().minorVersion(), 0);
    QCOMPARE(format.extensions(), QStringList());
    QCOMPARE(format.vendor(), QString());
    QVERIFY(!format.isValid());

    // WHEN
    format.setVersion(QVersionNumber(3));

    // THEN
    QCOMPARE(format.api(), QShaderFormat::OpenGLES);
    QCOMPARE(format.version().majorVersion(), 3);
    QCOMPARE(format.version().minorVersion(), 0);
    QCOMPARE(format.extensions(), QStringList());
    QCOMPARE(format.vendor(), QString());
    QVERIFY(format.isValid());

    // WHEN
    format.setVersion(QVersionNumber(3, 2));

    // THEN
    QCOMPARE(format.api(), QShaderFormat::OpenGLES);
    QCOMPARE(format.version().majorVersion(), 3);
    QCOMPARE(format.version().minorVersion(), 2);
    QCOMPARE(format.extensions(), QStringList());
    QCOMPARE(format.vendor(), QString());
    QVERIFY(format.isValid());

    // WHEN
    format.setExtensions({"foo", "bar"});

    // THEN
    QCOMPARE(format.api(), QShaderFormat::OpenGLES);
    QCOMPARE(format.version().majorVersion(), 3);
    QCOMPARE(format.version().minorVersion(), 2);
    QCOMPARE(format.extensions(), QStringList({"bar", "foo"}));
    QCOMPARE(format.vendor(), QString());
    QVERIFY(format.isValid());

    // WHEN
    format.setVendor(QStringLiteral("KDAB"));

    // THEN
    QCOMPARE(format.api(), QShaderFormat::OpenGLES);
    QCOMPARE(format.version().majorVersion(), 3);
    QCOMPARE(format.version().minorVersion(), 2);
    QCOMPARE(format.extensions(), QStringList({"bar", "foo"}));
    QCOMPARE(format.vendor(), QStringLiteral("KDAB"));
    QVERIFY(format.isValid());
}

void tst_QShaderNodes::shouldVerifyFormatsEquality_data()
{
    QTest::addColumn<QShaderFormat>("left");
    QTest::addColumn<QShaderFormat>("right");
    QTest::addColumn<bool>("expected");

    QTest::newRow("Equals") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"}, "KDAB")
                            << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"}, "KDAB")
                            << true;
    QTest::newRow("Apis") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"}, "KDAB")
                          << createFormat(QShaderFormat::OpenGLNoProfile, 3, 0, {"foo", "bar"}, "KDAB")
                          << false;
    QTest::newRow("Major") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"}, "KDAB")
                           << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0, {"foo", "bar"}, "KDAB")
                           << false;
    QTest::newRow("Minor") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"}, "KDAB")
                           << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 1, {"foo", "bar"}, "KDAB")
                           << false;
    QTest::newRow("Extensions") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"}, "KDAB")
                                << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo"}, "KDAB")
                                << false;
    QTest::newRow("Vendor") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"}, "KDAB")
                            << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"})
                            << false;
}

void tst_QShaderNodes::shouldVerifyFormatsEquality()
{
    // GIVEN
    QFETCH(QShaderFormat, left);
    QFETCH(QShaderFormat, right);

    // WHEN
    const auto equal = (left == right);
    const auto notEqual = (left != right);

    // THEN
    QFETCH(bool, expected);
    QCOMPARE(equal, expected);
    QCOMPARE(notEqual, !expected);
}

void tst_QShaderNodes::shouldVerifyFormatsCompatibilities_data()
{
    QTest::addColumn<QShaderFormat>("reference");
    QTest::addColumn<QShaderFormat>("tested");
    QTest::addColumn<bool>("expected");

    QTest::newRow("NoProfileVsES") << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                   << createFormat(QShaderFormat::OpenGLES, 2, 0)
                                   << true;
    QTest::newRow("CoreProfileVsES") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                     << createFormat(QShaderFormat::OpenGLES, 2, 0)
                                     << false;
    QTest::newRow("CompatProfileVsES") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                       << createFormat(QShaderFormat::OpenGLES, 2, 0)
                                       << true;

    QTest::newRow("ESVsNoProfile") << createFormat(QShaderFormat::OpenGLES, 2, 0)
                                   << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                   << false;
    QTest::newRow("ESVsCoreProfile") << createFormat(QShaderFormat::OpenGLES, 2, 0)
                                     << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                     << false;
    QTest::newRow("ESVsCompatProfile") << createFormat(QShaderFormat::OpenGLES, 2, 0)
                                       << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                       << false;

    QTest::newRow("CoreVsNoProfile") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                     << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                     << false;
    QTest::newRow("CoreVsCompat") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                  << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                  << false;
    QTest::newRow("CoreVsCore") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                << true;

    QTest::newRow("NoProfileVsCore") << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                     << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                     << true;
    QTest::newRow("NoProvileVsCompat") << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                       << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                       << true;
    QTest::newRow("NoProfileVsNoProfile") << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                          << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                          << true;

    QTest::newRow("CompatVsCore") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                  << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                  << true;
    QTest::newRow("CompatVsCompat") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                    << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                    << true;
    QTest::newRow("CompatVsNoProfile") << createFormat(QShaderFormat::OpenGLCompatibilityProfile, 2, 0)
                                       << createFormat(QShaderFormat::OpenGLNoProfile, 2, 0)
                                       << true;

    QTest::newRow("MajorForwardCompat_1") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0)
                                          << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                          << true;
    QTest::newRow("MajorForwardCompat_2") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0)
                                          << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 4)
                                          << true;
    QTest::newRow("MajorForwardCompat_3") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 0)
                                          << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0)
                                          << false;
    QTest::newRow("MajorForwardCompat_4") << createFormat(QShaderFormat::OpenGLCoreProfile, 2, 4)
                                          << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0)
                                          << false;

    QTest::newRow("MinorForwardCompat_1") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 1)
                                          << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0)
                                          << true;
    QTest::newRow("MinorForwardCompat_2") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0)
                                          << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 1)
                                          << false;

    QTest::newRow("Extensions_1") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"})
                                  << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo"})
                                  << true;
    QTest::newRow("Extensions_2") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo"})
                                  << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {"foo", "bar"})
                                  << false;

    QTest::newRow("Vendor_1") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {}, "KDAB")
                              << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {})
                              << true;
    QTest::newRow("Vendor_2") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {})
                              << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {}, "KDAB")
                              << false;
    QTest::newRow("Vendor_2") << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {}, "KDAB")
                              << createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0, {}, "KDAB")
                              << true;
}

void tst_QShaderNodes::shouldVerifyFormatsCompatibilities()
{
    // GIVEN
    QFETCH(QShaderFormat, reference);
    QFETCH(QShaderFormat, tested);

    // WHEN
    const auto supported = reference.supports(tested);

    // THEN
    QFETCH(bool, expected);
    QCOMPARE(supported, expected);
}

void tst_QShaderNodes::shouldHaveDefaultPortState()
{
    // GIVEN
    auto port = QShaderNodePort();

    // THEN
    QCOMPARE(port.direction, QShaderNodePort::Output);
    QVERIFY(port.name.isEmpty());
}

void tst_QShaderNodes::shouldVerifyPortsEquality_data()
{
    QTest::addColumn<QShaderNodePort>("left");
    QTest::addColumn<QShaderNodePort>("right");
    QTest::addColumn<bool>("expected");

    QTest::newRow("Equals") << createPort(QShaderNodePort::Input, "foo")
                            << createPort(QShaderNodePort::Input, "foo")
                            << true;
    QTest::newRow("Direction") << createPort(QShaderNodePort::Input, "foo")
                               << createPort(QShaderNodePort::Output, "foo")
                               << false;
    QTest::newRow("Name") << createPort(QShaderNodePort::Input, "foo")
                          << createPort(QShaderNodePort::Input, "bar")
                          << false;
}

void tst_QShaderNodes::shouldVerifyPortsEquality()
{
    // GIVEN
    QFETCH(QShaderNodePort, left);
    QFETCH(QShaderNodePort, right);

    // WHEN
    const auto equal = (left == right);
    const auto notEqual = (left != right);

    // THEN
    QFETCH(bool, expected);
    QCOMPARE(equal, expected);
    QCOMPARE(notEqual, !expected);
}

void tst_QShaderNodes::shouldManipulateNodeMembers()
{
    // GIVEN
    const auto openGLES2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
    const auto openGL3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0);

    const auto es2Rule = QShaderNode::Rule(QByteArrayLiteral("gles2"), {"#pragma include es2/foo.inc", "#pragma include es2/bar.inc"});
    const auto gl3Rule = QShaderNode::Rule(QByteArrayLiteral("gl3"), {"#pragma include gl3/foo.inc", "#pragma include gl3/bar.inc"});
    const auto gl3bisRule = QShaderNode::Rule(QByteArrayLiteral("gl3bis"), {"#pragma include gl3/foo.inc", "#pragma include gl3/bar.inc"});

    auto node = QShaderNode();

    // THEN (default state)
    QCOMPARE(node.type(), QShaderNode::Invalid);
    QVERIFY(node.uuid().isNull());
    QVERIFY(node.layers().isEmpty());
    QVERIFY(node.ports().isEmpty());
    QVERIFY(node.parameterNames().isEmpty());
    QVERIFY(node.availableFormats().isEmpty());

    // WHEN
    const auto uuid = QUuid::createUuid();
    node.setUuid(uuid);

    // THEN
    QCOMPARE(node.uuid(), uuid);

    // WHEN
    node.setLayers({"foo", "bar"});

    // THEN
    QCOMPARE(node.layers(), QStringList({"foo", "bar"}));

    // WHEN
    auto firstPort = QShaderNodePort();
    firstPort.direction = QShaderNodePort::Input;
    firstPort.name = QStringLiteral("foo");
    node.addPort(firstPort);

    // THEN
    QCOMPARE(node.type(), QShaderNode::Output);
    QCOMPARE(node.ports().size(), 1);
    QCOMPARE(node.ports().at(0), firstPort);
    QVERIFY(node.availableFormats().isEmpty());

    // WHEN
    auto secondPort = QShaderNodePort();
    secondPort.direction = QShaderNodePort::Output;
    secondPort.name = QStringLiteral("bar");
    node.addPort(secondPort);

    // THEN
    QCOMPARE(node.type(), QShaderNode::Function);
    QCOMPARE(node.ports().size(), 2);
    QCOMPARE(node.ports().at(0), firstPort);
    QCOMPARE(node.ports().at(1), secondPort);
    QVERIFY(node.availableFormats().isEmpty());

    // WHEN
    node.removePort(firstPort);

    // THEN
    QCOMPARE(node.type(), QShaderNode::Input);
    QCOMPARE(node.ports().size(), 1);
    QCOMPARE(node.ports().at(0), secondPort);
    QVERIFY(node.availableFormats().isEmpty());

    // WHEN
    node.setParameter(QStringLiteral("baz"), 42);

    // THEN
    QCOMPARE(node.type(), QShaderNode::Input);
    QCOMPARE(node.ports().size(), 1);
    QCOMPARE(node.ports().at(0), secondPort);
    auto parameterNames = node.parameterNames();
    parameterNames.sort();
    QCOMPARE(parameterNames.size(), 1);
    QCOMPARE(parameterNames.at(0), QStringLiteral("baz"));
    QCOMPARE(node.parameter(QStringLiteral("baz")), QVariant(42));
    QVERIFY(node.availableFormats().isEmpty());

    // WHEN
    node.setParameter(QStringLiteral("bleh"), QStringLiteral("value"));

    // THEN
    QCOMPARE(node.type(), QShaderNode::Input);
    QCOMPARE(node.ports().size(), 1);
    QCOMPARE(node.ports().at(0), secondPort);
    parameterNames = node.parameterNames();
    parameterNames.sort();
    QCOMPARE(parameterNames.size(), 2);
    QCOMPARE(parameterNames.at(0), QStringLiteral("baz"));
    QCOMPARE(parameterNames.at(1), QStringLiteral("bleh"));
    QCOMPARE(node.parameter(QStringLiteral("baz")), QVariant(42));
    QCOMPARE(node.parameter(QStringLiteral("bleh")), QVariant(QStringLiteral("value")));
    QVERIFY(node.availableFormats().isEmpty());

    // WHEN
    node.clearParameter(QStringLiteral("baz"));

    // THEN
    QCOMPARE(node.type(), QShaderNode::Input);
    QCOMPARE(node.ports().size(), 1);
    QCOMPARE(node.ports().at(0), secondPort);
    parameterNames = node.parameterNames();
    parameterNames.sort();
    QCOMPARE(parameterNames.size(), 1);
    QCOMPARE(parameterNames.at(0), QStringLiteral("bleh"));
    QCOMPARE(node.parameter(QStringLiteral("baz")), QVariant());
    QCOMPARE(node.parameter(QStringLiteral("bleh")), QVariant(QStringLiteral("value")));
    QVERIFY(node.availableFormats().isEmpty());

    // WHEN
    node.addRule(openGLES2, es2Rule);
    node.addRule(openGL3, gl3Rule);

    // THEN
    QCOMPARE(node.availableFormats().size(), 2);
    QCOMPARE(node.availableFormats().at(0), openGLES2);
    QCOMPARE(node.availableFormats().at(1), openGL3);
    QCOMPARE(node.rule(openGLES2), es2Rule);
    QCOMPARE(node.rule(openGL3), gl3Rule);

    // WHEN
    node.removeRule(openGLES2);

    // THEN
    QCOMPARE(node.availableFormats().size(), 1);
    QCOMPARE(node.availableFormats().at(0), openGL3);
    QCOMPARE(node.rule(openGL3), gl3Rule);

    // WHEN
    node.addRule(openGLES2, es2Rule);

    // THEN
    QCOMPARE(node.availableFormats().size(), 2);
    QCOMPARE(node.availableFormats().at(0), openGL3);
    QCOMPARE(node.availableFormats().at(1), openGLES2);
    QCOMPARE(node.rule(openGLES2), es2Rule);
    QCOMPARE(node.rule(openGL3), gl3Rule);

    // WHEN
    node.addRule(openGL3, gl3bisRule);

    // THEN
    QCOMPARE(node.availableFormats().size(), 2);
    QCOMPARE(node.availableFormats().at(0), openGLES2);
    QCOMPARE(node.availableFormats().at(1), openGL3);
    QCOMPARE(node.rule(openGLES2), es2Rule);
    QCOMPARE(node.rule(openGL3), gl3bisRule);
}

void tst_QShaderNodes::shouldHandleNodeRulesSupportAndOrder()
{
    // GIVEN
    const auto openGLES2 = createFormat(QShaderFormat::OpenGLES, 2, 0);
    const auto openGL3 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 0);
    const auto openGL32 = createFormat(QShaderFormat::OpenGLCoreProfile, 3, 2);
    const auto openGL4 = createFormat(QShaderFormat::OpenGLCoreProfile, 4, 0);

    const auto es2Rule = QShaderNode::Rule(QByteArrayLiteral("gles2"), {"#pragma include es2/foo.inc", "#pragma include es2/bar.inc"});
    const auto gl3Rule = QShaderNode::Rule(QByteArrayLiteral("gl3"), {"#pragma include gl3/foo.inc", "#pragma include gl3/bar.inc"});
    const auto gl32Rule = QShaderNode::Rule(QByteArrayLiteral("gl32"), {"#pragma include gl32/foo.inc", "#pragma include gl32/bar.inc"});
    const auto gl3bisRule = QShaderNode::Rule(QByteArrayLiteral("gl3bis"), {"#pragma include gl3/foo.inc", "#pragma include gl3/bar.inc"});

    auto node = QShaderNode();

    // WHEN
    node.addRule(openGLES2, es2Rule);
    node.addRule(openGL3, gl3Rule);

    // THEN
    QCOMPARE(node.availableFormats().size(), 2);
    QCOMPARE(node.availableFormats().at(0), openGLES2);
    QCOMPARE(node.availableFormats().at(1), openGL3);
    QCOMPARE(node.rule(openGLES2), es2Rule);
    QCOMPARE(node.rule(openGL3), gl3Rule);
    QCOMPARE(node.rule(openGL32), gl3Rule);
    QCOMPARE(node.rule(openGL4), gl3Rule);

    // WHEN
    node.addRule(openGL32, gl32Rule);

    // THEN
    QCOMPARE(node.availableFormats().size(), 3);
    QCOMPARE(node.availableFormats().at(0), openGLES2);
    QCOMPARE(node.availableFormats().at(1), openGL3);
    QCOMPARE(node.availableFormats().at(2), openGL32);
    QCOMPARE(node.rule(openGLES2), es2Rule);
    QCOMPARE(node.rule(openGL3), gl3Rule);
    QCOMPARE(node.rule(openGL32), gl32Rule);
    QCOMPARE(node.rule(openGL4), gl32Rule);

    // WHEN
    node.addRule(openGL3, gl3bisRule);

    // THEN
    QCOMPARE(node.availableFormats().size(), 3);
    QCOMPARE(node.availableFormats().at(0), openGLES2);
    QCOMPARE(node.availableFormats().at(1), openGL32);
    QCOMPARE(node.availableFormats().at(2), openGL3);
    QCOMPARE(node.rule(openGLES2), es2Rule);
    QCOMPARE(node.rule(openGL3), gl3bisRule);
    QCOMPARE(node.rule(openGL32), gl3bisRule);
    QCOMPARE(node.rule(openGL4), gl3bisRule);
}

QTEST_MAIN(tst_QShaderNodes)

#include "tst_qshadernodes.moc"
