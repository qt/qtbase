/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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
#include <qdir.h>
#include <qpluginloader.h>
#include "theplugin/plugininterface.h"

#if defined(QT_BUILD_INTERNAL) && defined(Q_OF_MACH_O)
#  include <QtCore/private/qmachparser_p.h>
#endif

// Helper macros to let us know if some suffixes are valid
#define bundle_VALID    false
#define dylib_VALID     false
#define sl_VALID        false
#define a_VALID         false
#define so_VALID        false
#define dll_VALID       false

#if defined(Q_OS_DARWIN)
# undef bundle_VALID
# undef dylib_VALID
# undef so_VALID
# define bundle_VALID   true
# define dylib_VALID    true
# define so_VALID       true
//# ifdef QT_NO_DEBUG
#  define SUFFIX         ".dylib"
//# else
//#  define SUFFIX         "_debug.dylib"
//#endif
# define PREFIX         "lib"

#elif defined(Q_OS_HPUX) && !defined(__ia64)
# undef sl_VALID
# define sl_VALID       true
# define SUFFIX         ".sl"
# define PREFIX         "lib"

#elif defined(Q_OS_AIX)
# undef a_VALID
# undef so_VALID
# define a_VALID        true
# define so_VALID       true
# define SUFFIX         ".so"
# define PREFIX         "lib"

#elif defined(Q_OS_WIN)
# undef dll_VALID
# define dll_VALID      true
//# ifdef QT_NO_DEBUG
#  define SUFFIX         ".dll"
//# else
//#  define SUFFIX         "d.dll"
//# endif
# define PREFIX         ""

#else  // all other Unix
# undef so_VALID
# define so_VALID       true
# define SUFFIX         ".so"
# define PREFIX         "lib"
#endif

static QString sys_qualifiedLibraryName(const QString &fileName)
{
    QString name = QLatin1String("bin/") + QLatin1String(PREFIX) + fileName + QLatin1String(SUFFIX);
    const QString libname = QFINDTESTDATA(name);
    QFileInfo fi(libname);
    if (fi.exists())
        return fi.canonicalFilePath();
    return libname;
}

QT_FORWARD_DECLARE_CLASS(QPluginLoader)
class tst_QPluginLoader : public QObject
{
    Q_OBJECT
public slots:
    void cleanup();
private slots:
    void errorString();
    void loadHints();
    void deleteinstanceOnUnload();
    void loadDebugObj();
    void loadCorruptElf();
    void loadMachO_data();
    void loadMachO();
#if defined (Q_OS_UNIX)
    void loadGarbage();
#endif
    void relativePath();
    void absolutePath();
    void reloadPlugin();
    void preloadedPlugin_data();
    void preloadedPlugin();
};

void tst_QPluginLoader::cleanup()
{
    // check if the library/plugin was leaked
    // we can't use QPluginLoader::isLoaded here because on some platforms the plugin is always loaded by QPluginLoader.
    // Also, if this test fails once, it will keep on failing because we can't force the unload,
    // so we report it only once.
    static bool failedAlready = false;
    if (!failedAlready) {
        QLibrary lib(sys_qualifiedLibraryName("theplugin"));
        failedAlready = true;
        QVERIFY2(!lib.isLoaded(), "Plugin was leaked - will not check again");
        failedAlready = false;
    }
}

