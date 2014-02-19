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

class tst_QLoggingRegistry : public QObject
{
    Q_OBJECT

private slots:

    void initTestCase()
    {
        // ensure a clean environment
        QStandardPaths::setTestModeEnabled(true);
        qunsetenv("QT_LOGGING_CONF");
    }

    void QLoggingSettingsParser_iniStyle()
    {
        //
        // Logging configuration can be described
        // in an .ini file. [rules] is the
        // default category, and optional ...
        //
        QLoggingSettingsParser parser;
        parser.setContent("[rules]\n"
                          "default=false\n"
                          "default=true");
        QCOMPARE(parser.rules().size(), 2);

        parser.setContent("[rules]\n"
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
        out << "[rules]\n";
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
        parser.setContent("[rules]\nDigia.*=false");
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

};

QTEST_MAIN(tst_QLoggingRegistry)

#include "tst_qloggingregistry.moc"
