/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QLoggingCategory>

#include <QtCore/private/qloggingregistry_p.h>

QT_USE_NAMESPACE
enum LoggingRuleState {
    Invalid,
    Match,
    NoMatch
};
Q_DECLARE_METATYPE(LoggingRuleState);
Q_DECLARE_METATYPE(QtMsgType);

class tst_QLoggingRegistry : public QObject
{
    Q_OBJECT

private slots:

    void initTestCase()
    {
        // ensure a clean environment
        QStandardPaths::setTestModeEnabled(true);
        qunsetenv("QT_LOGGING_CONF");
        qunsetenv("QT_LOGGING_RULES");
    }

    void QLoggingRule_parse_data()
    {
        QTest::addColumn<QString>("pattern");
        QTest::addColumn<QString>("category");
        QTest::addColumn<QtMsgType>("msgType");
        QTest::addColumn<LoggingRuleState>("result");

        // _empty_ should match (only) _empty_
        QTest::newRow("_empty_-_empty_")
                << QString("") << QString("") << QtDebugMsg << Match;
        QTest::newRow("_empty_-default")
                << QString("") << QString("default") << QtDebugMsg << NoMatch;
        QTest::newRow(".debug-_empty_")
                << QString(".debug") << QString("") << QtDebugMsg << Match;
        QTest::newRow(".warning-default")
                << QString(".warning") << QString("default") << QtDebugMsg << NoMatch;

        // literal should match only literal
        QTest::newRow("qt-qt")
                << QString("qt") << QString("qt") << QtDebugMsg << Match;
        QTest::newRow("qt-_empty_")
                << QString("qt") << QString("") << QtDebugMsg << NoMatch;
        QTest::newRow("qt-qtx")
                << QString("qt") << QString("qtx") << QtDebugMsg << NoMatch;
        QTest::newRow("qt-qt.io")
                << QString("qt") << QString("qt.io") << QtDebugMsg << NoMatch;
        QTest::newRow("qt.debug-qt")
                << QString("qt.debug") << QString("qt") << QtDebugMsg << Match;
        QTest::newRow("qt.critical-qt")
                << QString("qt.critical") << QString("qt") << QtDebugMsg << NoMatch;

        // * should match everything
        QTest::newRow("_star_-qt.io.debug")
                << QString("*") << QString("qt.io") << QtDebugMsg << Match;
        QTest::newRow("_star_-qt.io.warning")
                << QString("*") << QString("qt.io") << QtWarningMsg << Match;
        QTest::newRow("_star_-qt.io.critical")
                << QString("*") << QString("qt.io") << QtCriticalMsg << Match;
        QTest::newRow("_star_-_empty_")
                << QString("*") << QString("") << QtDebugMsg << Match;
        QTest::newRow("_star_.debug-qt.io")
                << QString("*.debug") << QString("qt.io") << QtDebugMsg << Match;
        QTest::newRow("_star_.warning-qt.io")
                << QString("*.warning") << QString("qt.io") << QtDebugMsg << NoMatch;

        // qt.* should match everything starting with 'qt.'
        QTest::newRow("qt._star_-qt.io")
                << QString("qt.*") << QString("qt.io") << QtDebugMsg << Match;
        QTest::newRow("qt._star_-qt")
                << QString("qt.*") << QString("qt") << QtDebugMsg << NoMatch;
        QTest::newRow("qt__star_-qt")
                << QString("qt*") << QString("qt") << QtDebugMsg << Match;
        QTest::newRow("qt._star_-qt.io.fs")
                << QString("qt.*") << QString("qt.io.fs") << QtDebugMsg << Match;
        QTest::newRow("qt._star_.debug-qt.io.fs")
                << QString("qt.*.debug") << QString("qt.io.fs") << QtDebugMsg << Match;
        QTest::newRow("qt._star_.warning-qt.io.fs")
                << QString("qt.*.warning") << QString("qt.io.fs") << QtDebugMsg << NoMatch;

        // *.io should match everything ending with .io
        QTest::newRow("_star_.io-qt.io")
                << QString("*.io") << QString("qt.io") << QtDebugMsg << Match;
        QTest::newRow("_star_io-qt.io")
                << QString("*io") << QString("qt.io") << QtDebugMsg << Match;
        QTest::newRow("_star_.io-io")
                << QString("*.io") << QString("io") << QtDebugMsg << NoMatch;
        QTest::newRow("_star_io-io")
                << QString("*io") << QString("io") << QtDebugMsg << Match;
        QTest::newRow("_star_.io-qt.ios")
                << QString("*.io") << QString("qt.ios") << QtDebugMsg << NoMatch;
        QTest::newRow("_star_.io-qt.io.x")
                << QString("*.io") << QString("qt.io.x") << QtDebugMsg << NoMatch;
        QTest::newRow("_star_.io.debug-qt.io")
                << QString("*.io.debug") << QString("qt.io") << QtDebugMsg << Match;
        QTest::newRow("_star_.io.warning-qt.io")
                << QString("*.io.warning") << QString("qt.io") << QtDebugMsg << NoMatch;

        // *qt* should match everything that contains 'qt'
        QTest::newRow("_star_qt_star_-qt.core.io")
                << QString("*qt*") << QString("qt.core.io") << QtDebugMsg << Match;
        QTest::newRow("_star_qt_star_-default")
                << QString("*qt*") << QString("default") << QtDebugMsg << NoMatch;
        QTest::newRow("_star_qt._star_.debug-qt.io")
                << QString("*qt.*.debug") << QString("qt.io") << QtDebugMsg << Match;
        QTest::newRow("_star_.qt._star_.warning-qt.io")
                << QString("*.qt.*.warning") << QString("qt.io") << QtDebugMsg << NoMatch;
        QTest::newRow("**")
                << QString("**") << QString("qt.core.io") << QtDebugMsg << Match;

        // * outside of start/end
        QTest::newRow("qt.*.io")
                << QString("qt.*.io") << QString("qt.core.io") << QtDebugMsg << Invalid;
        QTest::newRow("***")
                << QString("***") << QString("qt.core.io") << QtDebugMsg << Invalid;
    }