void tst_QPluginLoader::errorString()
{
    const QString unknown(QLatin1String("Unknown error"));

    {
    QPluginLoader loader; // default constructed
    bool loaded = loader.load();
    QCOMPARE(loader.errorString(), unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QCOMPARE(loader.errorString(), unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QCOMPARE(loader.errorString(), unknown);
    QVERIFY(!unloaded);
    }
    {
    QPluginLoader loader( sys_qualifiedLibraryName("tst_qpluginloaderlib"));     //not a plugin
    bool loaded = loader.load();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QVERIFY(loader.errorString() != unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!unloaded);
    }

    {
    QPluginLoader loader( sys_qualifiedLibraryName("nosuchfile"));     //not a file
    bool loaded = loader.load();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QVERIFY(loader.errorString() != unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!unloaded);
    }

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC) && !defined(Q_OS_HPUX)
    {
    QPluginLoader loader( sys_qualifiedLibraryName("almostplugin"));     //a plugin with unresolved symbols
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    bool loaded = loader.load();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!loaded);

    QObject *obj = loader.instance();
    QVERIFY(loader.errorString() != unknown);
    QCOMPARE(obj, static_cast<QObject*>(0));

    bool unloaded = loader.unload();
    QVERIFY(loader.errorString() != unknown);
    QVERIFY(!unloaded);
    }
#endif

    {
    QPluginLoader loader( sys_qualifiedLibraryName("theplugin"));     //a plugin
    QCOMPARE(loader.load(), true);
    QCOMPARE(loader.errorString(), unknown);

    QVERIFY(loader.instance() !=  static_cast<QObject*>(0));
    QCOMPARE(loader.errorString(), unknown);

    // Make sure that plugin really works
    PluginInterface* theplugin = qobject_cast<PluginInterface*>(loader.instance());
    QString pluginName = theplugin->pluginName();
    QCOMPARE(pluginName, QLatin1String("Plugin ok"));

    QCOMPARE(loader.unload(), true);
    QCOMPARE(loader.errorString(), unknown);
    }
}

void tst_QPluginLoader::loadHints()
{
    QPluginLoader loader;
    QCOMPARE(loader.loadHints(), (QLibrary::LoadHints)0);   //Do not crash
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    loader.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
    QCOMPARE(loader.loadHints(), QLibrary::ResolveAllSymbolsHint);
}

void tst_QPluginLoader::deleteinstanceOnUnload()
{
    for (int pass = 0; pass < 2; ++pass) {
        QPluginLoader loader1;
        loader1.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
        if (pass == 0)
            loader1.load(); // not recommended, instance() should do the job.
        PluginInterface *instance1 = qobject_cast<PluginInterface*>(loader1.instance());
        QVERIFY(instance1);
        QCOMPARE(instance1->pluginName(), QLatin1String("Plugin ok"));

        QPluginLoader loader2;
        loader2.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
        if (pass == 0)
            loader2.load(); // not recommended, instance() should do the job.
        PluginInterface *instance2 = qobject_cast<PluginInterface*>(loader2.instance());
        QCOMPARE(instance2->pluginName(), QLatin1String("Plugin ok"));

        QSignalSpy spy1(loader1.instance(), &QObject::destroyed);
        QSignalSpy spy2(loader2.instance(), &QObject::destroyed);
        QVERIFY(spy1.isValid());
        QVERIFY(spy2.isValid());
        if (pass == 0) {
            QCOMPARE(loader2.unload(), false);  // refcount not reached 0, not really unloaded
            QCOMPARE(spy1.count(), 0);
            QCOMPARE(spy2.count(), 0);
        }
        QCOMPARE(instance1->pluginName(), QLatin1String("Plugin ok"));
        QCOMPARE(instance2->pluginName(), QLatin1String("Plugin ok"));
        QVERIFY(loader1.unload());   // refcount reached 0, did really unload
        QCOMPARE(spy1.count(), 1);
        QCOMPARE(spy2.count(), 1);
    }
}

void tst_QPluginLoader::loadDebugObj()
{
#if defined (__ELF__)
    QVERIFY(QFile::exists(QFINDTESTDATA("elftest/debugobj.so")));
    QPluginLoader lib1(QFINDTESTDATA("elftest/debugobj.so"));
    QCOMPARE(lib1.load(), false);
#endif
}

void tst_QPluginLoader::loadCorruptElf()
{
#if defined (__ELF__)
    if (sizeof(void*) == 8) {
        QVERIFY(QFile::exists(QFINDTESTDATA("elftest/corrupt1.elf64.so")));

        QPluginLoader lib1(QFINDTESTDATA("elftest/corrupt1.elf64.so"));
        QCOMPARE(lib1.load(), false);
        QVERIFY2(lib1.errorString().contains("not an ELF object"), qPrintable(lib1.errorString()));

        QPluginLoader lib2(QFINDTESTDATA("elftest/corrupt2.elf64.so"));
        QCOMPARE(lib2.load(), false);
        QVERIFY2(lib2.errorString().contains("invalid"), qPrintable(lib2.errorString()));

        QPluginLoader lib3(QFINDTESTDATA("elftest/corrupt3.elf64.so"));
        QCOMPARE(lib3.load(), false);
        QVERIFY2(lib3.errorString().contains("invalid"), qPrintable(lib3.errorString()));
    } else if (sizeof(void*) == 4) {
        QPluginLoader libW(QFINDTESTDATA("elftest/corrupt3.elf64.so"));
        QCOMPARE(libW.load(), false);
        QVERIFY2(libW.errorString().contains("architecture"), qPrintable(libW.errorString()));
    } else {
        QFAIL("Please port QElfParser to this platform or blacklist this test.");
    }
#endif
}

