// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QStandardPaths>
#include <qcoreapplication.h>
#include <qstring.h>
#include <qtemporarydir.h>
#include <qfile.h>
#include <qdir.h>
#include <qset.h>
#include <QtTest/private/qtesthelpers_p.h>
#ifdef Q_OS_WIN
# include <shlwapi.h>
# include <qt_windows.h>
#endif
#ifdef Q_OS_UNIX // for geteuid()
# include <sys/types.h>
# include <unistd.h>
#endif

#ifdef Q_OS_INTEGRITY
#include "qplatformdefs.h"
#endif

#include <optional>

using namespace Qt::StringLiterals;

class tst_QTemporaryDir : public QObject
{
    Q_OBJECT
public:
public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void construction();
    void moveSemantics();
    void fileTemplate();
    void fileTemplate_data();
    void getSetCheck();
    void fileName();
    void filePath_data();
    void filePath();
    void autoRemove();
    void nonWritableCurrentDir();
    void openOnRootDrives();
    void stressTest();
    void rename();

    void QTBUG_4796_data();
    void QTBUG_4796();

    void nestedTempDirs();

private:
    QString m_previousCurrent;
};

void tst_QTemporaryDir::initTestCase()
{
    m_previousCurrent = QDir::currentPath();
    QDir::setCurrent(QDir::tempPath());
    QVERIFY(QDir("test-XXXXXX").exists() || QDir().mkdir("test-XXXXXX"));
    QCoreApplication::setApplicationName("tst_qtemporarydir");
}

void tst_QTemporaryDir::cleanupTestCase()
{
    QVERIFY(QDir().rmdir("test-XXXXXX"));

    QDir::setCurrent(m_previousCurrent);
}

void tst_QTemporaryDir::construction()
{
    QTemporaryDir dir;
    QString tmp = QDir::tempPath();
    QCOMPARE(dir.path().left(tmp.size()), tmp);
    QVERIFY(dir.path().contains("tst_qtemporarydir"));
    QVERIFY(QFileInfo(dir.path()).isDir());
    QCOMPARE(dir.errorString(), QString());
}

void tst_QTemporaryDir::moveSemantics()
{
    {
        auto original = std::optional<QTemporaryDir>(std::in_place);
        QVERIFY(original->isValid());

        original->setAutoRemove(true);

        auto OriginalDirectoryInfo = QFileInfo(original->path());
        OriginalDirectoryInfo.setCaching(false);
        QVERIFY(OriginalDirectoryInfo.exists());

        QTemporaryDir movedInto = std::move(*original);

        original.reset();

        QVERIFY(OriginalDirectoryInfo.exists());
        QVERIFY(movedInto.path() == OriginalDirectoryInfo.filePath());
    }

    {
        auto movedInto = QTemporaryDir();
        QVERIFY(movedInto.isValid());

        movedInto.setAutoRemove(true);

        auto movedIntoInitialDirectoryInfo = QFileInfo(movedInto.path());
        movedIntoInitialDirectoryInfo.setCaching(false);
        QVERIFY(movedIntoInitialDirectoryInfo.exists());

        auto OriginalDirectoryInfo = QFileInfo();
        OriginalDirectoryInfo.setCaching(false);

        {
            auto original = QTemporaryDir();
            QVERIFY(original.isValid());

            original.setAutoRemove(true);

            OriginalDirectoryInfo.setFile(original.path());
            QVERIFY(OriginalDirectoryInfo.exists());

            movedInto = std::move(original);
        }

        QVERIFY(!movedIntoInitialDirectoryInfo.exists());
        QVERIFY(OriginalDirectoryInfo.exists());

        QVERIFY(movedInto.path() == OriginalDirectoryInfo.filePath());
    }
}

// Testing get/set functions
void tst_QTemporaryDir::getSetCheck()
{
    QTemporaryDir obj1;
    // bool QTemporaryDir::autoRemove()
    // void QTemporaryDir::setAutoRemove(bool)
    obj1.setAutoRemove(false);
    QCOMPARE(false, obj1.autoRemove());
    obj1.setAutoRemove(true);
    QCOMPARE(true, obj1.autoRemove());
}

