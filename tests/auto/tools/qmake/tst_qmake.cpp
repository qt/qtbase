// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include "testcompiler.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QLibraryInfo>
#include <QProcessEnvironment>

class tst_qmake : public QObject
{
    Q_OBJECT

public:
    tst_qmake();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void simple_app();
    void simple_app_shadowbuild();
    void simple_app_shadowbuild2();
    void simple_app_versioned();
    void simple_lib();
    void simple_dll();
    void subdirs();
    void subdirsDependsOnUnnormalizedFile();
    void subdir_via_pro_file_extra_target();
    void duplicateLibraryEntries();
    void export_across_file_boundaries();
    void include_dir();
    void include_pwd();
    void include_missing();
    void install_files();
    void install_depends();
    void quotedfilenames();
    void prompt();
    void one_space();
    void findMocs();
    void findDeps();
    void rawString();
#if defined(Q_OS_DARWIN)
    void bundle_spaces();
    void invalid_info_plist();
#elif defined(Q_OS_WIN)
    void windowsResources();
#endif
#ifdef Q_OS_WIN
    void shellQuotePlusSign();
    void vcxprojExtraCompilersSharingInput();
    void vcxprojQrcContentFiles();
    void vcxprojFilterFoldersInShadowBuild();
    void msvcDiagnosticsWithFullPaths();
#endif
    void substitutes();
    void project();
    void proFileCache();
    void qinstall();
    void hostPathsInQtConf();
    void resources();
    void duplicateInputs();
    void embedTranslations();
    void resourcesSameName();
    void conflictingTargets();

private:
    TestCompiler test_compiler;
    QTemporaryDir tempWorkDir;
    QString base_path;
    const QString origCurrentDirPath;
};

tst_qmake::tst_qmake()
    : tempWorkDir(QDir::tempPath() + "/tst_qmake"),
      origCurrentDirPath(QDir::currentPath())
{
}

