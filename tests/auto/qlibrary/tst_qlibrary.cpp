/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qdir.h>
#include <qlibrary.h>
#include <QtCore/QRegExp>


// Helper macros to let us know if some suffixes and prefixes are valid
#define bundle_VALID    false
#define dylib_VALID     false
#define sl_VALID        false
#define a_VALID         false
#define so_VALID        false
#define dll_VALID       false
#define DLL_VALID       false

#if defined(Q_OS_DARWIN)
# undef bundle_VALID
# undef dylib_VALID
# undef so_VALID
# define bundle_VALID   true
# define dylib_VALID    true
# define so_VALID       true
# define SUFFIX         ".dylib"
# define PREFIX         "lib"

#elif defined(Q_OS_HPUX)
# undef sl_VALID
# define sl_VALID       true
#  ifndef __ia64
#   define SUFFIX         ".sl"
#   define PREFIX         "lib"
#  else
#   undef so_VALID
#   define so_VALID       true
#   define SUFFIX         ".so"
#   define PREFIX         "lib"
#  endif

#elif defined(Q_OS_AIX)
# undef a_VALID
# undef so_VALID
# define a_VALID        true
# define so_VALID       true
# define SUFFIX         ".a"
# define PREFIX         "lib"

#elif defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
# undef dll_VALID
# define dll_VALID      true
# undef DLL_VALID
# define DLL_VALID      true
# define SUFFIX         ".dll"
# define PREFIX         ""

#else  // all other Unix
# undef so_VALID
# define so_VALID       true
# define SUFFIX         ".so"
# define PREFIX         "lib"
#endif

static QString sys_qualifiedLibraryName(const QString &fileName)
{
#if defined(Q_OS_SYMBIAN)
    return PREFIX + fileName + SUFFIX;
#else
    QString currDir = QDir::currentPath();
    return currDir + "/" + PREFIX + fileName + SUFFIX;
#endif
}

//TESTED_CLASS=
//TESTED_FILES=

QT_FORWARD_DECLARE_CLASS(QLibrary)
class tst_QLibrary : public QObject
{
    Q_OBJECT

public:
    tst_QLibrary();
    virtual ~tst_QLibrary();

enum QLibraryOperation {
    Load = 1,
    Unload = 2,
    Resolve = 3,
    OperationMask = 7,
    DontSetFileName = 0x100
};
private slots:
    void load();
    void load_data();
    void library_data();
    void resolve_data();
    void resolve();
    void unload_data();
    void unload();
    void unload_after_implicit_load();
    void isLibrary_data();
    void isLibrary();
    void version_data();
    void version();
    void errorString_data();
    void errorString();
    void loadHints();
    void loadHints_data();
    void fileName_data();
    void fileName();
    void multipleInstancesForOneLibrary();

#ifdef Q_OS_WINCE
private:
    QCoreApplication* app;
#endif
};

tst_QLibrary::tst_QLibrary()

{
#ifdef Q_OS_WINCE
    char* argv = "app";
    int argc = 1;
    app = new QCoreApplication(argc,&argv);
#endif
}

tst_QLibrary::~tst_QLibrary()
{
#ifdef Q_OS_WINCE
    app->quit();
#endif
}


typedef int (*VersionFunction)(void);

void tst_QLibrary::version_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<int>("loadversion");
    QTest::addColumn<int>("resultversion");

    QTest::newRow( "ok00, version 1" ) << "mylib" << 1 << 1;
    QTest::newRow( "ok00, version 2" ) << "mylib" << 2 << 2;
    QTest::newRow( "ok00, default to last version" ) << "mylib" << -1 << 2;
}