static QString hanTestText()
{
    QString text;
    text += QChar(0x65B0);
    text += QChar(0x5E10);
    text += QChar(0x6237);
    return text;
}

static QString umlautTestText()
{
    QString text;
    text += QChar(0xc4);
    text += QChar(0xe4);
    text += QChar(0xd6);
    text += QChar(0xf6);
    text += QChar(0xdc);
    text += QChar(0xfc);
    text += QChar(0xdf);
    return text;
}

void tst_QTemporaryDir::fileTemplate_data()
{
    QTest::addColumn<QString>("constructorTemplate");
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("suffix");

    QTest::newRow("default") << "" << "tst_qtemporarydir-" << "";

    QTest::newRow("xxx-suffix") << "qt_XXXXXXxxx" << "qt_" << "xxx";
    QTest::newRow("xXx-suffix") << "qt_XXXXXXxXx" << "qt_" << "xXx";
    QTest::newRow("no-suffix") << "qt_XXXXXX" << "qt_" << "";
    QTest::newRow("10X") << "qt_XXXXXXXXXX" << "qt_" << "";
    QTest::newRow("4Xsuffix") << "qt_XXXXXX_XXXX" << "qt_" << "_XXXX";
    QTest::newRow("4Xprefix") << "qt_XXXX" << "qt_XXXX" << "";
    QTest::newRow("5Xprefix") << "qt_XXXXX" << "qt_XXXXX" << "";
    QTest::newRow("two placeholders") << "qt_XXXXXX_XXXXXX_" << "qt_XXXXXX_" << "_";
    if (QTestPrivate::canHandleUnicodeFileNames()) {
        // Test Umlauts (contained in Latin1)
        QString prefix = "qt_" + umlautTestText();
        QTest::newRow("Umlauts") << (prefix + "XXXXXX") << prefix << "";
        // test non-Latin1
        prefix = "qt_" + hanTestText();
        QTest::newRow("Chinese") << (prefix + "XXXXXX" + umlautTestText()) << prefix << umlautTestText();
    }

#ifdef Q_OS_WIN
    auto tmp = QDir::toNativeSeparators(QDir::tempPath());
    if (PathGetDriveNumber((const wchar_t *) tmp.utf16()) < 0)
        return; // skip if we have no drive letter

    tmp.data()[1] = u'$';
    const auto tmpPath = tmp + uR"(\UNC.XXXXXX.tmpDir)"_s;

    QTest::newRow("UNC-backslash")
            << uR"(\\localhost\)"_s + tmpPath << "UNC."
            << ".tmpDir";
    QTest::newRow("UNC-prefix")
            << uR"(\\?\UNC\localhost\)"_s + tmpPath << "UNC."
            << ".tmpDir";
    QTest::newRow("UNC-slash")
            << u"//localhost/"_s + QDir::fromNativeSeparators(tmpPath) << "UNC."
            << ".tmpDir";
    QTest::newRow("UNC-prefix-slash")
            << uR"(//?/UNC/localhost/)"_s + QDir::fromNativeSeparators(tmpPath) << "UNC."
            << ".tmpDir";
#endif
}

void tst_QTemporaryDir::fileTemplate()
{
    QFETCH(QString, constructorTemplate);
    QFETCH(QString, prefix);
    QFETCH(QString, suffix);

    QTemporaryDir tempDir(constructorTemplate);

    QVERIFY(tempDir.isValid());

    QString dirName = QDir(tempDir.path()).dirName();
    if (prefix.size()) {
        QCOMPARE(dirName.left(prefix.size()), prefix);
        QCOMPARE(dirName.right(suffix.size()), suffix);
    }
}


