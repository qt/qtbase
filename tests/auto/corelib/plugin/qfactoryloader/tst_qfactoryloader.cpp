/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtTest/qtest.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <private/qfactoryloader_p.h>
#include "plugin1/plugininterface1.h"
#include "plugin2/plugininterface2.h"

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

    QCoreApplication::setLibraryPaths(QStringList(QFileInfo(binFolder).absolutePath()));
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
