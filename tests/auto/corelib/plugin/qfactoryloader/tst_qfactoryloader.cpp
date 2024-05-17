// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qplugin.h>
#include <QtCore/qversionnumber.h>
#include <private/qfactoryloader_p.h>
#include <private/qlibrary_p.h>
#include "plugin1/plugininterface1.h"
#include "plugin2/plugininterface2.h"

#if !QT_CONFIG(library)
Q_IMPORT_PLUGIN(Plugin1)
Q_IMPORT_PLUGIN(Plugin2)
#endif

class tst_QFactoryLoader : public QObject
{
    Q_OBJECT

    QString binFolder;
public slots:
    void initTestCase();

private slots:
    void usingTwoFactoriesFromSameDir();
    void extraSearchPath();
    void multiplePaths();
    void staticPlugin_data();
    void staticPlugin();
};

static const char binFolderC[] = "bin";

void tst_QFactoryLoader::initTestCase()
{
    // On Android the plugins are bundled into APK's libs subdir
#ifndef Q_OS_ANDROID
    binFolder = QFINDTESTDATA(binFolderC);
    QVERIFY2(!binFolder.isEmpty(), "Unable to locate 'bin' folder");
#endif
}

void tst_QFactoryLoader::usingTwoFactoriesFromSameDir()
{
#if QT_CONFIG(library) && !defined(Q_OS_ANDROID)
    // set the library path to contain the directory where the 'bin' dir is located
    QCoreApplication::setLibraryPaths( { QFileInfo(binFolder).absolutePath() });
#endif
    auto versionNumber = [](const QCborValue &value) {
        // Qt plugins only store major & minor versions in the metadata, so
        // the low 8 bits are always zero.
        qint64 v = value.toInteger();
        return QVersionNumber(v >> 16, uchar(v >> 8));
    };
    QVersionNumber qtVersion(QT_VERSION_MAJOR, 0);

    const QString suffix = QLatin1Char('/') + QLatin1String(binFolderC);
    QFactoryLoader loader1(PluginInterface1_iid, suffix);
    const QFactoryLoader::MetaDataList list1 = loader1.metaData();
    const QList<QCborArray> keys1 = loader1.metaDataKeys();
    QCOMPARE(list1.size(), 1);
    QCOMPARE(keys1.size(), 1);
    QCOMPARE_GE(versionNumber(list1[0].value(QtPluginMetaDataKeys::QtVersion)), qtVersion);
    QCOMPARE(list1[0].value(QtPluginMetaDataKeys::IID), PluginInterface1_iid);
    QCOMPARE(list1[0].value(QtPluginMetaDataKeys::ClassName), "Plugin1");

    // plugin1's Q_PLUGIN_METADATA has FILE "plugin1.json"
    QCborValue metadata1 = list1[0].value(QtPluginMetaDataKeys::MetaData);
    QCOMPARE(metadata1.type(), QCborValue::Map);
    QCOMPARE(metadata1["Keys"], QCborArray{ "plugin1" });
    QCOMPARE(keys1[0], QCborArray{ "plugin1" });
    QCOMPARE(loader1.indexOf("Plugin1"), 0);
    QCOMPARE(loader1.indexOf("PLUGIN1"), 0);
    QCOMPARE(loader1.indexOf("Plugin2"), -1);

    QFactoryLoader loader2(PluginInterface2_iid, suffix);
    const QFactoryLoader::MetaDataList list2 = loader2.metaData();
    const QList<QCborArray> keys2 = loader2.metaDataKeys();
    QCOMPARE(list2.size(), 1);
    QCOMPARE(keys2.size(), 1);
    QCOMPARE_GE(versionNumber(list2[0].value(QtPluginMetaDataKeys::QtVersion)), qtVersion);
    QCOMPARE(list2[0].value(QtPluginMetaDataKeys::IID), PluginInterface2_iid);
    QCOMPARE(list2[0].value(QtPluginMetaDataKeys::ClassName), "Plugin2");

    // plugin2's Q_PLUGIN_METADATA does not have FILE
    QCOMPARE(list2[0].value(QtPluginMetaDataKeys::MetaData), QCborValue());
    QCOMPARE(keys2[0], QCborArray());
    QCOMPARE(loader2.indexOf("Plugin1"), -1);
    QCOMPARE(loader2.indexOf("Plugin2"), -1);

    QObject *obj1 = loader1.instance(0);
    PluginInterface1 *plugin1 = qobject_cast<PluginInterface1 *>(obj1);
    QVERIFY2(plugin1,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface1_iid))));
    QCOMPARE(obj1->metaObject()->className(), "Plugin1");

    QObject *obj2 = loader2.instance(0);
    PluginInterface2 *plugin2 = qobject_cast<PluginInterface2 *>(obj2);
    QVERIFY2(plugin2,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface2_iid))));
    QCOMPARE(obj2->metaObject()->className(), "Plugin2");

    QCOMPARE(plugin1->pluginName(), QLatin1String("Plugin1 ok"));
    QCOMPARE(plugin2->pluginName(), QLatin1String("Plugin2 ok"));
}