void tst_QPluginLoader::loadMachO_data()
{
#if defined(QT_BUILD_INTERNAL) && defined(Q_OF_MACH_O)
    QTest::addColumn<int>("parseResult");

    QTest::newRow("/dev/null") << int(QMachOParser::NotSuitable);
    QTest::newRow("elftest/debugobj.so") << int(QMachOParser::NotSuitable);
    QTest::newRow("tst_qpluginloader.cpp") << int(QMachOParser::NotSuitable);
    QTest::newRow("tst_qpluginloader") << int(QMachOParser::NotSuitable);

#  ifdef Q_PROCESSOR_X86_64
    QTest::newRow("machtest/good.x86_64.dylib") << int(QMachOParser::QtMetaDataSection);
    QTest::newRow("machtest/good.i386.dylib") << int(QMachOParser::NotSuitable);
    QTest::newRow("machtest/good.fat.no-x86_64.dylib") << int(QMachOParser::NotSuitable);
    QTest::newRow("machtest/good.fat.no-i386.dylib") << int(QMachOParser::QtMetaDataSection);
#  elif defined(Q_PROCESSOR_X86_32)
    QTest::newRow("machtest/good.i386.dylib") << int(QMachOParser::QtMetaDataSection);
    QTest::newRow("machtest/good.x86_64.dylib") << int(QMachOParser::NotSuitable);
    QTest::newRow("machtest/good.fat.no-i386.dylib") << int(QMachOParser::NotSuitable);
    QTest::newRow("machtest/good.fat.no-x86_64.dylib") << int(QMachOParser::QtMetaDataSection);
#  endif
#  ifndef Q_PROCESSOR_POWER_64
    QTest::newRow("machtest/good.ppc64.dylib") << int(QMachOParser::NotSuitable);
#  endif

    QTest::newRow("machtest/good.fat.all.dylib") << int(QMachOParser::QtMetaDataSection);
    QTest::newRow("machtest/good.fat.stub-x86_64.dylib") << int(QMachOParser::NotSuitable);
    QTest::newRow("machtest/good.fat.stub-i386.dylib") << int(QMachOParser::NotSuitable);

    QDir d(QFINDTESTDATA("machtest"));
    QStringList badlist = d.entryList(QStringList() << "bad*.dylib");
    foreach (const QString &bad, badlist)
        QTest::newRow(qPrintable("machtest/" + bad)) << int(QMachOParser::NotSuitable);
#endif
}

void tst_QPluginLoader::loadMachO()
{
#if defined(QT_BUILD_INTERNAL) && defined(Q_OF_MACH_O)
    QFile f(QFINDTESTDATA(QTest::currentDataTag()));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();

    qsizetype pos;
    qsizetype len;
    QString errorString;
    int r = QMachOParser::parse(data.constData(), data.size(), f.fileName(), &errorString, &pos, &len);

    QFETCH(int, parseResult);
    QCOMPARE(r, parseResult);

    if (r == QMachOParser::NotSuitable)
        return;

    QVERIFY(pos > 0);
    QVERIFY(len >= sizeof(void*));
    QVERIFY(pos + long(len) < data.size());
    QCOMPARE(pos & (sizeof(void*) - 1), 0UL);

    void *value = *(void**)(data.constData() + pos);
    QCOMPARE(value, sizeof(void*) > 4 ? (void*)(0xc0ffeec0ffeeL) : (void*)0xc0ffee);

    // now that we know it's valid, let's try to make it invalid
    ulong offeredlen = pos;
    do {
        --offeredlen;
        r = QMachOParser::parse(data.constData(), offeredlen, f.fileName(), &errorString, &pos, &len);
        QVERIFY2(r == QMachOParser::NotSuitable, qPrintable(QString("Failed at size 0x%1").arg(offeredlen, 0, 16)));
    } while (offeredlen);
#endif
}

