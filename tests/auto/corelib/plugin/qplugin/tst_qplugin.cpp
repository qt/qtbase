/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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
#include <QtTest/QtTest>

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
    const auto fileNames = dir.entryList(QStringList() << "*debug*", QDir::Files);
    if (fileNames.isEmpty())
        QSKIP("No debug plugins found - skipping test");

    for (const QString &fileName : fileNames) {
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
    const auto fileNames = dir.entryList(QStringList() << "*release*", QDir::Files);
    if (fileNames.isEmpty())
        QSKIP("No release plugins found - skipping test");

    for (const QString &fileName : fileNames) {
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

void tst_QPlugin::scanInvalidPlugin_data()
{
    QTest::addColumn<QByteArray>("metadata");
    QTest::addColumn<bool>("loads");
    QTest::addColumn<QString>("errMsg");

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Binary JSON metadata
    QByteArray prefix = "QTMETADATA  ";

    {
        QJsonObject obj;
        obj.insert("IID", "org.qt-project.tst_qplugin");
        obj.insert("className", "tst");
        obj.insert("version", int(QT_VERSION));
#ifdef QT_NO_DEBUG
        obj.insert("debug", false);
#else
        obj.insert("debug", true);
#endif
        obj.insert("MetaData", QJsonObject());
        QTest::newRow("json-control") << (prefix + QJsonDocument(obj).toBinaryData()) << true << "";
    }

    QTest::newRow("json-zeroes") << prefix << false << " ";

    prefix += "qbjs";
    QTest::newRow("bad-json-version0") << prefix << false << " ";
    QTest::newRow("bad-json-version2") << (prefix + QByteArray("\2\0\0\0", 4)) << false << " ";

    // valid qbjs version 1
    prefix += QByteArray("\1\0\0\0");

    // too large for the file (100 MB)
    QTest::newRow("bad-json-size-large1") << (prefix + QByteArray("\0\0\x40\x06")) << false << " ";

    // too large for binary JSON (512 MB)
    QTest::newRow("bad-json-size-large2") << (prefix + QByteArray("\0\0\0\x20")) << false << " ";

    // could overflow
    QTest::newRow("bad-json-size-large3") << (prefix + "\xff\xff\xff\x7f") << false << " ";
#endif

    // CBOR metadata
    QByteArray cprefix = "QTMETADATA !1234";
    cprefix[12] = 0; // current version
    cprefix[13] = QT_VERSION_MAJOR;
    cprefix[14] = QT_VERSION_MINOR;
    cprefix[15] = qPluginArchRequirements();

    QByteArray cborValid = [] {
        QCborMap m;
        m.insert(int(QtPluginMetaDataKeys::IID), QLatin1String("org.qt-project.tst_qplugin"));
        m.insert(int(QtPluginMetaDataKeys::ClassName), QLatin1String("tst"));
        m.insert(int(QtPluginMetaDataKeys::MetaData), QCborMap());
        return QCborValue(m).toCbor();
    }();
    QTest::newRow("cbor-control") << (cprefix + cborValid) << true << "";

    cprefix[12] = 1;
    QTest::newRow("cbor-major-too-new") << (cprefix + cborValid) << false
                                        << " Invalid metadata version";

    cprefix[12] = 0;
    cprefix[13] = QT_VERSION_MAJOR + 1;
    QTest::newRow("cbor-major-too-new") << (cprefix + cborValid) << false << "";

    cprefix[13] = QT_VERSION_MAJOR - 1;
    QTest::newRow("cbor-major-too-old") << (cprefix + cborValid) << false << "";

    cprefix[13] = QT_VERSION_MAJOR;
    cprefix[14] = QT_VERSION_MINOR + 1;
    QTest::newRow("cbor-minor-too-new") << (cprefix + cborValid) << false << "";

    QTest::newRow("cbor-invalid") << (cprefix + "\xff") << false
                                  << " Metadata parsing error: Invalid CBOR stream: unexpected 'break' byte";
    QTest::newRow("cbor-not-map1") << (cprefix + "\x01") << false
                                   << " Unexpected metadata contents";
    QTest::newRow("cbor-not-map2") << (cprefix + "\x81\x01") << false
                                   << " Unexpected metadata contents";
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

    // now try to load this
    QFETCH(bool, loads);
    QFETCH(QString, errMsg);
    if (!errMsg.isEmpty())
        QTest::ignoreMessage(QtWarningMsg,
                             "Found invalid metadata in lib " + QFile::encodeName(newName) +
                             ":" + errMsg.toUtf8());
    QPluginLoader loader(newName);
    QCOMPARE(loader.load(), loads);
    if (loads)
        loader.unload();
}

QTEST_MAIN(tst_QPlugin)
#include "tst_qplugin.moc"
