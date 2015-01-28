/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QPluginLoader>

class tst_QPlugin : public QObject
{
    Q_OBJECT

    QDir dir;

public:
    tst_QPlugin();

private slots:
    void initTestCase();
    void loadDebugPlugin();
    void loadReleasePlugin();
};

tst_QPlugin::tst_QPlugin()
    : dir(QFINDTESTDATA("plugins"))
{
}

void tst_QPlugin::initTestCase()
{
    QVERIFY2(dir.exists(),
             qPrintable(QString::fromLatin1("Cannot find the 'plugins' directory starting from '%1'").
                        arg(QDir::toNativeSeparators(QDir::currentPath()))));
}

void tst_QPlugin::loadDebugPlugin()
{
    foreach (QString fileName, dir.entryList(QStringList() << "*debug*", QDir::Files)) {
        if (!QLibrary::isLibrary(fileName))
            continue;
        QPluginLoader loader(dir.filePath(fileName));
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        // we can always load a plugin on unix
        QVERIFY(loader.load());
        QObject *object = loader.instance();
        QVERIFY(object != 0);
#else
        // loading a plugin is dependent on which lib we are running against
#  if defined(QT_NO_DEBUG)
        // release build, we cannot load debug plugins
        QVERIFY(!loader.load());
#  else
        // debug build, we can load debug plugins
        QVERIFY(loader.load());
        QObject *object = loader.instance();
        QVERIFY(object != 0);
#  endif
#endif
    }
}

void tst_QPlugin::loadReleasePlugin()
{
    foreach (QString fileName, dir.entryList(QStringList() << "*release*", QDir::Files)) {
        if (!QLibrary::isLibrary(fileName))
            continue;
        QPluginLoader loader(dir.filePath(fileName));
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        // we can always load a plugin on unix
        QVERIFY(loader.load());
        QObject *object = loader.instance();
        QVERIFY(object != 0);
#else
        // loading a plugin is dependent on which lib we are running against
#  if defined(QT_NO_DEBUG)
        // release build, we can load debug plugins
        QVERIFY(loader.load());
        QObject *object = loader.instance();
        QVERIFY(object != 0);
#  else
        // debug build, we cannot load debug plugins
        QVERIFY(!loader.load());
#  endif
#endif
    }
}

QTEST_MAIN(tst_QPlugin)
#include "tst_qplugin.moc"