void tst_QLibrary::version()
{
    QFETCH( QString, lib );
    QFETCH( int, loadversion );
    QFETCH( int, resultversion );

#if !defined(Q_OS_AIX) && !defined(Q_OS_WIN) && !defined(Q_OS_SYMBIAN)
    QString currDir = QDir::currentPath();
    QLibrary library( currDir + QLatin1Char('/') + lib, loadversion );
    bool ok = library.load();
    QVERIFY(ok);

    VersionFunction fnVersion = (VersionFunction)library.resolve("mylibversion");
    QVERIFY(fnVersion);
    QCOMPARE(fnVersion(), resultversion);
	QVERIFY(library.unload());
#else
    Q_UNUSED(lib);
    Q_UNUSED(loadversion);
    Q_UNUSED(resultversion);
#endif

}

void tst_QLibrary::load_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<bool>("result");

#if defined(Q_OS_SYMBIAN)
    QString currDir;
#else
    QString currDir = QDir::currentPath();
#endif
    QTest::newRow( "ok00" ) << currDir + "/mylib" << true;
    QTest::newRow( "notexist" ) << currDir + "/nolib" << false;
    QTest::newRow( "badlibrary" ) << currDir + "/qlibrary.pro" << false;

#ifdef Q_OS_MAC
    QTest::newRow("ok (libmylib ver. 1)") << currDir + "/libmylib" <<true;
#endif

# if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    QTest::newRow( "ok01 (with suffix)" ) << currDir + "/mylib.dll" << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << currDir + "/mylib.dl2" << true;
    QTest::newRow( "ok03 (with many dots)" ) << currDir + "/system.trolltech.test.mylib.dll" << true;
# elif defined Q_OS_UNIX
    QTest::newRow( "ok01 (with suffix)" ) << currDir + "/libmylib" SUFFIX << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << currDir + "/libmylib.so2" << true;
    QTest::newRow( "ok03 (with many dots)" ) << currDir + "/system.trolltech.test.mylib.so" << true;
# endif  // Q_OS_UNIX
}

void tst_QLibrary::load()
{
    QFETCH( QString, lib );
    QFETCH( bool, result );

    QLibrary library( lib );
    bool ok = library.load();
    if ( result ) {
	QVERIFY( ok );
	QVERIFY(library.unload());
    } else {
	QVERIFY( !ok );
    }
}

void tst_QLibrary::unload_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<bool>("result");

#if defined(Q_OS_SYMBIAN)
    QString currDir;
#else
    QString currDir = QDir::currentPath();
#endif

    QTest::newRow( "mylib" ) << currDir + "/mylib" << true;
#ifdef Q_WS_MAC
    if (QSysInfo::MacintoshVersion <= QSysInfo::MV_10_3)
        QEXPECT_FAIL("mylib", "dlcompat cannot unload libraries", Continue);
#endif
    QTest::newRow( "ok01" ) << currDir + "/nolib" << false;
}

void tst_QLibrary::unload()
{
    QFETCH( QString, lib );
    QFETCH( bool, result );

    QLibrary library( lib );
    library.load();
    bool ok = library.unload();
    if ( result ) {
	QVERIFY( ok );
    } else {
	QVERIFY( !ok );
    }
}

void tst_QLibrary::unload_after_implicit_load()
{
#if defined(Q_OS_SYMBIAN)
    QSKIP("SYMBIAN does not support symbols on non-STDDLL libraries.", SkipAll);
#endif

    QLibrary library( "./mylib" );
    void *p = library.resolve("mylibversion");
    QVERIFY(p); // Check if it was loaded
    QVERIFY(library.isLoaded());
    QVERIFY(library.unload());
    QCOMPARE(library.isLoaded(), false);

}

void tst_QLibrary::resolve_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<QString>("symbol");
    QTest::addColumn<bool>("goodPointer");

#if defined(Q_OS_SYMBIAN)
    QString currDir;
#else
    QString currDir = QDir::currentPath();
#endif

    QTest::newRow( "ok00" ) << currDir + "/mylib" << QString("mylibversion") << true;
    QTest::newRow( "bad00" ) << currDir + "/mylib" << QString("nosym") << false;
    QTest::newRow( "bad01" ) << currDir + "/nolib" << QString("nosym") << false;
}

