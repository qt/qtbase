// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QTest>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <qplugin.h>
#include <QPluginLoader>

#include <private/qplugin_p.h>

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
    void scanInvalidPlugin_data();
    void scanInvalidPlugin();
};

tst_QPlugin::tst_QPlugin()
{
    // On Android the plugins must be located in the APK's libs subdir
#ifndef Q_OS_ANDROID
    dir = QFINDTESTDATA("plugins");
#else
    const QStringList paths = QCoreApplication::libraryPaths();
    if (!paths.isEmpty())
        dir = paths.first();
#endif
}

void tst_QPlugin::initTestCase()
{
    QVERIFY2(dir.exists(),
             qPrintable(QString::fromLatin1("Cannot find the 'plugins' directory starting from '%1'").
                        arg(QDir::toNativeSeparators(QDir::currentPath()))));
}

void tst_QPlugin::loadDebugPlugin()
{
    const auto fileNames = dir.entryList(QStringList() << "*debug*", QDir::Files);
    if (fileNames.isEmpty())
        QSKIP("No debug plugins found - skipping test");

    for (const QString &fileName : fileNames) {
        if (!QLibrary::isLibrary(fileName))
            continue;
        QPluginLoader loader(dir.filePath(fileName));
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
        // we can always load a plugin on unix
        QVERIFY(loader.load());
        QObject *object = loader.instance();
        QVERIFY(object != 0);
#else
#  if defined(CMAKE_BUILD) && defined(QT_NO_DEBUG)
    QSKIP("Skipping test as it is not possible to disable build targets based on configuration with CMake");
#  endif
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
    const auto fileNames = dir.entryList(QStringList() << "*release*", QDir::Files);
    if (fileNames.isEmpty())
        QSKIP("No release plugins found - skipping test");

    for (const QString &fileName : fileNames) {
        if (!QLibrary::isLibrary(fileName))
            continue;
        QPluginLoader loader(dir.filePath(fileName));
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
        // we can always load a plugin on unix
        QVERIFY(loader.load());
        QObject *object = loader.instance();
        QVERIFY(object != 0);
#else
#  if defined(CMAKE_BUILD) && !defined(QT_NO_DEBUG)
    QSKIP("Skipping test as it is not possible to disable build targets based on configuration with CMake");
#  endif
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

void tst_QPlugin::scanInvalidPlugin_data()
{
    QTest::addColumn<QByteArray>("metadata");
    QTest::addColumn<bool>("loads");
    QTest::addColumn<QString>("errMsg");

    // CBOR metadata
    static constexpr QPluginMetaData::MagicHeader header = {};
    static constexpr qsizetype MagicLen = sizeof(header.magic);
    QByteArray cprefix(reinterpret_cast<const char *>(&header), sizeof(header));

    QByteArray cborValid = [] {
        QCborMap m;
        m.insert(int(QtPluginMetaDataKeys::IID), QLatin1String("org.qt-project.tst_qplugin"));
        m.insert(int(QtPluginMetaDataKeys::ClassName), QLatin1String("tst"));
        m.insert(int(QtPluginMetaDataKeys::MetaData), QCborMap());
        return QCborValue(m).toCbor();
    }();
    QTest::newRow("cbor-control") << (cprefix + cborValid) << true << "";

    cprefix[MagicLen + 1] = QT_VERSION_MAJOR + 1;
    QTest::newRow("cbor-major-too-new") << (cprefix + cborValid) << false << "";

    cprefix[MagicLen + 1] = QT_VERSION_MAJOR - 1;
    QTest::newRow("cbor-major-too-old") << (cprefix + cborValid) << false << "";

    cprefix[MagicLen + 1] = QT_VERSION_MAJOR;
    cprefix[MagicLen + 2] = QT_VERSION_MINOR + 1;
    QTest::newRow("cbor-minor-too-new") << (cprefix + cborValid) << false << "";

    cprefix[MagicLen + 2] = QT_VERSION_MINOR;
    QTest::newRow("cbor-invalid") << (cprefix + "\xff") << false
                                  << " Metadata parsing error: Invalid CBOR stream: unexpected 'break' byte";
    QTest::newRow("cbor-not-map1") << (cprefix + "\x01") << false
                                   << " Unexpected metadata contents";
    QTest::newRow("cbor-not-map2") << (cprefix + "\x81\x01") << false
                                   << " Unexpected metadata contents";

    ++cprefix[MagicLen + 0];
    QTest::newRow("cbor-major-too-new-invalid")
        << (cprefix + cborValid) << false << " Invalid metadata version";
}

static const char invalidPluginSignature[] = "qplugin testfile";
static qsizetype locateMetadata(const uchar *data, qsizetype len)
{
    const uchar *dataend = data + len - strlen(invalidPluginSignature);

    for (const uchar *ptr = data; ptr < dataend; ++ptr) {
        if (*ptr != invalidPluginSignature[0])
            continue;

        int r = memcmp(ptr, invalidPluginSignature, strlen(invalidPluginSignature));
        if (r)
            continue;

        return ptr - data;
    }

    return -1;
}

void tst_QPlugin::scanInvalidPlugin()
{
#if defined(Q_OS_MACOS) && defined(Q_PROCESSOR_ARM)
    QSKIP("This test crashes on ARM macOS");
#endif
    const auto fileNames = dir.entryList({"*invalid*"}, QDir::Files);
    QString invalidPluginName;
    if (fileNames.isEmpty())
        QSKIP("No invalid plugin found - skipping test");
    else
        invalidPluginName = dir.absoluteFilePath(fileNames.first());


    // copy the file
    QFileInfo fn(invalidPluginName);
    QTemporaryDir tmpdir;
    QVERIFY(tmpdir.isValid());

    QString newName = tmpdir.path() + '/' + fn.fileName();
    QVERIFY(QFile::copy(invalidPluginName, newName));

    {
        QFile f(newName);
        QVERIFY(f.open(QIODevice::ReadWrite | QIODevice::Unbuffered));
        QVERIFY(f.size() > qint64(strlen(invalidPluginSignature)));
        uchar *data = f.map(0, f.size());
        QVERIFY(data);

        static const qsizetype offset = locateMetadata(data, f.size());
        QVERIFY(offset > 0);

        QFETCH(QByteArray, metadata);

        // sanity check
        QVERIFY(metadata.size() < 512);

        // replace the data
        memcpy(data + offset, metadata.constData(), metadata.size());
        memset(data + offset + metadata.size(), 0, 512 - metadata.size());
    }

#if defined(Q_OS_QNX)
    // On QNX plugin access is still too early
    QTest::qSleep(1000);
#endif

    // now try to load this
    QFETCH(bool, loads);
    QFETCH(QString, errMsg);
    QPluginLoader loader(newName);
    QCOMPARE(loader.load(), loads);
    if (loads)
        loader.unload();
}

QTEST_MAIN(tst_QPlugin)
#include "tst_qplugin.moc"