#if defined (Q_OS_UNIX)
void tst_QPluginLoader::loadGarbage()
{
    for (int i=0; i<5; i++) {
        const QString name = QLatin1String("elftest/garbage") + QString::number(i + 1) + QLatin1String(".so");
        QPluginLoader lib(QFINDTESTDATA(name));
        QCOMPARE(lib.load(), false);
        QVERIFY(lib.errorString() != QString("Unknown error"));
    }
}
#endif

void tst_QPluginLoader::relativePath()
{
    // Windows binaries run from release and debug subdirs, so we can't rely on the current dir.
    const QString binDir = QFINDTESTDATA("bin");
    QVERIFY(!binDir.isEmpty());
    QCoreApplication::addLibraryPath(binDir);
    QPluginLoader loader("theplugin");
    loader.load(); // not recommended, instance() should do the job.
    PluginInterface *instance = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance);
    QCOMPARE(instance->pluginName(), QLatin1String("Plugin ok"));
    QVERIFY(loader.unload());
}

void tst_QPluginLoader::absolutePath()
{
    // Windows binaries run from release and debug subdirs, so we can't rely on the current dir.
    const QString binDir = QFINDTESTDATA("bin");
    QVERIFY(!binDir.isEmpty());
    QVERIFY(QDir::isAbsolutePath(binDir));
    QPluginLoader loader(binDir + "/theplugin");
    loader.load(); // not recommended, instance() should do the job.
    PluginInterface *instance = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance);
    QCOMPARE(instance->pluginName(), QLatin1String("Plugin ok"));
    QVERIFY(loader.unload());
}

void tst_QPluginLoader::reloadPlugin()
{
    QPluginLoader loader;
    loader.setFileName( sys_qualifiedLibraryName("theplugin"));     //a plugin
    loader.load(); // not recommended, instance() should do the job.
    PluginInterface *instance = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance);
    QCOMPARE(instance->pluginName(), QLatin1String("Plugin ok"));

    QSignalSpy spy(loader.instance(), &QObject::destroyed);
    QVERIFY(spy.isValid());
    QVERIFY(loader.unload());   // refcount reached 0, did really unload
    QCOMPARE(spy.count(), 1);

    // reload plugin
    QVERIFY(loader.load());
    QVERIFY(loader.isLoaded());

    PluginInterface *instance2 = qobject_cast<PluginInterface*>(loader.instance());
    QVERIFY(instance2);
    QCOMPARE(instance2->pluginName(), QLatin1String("Plugin ok"));

    QVERIFY(loader.unload());
}

void tst_QPluginLoader::preloadedPlugin_data()
{
    QTest::addColumn<bool>("doLoad");
    QTest::addColumn<QString>("libname");
    QTest::newRow("create-plugin") << false << sys_qualifiedLibraryName("theplugin");
    QTest::newRow("load-plugin") << true << sys_qualifiedLibraryName("theplugin");
    QTest::newRow("create-non-plugin") << false << sys_qualifiedLibraryName("tst_qpluginloaderlib");
    QTest::newRow("load-non-plugin") << true << sys_qualifiedLibraryName("tst_qpluginloaderlib");
}

void tst_QPluginLoader::preloadedPlugin()
{
    // check that using QPluginLoader does not interfere with QLibrary
    QFETCH(QString, libname);
    QLibrary lib(libname);
    QVERIFY(lib.load());

    typedef int *(*pf_t)();
    pf_t pf = (pf_t)lib.resolve("pointerAddress");
    QVERIFY(pf);

    int *pluginVariable = pf();
    QVERIFY(pluginVariable);
    QCOMPARE(*pluginVariable, 0xc0ffee);

    {
        // load the plugin
        QPluginLoader loader(libname);
        QFETCH(bool, doLoad);
        if (doLoad && loader.load()) {
            // unload() returns false because QLibrary has it loaded
            QVERIFY(!loader.unload());
        }
    }

    QVERIFY(lib.isLoaded());

    // if the library was unloaded behind our backs, the following will crash:
    QCOMPARE(*pluginVariable, 0xc0ffee);
    QVERIFY(lib.unload());
}

QTEST_MAIN(tst_QPluginLoader)
#include "tst_qpluginloader.moc"
