/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest/qtest.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <private/qfactoryloader_p.h>
#include "plugin1/plugininterface1.h"
#include "plugin2/plugininterface2.h"

#ifdef QT_NO_LIBRARY
Q_IMPORT_PLUGIN(Plugin1)
Q_IMPORT_PLUGIN(Plugin2)
#endif

class tst_QFactoryLoader : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void usingTwoFactoriesFromSameDir();
};

static const char binFolderC[] = "bin";

void tst_QFactoryLoader::initTestCase()
{
    const QString binFolder = QFINDTESTDATA(binFolderC);
    QVERIFY2(!binFolder.isEmpty(), "Unable to locate 'bin' folder");
#ifndef QT_NO_LIBRARY
    QCoreApplication::setLibraryPaths(QStringList(QFileInfo(binFolder).absolutePath()));
#endif
}

void tst_QFactoryLoader::usingTwoFactoriesFromSameDir()
{
    const QString suffix = QLatin1Char('/') + QLatin1String(binFolderC);
    QFactoryLoader loader1(PluginInterface1_iid, suffix);

    PluginInterface1 *plugin1 = qobject_cast<PluginInterface1 *>(loader1.instance(0));
    QVERIFY2(plugin1,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface1_iid))));

    QFactoryLoader loader2(PluginInterface2_iid, suffix);

    PluginInterface2 *plugin2 = qobject_cast<PluginInterface2 *>(loader2.instance(0));
    QVERIFY2(plugin2,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface2_iid))));

    QCOMPARE(plugin1->pluginName(), QLatin1String("Plugin1 ok"));
    QCOMPARE(plugin2->pluginName(), QLatin1String("Plugin2 ok"));
}

QTEST_MAIN(tst_QFactoryLoader)
#include "tst_qfactoryloader.moc"