void tst_QLibrary::resolve()
{
#if defined(Q_OS_SYMBIAN)
    QSKIP("SYMBIAN does not support symbols on non-STDDLL libraries.", SkipAll);
#endif

    typedef int (*testFunc)();
    QFETCH( QString, lib );
    QFETCH( QString, symbol );
    QFETCH( bool, goodPointer );

    QLibrary library( lib );
    testFunc func = (testFunc) library.resolve( symbol.toLatin1() );
    if ( goodPointer ) {
	QVERIFY( func != 0 );
    } else {
	QVERIFY( func == 0 );
    }
	library.unload();
}

void tst_QLibrary::library_data()
{
    QTest::addColumn<QString>("lib");
}

void tst_QLibrary::isLibrary_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<bool>("valid");

    // use the macros #defined at the top of the file
    QTest::newRow("bad") << QString("mylib.bad") << false;
    QTest::newRow(".a") << QString("mylib.a") << a_VALID;
    QTest::newRow(".bundle") << QString("mylib.bundle") << bundle_VALID;
    QTest::newRow(".dll") << QString("mylib.dll") << dll_VALID;
    QTest::newRow(".DLL") << QString("MYLIB.DLL") << DLL_VALID;
    QTest::newRow(".dl2" ) << QString("mylib.dl2") << false;
    QTest::newRow(".dylib") << QString("mylib.dylib") << dylib_VALID;
    QTest::newRow(".sl") << QString("mylib.sl") << sl_VALID;
    QTest::newRow(".so") << QString("mylib.so") << so_VALID;
    QTest::newRow(".so+version") << QString("mylib.so.0") << so_VALID;

    // special tests:
#ifndef Q_OS_MAC
    QTest::newRow("version+.so") << QString("libc-2.7.so") << so_VALID;
    QTest::newRow("version+.so+version") << QString("liboil-0.3.so.0.1.0") << so_VALID;
#else
    QTest::newRow("version+.so") << QString("libc-2.7.so") << false;
    QTest::newRow("version+.so+version") << QString("liboil-0.3.so.0.1.0") << false;
#endif
#ifdef Q_OS_MAC
    QTest::newRow("good (libmylib.1.0.0.dylib)") << QString("libmylib.1.0.0.dylib") << true;
    QTest::newRow("good (libmylib.dylib)") << QString("libmylib.dylib") << true;
    QTest::newRow("good (libmylib.so)") << QString("libmylib.so") << true;
    QTest::newRow("good (libmylib.so.1.0.0)") << QString("libmylib.so.1.0.0") << true;

    QTest::newRow("bad (libmylib.1.0.0.foo)") << QString("libmylib.1.0.0.foo") << false;
#elif defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QTest::newRow("good (with many dots)" ) << "/system.trolltech.test.mylib.dll" << true;
#endif
}

void tst_QLibrary::isLibrary()
{
    QFETCH( QString, filename );
    QFETCH( bool, valid );

    QCOMPARE(QLibrary::isLibrary(filename), valid);
}