/*
    This tests whether the temporary dir really gets placed in QDir::tempPath
*/
void tst_QTemporaryDir::fileName()
{
    // Get QDir::tempPath and make an absolute path.
    QString tempPath = QDir::tempPath();
    QString absoluteTempPath = QDir(tempPath).absolutePath();
    QTemporaryDir dir;
    dir.setAutoRemove(true);
    QString fileName = dir.path();
    QVERIFY2(fileName.contains("/tst_qtemporarydir-"), qPrintable(fileName));
    QVERIFY(QDir(fileName).exists());
    // Get path to the temp dir, without the file name.
    QString absoluteFilePath = QFileInfo(fileName).absolutePath();
#if defined(Q_OS_WIN)
    absoluteFilePath = absoluteFilePath.toLower();
    absoluteTempPath = absoluteTempPath.toLower();
#endif
    QCOMPARE(absoluteFilePath, absoluteTempPath);
}

void tst_QTemporaryDir::filePath_data()
{
    QTest::addColumn<QString>("templatePath");
    QTest::addColumn<QString>("fileName");

    QTest::newRow("0") << QString() << "/tmpfile";
    QTest::newRow("1") << QString() << "tmpfile";
    QTest::newRow("2") << "XXXXX" << "tmpfile";
    QTest::newRow("3") << "YYYYY" << "subdir/file";
}

void tst_QTemporaryDir::filePath()
{
    QFETCH(QString, templatePath);
    QFETCH(QString, fileName);

    QTemporaryDir dir(templatePath);
    const QString filePath = dir.filePath(fileName);
    const QString expectedFilePath = QDir::isAbsolutePath(fileName) ?
                                     QString() : dir.path() + QLatin1Char('/') + fileName;
    QCOMPARE(filePath, expectedFilePath);
}

void tst_QTemporaryDir::autoRemove()
{
    // Test auto remove
    QString dirName;
    {
        QTemporaryDir dir("tempXXXXXX");
        dir.setAutoRemove(true);
        QVERIFY(dir.isValid());
        dirName = dir.path();
    }
#ifdef Q_OS_WIN
    // Windows seems unreliable here: sometimes it says the directory still exists,
    // immediately after we deleted it.
    QTRY_VERIFY(!QDir(dirName).exists());
#else
    QVERIFY(!QDir(dirName).exists());
#endif

    // Test if disabling auto remove works.
    {
        QTemporaryDir dir("tempXXXXXX");
        dir.setAutoRemove(false);
        QVERIFY(dir.isValid());
        dirName = dir.path();
    }
    QVERIFY(QDir(dirName).exists());
    QVERIFY(QDir().rmdir(dirName));
    QVERIFY(!QDir(dirName).exists());

    // Do not explicitly call setAutoRemove (tests if it really is the default as documented)
    {
        QTemporaryDir dir("tempXXXXXX");
        QVERIFY(dir.isValid());
        dirName = dir.path();
    }
#ifdef Q_OS_WIN
    QTRY_VERIFY(!QDir(dirName).exists());
#else
    QVERIFY(!QDir(dirName).exists());
#endif

    // Test autoremove with files and subdirs in the temp dir
    {
        QTemporaryDir tempDir("tempXXXXXX");
        QVERIFY(tempDir.isValid());
        dirName = tempDir.path();
        QDir dir(dirName);
        QVERIFY(dir.mkdir(QString::fromLatin1("dir1")));
        QVERIFY(dir.mkdir(QString::fromLatin1("dir2")));
        QVERIFY(dir.mkdir(QString::fromLatin1("dir2/nested")));
        QFile file(dirName + "/dir1/file");
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("Hello"), 5LL);
        file.close();
        QVERIFY(file.setPermissions(QFile::ReadUser));
    }
#ifdef Q_OS_WIN
    QTRY_VERIFY(!QDir(dirName).exists());
#else
    QVERIFY(!QDir(dirName).exists());
#endif
}

