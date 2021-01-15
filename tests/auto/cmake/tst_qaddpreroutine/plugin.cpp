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

#include <qpa/qplatformthemeplugin.h>
#include <qpa/qplatformtheme.h>
#include <QCoreApplication>

QT_BEGIN_NAMESPACE

// The "test_function_call_count" property will be used in the "tst_qaddpreroutine.cpp".
// This plugin is part of the test case. It is used to call qAddPreRoutine through
// Q_COREAPP_STARTUP_FUNCTION on the plugin loading. Please treat it as a whole
// with "tst_qaddpreroutine.cpp".
static void test()
{
    Q_ASSERT(qApp != nullptr);
    int call_count = qApp->property("test_function_call_count").toInt();
    // Record the number of times A is called, in this example, it should be called only once.
    qApp->setProperty("test_function_call_count", call_count + 1);
}
Q_COREAPP_STARTUP_FUNCTION(test)

class ThemePlugin : public QPlatformThemePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformThemeFactoryInterface_iid FILE "plugin.json")

public:
    QPlatformTheme *create(const QString &key, const QStringList &params) override;
};

QPlatformTheme *ThemePlugin::create(const QString &key, const QStringList &params)
{
    Q_UNUSED(key)
    Q_UNUSED(params);

    // Used to verify whether this plugin was successfully loaded.
    qputenv("QTBUG_90341_ThemePlugin", "1");

    return new QPlatformTheme();
}

QT_END_NAMESPACE

#include "plugin.moc"