void tst_QLibrary::errorString_data()
{
    QTest::addColumn<int>("operation");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QString>("errorString");

#if defined(Q_OS_SYMBIAN)
    QString currDir;
#else
    QString currDir = QDir::currentPath();

    QString srcDir = SRCDIR;
    if (srcDir.isEmpty())
        srcDir = currDir;
#endif

    QTest::newRow("bad load()") << (int)Load << QString("nosuchlib") << false << QString("Cannot load library nosuchlib: .*");
    QTest::newRow("call errorString() on QLibrary with no d-pointer (crashtest)") << (int)(Load | DontSetFileName) << QString() << false << QString("Unknown error");
#ifdef Q_OS_WINCE
    QTest::newRow("bad resolve") << (int)Resolve << currDir + "/mylib" << false << QString("Cannot resolve symbol \"nosuchsymbol\" in .*: .*");
#else
    QTest::newRow("bad resolve") << (int)Resolve << currDir + "/mylib" << false << QString("Cannot resolve symbol \"nosuchsymbol\" in \\S+: .*");
#endif
    QTest::newRow("good resolve") << (int)Resolve << currDir + "/mylib" << true << QString("Unknown error");

#ifdef Q_OS_WIN
    QTest::newRow("bad load() with .dll suffix") << (int)Load << QString("nosuchlib.dll") << false << QString("Cannot load library nosuchlib.dll: The specified module could not be found.");
//    QTest::newRow("bad unload") << (int)Unload << QString("nosuchlib.dll") << false << QString("QLibrary::unload_sys: Cannot unload nosuchlib.dll (The specified module could not be found.)");
#elif defined Q_OS_MAC
#elif defined Q_OS_SYMBIAN
    QTest::newRow("load invalid file") << (int)Load << "tst_qlibrary.exe" << false << QString("Cannot load library.*");
#else
    QTest::newRow("load invalid file") << (int)Load << srcDir + "/library_path/invalid.so" << false << QString("Cannot load library.*");
#endif
}

void tst_QLibrary::errorString()
{
    QFETCH(int, operation);
    QFETCH(QString, fileName);
    QFETCH(bool, success);
    QFETCH(QString, errorString);

#if defined(Q_OS_SYMBIAN)
    if ( success )
        {
        QSKIP("SYMBIAN does not support symbols on non-STDDLL libraries.", SkipSingle );
        }
#endif

    QLibrary lib;
    if (!(operation & DontSetFileName)) {
        lib.setFileName(fileName);
    }

    bool ok = false;
    switch (operation & OperationMask) {
        case Load:
            ok = lib.load();
            break;
        case Unload:
            ok = lib.load();    //###
            ok = lib.unload();
            break;
        case Resolve: {
            ok = lib.load();
            QCOMPARE(ok, true);
            if (success) {
                ok = lib.resolve("mylibversion");
            } else {
                ok = lib.resolve("nosuchsymbol");
            }
            break;}
        default:
            QFAIL(qPrintable(QString("Unknown operation: %1").arg(operation)));
            break;
    }
    QRegExp re(errorString);
	QString libErrorString = lib.errorString();
    QVERIFY(!lib.isLoaded() || lib.unload());
    QVERIFY2(re.exactMatch(libErrorString), qPrintable(libErrorString));
    QCOMPARE(ok, success);
}

void tst_QLibrary::loadHints_data()
{
    QTest::addColumn<QString>("lib");
    QTest::addColumn<int>("loadHints");
    QTest::addColumn<bool>("result");

    QLibrary::LoadHints lh;
#if defined(Q_OS_AIX)
    if (QFile::exists("/usr/lib/libGL.a") || QFile::exists("/usr/X11R6/lib/libGL.a")) {
# if QT_POINTER_SIZE == 4
        QTest::newRow( "ok03 (Archive member)" ) << "libGL.a(shr.o)" << int(QLibrary::LoadArchiveMemberHint) << true;
# else
        QTest::newRow( "ok03 (Archive member)" ) << "libGL.a(shr_64.o)" << int(QLibrary::LoadArchiveMemberHint) << true;
#endif
    }
#endif

#if defined(Q_OS_SYMBIAN)
    QString currDir;
#else
    QString currDir = QDir::currentPath();
#endif

    lh |= QLibrary::ResolveAllSymbolsHint;
# if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_SYMBIAN)
    QTest::newRow( "ok01 (with suffix)" ) << currDir + "/mylib.dll" << int(lh) << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << currDir + "/mylib.dl2" << int(lh) << true;
    QTest::newRow( "ok03 (with many dots)" ) << currDir + "/system.trolltech.test.mylib.dll" << int(lh) << true;
# elif defined Q_OS_UNIX
    QTest::newRow( "ok01 (with suffix)" ) << currDir + "/libmylib" SUFFIX << int(lh) << true;
    QTest::newRow( "ok02 (with non-standard suffix)" ) << currDir + "/libmylib.so2" << int(lh) << true;
    QTest::newRow( "ok03 (with many dots)" ) << currDir + "/system.trolltech.test.mylib.so" << int(lh) << true;
# endif  // Q_OS_UNIX
}