    void QLoggingRule_parse()
    {
        QFETCH(QString, pattern);
        QFETCH(QString, category);
        QFETCH(QtMsgType, msgType);
        QFETCH(LoggingRuleState, result);

        QLoggingRule rule(QStringRef(&pattern), true);
        LoggingRuleState state = Invalid;
        if (rule.flags != 0) {
            switch (rule.pass(category, msgType)) {
            case -1: QFAIL("Shoudn't happen, we set pattern to true"); break;
            case 0: state = NoMatch; break;
            case 1: state = Match; break;
            }
        }
        QCOMPARE(state, result);
    }

    void QLoggingSettingsParser_iniStyle()
    {
        //
        // Logging configuration can be described
        // in an .ini file. [Rules] is the
        // default category, and optional ...
        //
        QLoggingSettingsParser parser;
        parser.setContent("[Rules]\n"
                          "default=false\n"
                          "default=true");
        QCOMPARE(parser.rules().size(), 2);

        parser.setContent("[Rules]\n"
                          "default=false");
        QCOMPARE(parser.rules().size(), 1);

        parser.setContent("[OtherSection]\n"
                          "default=false");
        QCOMPARE(parser.rules().size(), 0);
    }

    void QLoggingRegistry_environment()
    {
        //
        // Check whether QT_LOGGING_CONF is picked up from environment
        //

        qputenv("QT_LOGGING_CONF", QFINDTESTDATA("qtlogging.ini").toLocal8Bit());

        QLoggingRegistry registry;
        registry.init();

        QCOMPARE(registry.apiRules.size(), 0);
        QCOMPARE(registry.configRules.size(), 0);
        QCOMPARE(registry.envRules.size(), 1);

        QCOMPARE(registry.rules.size(), 1);

        // check that QT_LOGGING_RULES take precedence
        qputenv("QT_LOGGING_RULES", "Digia.*=true");
        registry.init();
        QCOMPARE(registry.envRules.size(), 2);
        QCOMPARE(registry.envRules.at(1).enabled, true);
        QCOMPARE(registry.rules.size(), 2);
    }

    void QLoggingRegistry_config()
    {
        //
        // Check whether QtProject/qtlogging.ini is loaded automatically
        //

        // first try to create a test file..
        QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
        QVERIFY(!path.isEmpty());
        QDir dir(path + "/QtProject");
        if (!dir.exists())
            QVERIFY(dir.mkpath(path + "/QtProject"));

        QFile file(dir.absoluteFilePath("qtlogging.ini"));
        QVERIFY(file.open(QFile::WriteOnly | QFile::Text));
        QTextStream out(&file);
        out << "[Rules]\n";
        out << "Digia.*=false\n";
        file.close();

        QLoggingRegistry registry;
        registry.init();
        QCOMPARE(registry.configRules.size(), 1);

        // remove file again
        QVERIFY(file.remove());
    }

    void QLoggingRegistry_rulePriorities()
    {
        //
        // Rules can stem from 3 sources:
        //   via QLoggingCategory::setFilterRules (API)
        //   via qtlogging.ini file in settings (Config)
        //   via QT_LOGGING_CONF environment variable (Env)
        //
        // Rules set by environment should get higher precedence than qtlogging.conf,
        // than QLoggingCategory::setFilterRules
        //

        QLoggingCategory cat("Digia.Berlin");
        QLoggingRegistry *registry = QLoggingRegistry::instance();

        // empty all rules , check default
        registry->rules.clear();
        registry->apiRules.clear();
        registry->configRules.clear();
        registry->envRules.clear();
        registry->updateRules();

        QVERIFY(cat.isWarningEnabled());

        // set Config rule
        QLoggingSettingsParser parser;
        parser.setContent("[Rules]\nDigia.*=false");
        registry->configRules=parser.rules();
        registry->updateRules();

        QVERIFY(!cat.isWarningEnabled());

        // set API rule, should overwrite API one
        QLoggingCategory::setFilterRules("Digia.*=true");

        QVERIFY(cat.isWarningEnabled());

        // set Env rule, should overwrite Config one
        parser.setContent("Digia.*=false");
        registry->envRules=parser.rules();
        registry->updateRules();

        QVERIFY(!cat.isWarningEnabled());
    }


    void QLoggingRegistry_checkErrors()
    {
        QLoggingSettingsParser parser;
        QTest::ignoreMessage(QtWarningMsg, "Ignoring malformed logging rule: '***=false'");
        QTest::ignoreMessage(QtWarningMsg, "Ignoring malformed logging rule: '*=0'");
        QTest::ignoreMessage(QtWarningMsg, "Ignoring malformed logging rule: '*=TRUE'");
        parser.setContent("[Rules]\n"
                          "***=false\n"
                          "*=0\n"
                          "*=TRUE\n");
        QVERIFY(parser.rules().isEmpty());
    }
};

QTEST_MAIN(tst_QLoggingRegistry)

#include "tst_qloggingregistry.moc"
