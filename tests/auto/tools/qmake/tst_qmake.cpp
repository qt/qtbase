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

#include <QtTest/QtTest>

#include "testcompiler.h"

#include <QObject>
#include <QStandardPaths>
#include <QDir>

#if defined(DEBUG_BUILD)
#  define DIR_INFIX "debug/"
#elif defined(RELEASE_BUILD)
#  define DIR_INFIX "release/"
#else
#  define DIR_INFIX ""
#endif

class tst_qmake : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void simple_app();
    void simple_app_shadowbuild();
    void simple_app_shadowbuild2();
    void simple_lib();
    void simple_dll();
    void subdirs();
    void subdir_via_pro_file_extra_target();
    void duplicateLibraryEntries();
    void export_across_file_boundaries();
    void include_dir();
    void include_pwd();
    void install_files();
    void install_depends();
    void quotedfilenames();
    void prompt();
    void one_space();
    void findMocs();
    void findDeps();
    void rawString();
#if defined(Q_OS_MAC)
    void bundle_spaces();
#endif
    void substitutes();
    void project();
    void proFileCache();
    void resources();

private:
    TestCompiler test_compiler;
    QString base_path;
};

void tst_qmake::initTestCase()
{
    QString binpath = QLibraryInfo::location(QLibraryInfo::BinariesPath);
    QString cmd = QString("%1/qmake").arg(binpath);
#ifdef Q_CC_MSVC
    const QString jom = QStandardPaths::findExecutable(QLatin1String("jom.exe"));
    if (jom.isEmpty()) {
        test_compiler.setBaseCommands( QLatin1String("nmake"), cmd );
    } else {
        test_compiler.setBaseCommands( jom, cmd );
    }
#elif defined(Q_CC_MINGW)
    test_compiler.setBaseCommands( "mingw32-make", cmd );
#elif defined(Q_OS_WIN) && defined(Q_CC_GNU)
    test_compiler.setBaseCommands( "mmmake", cmd );
#else
    test_compiler.setBaseCommands( "make", cmd );
#endif
    //Detect the location of the testdata
    QString subProgram  = QLatin1String("testdata/simple_app/main.cpp");
    base_path = QFINDTESTDATA(subProgram);
    if (base_path.lastIndexOf(subProgram) > 0)
        base_path = base_path.left(base_path.lastIndexOf(subProgram));
    else
        base_path = QCoreApplication::applicationDirPath();
}

void tst_qmake::cleanup()
{
    test_compiler.resetArguments();
    test_compiler.resetEnvironment();
    test_compiler.clearCommandOutput();
}