static void copyDir(const QString &sourceDirPath, const QString &targetDirPath)
{
    QDir currentDir;
    QDirIterator dit(sourceDirPath, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    while (dit.hasNext()) {
        dit.next();
        const QString targetPath = targetDirPath + QLatin1Char('/') + dit.fileName();
        currentDir.mkpath(targetPath);
        copyDir(dit.filePath(), targetPath);
    }

    QDirIterator fit(sourceDirPath, QDir::Files | QDir::Hidden);
    while (fit.hasNext()) {
        fit.next();
        const QString targetPath = targetDirPath + QLatin1Char('/') + fit.fileName();
        QFile::remove(targetPath);  // allowed to fail
        QFile src(fit.filePath());
        QVERIFY2(src.copy(targetPath), qPrintable(src.errorString()));
    }
}

void tst_qmake::initTestCase()
{
#if defined(Q_OS_APPLE)
    if (QProcess::execute("xcode-select", { "-p" }) != 0)
        QSKIP("Xcode or Xcode command line tools not installed");
#endif

    QVERIFY2(tempWorkDir.isValid(), qPrintable(tempWorkDir.errorString()));
    QString binpath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    QString cmd = QString("%1/qmake").arg(binpath);

    // If Qt is cross-compiled with CMake, we might also cross-compile qmake for the device.
    // In this case we don't want to use the cross-compiled qmake, but rather the host qmake
    // shell wrapper (if it's available).
    const QString hostQmake = QString("%1/host-qmake").arg(binpath);
    if (QFile::exists(hostQmake)) {
        cmd = hostQmake;
    }

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
    const QString testDataSubDir = QStringLiteral("testdata");
    const QString subProgram = testDataSubDir + QLatin1String("/simple_app/main.cpp");
    QString testDataPath = QFINDTESTDATA(subProgram);
    if (!testDataPath.endsWith(subProgram))
        QFAIL("Cannot find test data directory.");
    testDataPath.chop(subProgram.size() - testDataSubDir.size());

    QString userWorkDir = qgetenv("TST_QMAKE_BUILD_DIR");
    if (userWorkDir.isEmpty()) {
        base_path = tempWorkDir.path();
    } else {
        if (!QFile::exists(userWorkDir)) {
            QFAIL(qUtf8Printable(QStringLiteral("TST_QMAKE_BUILD_DIR %1 does not exist.")
                                 .arg(userWorkDir)));
        }
        base_path = userWorkDir;
    }

    copyDir(testDataPath, base_path + QLatin1Char('/') + testDataSubDir);
}

void tst_qmake::cleanupTestCase()
{
    // On Windows, ~QTemporaryDir fails to remove the directory if we're still in there.
    QDir::setCurrent(origCurrentDirPath);
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
    QString installDir = workDir + "/dist";

    QVERIFY( test_compiler.qmake( workDir, "simple_app", QString() ));
    QVERIFY( test_compiler.make( workDir ));
    QVERIFY( test_compiler.exists( destDir, "simple app", Exe, "1.0.0" ));

    QVERIFY(test_compiler.make(workDir, "install"));
    QVERIFY(test_compiler.exists(installDir, "simple app", Exe, "1.0.0"));

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

void tst_qmake::simple_app_versioned()
{
    QString workDir = base_path + "/testdata/simple_app";
    QString buildDir = base_path + "/testdata/simple_app_versioned_build";
    QString destDir = buildDir + "/dest dir";
    QString installDir = buildDir + "/dist";

    QString version = "4.5.6";
    QVERIFY(test_compiler.qmake(workDir, "simple_app", buildDir, QStringList{ "VERSION=" + version }));
    QString qmakeOutput = test_compiler.commandOutput();
    QVERIFY(test_compiler.make(buildDir));
    QVERIFY(test_compiler.exists(destDir, "simple app", Exe, version));

    QString pdbFilePath;
    bool checkPdb = qmakeOutput.contains("Project MESSAGE: check for pdb, please");
    if (checkPdb) {
        QString targetBase = QFileInfo(TestCompiler::targetName(Exe, "simple app", version))
                .completeBaseName();
        pdbFilePath = destDir + '/' + targetBase + ".pdb";
        QVERIFY2(QFile::exists(pdbFilePath), qPrintable(pdbFilePath));
        QVERIFY(test_compiler.make(buildDir, "install"));
        QString installedPdbFilePath = installDir + '/' + targetBase + ".pdb";
        QVERIFY2(QFile::exists(installedPdbFilePath), qPrintable(installedPdbFilePath));
    }

    QVERIFY(test_compiler.makeClean(buildDir));
    QVERIFY(test_compiler.exists(destDir, "simple app", Exe, version));
    QVERIFY(test_compiler.makeDistClean(buildDir));
    QVERIFY(!test_compiler.exists(destDir, "simple app", Exe, version));
    if (checkPdb)
        QVERIFY(!QFile::exists(pdbFilePath));
    QVERIFY(test_compiler.removeMakefile(buildDir));
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
    QVERIFY( test_compiler.exists(workDir + "/simple_app/dest dir", "simple app", Exe));
    QVERIFY( test_compiler.exists(workDir + "/simple_dll/dest dir", "simple dll", Dll));
    QVERIFY( test_compiler.makeClean( workDir ));
    // Should still exist after a make clean
    QVERIFY( test_compiler.exists(workDir + "/simple_app/dest dir", "simple app", Exe));
    QVERIFY( test_compiler.exists(workDir + "/simple_dll/dest dir", "simple dll", Dll));
    // Since subdirs templates do not have a make dist clean, we should clean up ourselves
    // properly
    QVERIFY( test_compiler.makeDistClean( workDir ));
    QVERIFY( test_compiler.removeMakefile( workDir ) );
}

void tst_qmake::subdirsDependsOnUnnormalizedFile()
{
    QString workDir = base_path + "/testdata/subdirs_depends";
    QVERIFY(test_compiler.qmake(workDir, "subdirs_depends"));

    QFile makefile(workDir + "/Makefile");
    QVERIFY(makefile.open(QFile::ReadOnly));
    const QByteArray contents = makefile.readAll();
    makefile.close();
    // "./foo.pro" must result in the same target name in the rule and in the dependency.
    QVERIFY(contents.contains("sub-foo-pro-qmake_all:"));
    QVERIFY(contents.contains("sub-bar-pro-qmake_all: sub-foo-pro-qmake_all"));
    QVERIFY(!contents.contains("sub---foo-pro"));
    QVERIFY(test_compiler.removeMakefile(workDir));
}

void tst_qmake::subdir_via_pro_file_extra_target()
{
    if (QProcessEnvironment::systemEnvironment().contains(QStringLiteral("QT_TEST_RUNNING_IN_CTEST")))
        QSKIP("This test does not run properly when invoked from CTest.");

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

void tst_qmake::include_missing()
{
    const QString workDir = base_path + "/testdata/include_missing";

    // qmake carries on after a missing file, but reports failure in its exit code.
    for (const char *name : { "missing", "scoped" }) {
        const QString makefile = QLatin1String("Makefile.") + name;
        QVERIFY(test_compiler.qmake(workDir, { QLatin1String(name) + ".pro", "-o", makefile },
                                    true));
        QVERIFY(QFile::exists(workDir + '/' + makefile));
        QVERIFY(QFile::remove(workDir + '/' + makefile));
    }

    // The silent flag of include() suppresses the error, and so does using the result of
    // include() as a condition.
    for (const char *name : { "silent", "handled" }) {
        const QString makefile = QLatin1String("Makefile.") + name;
        QVERIFY(test_compiler.qmake(workDir, { QLatin1String(name) + ".pro", "-o", makefile }));
        QVERIFY(QFile::remove(workDir + '/' + makefile));
    }
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
    QCOMPARE(QFileInfo(workDir + "/test.txt").lastModified(), QFileInfo(workDir + "/dist/test.txt").lastModified());
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
    qmake.setProcessChannelMode(QProcess::MergedChannels);
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

#if defined(Q_OS_DARWIN)

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

void tst_qmake::invalid_info_plist()
{
    QString workDir = base_path + "/testdata/invalid-info-plist";

    // We set up alternate arguments here, to make sure we're testing Mac
    // Bundles. We need to actually run make to check whether the failing
    // plutil invocation breaks the build.

    test_compiler.setArguments(QStringList(),
                               QStringList() << "-spec" << "macx-clang");

    QVERIFY( test_compiler.qmake(workDir, "invalid-info-plist") );

    // Make fails: plutil fails to parse the Info.plist file
    QVERIFY( test_compiler.make(workDir, QString(), true) );
}

#elif defined(Q_OS_WIN) // defined(Q_OS_DARWIN)

void tst_qmake::windowsResources()
{
    QString workDir = base_path + "/testdata/windows_resources";
    QVERIFY(test_compiler.qmake(workDir, "windows_resources"));
    QVERIFY(test_compiler.make(workDir));

    // Another "make" must not rebuild the .res file
    test_compiler.clearCommandOutput();
    QVERIFY(test_compiler.make(workDir));
    QVERIFY(!test_compiler.commandOutput().contains("windows_resources.rc"));
    test_compiler.clearCommandOutput();

    // Wait a second to make sure we get a new timestamp in the touch below
    QTest::qWait(1000);

    // Touch the deepest include of the .rc file
    QVERIFY(test_compiler.runCommand("cmd", QStringList{"/c",
                        "echo.>>" + QDir::toNativeSeparators(workDir + "/version.inc")}));

    // The next "make" must rebuild the .res file
    QVERIFY(test_compiler.make(workDir));
    QVERIFY(test_compiler.commandOutput().contains("windows_resources.rc"));
}

#endif // defined(Q_OS_WIN)

#ifdef Q_OS_WIN
void tst_qmake::shellQuotePlusSign()
{
    // The copy command treats a plus sign as a concatenation operator, so arguments with one
    // must be quoted.
    QString workDir = base_path + "/testdata/shell_quote_plus";
    test_compiler.clearCommandOutput();
    QVERIFY(test_compiler.qmake(workDir, "shell_quote_plus"));
    const QString output = test_compiler.commandOutput();
    QVERIFY2(output.contains("QUOTED=\"C:\\C++\\dir\\file.txt\""), qPrintable(output));
    QVERIFY2(output.contains("PLAIN=C:\\dir\\file.txt"), qPrintable(output));
    test_compiler.clearCommandOutput();
}

void tst_qmake::vcxprojExtraCompilersSharingInput()
{
#  ifndef Q_CC_MSVC
    QSKIP("The Visual Studio generator is only used with the MSVC spec.");
#  endif
    QString workDir = base_path + "/testdata/vcxproj_extra_compilers";
    QVERIFY(test_compiler.qmake(workDir, { "-tp", "vc", "extra_compilers.pro" }));

    QFile project(workDir + "/extra_compilers.vcxproj");
    QVERIFY(project.open(QFile::ReadOnly));
    const QByteArray contents = project.readAll();
    project.close();

    // All extra compilers of an input are merged into one custom build step. A repeated item
    // makes MSBuild fail.
    QCOMPARE(contents.count("<CustomBuild Include=\"gen.in\">"), 1);
    // The one step must still produce the output of both compilers.
    QVERIFY(contents.contains("gen_gen.h"));
    QVERIFY(contents.contains("gen_gen.cpp"));
}

void tst_qmake::vcxprojQrcContentFiles()
{
#  ifndef Q_CC_MSVC
    QSKIP("The Visual Studio generator is only used with the MSVC spec.");
#  endif
    QString workDir = base_path + "/testdata/vcxproj_qrc_content";
    QVERIFY(test_compiler.qmake(workDir, { "-tp", "vc", "qrc_content.pro" }));

    QFile project(workDir + "/qrc_content.vcxproj");
    QVERIFY(project.open(QFile::ReadOnly));
    const QByteArray contents = project.readAll();
    project.close();

    // Files that are also sources or headers must not become a second item of the project.
    // Visual Studio refuses to load a project that includes a file as ClCompile and as None.
    QVERIFY(contents.contains("<ClCompile Include=\"helper.cpp\""));
    QVERIFY(contents.contains("<ClCompile Include=\"sub\\deep.cpp\""));
    QVERIFY(contents.contains("<ClInclude Include=\"h.h\""));
    QVERIFY(!contents.contains("<None Include=\"helper.cpp\""));
    QVERIFY(!contents.contains("<None Include=\"Helper.cpp\"")); // Spelled like in the .qrc file.
    QVERIFY(!contents.contains("<None Include=\"sub\\deep.cpp\""));
    QVERIFY(!contents.contains("<None Include=\"h.h\""));
    // Other files of the resource stay in the project.
    QVERIFY(contents.contains("<None Include=\"data.txt\""));
}

void tst_qmake::vcxprojFilterFoldersInShadowBuild()
{
#  ifndef Q_CC_MSVC
    QSKIP("The Visual Studio generator is only used with the MSVC spec.");
#  endif
    QString workDir = base_path + "/testdata/vcxproj_filter_folders";
    QTemporaryDir buildDir;
    QVERIFY(buildDir.isValid());
    QVERIFY(test_compiler.qmake(buildDir.path(),
                                { "-tp", "vc", workDir + "/filter_folders.pro", "CONFIG-=flat" }));

    QFile filters(buildDir.path() + "/filter_folders.vcxproj.filters");
    QVERIFY(filters.open(QFile::ReadOnly));
    const QByteArray contents = filters.readAll();
    filters.close();

    // The folders follow the directories of the project, not the path from the build directory
    // to the sources, which starts with "..".
    QVERIFY(contents.contains("<Filter Include=\"Source Files\\Filter1\""));
    QVERIFY(contents.contains("<Filter Include=\"Header Files\\Filter1\""));
    QVERIFY2(!contents.contains("Files\\.."), contents.constData());
}

void tst_qmake::msvcDiagnosticsWithFullPaths()
{
#  ifndef Q_CC_MSVC
    QSKIP("The -FC option is only used with the MSVC spec.");
#  endif
    // Without -FC, the compiler prints the file name as qmake passes it, for example
    // ".\\main.cpp(3)". Then files with the same name in different directories look the same.
    QString workDir = base_path + "/testdata/msvc_full_paths";
    QVERIFY(test_compiler.qmake(workDir, "full_paths"));
    test_compiler.clearCommandOutput();
    QVERIFY(test_compiler.make(workDir, QString(), true)); // main.cpp does not compile
    const QString output = test_compiler.commandOutput();
    const QRegularExpression fullPath(R"([A-Za-z]:\\[^\n]*main\.cpp\(\d+\))");
    QVERIFY2(fullPath.match(output).hasMatch(), qPrintable(output));
    test_compiler.clearCommandOutput();
    QVERIFY(test_compiler.removeMakefile(workDir));
}
#endif

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

void tst_qmake::hostPathsInQtConf()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString prefix = QDir::cleanPath(dir.path());

    // The Host* keys are missing, as in the qt.conf files that package builders write.
    QFile conf(prefix + "/qt.conf");
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    const QString text = QStringLiteral("[Paths]\nPrefix = %1\nBinaries = custom/bin\n"
                                        "LibraryExecutables = custom/libexec\n"
                                        "Libraries = custom/lib\n")
                                 .arg(prefix);
    conf.write(text.toUtf8());
    conf.close();

    test_compiler.clearCommandOutput();
    QVERIFY(test_compiler.qmake(prefix, { "-qtconf", conf.fileName(), "-query" }));
    const QString output = test_compiler.commandOutput();
    QVERIFY2(output.contains("QT_HOST_BINS:" + prefix + "/custom/bin"), qPrintable(output));
    QVERIFY2(output.contains("QT_HOST_LIBEXECS:" + prefix + "/custom/libexec"), qPrintable(output));
    QVERIFY2(output.contains("QT_HOST_LIBS:" + prefix + "/custom/lib"), qPrintable(output));
    test_compiler.clearCommandOutput();
}

void tst_qmake::qinstall()
{
    const QString testName = "qinstall";
    QDir testDataDir = base_path + "/testdata";
    if (testDataDir.exists(testName))
        testDataDir.rmdir(testName);
    QVERIFY(testDataDir.mkdir(testName));
    const QString workDir = testDataDir.filePath(testName);
    auto qinstall = [&](const QString &src, const QString &dst, bool executable = false) {
                        QStringList args = {"-install", "qinstall"};
                        if (executable)
                            args << "-exe";
                        args << src << dst;
                        return test_compiler.qmake(workDir, args);
                    };
    const QFileDevice::Permissions readFlags
            = QFileDevice::ReadOwner | QFileDevice::ReadUser
            | QFileDevice::ReadGroup | QFileDevice::ReadOther;
    const QFileDevice::Permissions writeFlags
            = QFileDevice::WriteOwner | QFileDevice::WriteUser
            | QFileDevice::WriteGroup | QFileDevice::WriteOther;
    const QFileDevice::Permissions exeFlags
            = QFileDevice::ExeOwner | QFileDevice::ExeUser
            | QFileDevice::ExeGroup | QFileDevice::ExeOther;

    // install a regular file
    {
        QFileInfo src(testDataDir.filePath("project/main.cpp"));
        QFileInfo dst("foo.cpp");
        QVERIFY(qinstall(src.filePath(), dst.filePath()));
        QVERIFY(dst.exists());
        QCOMPARE(src.size(), dst.size());
        QVERIFY(dst.permissions() & readFlags);
        QVERIFY(dst.permissions() & writeFlags);
        QVERIFY(!(dst.permissions() & exeFlags));
        test_compiler.clearCommandOutput();
    }

    // install an executable file
    {
        const QString mocFilePath = QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath)
                + "/moc"
#ifdef Q_OS_WIN
                + ".exe"
#endif
                ;
        QFileInfo src(mocFilePath);
        QVERIFY(src.exists());
        QVERIFY(src.permissions() & exeFlags);
        QFileInfo dst("copied_" + src.fileName());
        QVERIFY(qinstall(src.filePath(), dst.filePath(), true));
        QVERIFY(dst.exists());
        QCOMPARE(src.size(), dst.size());
        QVERIFY(dst.permissions() & readFlags);
        QVERIFY(dst.permissions() & writeFlags);
        QVERIFY(dst.permissions() & exeFlags);
        test_compiler.clearCommandOutput();
    }

    // install a read-only file
    {
        QFile srcfile("foo.cpp");
        QVERIFY(srcfile.setPermissions(srcfile.permissions() & ~writeFlags));
        QFileInfo src(srcfile);
        QFileInfo dst("bar.cpp");
        QVERIFY(qinstall(src.filePath(), dst.filePath()));
        QVERIFY(dst.exists());
        QCOMPARE(src.size(), dst.size());
        QVERIFY(dst.permissions() & readFlags);
        QVERIFY(dst.permissions() & writeFlags);
        QVERIFY(!(dst.permissions() & exeFlags));
        test_compiler.clearCommandOutput();
    }

    // install a directory
    {
        QDir src = testDataDir;
        src.cd("project");
        QDir dst("narf");
        QVERIFY(qinstall(src.absolutePath(), dst.absolutePath()));
        QCOMPARE(src.entryList(QDir::Files, QDir::Name), dst.entryList(QDir::Files, QDir::Name));
        test_compiler.clearCommandOutput();
    }

    // install a directory with a read-only file
    {
        QDir src("narf");
        QFile srcfile(src.filePath("main.cpp"));
        QVERIFY(srcfile.setPermissions(srcfile.permissions() & ~writeFlags));
        QDir dst("zort");
        QVERIFY(qinstall(src.absolutePath(), dst.absolutePath()));
        QCOMPARE(src.entryList(QDir::Files, QDir::Name), dst.entryList(QDir::Files, QDir::Name));
    }
}

void tst_qmake::duplicateInputs()
{
    QString workDir = base_path + "/testdata/duplicate_inputs";
    QVERIFY(test_compiler.qmake(workDir, "duplicate_inputs"));

    QFile makefile(workDir + "/Makefile");
    QVERIFY(makefile.open(QFile::ReadOnly));
    const QString contents = QString::fromLocal8Bit(makefile.readAll());
    makefile.close();

    // A source that is listed several times, also with different spellings, gets one rule.
    const QRegularExpression ruleForHelper("^\\S*helper\\.(o|obj):",
                                           QRegularExpression::MultilineOption);
    QCOMPARE(contents.count(ruleForHelper), 1);

    // Other lists keep their duplicates and their order.
    QVERIFY2(contents.contains("-Iinc1 -Iinc2 -Iinc1"), qPrintable(contents));

    QVERIFY(test_compiler.removeMakefile(workDir));
}

void tst_qmake::embedTranslations()
{
    // Each binary needs its own resource name. Otherwise the init functions of the embedded
    // translations clash, and the translations of shared libraries are not registered.
    QString workDir = base_path + "/testdata/embed_translations";
    QVERIFY(test_compiler.qmake(workDir, "first"));
    QVERIFY(test_compiler.qmake(workDir, "second"));

    auto readFile = [&workDir](const QString &fileName) {
        QFile file(workDir + '/' + fileName);
        return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
    };
    QVERIFY(readFile("qmake_qmake_qm_files_first.qrc").contains("first_de.qm"));
    QVERIFY(readFile("qmake_qmake_qm_files_second_target_x.qrc").contains("second_de.qm"));
    QVERIFY(!QFile::exists(workDir + "/qmake_qmake_qm_files.qrc"));
}

void tst_qmake::resources()
{
    QString workDir = base_path + "/testdata/resources";
    QVERIFY(test_compiler.qmake(workDir, "resources"));

    {
        QFile qrcFile(workDir + '/' + "qmake_pro_file.qrc");
        QVERIFY2(qrcFile.exists(), qPrintable(qrcFile.fileName()));
        QVERIFY(qrcFile.open(QFile::ReadOnly));
        QByteArray qrcXml = qrcFile.readAll();
        QVERIFY(qrcXml.contains("alias=\"resources.pro\""));
        QVERIFY(qrcXml.contains("prefix=\"/prefix\""));
    }

    {
        QFile qrcFile(workDir + '/' + "qmake_subdir.qrc");
        QVERIFY(qrcFile.exists());
        QVERIFY(qrcFile.open(QFile::ReadOnly));
        QByteArray qrcXml = qrcFile.readAll();
        QVERIFY(qrcXml.contains("alias=\"file.txt\""));
    }

    {
        QFile qrcFile(workDir + '/' + "qmake_qmake_immediate.qrc");
        QVERIFY(qrcFile.exists());
        QVERIFY(qrcFile.open(QFile::ReadOnly));
        QByteArray qrcXml = qrcFile.readAll();
        QVERIFY(qrcXml.contains("alias=\"main.cpp\""));
    }

    QVERIFY(test_compiler.make(workDir));
}

void tst_qmake::resourcesSameName()
{
    // The generated files and the init function are named after the .qrc file, so two files
    // with the same name in one project clash.
    QString workDir = base_path + "/testdata/resources_same_name";
    test_compiler.clearCommandOutput();
    QVERIFY(!test_compiler.qmake(workDir, "clash"));
    QVERIFY2(test_compiler.commandOutput().contains("have the same name"),
             qPrintable(test_compiler.commandOutput()));
    test_compiler.clearCommandOutput();

    // Listing a file more than once is not a clash.
    QVERIFY(test_compiler.qmake(workDir, "same_file"));
}

void tst_qmake::conflictingTargets()
{
    QString workDir = base_path + "/testdata/conflicting_targets";
    QVERIFY(test_compiler.qmake(workDir, "conflicting_targets"));
    const QRegularExpression rex("Targets of builds '([^']+)' and '([^']+)' conflict");
    auto match = rex.match(test_compiler.commandOutput());
    QVERIFY(match.hasMatch());
    QStringList builds = { match.captured(1), match.captured(2) };
    std::sort(builds.begin(), builds.end());
    const QStringList expectedBuilds{"Debug", "Release"};
    QCOMPARE(builds, expectedBuilds);
}

QTEST_MAIN(tst_qmake)
#include "tst_qmake.moc"