void tst_QFactoryLoader::extraSearchPath()
{
#if defined(Q_OS_ANDROID) && !QT_CONFIG(library)
    QSKIP("Test not applicable in this configuration.");
#else
#ifdef Q_OS_ANDROID
    // On Android the libs are not stored in binFolder, but bundled into
    // APK's libs subdir
    const QStringList androidLibsPaths = QCoreApplication::libraryPaths();
    QCOMPARE(androidLibsPaths.size(), 1);
#endif
    QCoreApplication::setLibraryPaths(QStringList());

#ifndef Q_OS_ANDROID
    QString pluginsPath = QFileInfo(binFolder).absoluteFilePath();
    QFactoryLoader loader1(PluginInterface1_iid, "/nonexistent");
#else
    QString pluginsPath = androidLibsPaths.first();
    // On Android we still need to specify a valid suffix, because it's a part
    // of a file name, not directory structure
    const QString suffix = QLatin1Char('/') + QLatin1String(binFolderC);
    QFactoryLoader loader1(PluginInterface1_iid, suffix);
#endif

    // it shouldn't have scanned anything because we haven't given it a path yet
    QVERIFY(loader1.metaData().isEmpty());

    loader1.setExtraSearchPath(pluginsPath);
    PluginInterface1 *plugin1 = qobject_cast<PluginInterface1 *>(loader1.instance(0));
    QVERIFY2(plugin1,
             qPrintable(QString::fromLatin1("Cannot load plugin '%1'")
                        .arg(QLatin1String(PluginInterface1_iid))));

    QCOMPARE(plugin1->pluginName(), QLatin1String("Plugin1 ok"));

    // check that it forgets that plugin
    loader1.setExtraSearchPath(QString());
    QVERIFY(loader1.metaData().isEmpty());
#endif
}

void tst_QFactoryLoader::multiplePaths()
{
#if !QT_CONFIG(library) || !(defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)) || defined(Q_OS_ANDROID)
    QSKIP("Test not applicable in this configuration.");
#else
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString pluginsPath = QFileInfo(binFolder, binFolderC).absolutePath();
    QString linkPath = dir.filePath(binFolderC);
    QVERIFY(QFile::link(pluginsPath, linkPath));

    QCoreApplication::setLibraryPaths({ QFileInfo(binFolder).absolutePath(), dir.path() });

    const QString suffix = QLatin1Char('/') + QLatin1String(binFolderC);
    QFactoryLoader loader1(PluginInterface1_iid, suffix);

    QLibraryPrivate *library1 = loader1.library("plugin1");
    QVERIFY(library1);
    QCOMPARE(library1->loadHints(), QLibrary::PreventUnloadHint);
#endif
}

Q_IMPORT_PLUGIN(StaticPlugin1)
Q_IMPORT_PLUGIN(StaticPlugin2)
constexpr bool IsDebug =
#ifdef QT_NO_DEBUG
        false &&
#endif
        true;

void tst_QFactoryLoader::staticPlugin_data()
{
    QTest::addColumn<QString>("iid");
    auto addRow = [](const char *iid) {
        QTest::addRow("%s", iid) << QString(iid);
    };
    addRow("StaticPlugin1");
    addRow("StaticPlugin2");
}

void tst_QFactoryLoader::staticPlugin()
{
    QFETCH(QString, iid);
    QFactoryLoader loader(iid.toLatin1(), "/irrelevant");
    QFactoryLoader::MetaDataList list = loader.metaData();
    QCOMPARE(list.size(), 1);

    QCborMap map = list.at(0).toCbor();
    QCOMPARE(map[int(QtPluginMetaDataKeys::QtVersion)],
            QT_VERSION_CHECK(QT_VERSION_MAJOR, QT_VERSION_MINOR, 0));
    QCOMPARE(map[int(QtPluginMetaDataKeys::IID)], iid);
    QCOMPARE(map[int(QtPluginMetaDataKeys::ClassName)], iid);
    QCOMPARE(map[int(QtPluginMetaDataKeys::IsDebug)], IsDebug);

    QCborValue metaData = map[int(QtPluginMetaDataKeys::MetaData)];
    QVERIFY(metaData.isMap());
    QCOMPARE(metaData["Keys"], QCborArray{ "Value" });
    QCOMPARE(loader.metaDataKeys(), QList{ QCborArray{ "Value" } });
    QCOMPARE(loader.indexOf("Value"), 0);

    // instantiate
    QObject *instance = loader.instance(0);
    QVERIFY(instance);
    QCOMPARE(instance->metaObject()->className(), iid);
}

QTEST_MAIN(tst_QFactoryLoader)
#include "tst_qfactoryloader.moc"