void tst_QLibrary::loadHints()
{
    QFETCH( QString, lib );
    QFETCH( int, loadHints);
    QFETCH( bool, result );
    //QLibrary library( lib );
    QLibrary library;
    QLibrary::LoadHints lh(loadHints);
    if (int(loadHints) != 0) {
        lh |= library.loadHints();
        library.setLoadHints(lh);
    }
    library.setFileName(lib);
    QCOMPARE(library.loadHints(), lh);
    bool ok = library.load();
    if ( result ) {
        QVERIFY( ok );
		QVERIFY(library.unload());
    } else {
        QVERIFY( !ok );
    }
}

void tst_QLibrary::fileName_data()
{
    QTest::addColumn<QString>("libName");
    QTest::addColumn<QString>("expectedFilename");

    QTest::newRow( "ok02" ) << sys_qualifiedLibraryName(QLatin1String("mylib"))
                            << sys_qualifiedLibraryName(QLatin1String("mylib"));
#ifdef Q_WS_WIN
#ifndef Q_OS_WINCE
    QTest::newRow( "ok03" ) << "user32"
                            << "USER32.dll";
#else
    QTest::newRow( "ok03" ) << "coredll"
                            << "coredll.dll";
#endif
#endif
}

void tst_QLibrary::fileName()
{
    QFETCH( QString, libName);
    QFETCH( QString, expectedFilename);

    QLibrary lib(libName);
    bool ok = lib.load();
    if (!ok) {
        qDebug() << lib.errorString();
    }

    QVERIFY(ok);
#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
    QCOMPARE(lib.fileName().toLower(), expectedFilename.toLower());
#else
    QCOMPARE(lib.fileName(), expectedFilename);
#endif
    QVERIFY(lib.unload());

}

void tst_QLibrary::multipleInstancesForOneLibrary()
{
#if defined(Q_OS_SYMBIAN)
    QString lib = "/mylib";
#else
    QString lib = QDir::currentPath() + "/mylib";
#endif

    {
		QLibrary lib1(lib);
		QLibrary lib2(lib);
		QCOMPARE(lib1.isLoaded(), false);
		QCOMPARE(lib2.isLoaded(), false);
		lib1.load();
		QCOMPARE(lib1.isLoaded(), true);
		QCOMPARE(lib2.isLoaded(), true);
		QCOMPARE(lib1.unload(), true);
		QCOMPARE(lib1.isLoaded(), false);
		QCOMPARE(lib2.isLoaded(), false);
		lib1.load();
		lib2.load();
		QCOMPARE(lib1.isLoaded(), true);
		QCOMPARE(lib2.isLoaded(), true);
		QCOMPARE(lib1.unload(), false);
		QCOMPARE(lib1.isLoaded(), true);
		QCOMPARE(lib2.isLoaded(), true);
		QCOMPARE(lib2.unload(), true);
		QCOMPARE(lib1.isLoaded(), false);
		QCOMPARE(lib2.isLoaded(), false);

		// Finally; unload on that is already unloaded
		QCOMPARE(lib1.unload(), false);
    }

    //now let's try with a 3rd one that will go out of scope
    {
        QLibrary lib1(lib);
        QCOMPARE(lib1.isLoaded(), false);
        lib1.load();
        QCOMPARE(lib1.isLoaded(), true);
	}
	QLibrary lib2(lib);
	//lib2 should be loaded because lib1 was loaded and never unloaded
    QCOMPARE(lib2.isLoaded(), true);

/*
    lib1.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    lib2.setLoadHints(QLibrary::ExportExternalSymbolHint);
*/
}

QTEST_APPLESS_MAIN(tst_QLibrary)
#include "tst_qlibrary.moc"
