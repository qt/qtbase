// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtCore/qfile.h>
#include <QtCore/qobject.h>
#include <QtCore/qpluginloader.h>
#include <QtPlugin>

Q_IMPORT_PLUGIN(TestStaticPlugin)

class TestInitResourcesStaticPlugin : public QObject
{
    Q_OBJECT
private slots:
    void resourceFilesExist();
};

void TestInitResourcesStaticPlugin::resourceFilesExist()
{
    bool result = false;
    for (QObject *obj : QPluginLoader::staticInstances()) {
        if (obj->metaObject()->className() == QLatin1String("TestStaticPlugin")) {
            QMetaObject::invokeMethod(obj, "checkResources", Qt::DirectConnection,
                                      Q_RETURN_ARG(bool, result));
        }
        break;
    }
    QVERIFY(result);
}

QTEST_MAIN(TestInitResourcesStaticPlugin)
#include "test_init_resources_static_plugin.moc"