void tst_QTemporaryDir::nonWritableCurrentDir()
{
#ifdef Q_OS_UNIX

#  ifdef Q_OS_ANDROID
    const char nonWritableDir[] = "/data";
#  else
    const char nonWritableDir[] = "/home";
#  endif

    if (::geteuid() == 0)
        QSKIP("not valid running this test as root");

    struct ChdirOnReturn
    {
        ChdirOnReturn(const QString& d) : dir(d) {}
        ~ChdirOnReturn() {
            QDir::setCurrent(dir);
        }
        QString dir;
    };

    const QFileInfo nonWritableDirFi = QFileInfo(QLatin1String(nonWritableDir));
    QVERIFY(nonWritableDirFi.isDir());

    QVERIFY(!nonWritableDirFi.isWritable());

    ChdirOnReturn cor(QDir::currentPath());
    QVERIFY(QDir::setCurrent(nonWritableDirFi.absoluteFilePath()));
    // QTemporaryDir("tempXXXXXX") is probably a bad idea in any app
    // where the current dir could anything...
    QTemporaryDir dir("tempXXXXXX");
    dir.setAutoRemove(true);
    QVERIFY(!dir.isValid());
    QVERIFY(!dir.errorString().isEmpty());
    QVERIFY(dir.path().isEmpty());
#endif
}

void tst_QTemporaryDir::openOnRootDrives()
{
#if defined(Q_OS_WIN)
    unsigned int lastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
    // If it's possible to create a file in the root directory, it
    // must be possible to create a temp dir there too.
    const auto drives = QDir::drives();
    for (const QFileInfo &driveInfo : drives) {
        QFile testFile(driveInfo.filePath() + "XXXXXX");
        if (testFile.open(QIODevice::ReadWrite)) {
            testFile.remove();
            QTemporaryDir dir(driveInfo.filePath() + "XXXXXX");
            dir.setAutoRemove(true);
            QVERIFY(dir.isValid());
        }
    }
#if defined(Q_OS_WIN)
    SetErrorMode(lastErrorMode);
#endif
}

void tst_QTemporaryDir::stressTest()
{
    const int iterations = 1000;
    QTemporaryDir rootDir;
    QVERIFY(rootDir.isValid());

    QSet<QString> names;
    const QString pattern = rootDir.path() + QStringLiteral("/XXXXXX");
    for (int i = 0; i < iterations; ++i) {
        QTemporaryDir dir(pattern);
        dir.setAutoRemove(false);
        QVERIFY2(dir.isValid(),
                 qPrintable(QString::fromLatin1("Failed to create #%1 under %2: %3.")
                            .arg(i)
                            .arg(QDir::toNativeSeparators(pattern))
                            .arg(dir.errorString())));
        QVERIFY(!names.contains(dir.path()));
        names.insert(dir.path());
    }
}

void tst_QTemporaryDir::rename()
{
    // This test checks what happens if the temporary dir is renamed.
    // Then the autodelete feature can't possibly find it.

    QDir dir;
    QVERIFY(!dir.exists("temporary-dir.renamed"));

    QString tempname;
    {
        QTemporaryDir tempDir(dir.filePath("temporary-dir.XXXXXX"));

        QVERIFY(tempDir.isValid());
        tempname = tempDir.path();

        QVERIFY(QDir().rename(tempname, "temporary-dir.renamed"));
        QVERIFY(!QDir(tempname).exists());
        dir.setPath("temporary-dir.renamed");
        QCOMPARE(dir.path(), QString("temporary-dir.renamed"));
        QVERIFY(dir.exists());
    }

    // Auto-delete couldn't find it
    QVERIFY(dir.exists());
    // Clean up by hand
    QVERIFY(dir.removeRecursively());
    QVERIFY(!dir.exists());
}

