/****************************************************************************
**
** Copyright (C) 2021 zccrs <zccrs@live.com>, JiDe Zhang <zhangjide@uniontech.com>.
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

#include <QTest>
#include <QGuiApplication>

class tst_qAddPreRoutine : public QObject
{
    Q_OBJECT

public:
    static void initMain()
    {
        // The purpose of this use case is indeed to test "qAddPreRoutine", but
        // as you can see, there is nowhere to call "qAddPreRoutine". Please see
        // the following two lines of code, which set the "QT_QPA_PLATFORM_PLUGIN_PATH"
        // and "QT_QPA_PLATFORMTHEME" environment variables that a new platform
        // theme plugin will be loaded, and the Q_COREAPP_STARTUP_FUNCTION macro
        // is used in this plugin, which will cause "qAddPreRoutine" to be called
        // indirectly in the Q*Application class when load the platform theme plugin.
        // See the "plugin.cpp" file.
#ifndef Q_OS_ANDROID // The plug-in is in the apk package, no need to specify its directory
        qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", QT_QPA_PLATFORM_PLUGIN_PATH);
#endif
        qputenv("QT_QPA_PLATFORMTHEME", "QTBUG_90341");
    }

private slots:
    void tst_QTBUG_90341()
    {
#ifdef Q_OS_ANDROID
        QSKIP("Android can't load the platform theme plugin this test needs, see QTBUG-92893");
#endif
        QVERIFY2(qEnvironmentVariableIsSet("QTBUG_90341_ThemePlugin"),
                 "The \"QTBUG_90341\" theme plugin not loaded.");
        // This "test_function_call_count" property is assigned in the "QTBUG_90341" plugin.
        // See the "plugin.cpp" file.
        QCOMPARE(qApp->property("test_function_call_count").toInt(), 1);
    }
};

QTEST_MAIN(tst_qAddPreRoutine)

#include "tst_qaddpreroutine.moc"