void tst_qmake::simple_app()
{
    QString workDir = base_path + "/testdata/simple_app";
    QString destDir = workDir + "/dest dir";

    QVERIFY( test_compiler.qmake( workDir, "simple_app" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( destDir, "simple app", Exe, "1.0.0" ));
    QVERIFY( test_compiler.makeClean( workDir ));
    QVERIFY( test_compiler.exists( destDir, "simple app", Exe, "1.0.0" )); // Should still exist after a make clean
    QVERIFY( test_compiler.makeDistClean( workDir ));
    QVERIFY( !test_compiler.exists( destDir, "simple app", Exe, "1.0.0" )); // Should not exist after a make distclean
    QVERIFY( test_compiler.removeMakefile( workDir ) );
}

void tst_qmake::simple_app_shadowbuild()
{
    QString workDir = base_path + "/testdata/simple_app";
    QString buildDir = base_path + "/testdata/simple_app_build";
    QString destDir = buildDir + "/dest dir";

    QVERIFY( test_compiler.qmake( workDir, "simple_app", buildDir ));
    QVERIFY( test_compiler.make( buildDir ));
    QVERIFY( test_compiler.exists( destDir, "simple app", Exe, "1.0.0" ));
    QVERIFY( test_compiler.makeClean( buildDir ));
    QVERIFY( test_compiler.exists( destDir, "simple app", Exe, "1.0.0" )); // Should still exist after a make clean
    QVERIFY( test_compiler.makeDistClean( buildDir ));
    QVERIFY( !test_compiler.exists( destDir, "simple app", Exe, "1.0.0" )); // Should not exist after a make distclean
    QVERIFY( test_compiler.removeMakefile( buildDir ) );
}

void tst_qmake::simple_app_shadowbuild2()
{
    QString workDir = base_path + "/testdata/simple_app";
    QString buildDir = base_path + "/testdata/simple_app/build";
    QString destDir = buildDir + "/dest dir";

    QVERIFY( test_compiler.qmake( workDir, "simple_app", buildDir ));
    QVERIFY( test_compiler.make( buildDir ));
    QVERIFY( test_compiler.exists( destDir, "simple app", Exe, "1.0.0" ));
    QVERIFY( test_compiler.makeClean( buildDir ));
    QVERIFY( test_compiler.exists( destDir, "simple app", Exe, "1.0.0" )); // Should still exist after a make clean
    QVERIFY( test_compiler.makeDistClean( buildDir ));
    QVERIFY( !test_compiler.exists( destDir, "simple app", Exe, "1.0.0" )); // Should not exist after a make distclean
    QVERIFY( test_compiler.removeMakefile( buildDir ) );
}

void tst_qmake::simple_dll()
{
    QString workDir = base_path + "/testdata/simple_dll";
    QString destDir = workDir + "/dest dir";

    QDir D;
    D.remove( workDir + "/Makefile");
    QVERIFY( test_compiler.qmake( workDir, "simple_dll" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( destDir, "simple dll", Dll, "1.0.0" ));
    QVERIFY( test_compiler.makeClean( workDir ));
    QVERIFY( test_compiler.exists( destDir, "simple dll", Dll, "1.0.0" )); // Should still exist after a make clean
    QVERIFY( test_compiler.makeDistClean( workDir ));
    QVERIFY( !test_compiler.exists( destDir, "simple dll", Dll, "1.0.0" )); // Should not exist after a make distclean
    QVERIFY( test_compiler.removeMakefile( workDir ) );
}

void tst_qmake::simple_lib()
{
    QString workDir = base_path + "/testdata/simple_lib";
    QString destDir = workDir + "/dest dir";

    QDir D;
    D.remove( workDir + "/Makefile");
    QVERIFY( test_compiler.qmake( workDir, "simple_lib" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( destDir, "simple lib", Lib, "1.0.0" ));
    QVERIFY( test_compiler.makeClean( workDir ));
    QVERIFY( test_compiler.exists( destDir, "simple lib", Lib, "1.0.0" )); // Should still exist after a make clean
    QVERIFY( test_compiler.makeDistClean( workDir ));
    QVERIFY( !test_compiler.exists( destDir, "simple lib", Lib, "1.0.0" )); // Should not exist after a make distclean
    QVERIFY( test_compiler.removeMakefile( workDir ) );
}

void tst_qmake::subdirs()
{
    QString workDir = base_path + "/testdata/subdirs";

    QDir D;
    D.remove( workDir + "/simple_app/Makefile");
    D.remove( workDir + "/simple_dll/Makefile");
    QVERIFY( test_compiler.qmake( workDir, "subdirs" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( workDir + "/simple_app/dest dir", "simple app", Exe, "1.0.0" ));
    QVERIFY( test_compiler.exists( workDir + "/simple_dll/dest dir", "simple dll", Dll, "1.0.0" ));
    QVERIFY( test_compiler.makeClean( workDir ));
    // Should still exist after a make clean
    QVERIFY( test_compiler.exists( workDir + "/simple_app/dest dir", "simple app", Exe, "1.0.0" ));
    QVERIFY( test_compiler.exists( workDir + "/simple_dll/dest dir", "simple dll", Dll, "1.0.0" ));
    // Since subdirs templates do not have a make dist clean, we should clean up ourselves
    // properly
    QVERIFY( test_compiler.makeDistClean( workDir ));
    QVERIFY( test_compiler.removeMakefile( workDir ) );
}

void tst_qmake::subdir_via_pro_file_extra_target()
{
    QString workDir = base_path + "/testdata/subdir_via_pro_file_extra_target";

    QDir D;
    D.remove( workDir + "/Makefile");
    D.remove( workDir + "/Makefile.subdir");
    D.remove( workDir + "/simple/Makefile");
    D.remove( workDir + "/simple/Makefile.subdir");
    QVERIFY( test_compiler.qmake( workDir, "subdir_via_pro_file_extra_target" ));
    QVERIFY( test_compiler.make( workDir, "extratarget" ));
}

void tst_qmake::duplicateLibraryEntries()
{
    QVERIFY(true);
    /* TODO: this test does not work as the problem it tests doesn't happen
    until after the parsing of the pro-file and thus has to be tested
    by parsing the Makefile. This is not doable with the current
    testcompiler framework and has as such been put on hold.

    QString workDir = base_path + "/testdata/duplicateLibraryEntries";
    QVERIFY(test_compiler.qmake(workDir, "duplicateLibraryEntries")); */
}

void tst_qmake::export_across_file_boundaries()
{
    // This relies on features so we need to set the QMAKEFEATURES environment variable
    test_compiler.addToEnvironment("QMAKEFEATURES=.");
    QString workDir = base_path + "/testdata/export_across_file_boundaries";
    QVERIFY( test_compiler.qmake( workDir, "foo" ));
}

void tst_qmake::include_dir()
{
#ifdef QT_NO_WIDGETS
    QSKIP("This test depends on QtWidgets");
#else
    QString workDir = base_path + "/testdata/include_dir";
    QVERIFY( test_compiler.qmake( workDir, "foo" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( workDir, "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.makeDistClean( workDir ));

    QString buildDir = base_path + "/testdata/include_dir_build";
    QVERIFY( test_compiler.qmake( workDir, "foo", buildDir ));
    QVERIFY( test_compiler.make( buildDir ));
    QVERIFY( test_compiler.exists( buildDir, "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.makeDistClean( buildDir ));
#endif
}

void tst_qmake::include_pwd()
{
    QString workDir = base_path + "/testdata/include_pwd";
    QVERIFY( test_compiler.qmake( workDir, "include_pwd" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.makeDistClean( workDir ));
}

void tst_qmake::install_files()
{
    QString workDir = base_path + "/testdata/shadow_files";
    QVERIFY( test_compiler.qmake( workDir, "foo" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( workDir, "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.make( workDir, "install" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "test.txt", Plain, "1.0.0" ));
    QVERIFY( test_compiler.make( workDir, "uninstall" ));
    QVERIFY( test_compiler.makeDistClean( workDir ));

    QString buildDir = base_path + "/testdata/shadow_files_build";
    QVERIFY( test_compiler.qmake( workDir, "foo", buildDir ));
    QVERIFY( test_compiler.make( buildDir ));
    QVERIFY( test_compiler.exists( buildDir, "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.make( buildDir, "install" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "test.txt", Plain, "1.0.0" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "foo.bar", Plain, "1.0.0" ));
    QVERIFY( test_compiler.make( buildDir, "uninstall" ));
    QVERIFY( test_compiler.makeDistClean( buildDir ));
}

void tst_qmake::install_depends()
{
    QString workDir = base_path + "/testdata/install_depends";
    QVERIFY( test_compiler.qmake( workDir, "foo" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( workDir, "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.make( workDir, "install" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "foo", Exe, "1.0.0" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "test1", Plain, "1.0.0" ));
    QVERIFY( test_compiler.exists( workDir + "/dist", "test2", Plain, "1.0.0" ));
    QVERIFY( test_compiler.make( workDir, "uninstall" ));
    QVERIFY( test_compiler.makeDistClean( workDir ));
}
void tst_qmake::quotedfilenames()
{
    QString workDir = base_path + "/testdata/quotedfilenames";
    QVERIFY( test_compiler.qmake( workDir, "quotedfilenames" ));
    QVERIFY( test_compiler.makeClean( workDir ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( workDir, "quotedfilenames", Exe, "1.0.0" ));
}

void tst_qmake::prompt()
{
#if 0
    QProcess qmake;
    qmake.setReadChannelMode(QProcess::MergedChannels);
    qmake.setWorkingDirectory(QLatin1String("testdata/prompt"));
    qmake.start(QLatin1String("qmake CONFIG-=debug_and_release CONFIG-=debug CONFIG+=release"),
                QIODevice::Text | QIODevice::ReadWrite);
    QVERIFY(qmake.waitForStarted(20000));
    QByteArray read = qmake.readAll();
    qDebug() << read;
    QCOMPARE(read, QByteArray("Project PROMPT: Prompteroo? "));
    qmake.write("promptetiprompt\n");
    QVERIFY(qmake.waitForFinished(20000));
#endif
}

void tst_qmake::one_space()
{
    QString workDir = base_path + "/testdata/one_space";

    QVERIFY( test_compiler.qmake( workDir, "one_space" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( workDir, "one space", Exe, "1.0.0" ));
    QVERIFY( test_compiler.makeClean( workDir ));
    QVERIFY( test_compiler.exists( workDir, "one space", Exe, "1.0.0" )); // Should still exist after a make clean
    QVERIFY( test_compiler.makeDistClean( workDir ));
    QVERIFY( !test_compiler.exists( workDir, "one space", Exe, "1.0.0" )); // Should not exist after a make distclean
    QVERIFY( test_compiler.removeMakefile( workDir ) );
}

void tst_qmake::findMocs()
{
    QString workDir = base_path + "/testdata/findMocs";

    QVERIFY( test_compiler.qmake(workDir, "findMocs") );
    QVERIFY( test_compiler.make(workDir) );
    QVERIFY( test_compiler.exists(workDir, "findMocs", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.makeClean(workDir) );
    QVERIFY( test_compiler.exists(workDir, "findMocs", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.makeDistClean(workDir ) );
    QVERIFY( !test_compiler.exists(workDir, "findMocs", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.removeMakefile(workDir) );
}

void tst_qmake::findDeps()
{
    QString workDir = base_path + "/testdata/findDeps";

    QVERIFY( test_compiler.qmake(workDir, "findDeps") );
    QVERIFY( test_compiler.make(workDir) );
    QVERIFY( test_compiler.exists(workDir, "findDeps", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.makeClean(workDir) );
    QVERIFY( test_compiler.exists(workDir, "findDeps", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.makeDistClean(workDir ) );
    QVERIFY( !test_compiler.exists(workDir, "findDeps", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.removeMakefile(workDir) );
}

void tst_qmake::rawString()
{
#ifdef Q_COMPILER_RAW_STRINGS
    QString workDir = base_path + "/testdata/rawString";

    QVERIFY( test_compiler.qmake(workDir, "rawString") );
    QVERIFY( test_compiler.make(workDir) );
    QVERIFY( test_compiler.exists(workDir, "rawString", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.makeClean(workDir) );
    QVERIFY( test_compiler.exists(workDir, "rawString", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.makeDistClean(workDir ) );
    QVERIFY( !test_compiler.exists(workDir, "rawString", Exe, "1.0.0" ) );
    QVERIFY( test_compiler.removeMakefile(workDir) );
#else
    QSKIP("Test for C++11 raw strings depends on compiler support for them");
#endif
}

struct TempFile
    : QFile
{
    TempFile(QString filename)
        : QFile(filename)
    {
    }

    ~TempFile()
    {
        if (this->exists())
            this->remove();
    }
};

#if defined(Q_OS_MAC)
void tst_qmake::bundle_spaces()
{
    QString workDir = base_path + "/testdata/bundle-spaces";

    // We set up alternate arguments here, to make sure we're testing Mac
    // Bundles and since this might be the wrong output we rely on dry-running
    // make (-n).

    test_compiler.setArguments(QStringList() << "-n",
                               QStringList() << "-spec" << "macx-clang");

    QVERIFY( test_compiler.qmake(workDir, "bundle-spaces") );

    TempFile non_existing_file(workDir + "/non-existing file");
    QVERIFY( !non_existing_file.exists() );

    // Make fails: no rule to make "non-existing file"
    QVERIFY( test_compiler.make(workDir, QString(), true) );

    QVERIFY( non_existing_file.open(QIODevice::WriteOnly) );
    QVERIFY( non_existing_file.exists() );

    // Aha!
    QVERIFY( test_compiler.make(workDir) );

    // Cleanup
    QVERIFY( non_existing_file.remove() );
    QVERIFY( !non_existing_file.exists() );
    QVERIFY( test_compiler.removeMakefile(workDir) );
}
#endif // defined(Q_OS_MAC)

void tst_qmake::substitutes()
{
    QString workDir = base_path + "/testdata/substitutes";
    QVERIFY( test_compiler.qmake( workDir, "test" ));
    QVERIFY( test_compiler.exists( workDir, "test", Plain, "" ));
    QVERIFY( test_compiler.exists( workDir, "sub/test2", Plain, "" ));
    QVERIFY( test_compiler.exists( workDir, "sub/indirect_test.txt", Plain, "" ));
    QVERIFY( test_compiler.makeDistClean( workDir ));

    QString buildDir = base_path + "/testdata/substitutes_build";
    QVERIFY( test_compiler.qmake( workDir, "test", buildDir ));
    QVERIFY( test_compiler.exists( buildDir, "test", Plain, "" ));
    QVERIFY( test_compiler.exists( buildDir, "sub/test2", Plain, "" ));
    QVERIFY( test_compiler.exists( buildDir, "sub/indirect_test.txt", Plain, "" ));

    QFile copySource(workDir + "/copy.txt");
    QFile copyDestination(buildDir + "/copy_test.txt");

    QVERIFY(copySource.open(QFile::ReadOnly));
    QVERIFY(copyDestination.open(QFile::ReadOnly));
    QCOMPARE(copySource.readAll(), copyDestination.readAll());

    QVERIFY( test_compiler.makeDistClean( buildDir ));
}

void tst_qmake::project()
{
    QString workDir = base_path + "/testdata/project";

    QVERIFY( test_compiler.qmakeProject( workDir, "project" ));
    QVERIFY( test_compiler.exists( workDir, "project.pro", Plain, "" ));
    QVERIFY( test_compiler.qmake( workDir, "project" ));
    QVERIFY( test_compiler.exists( workDir, "Makefile", Plain, "" ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( workDir, "project", Exe, "" ));
    QVERIFY( test_compiler.makeDistClean( workDir ));
    QVERIFY( test_compiler.removeProject( workDir, "project" ));
}

void tst_qmake::proFileCache()
{
    QString workDir = base_path + "/testdata/pro_file_cache";
    QVERIFY( test_compiler.qmake( workDir, "pro_file_cache" ));
}

void tst_qmake::resources()
{
    QString workDir = base_path + "/testdata/resources";
    QVERIFY(test_compiler.qmake(workDir, "resources"));

    {
        QFile qrcFile(workDir + "/.rcc/" DIR_INFIX "qmake_pro_file.qrc");
        QVERIFY(qrcFile.exists());
        QVERIFY(qrcFile.open(QFile::ReadOnly));
        QByteArray qrcXml = qrcFile.readAll();
        QVERIFY(qrcXml.contains("alias=\"resources.pro\""));
        QVERIFY(qrcXml.contains("prefix=\"/prefix\""));
    }

    {
        QFile qrcFile(workDir + "/.rcc/" DIR_INFIX "qmake_subdir.qrc");
        QVERIFY(qrcFile.exists());
        QVERIFY(qrcFile.open(QFile::ReadOnly));
        QByteArray qrcXml = qrcFile.readAll();
        QVERIFY(qrcXml.contains("alias=\"file.txt\""));
    }

    {
        QFile qrcFile(workDir + "/.rcc/" DIR_INFIX "qmake_qmake_immediate.qrc");
        QVERIFY(qrcFile.exists());
        QVERIFY(qrcFile.open(QFile::ReadOnly));
        QByteArray qrcXml = qrcFile.readAll();
        QVERIFY(qrcXml.contains("alias=\"main.cpp\""));
    }

    QVERIFY(test_compiler.make(workDir));
}

QTEST_MAIN(tst_qmake)
#include "tst_qmake.moc"