void tst_QTemporaryDir::QTBUG_4796_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<bool>("openResult");

    QString unicode = QString::fromUtf8("\xc3\xa5\xc3\xa6\xc3\xb8");

    QTest::newRow("<empty>") << QString() << QString() << true;
    QTest::newRow(".") << QString(".") << QString() << true;
    QTest::newRow("..") << QString("..") << QString() << true;
    QTest::newRow("blaXXXXXX") << QString("bla") << QString() << true;
    QTest::newRow("does-not-exist/qt_temp.XXXXXX") << QString("does-not-exist/qt_temp") << QString() << false;
    QTest::newRow("XXXXXX<unicode>") << QString() << unicode << true;
    QTest::newRow("<unicode>XXXXXX") << unicode << QString() << true;
}

void tst_QTemporaryDir::QTBUG_4796() // unicode support
{
    QVERIFY(QDir("test-XXXXXX").exists());

    struct CleanOnReturn
    {
        ~CleanOnReturn()
        {
            for (const QString &tempName : std::as_const(tempNames))
                QVERIFY(QDir(tempName).removeRecursively());
        }

        void reset()
        {
            tempNames.clear();
        }

        QStringList tempNames;
    };

    CleanOnReturn cleaner;

    QFETCH(QString, prefix);
    QFETCH(QString, suffix);
    QFETCH(bool, openResult);

    {
        QString fileTemplate1 = prefix + QString("XX") + suffix;
        QString fileTemplate2 = prefix + QString("XXXX") + suffix;
        QString fileTemplate3 = prefix + QString("XXXXXX") + suffix;
        QString fileTemplate4 = prefix + QString("XXXXXXXX") + suffix;

        QTemporaryDir dir1(fileTemplate1);
        QTemporaryDir dir2(fileTemplate2);
        QTemporaryDir dir3(fileTemplate3);
        QTemporaryDir dir4(fileTemplate4);
        QTemporaryDir dir5("test-XXXXXX/" + fileTemplate1);
        QTemporaryDir dir6("test-XXXXXX/" + fileTemplate3);

        QCOMPARE(dir1.isValid(), openResult);
        QCOMPARE(dir2.isValid(), openResult);
        QCOMPARE(dir3.isValid(), openResult);
        QCOMPARE(dir4.isValid(), openResult);
        QCOMPARE(dir5.isValid(), openResult);
        QCOMPARE(dir6.isValid(), openResult);

        // make sure the dir exists under the *correct* name
        if (openResult) {
            cleaner.tempNames << dir1.path()
                << dir2.path()
                << dir3.path()
                << dir4.path()
                << dir5.path()
                << dir6.path();

            QDir currentDir;
            QString fileName1 = currentDir.relativeFilePath(dir1.path());
            QString fileName2 = currentDir.relativeFilePath(dir2.path());
            QString fileName3 = currentDir.relativeFilePath(dir3.path());
            QString fileName4 = currentDir.relativeFilePath(dir4.path());
            QString fileName5 = currentDir.relativeFilePath(dir5.path());
            QString fileName6 = currentDir.relativeFilePath(dir6.path());

            QVERIFY(fileName1.startsWith(prefix));
            QVERIFY(fileName2.startsWith(prefix));
            QVERIFY(fileName5.startsWith("test-XXXXXX/" + prefix));
            QVERIFY(fileName6.startsWith("test-XXXXXX/" + prefix));

            if (!prefix.isEmpty()) {
                QVERIFY(fileName3.startsWith(prefix));
                QVERIFY(fileName4.startsWith(prefix));
            }
        }
    }

#ifdef Q_OS_WIN
    QTest::qWait(20);
#endif
    for (const QString &tempName : std::as_const(cleaner.tempNames))
        QVERIFY2(!QDir(tempName).exists(), qPrintable(tempName));

    cleaner.reset();
}

void tst_QTemporaryDir::nestedTempDirs()
{
    QTemporaryDir parentDir;
    const QString &parentPath = parentDir.path();

    {
        QTemporaryDir tempdir(parentPath);
    }

    QDir dir(parentPath);
    dir.setFilter(QDir::NoDotAndDotDot);
    QCOMPARE(dir.count(), 0);
}

QTEST_MAIN(tst_QTemporaryDir)
#include "tst_qtemporarydir.moc"
