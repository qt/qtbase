// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qcoreapplication.h>
#include <qfile.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qstandardpaths.h>
#include <qstring.h>
#include <qtemporarydir.h>
#include <qtemporaryfile.h>

#include <QtTest/private/qtesthelpers_p.h>

#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/qscopeguard.h>

#include <optional>

#if defined(Q_OS_WIN)
# include <shlwapi.h>
# include <qt_windows.h>
#endif
#if defined(Q_OS_UNIX)
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <fcntl.h>             // open(2)
# include <unistd.h>            // close(2)
#endif

#ifdef Q_OS_ANDROID
#include <QDirIterator>
#include <QStandardPaths>
#endif

#ifdef Q_OS_INTEGRITY
#include "qplatformdefs.h"
#endif

using namespace Qt::StringLiterals;

class tst_QTemporaryFile : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void construction();
    void fileTemplate();
    void fileTemplate_data();
    void getSetCheck();
    void fileName();
    void fileNameIsEmpty();
    void autoRemove();
    void nonWritableCurrentDir();
    void io();
    void openCloseOpenClose();
    void removeAndReOpen();
    void removeUnnamed();
    void size();
    void resize();
    void openOnRootDrives();
    void stressTest();
    void rename_data();
    void rename();
    void renameFdLeak();
    void moveToTrash();
    void reOpenThroughQFile();
    void keepOpenMode();
    void resetTemplateAfterError();
    void setTemplateAfterOpen();
    void autoRemoveAfterFailedRename();
    void createNativeFile_data();
    void createNativeFile();
    void QTBUG_4796_data();
    void QTBUG_4796();
    void guaranteeUnique();
    void stdfilesystem();
private:
    QTemporaryDir m_temporaryDir;
    QString m_previousCurrent;
};

void tst_QTemporaryFile::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));

    QVERIFY2(m_temporaryDir.isValid(), qPrintable(m_temporaryDir.errorString()));
    m_previousCurrent = QDir::currentPath();
    QVERIFY(QDir::setCurrent(m_temporaryDir.path()));

    // For QTBUG_4796
    QVERIFY(QDir("test-XXXXXX").exists() || QDir().mkdir("test-XXXXXX"));
    QCoreApplication::setApplicationName("tst_qtemporaryfile");

#ifdef Q_OS_ANDROID
    QString sourceDir(":/android_testdata/");
    QDirIterator it(sourceDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo sourceFileInfo = it.nextFileInfo();
        if (!sourceFileInfo.isDir()) {
            QFileInfo destinationFileInfo(
                QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/')
                + sourceFileInfo.filePath().mid(sourceDir.length()));

            if (!destinationFileInfo.exists()) {
                QVERIFY(QDir().mkpath(destinationFileInfo.path()));
                QVERIFY(QFile::copy(sourceFileInfo.filePath(), destinationFileInfo.filePath()));
            }
        }
    }
#endif
}

void tst_QTemporaryFile::cleanupTestCase()
{
    QDir::setCurrent(m_previousCurrent);
}

void tst_QTemporaryFile::construction()
{
    QTemporaryFile file(0);
    QString tmp = QDir::tempPath();
    QCOMPARE(file.fileTemplate().left(tmp.size()), tmp);
    QCOMPARE(file.fileTemplate().at(tmp.size()), QChar('/'));
}

// Testing get/set functions
void tst_QTemporaryFile::getSetCheck()
{
    QTemporaryFile obj1;
    // bool QTemporaryFile::autoRemove()
    // void QTemporaryFile::setAutoRemove(bool)
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

void tst_QTemporaryFile::fileTemplate_data()
{
    QTest::addColumn<QString>("constructorTemplate");
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<QString>("fileTemplate");

    QTest::newRow("constructor default") << "" << "." << "" << "";
    QTest::newRow("constructor with xxx sufix") << "qt_XXXXXXxxx" << "qt_" << "xxx" << "";
    QTest::newRow("constructor with xXx sufix") << "qt_XXXXXXxXx" << "qt_" << "xXx" << "";
    QTest::newRow("constructor with no sufix") << "qt_XXXXXX" << "qt_" << "" << "";
    QTest::newRow("constructor with >6 X's and xxx suffix") << "qt_XXXXXXXXXXxxx" << "qt_" << "xxx" << "";
    QTest::newRow("constructor with >6 X's, no suffix") << "qt_XXXXXXXXXX" << "qt_" << "" << "";

    QTest::newRow("constructor with XXXX suffix") << "qt_XXXXXX_XXXX" << "qt_" << "_XXXX" << "";
    QTest::newRow("constructor with XXXXX suffix") << "qt_XXXXXX_XXXXX" << "qt_" << "_XXXXX" << "";
    QTest::newRow("constructor with XXXX prefix") << "qt_XXXX" << "qt_XXXX." << "" << "";
    QTest::newRow("constructor with XXXXX prefix") << "qt_XXXXX" << "qt_XXXXX." << "" << "";
    QTest::newRow("constructor with XXXX  prefix and suffix") << "qt_XXXX_XXXXXX_XXXX" << "qt_XXXX_" << "_XXXX" << "";
    QTest::newRow("constructor with XXXXX prefix and suffix") << "qt_XXXXX_XXXXXX_XXXXX" << "qt_XXXXX_" << "_XXXXX" << "";
    QTest::newRow("constructor with two placeholders") << "qt_XXXXXX_XXXXXX_" << "qt_XXXXXX_" << "_" << "";

    QTest::newRow("set template, no suffix") << "" << "foo" << "" << "foo";
    QTest::newRow("set template, with lowercase XXXXXX") << "" << "qt_" << "xxxxxx" << "qt_XXXXXXxxxxxx";
    QTest::newRow("set template, with xxx") << "" << "qt_" << ".xxx" << "qt_XXXXXX.xxx";
    QTest::newRow("set template, with >6 X's") << "" << "qt_" << ".xxx" << "qt_XXXXXXXXXXXXXX.xxx";
    QTest::newRow("set template, with >6 X's, no suffix") << "" << "qt_" << "" << "qt_XXXXXXXXXXXXXX";
    if (QTestPrivate::canHandleUnicodeFileNames()) {
        // Test Umlauts (contained in Latin1)
        QString prefix = "qt_" + umlautTestText();
        QTest::newRow("Umlauts") << (prefix + "XXXXXX") << prefix << QString() << QString();
        // Test Chinese
        prefix = "qt_" + hanTestText();
        QTest::newRow("Chinese characters") << (prefix + "XXXXXX") << prefix << QString() << QString();
    }

#ifdef Q_OS_WIN
    auto tmp = QDir::toNativeSeparators(QDir::tempPath());
    if (PathGetDriveNumber((const wchar_t *) tmp.utf16()) < 0)
        return; // skip if we have no drive letter

    tmp.data()[1] = u'$';
    const auto tmpPath = tmp + uR"(\QTBUG-74291.XXXXXX.tmpFile)"_s;

    QTest::newRow("UNC-backslash")
            << uR"(\\localhost\)"_s + tmpPath << "QTBUG-74291."
            << ".tmpFile"
            << "";
    QTest::newRow("UNC-prefix")
            << uR"(\\?\UNC\localhost\)"_s + tmpPath << "QTBUG-74291."
            << ".tmpFile"
            << "";
    QTest::newRow("UNC-slash")
            << u"//localhost/"_s + QDir::fromNativeSeparators(tmpPath) << "QTBUG-74291."
            << ".tmpFile"
            << "";
    QTest::newRow("UNC-prefix-slash")
            << uR"(//?/UNC/localhost/)"_s + QDir::fromNativeSeparators(tmpPath) << "QTBUG-74291."
            << ".tmpFile"
            << "";
#endif
}

void tst_QTemporaryFile::fileTemplate()
{
    QFETCH(QString, constructorTemplate);
    QFETCH(QString, prefix);
    QFETCH(QString, suffix);
    QFETCH(QString, fileTemplate);

    QTemporaryFile file(constructorTemplate);
    if (!fileTemplate.isEmpty())
        file.setFileTemplate(fileTemplate);

    QVERIFY2(file.open(), qPrintable(file.errorString()));

    QString fileName = QFileInfo(file).fileName();
    if (prefix.size())
        QCOMPARE(fileName.left(prefix.size()), prefix);

    if (suffix.size())
        QCOMPARE(fileName.right(suffix.size()), suffix);
}


/*
    This tests whether the temporary file really gets placed in QDir::tempPath
*/
void tst_QTemporaryFile::fileName()
{
    // Get QDir::tempPath and make an absolute path.
    QString tempPath = QDir::tempPath();
    QString absoluteTempPath = QDir(tempPath).absolutePath();
    QTemporaryFile file;
    file.setAutoRemove(true);
    QVERIFY(file.open());
    QString fileName = file.fileName();
    QVERIFY2(fileName.contains("/tst_qtemporaryfile."), qPrintable(fileName));
    QVERIFY(QFile::exists(fileName));
    // Get path to the temp file, without the file name.
    QString absoluteFilePath = QFileInfo(fileName).absolutePath();
#if defined(Q_OS_WIN)
    absoluteFilePath = absoluteFilePath.toLower();
    absoluteTempPath = absoluteTempPath.toLower();
#endif
    QCOMPARE(absoluteFilePath, absoluteTempPath);
}

void tst_QTemporaryFile::fileNameIsEmpty()
{
    QString filename;
    {
        QTemporaryFile file;
        QVERIFY(file.fileName().isEmpty());

        QVERIFY(file.open());
        QVERIFY(!file.fileName().isEmpty());

        filename = file.fileName();
        QVERIFY(QFile::exists(filename));

        file.close();
        QVERIFY(!file.isOpen());
        QVERIFY(QFile::exists(filename));
        QVERIFY(!file.fileName().isEmpty());
    }
    QVERIFY(!QFile::exists(filename));
}

void tst_QTemporaryFile::autoRemove()
{
    // Test auto remove
    QString fileName;
    {
        QTemporaryFile file("tempXXXXXX");
        file.setAutoRemove(true);
        QVERIFY(file.open());
        fileName = file.fileName();
        file.close();
    }
    QVERIFY(!fileName.isEmpty());
    QVERIFY(!QFile::exists(fileName));

    // same, but gets the file name after closing
    {
        QTemporaryFile file("tempXXXXXX");
        file.setAutoRemove(true);
        QVERIFY(file.open());
        file.close();
        fileName = file.fileName();
    }
    QVERIFY(!fileName.isEmpty());
    QVERIFY(!QFile::exists(fileName));

    // Test if disabling auto remove works.
    {
        QTemporaryFile file("tempXXXXXX");
        file.setAutoRemove(false);
        QVERIFY(file.open());
        fileName = file.fileName();
        file.close();
    }
    QVERIFY(!fileName.isEmpty());
    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile::remove(fileName));

    // same, but gets the file name after closing
    {
        QTemporaryFile file("tempXXXXXX");
        file.setAutoRemove(false);
        QVERIFY(file.open());
        file.close();
        fileName = file.fileName();
    }
    QVERIFY(!fileName.isEmpty());
    QVERIFY(QFile::exists(fileName));
    QVERIFY(QFile::remove(fileName));

    // Do not explicitly call setAutoRemove (tests if it really is the default as documented)
    {
        QTemporaryFile file("tempXXXXXX");
        QVERIFY(file.open());
        fileName = file.fileName();
        // QTBUG-39976, file mappings should be cleared as well.
        QVERIFY(file.write("test"));
        QVERIFY(file.flush());
        uchar *mapped = file.map(0, file.size());
        QVERIFY(mapped);
        file.close();
    }
    QVERIFY(!QFile::exists(fileName));
}

struct ChdirOnReturn
{
    ChdirOnReturn(const QString& d) : dir(d) {}
    ~ChdirOnReturn() {
        QDir::setCurrent(dir);
    }
    QString dir;
};

void tst_QTemporaryFile::nonWritableCurrentDir()
{
#ifdef Q_OS_UNIX
    if (::geteuid() == 0)
        QSKIP("not valid running this test as root");

    ChdirOnReturn cor(QDir::currentPath());

#ifdef Q_OS_ANDROID
    QDir::setCurrent("/data");
#else
    QDir::setCurrent("/home");
#endif

    // QTemporaryFile("tempXXXXXX") is probably a bad idea in any app
    // where the current dir could anything...
    QTemporaryFile file("tempXXXXXX");
    file.setAutoRemove(true);
    QVERIFY(!file.open());
    QVERIFY(file.fileName().isEmpty());
#endif
}

void tst_QTemporaryFile::io()
{
    QByteArray data("OLE\nOLE\nOLE");
    QTemporaryFile file;
    QDateTime before = QDateTime::currentDateTimeUtc().addMSecs(-250);

    // discard msec component (round down) - not all FSs and OSs support them
    before.setSecsSinceEpoch(before.toSecsSinceEpoch());

    QVERIFY(file.open());
    QVERIFY(file.symLinkTarget().isEmpty()); // it's not a link!
    QFile::Permissions perm = file.permissions();
    QVERIFY(perm & QFile::ReadOwner);
    QVERIFY(file.setPermissions(perm));

    QCOMPARE(int(file.size()), 0);
    QVERIFY(file.resize(data.size()));
    QCOMPARE(int(file.size()), data.size());
    QCOMPARE((int)file.write(data), data.size());
    QCOMPARE(int(file.size()), data.size());

    QDateTime mtime = file.fileTime(QFile::FileModificationTime).toUTC();
    QDateTime btime = file.fileTime(QFile::FileBirthTime).toUTC();
    QDateTime ctime = file.fileTime(QFile::FileMetadataChangeTime).toUTC();
    QDateTime atime = file.fileTime(QFile::FileAccessTime).toUTC();

    QDateTime after = QDateTime::currentDateTimeUtc().toUTC().addMSecs(250);
    // round msecs up
    after.setSecsSinceEpoch(after.toSecsSinceEpoch() + 1);

    // mtime must be valid, the rest could fail
    QVERIFY(mtime <= after && mtime >= before);
    QVERIFY(!btime.isValid() || (btime <= after && btime >= before));
    QVERIFY(!ctime.isValid() || (ctime <= after && ctime >= before));
    QVERIFY(!btime.isValid() || (btime <= after && btime >= before));

    QVERIFY(file.setFileTime(before.addSecs(-10), QFile::FileModificationTime));
    mtime = file.fileTime(QFile::FileModificationTime).toUTC();
    QCOMPARE(mtime, before.addSecs(-10));

    file.reset();
    QFile compare(file.fileName());
    QVERIFY(compare.open(QIODevice::ReadOnly));
    QCOMPARE(compare.readAll() , data);
    QCOMPARE(compare.fileTime(QFile::FileModificationTime), mtime);
}

void tst_QTemporaryFile::openCloseOpenClose()
{
    QString fileName;
    {
        // Create a temp file
        QTemporaryFile file("tempXXXXXX");
        file.setAutoRemove(true);
        QVERIFY(file.open());
        file.write("OLE");
        fileName = file.fileName();
        QVERIFY(QFile::exists(fileName));
        file.close();

        // Check that it still exists after being closed
        QVERIFY(QFile::exists(fileName));
        QVERIFY(!file.isOpen());
        QVERIFY(file.open());
        QCOMPARE(file.readAll(), QByteArray("OLE"));
        // Check that it's still the same file after being opened again.
        QCOMPARE(file.fileName(), fileName);
    }
    QVERIFY(!QFile::exists(fileName));
}

void tst_QTemporaryFile::removeAndReOpen()
{
    QString fileName;
    {
        QTemporaryFile file;
        QVERIFY(file.open());
        fileName = file.fileName();     // materializes any unnamed file
        QVERIFY(QFile::exists(fileName));

        QVERIFY(file.remove());
        QVERIFY(file.fileName().isEmpty());
        QVERIFY(!QFile::exists(fileName));
        QVERIFY(!file.remove());

        QVERIFY(file.open());
        QCOMPARE(QFileInfo(file.fileName()).path(), QFileInfo(fileName).path());
        fileName = file.fileName();
        QVERIFY(QFile::exists(fileName));
    }
    QVERIFY(!QFile::exists(fileName));
}

void tst_QTemporaryFile::removeUnnamed()
{
    QTemporaryFile file;
    QVERIFY(file.open());

    // we did not call fileName(), so the file name may not have a name
    QVERIFY(file.remove());
    QVERIFY(file.fileName().isEmpty());

    // if it was unnamed, this will succeed again, so we can't check the result
    file.remove();
}

void tst_QTemporaryFile::size()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(!file.isSequential());
    QByteArray str("foobar");
    file.write(str);

    // On CE it takes more time for the filesystem to update
    // the information. Usually you have to close it or seek
    // to get latest information. flush() does not help either.
    QCOMPARE(file.size(), qint64(6));
    file.seek(0);
    QCOMPARE(file.size(), qint64(6));

    QVERIFY(QFile::exists(file.fileName()));
    QVERIFY(file.exists());
}

void tst_QTemporaryFile::resize()
{
    QTemporaryFile file;
    file.setAutoRemove(true);
    QVERIFY(file.open());
    QVERIFY(file.resize(100));

    QCOMPARE(QFileInfo(file.fileName()).size(), qint64(100));

    file.close();
}

void tst_QTemporaryFile::openOnRootDrives()
{
#if defined(Q_OS_WIN)
    unsigned int lastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
    // If it's possible to create a file in the root directory, it
    // must be possible to create a temp file there too.
    const auto drives = QDir::drives();
    for (const QFileInfo &driveInfo : drives) {
        QFile testFile(driveInfo.filePath() + "XXXXXX.txt");
        if (testFile.open(QIODevice::ReadWrite)) {
            testFile.remove();
            QTemporaryFile file(driveInfo.filePath() + "XXXXXX.txt");
            file.setAutoRemove(true);
            QVERIFY(file.open());

            QFileInfo fi(file.fileName());
            QCOMPARE(fi.absoluteDir(), driveInfo.filePath());
        }
    }
#if defined(Q_OS_WIN)
    SetErrorMode(lastErrorMode);
#endif
}

void tst_QTemporaryFile::stressTest()
{
    constexpr int iterations = 1000;

    QDuplicateTracker<QString, iterations> names;

    const auto remover = qScopeGuard([&] {
            for (const QString &s : std::as_const(names))
                QFile::remove(s);
        });

    for (int i = 0; i < iterations; ++i) {
        QTemporaryFile file;
        file.setAutoRemove(false);
        QVERIFY2(file.open(), qPrintable(file.errorString()));
        QVERIFY(!names.hasSeen(file.fileName()));
    }
}

void tst_QTemporaryFile::rename_data()
{
    QTest::addColumn<bool>("targetExists");
    QTest::addColumn<bool>("expectedRenameResult");

    QTest::addRow("success") << false << true;
    QTest::addRow("failure") << true  << false;
}

static QByteArray read_all(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly))
        return file.readAll();
    else
        return QByteArray();
}

void tst_QTemporaryFile::rename()
{
    QFETCH(const bool, targetExists);
    QFETCH(const bool, expectedRenameResult);

    QDir dir;
    QVERIFY(!dir.exists("temporary-file.txt"));

    QString tempname;
    {
        QTemporaryFile file(dir.filePath("temporary-file.XXXXXX"));

        QVERIFY(file.open());
        QCOMPARE(file.write("I am the tmpfile"), 16);
        tempname = file.fileName();
        QVERIFY(dir.exists(tempname));

        std::optional<QFile> renameBlocker;
        const auto cleanup = qScopeGuard([&] {
                if (renameBlocker)
                    renameBlocker->remove();
            });

        if (targetExists) {
            renameBlocker.emplace(dir.filePath("temporary-file.txt"_L1));
            QVERIFY2(renameBlocker->open(QFile::ReadWrite),
                     qPrintable(renameBlocker->errorString()));
            QCOMPARE(renameBlocker->write("I am the blocker"), 16);
            renameBlocker->close();
            QVERIFY(QFile::exists(dir.filePath("temporary-file.txt"_L1)));
        }

#ifdef Q_OS_ANDROID
        QEXPECT_FAIL("failure", "QTBUG-138610", Abort);
#endif
        QCOMPARE(file.rename("temporary-file.txt"_L1), expectedRenameResult);
        QCOMPARE(dir.exists(tempname), !expectedRenameResult);
        QVERIFY(dir.exists("temporary-file.txt"));
        QCOMPARE(read_all(dir.filePath("temporary-file.txt"_L1)),
                 expectedRenameResult ? "I am the tmpfile" : "I am the blocker");
        QCOMPARE(file.fileName(),
                 expectedRenameResult ? QString("temporary-file.txt") : tempname);
    }

    QVERIFY(!dir.exists(tempname));
    QVERIFY(!dir.exists("temporary-file.txt"));
}

void tst_QTemporaryFile::renameFdLeak()
{
#if defined(Q_OS_VXWORKS)
    QSKIP("QTBUG-130066");
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID)
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray sourceFile = QFile::encodeName(file.fileName());
    QVERIFY(!sourceFile.isEmpty());
    // Test this on Unix only

    // Open a bunch of files to force the fd count to go up
    static const int count = 10;
    int bunch_of_files[count];
    for (int i = 0; i < count; ++i) {
        bunch_of_files[i] = ::open(sourceFile.constData(), O_RDONLY);
        QVERIFY(bunch_of_files[i] != -1);
    }

    int fd;
    {
        QTemporaryFile file;
        file.setAutoRemove(false);
        QVERIFY(file.open());

        // close the bunch of files
        for (int i = 0; i < count; ++i)
            ::close(bunch_of_files[i]);

        // save the file descriptor for later
        fd = file.handle();

        // rename the file to something
        QString newPath = QDir::tempPath() + "/tst_qtemporaryfile-renameFdLeak-" + QString::number(getpid());
        file.rename(newPath);
        QFile::remove(newPath);
    }

    // check if QTemporaryFile closed the file
    QVERIFY(::close(fd) == -1 && errno == EBADF);
#endif
}

void tst_QTemporaryFile::moveToTrash()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_WEBOS) || defined(Q_OS_VXWORKS)
    QSKIP("This platform doesn't implement a trash bin");
#endif
#ifdef Q_OS_WIN
    // QTemporaryFile won't really close the file with close(), so this is
    // expected to fail with a sharing violation error.
    constexpr bool expectSuccess = false;
#else
    constexpr bool expectSuccess = true;
#endif
    const QByteArrayView contents = "Hello, World\n";

    QTemporaryFile f(QDir::homePath() + "/tst_qtemporaryfile.moveToTrash.XXXXXX");
    QString origFileName;
    auto cleanup = qScopeGuard([&] {
        if (!origFileName.isEmpty())
            QFile::remove(origFileName);
        if (QString fn = f.fileName(); !fn.isEmpty() && fn != origFileName)
            QFile::remove(fn);
    });

    if (!f.open())
        QSKIP("Failed to create temporary file");
    f.write(contents.data(), contents.size());

    // we need an actual file name:
    // 1) so we can delete it in the clean-up guard in case we fail to trash
    // 2) so that the file exists on Linux in the first place (no sense in
    //    trashing an unnamed file)
    origFileName = f.fileName();

    if (expectSuccess) {
        QVERIFY2(f.moveToTrash(), qPrintable(f.errorString()));
        QCOMPARE_NE(f.fileName(), origFileName);        // must have changed!
        QCOMPARE_NE(f.fileName(), QString());
        QVERIFY(!QFile::exists(origFileName));
        QVERIFY(QFile::exists(f.fileName()));
        QCOMPARE(QFileInfo(f.fileName()).size(), contents.size());
    } else {
        QVERIFY(!f.moveToTrash());
        QCOMPARE(f.fileName(), origFileName);           // mustn't have changed!
        QCOMPARE_NE(f.error(), QFile::NoError);
        QCOMPARE_NE(f.errorString(), "Unknown error");
        QVERIFY(QFile::exists(origFileName));
    }
}

void tst_QTemporaryFile::reOpenThroughQFile()
{
    QByteArray data("abcdefghij");

    QTemporaryFile file;
    QVERIFY(((QFile &)file).open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), (qint64)data.size());

    file.close();
    QVERIFY(file.open());
    QCOMPARE(file.readAll(), data);
}

void tst_QTemporaryFile::keepOpenMode()
{
    QByteArray data("abcdefghij");

    {
        QTemporaryFile file;
        QVERIFY(((QFile &)file).open(QIODevice::WriteOnly));
        QVERIFY(QIODevice::WriteOnly == file.openMode());

        QCOMPARE(file.write(data), (qint64)data.size());
        file.close();

        QVERIFY(((QFile &)file).open(QIODevice::ReadOnly));
        QVERIFY(QIODevice::ReadOnly == file.openMode());
        QCOMPARE(file.readAll(), data);
    }

    {
        QTemporaryFile file;
        QVERIFY(file.open());
        QCOMPARE(file.openMode(), QIODevice::ReadWrite);
        QCOMPARE(file.write(data), (qint64)data.size());
        QVERIFY(file.rename("temporary-file.txt"));

        QVERIFY(((QFile &)file).open(QIODevice::ReadOnly));
        QCOMPARE(file.openMode(), QIODevice::ReadOnly);
        QCOMPARE(file.readAll(), data);

        QVERIFY(((QFile &)file).open(QIODevice::WriteOnly));
        QCOMPARE(file.openMode(), QIODevice::WriteOnly);
    }
}

void tst_QTemporaryFile::resetTemplateAfterError()
{
#if defined(Q_OS_VXWORKS)
    QSKIP("QTBUG-130066");
#endif
    // calling setFileTemplate on a failed open

    QString tempPath = QDir::tempPath();

    QString const fileTemplate("destination/qt_temp_file_test.XXXXXX");
    QString const fileTemplate2(tempPath + "/qt_temp_file_test.XXXXXX");

    QVERIFY2( QDir(tempPath).exists() || QDir().mkpath(tempPath), "Test precondition" );
    QVERIFY2( !QFile::exists("destination"), "Test precondition" );
    QVERIFY2( !QFile::exists(fileTemplate2) || QFile::remove(fileTemplate2), "Test precondition" );

    QFile file(fileTemplate2);
    QByteArray fileContent("This file is intentionally NOT left empty.");

    QVERIFY( file.open(QIODevice::ReadWrite | QIODevice::Truncate) );
    QCOMPARE( file.write(fileContent), (qint64)fileContent.size() );
    QVERIFY( file.flush() );

    QString fileName;
    {
        QTemporaryFile temp;

        QVERIFY( temp.fileName().isEmpty() );
        QVERIFY( !temp.fileTemplate().isEmpty() );

        temp.setFileTemplate( fileTemplate );

        QVERIFY( temp.fileName().isEmpty() );
        QCOMPARE( temp.fileTemplate(), fileTemplate );

        QVERIFY( !temp.open() );

        QVERIFY( temp.fileName().isEmpty() );
        QCOMPARE( temp.fileTemplate(), fileTemplate );

        temp.setFileTemplate( fileTemplate2 );
        QVERIFY( temp.open() );

        fileName = temp.fileName();
        QVERIFY( QFile::exists(fileName) );
        QVERIFY( !fileName.isEmpty() );
        QVERIFY2( fileName != fileTemplate2,
            ("Generated name shouldn't be same as template: " + fileTemplate2).toLocal8Bit().constData() );
    }

    QVERIFY( !QFile::exists(fileName) );

    file.seek(0);
    QCOMPARE( QString(file.readAll()), QString(fileContent) );
    QVERIFY( file.remove() );
}

void tst_QTemporaryFile::setTemplateAfterOpen()
{
#if defined(Q_OS_VXWORKS)
    QSKIP("QTBUG-130066");
#endif
    QTemporaryFile temp;

    QVERIFY( temp.fileName().isEmpty() );
    QVERIFY( !temp.fileTemplate().isEmpty() );

    QVERIFY( temp.open() );

    QString const fileName = temp.fileName();
    QString const newTemplate("funny-path/funny-name-XXXXXX.tmp");

    QVERIFY( !fileName.isEmpty() );
    QVERIFY( QFile::exists(fileName) );
    QVERIFY( !temp.fileTemplate().isEmpty() );
    QVERIFY( temp.fileTemplate() != newTemplate );

    temp.close(); // QTemporaryFile::setFileTemplate will assert on isOpen() up to 4.5.2
    temp.setFileTemplate(newTemplate);
    QCOMPARE( temp.fileTemplate(), newTemplate );

    QVERIFY( temp.open() );
    QCOMPARE( temp.fileName(), fileName );
    QCOMPARE( temp.fileTemplate(), newTemplate );
}

void tst_QTemporaryFile::autoRemoveAfterFailedRename()
{
#if defined(Q_OS_VXWORKS)
    QSKIP("QTBUG-130066");
#endif

    QString tempName;
    auto cleaner = qScopeGuard([&] {
            if (!tempName.isEmpty())
                QFile::remove(tempName);
        });

    {
        QTemporaryFile file;
        QVERIFY( file.open() );
        tempName = file.fileName();

        QVERIFY(QFile::exists(tempName));
        QVERIFY( !QFileInfo("i-do-not-exist").isDir() );
        QVERIFY( !file.rename("i-do-not-exist/file.txt") );
        QVERIFY(QFile::exists(tempName));
    }

    QVERIFY(!QFile::exists(tempName));
    cleaner.dismiss(); // would fail: file is known to no longer exist
}

void tst_QTemporaryFile::createNativeFile_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<qint64>("currentPos");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("content");

#ifdef Q_OS_ANDROID
    const QString nativeFilePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                   + QStringLiteral("/resources/test.txt");
#else
    const QString nativeFilePath = QFINDTESTDATA("resources/test.txt");
#endif

    // File might not exist locally in case of sandboxing or remote testing
    if (!nativeFilePath.startsWith(QLatin1String(":/"))) {
        QTest::newRow("nativeFile") << nativeFilePath << (qint64)-1 << false << QByteArray();
        QTest::newRow("nativeFileWithPos") << nativeFilePath << (qint64)5 << false << QByteArray();
    }
    QTest::newRow("resourceFile") << ":/resources/test.txt" << (qint64)-1 << true << QByteArray("This is a test");
    QTest::newRow("resourceFileWithPos") << ":/resources/test.txt" << (qint64)5 << true << QByteArray("This is a test");
}

void tst_QTemporaryFile::createNativeFile()
{
    QFETCH(QString, filePath);
    QFETCH(qint64, currentPos);
    QFETCH(bool, valid);
    QFETCH(QByteArray, content);

    QFile f(filePath);
    if (currentPos != -1) {
        QVERIFY(f.open(QIODevice::ReadOnly));
        f.seek(currentPos);
    }
    QTemporaryFile *tempFile = QTemporaryFile::createNativeFile(f);
    QCOMPARE(valid, (bool)tempFile);
    if (currentPos != -1)
        QCOMPARE(currentPos, f.pos());
    if (valid) {
        QCOMPARE(content, tempFile->readAll());
        delete tempFile;
    }
}

void tst_QTemporaryFile::QTBUG_4796_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<bool>("openResult");

    QString unicode = QString::fromUtf8("\xc3\xa5\xc3\xa6\xc3\xb8");

    QTest::newRow("<empty>") << QString() << QString() << true;
    QTest::newRow(".") << QString(".") << QString() << true;
    QTest::newRow("..") << QString("..") << QString() << true;
    QTest::newRow("blaXXXXXX") << QString("bla") << QString() << true;
    QTest::newRow("XXXXXXbla") << QString() << QString("bla") << true;
    QTest::newRow("does-not-exist/qt_temp.XXXXXX") << QString("does-not-exist/qt_temp") << QString() << false;

    if (QTestPrivate::canHandleUnicodeFileNames()) {
        QTest::newRow("XXXXXX<unicode>") << QString() << unicode << true;
        QTest::newRow("<unicode>XXXXXX") << unicode << QString() << true;
        QTest::newRow("<unicode>XXXXXX<unicode>") << unicode << unicode << true;
    }
}

void tst_QTemporaryFile::QTBUG_4796()
{
    QVERIFY(QDir("test-XXXXXX").exists());

    struct CleanOnReturn
    {
        ~CleanOnReturn()
        {
            for (const QString &tempName : std::as_const(tempNames))
                QFile::remove(tempName);
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

        QTemporaryFile file1(fileTemplate1);
        QTemporaryFile file2(fileTemplate2);
        QTemporaryFile file3(fileTemplate3);
        QTemporaryFile file4(fileTemplate4);
        QTemporaryFile file5("test-XXXXXX/" + fileTemplate1);
        QTemporaryFile file6("test-XXXXXX/" + fileTemplate3);

        QCOMPARE(file1.open(), openResult);
        QCOMPARE(file2.open(), openResult);
        QCOMPARE(file3.open(), openResult);
        QCOMPARE(file4.open(), openResult);
        QCOMPARE(file5.open(), openResult);
        QCOMPARE(file6.open(), openResult);

        // force the files to exist, if they are supposed to
        QCOMPARE(!file1.fileName().isEmpty(), openResult);
        QCOMPARE(!file2.fileName().isEmpty(), openResult);
        QCOMPARE(!file3.fileName().isEmpty(), openResult);
        QCOMPARE(!file4.fileName().isEmpty(), openResult);
        QCOMPARE(!file5.fileName().isEmpty(), openResult);
        QCOMPARE(!file6.fileName().isEmpty(), openResult);

        QCOMPARE(file1.exists(), openResult);
        QCOMPARE(file2.exists(), openResult);
        QCOMPARE(file3.exists(), openResult);
        QCOMPARE(file4.exists(), openResult);
        QCOMPARE(file5.exists(), openResult);
        QCOMPARE(file6.exists(), openResult);

        // make sure the file exists under the *correct* name
        if (openResult) {
            cleaner.tempNames << file1.fileName()
                << file2.fileName()
                << file3.fileName()
                << file4.fileName()
                << file5.fileName()
                << file6.fileName();

            QDir currentDir;
            QString fileName1 = currentDir.relativeFilePath(file1.fileName());
            QString fileName2 = currentDir.relativeFilePath(file2.fileName());
            QString fileName3 = currentDir.relativeFilePath(file3.fileName());
            QString fileName4 = currentDir.relativeFilePath(file4.fileName());
            QString fileName5 = currentDir.relativeFilePath(file5.fileName());
            QString fileName6 = currentDir.relativeFilePath(file6.fileName());

            QVERIFY(fileName1.startsWith(fileTemplate1 + QLatin1Char('.')));
            QVERIFY(fileName2.startsWith(fileTemplate2 + QLatin1Char('.')));
            QVERIFY(fileName5.startsWith("test-XXXXXX/" + fileTemplate1 + QLatin1Char('.')));
            QVERIFY(fileName6.startsWith("test-XXXXXX/" + prefix));

            if (!prefix.isEmpty()) {
                QVERIFY(fileName3.startsWith(prefix));
                QVERIFY(fileName4.startsWith(prefix));
            }

            if (!suffix.isEmpty()) {
                QVERIFY(fileName3.endsWith(suffix));
                QVERIFY(fileName4.endsWith(suffix));
                QVERIFY(fileName6.endsWith(suffix));
            }
        }
    }

    for (const QString &tempName : std::as_const(cleaner.tempNames))
        QVERIFY( !QFile::exists(tempName) );

    cleaner.reset();
}

void tst_QTemporaryFile::guaranteeUnique()
{
#if defined(Q_OS_VXWORKS)
    QSKIP("QTBUG-130066");
#endif
    QDir dir(QDir::tempPath());
    QString takenFileName;

    // First pass. See which filename QTemporaryFile will try first.
    {
        QTemporaryFile tmpFile("testFile1.XXXXXX");
        QVERIFY(tmpFile.open());
        takenFileName = tmpFile.fileName();
        QVERIFY(QFile::exists(takenFileName));
    }

    QVERIFY(!QFile::exists(takenFileName));

    // Create a directory with same name.
    QVERIFY(dir.mkdir(takenFileName));

    // Second pass, now we have blocked its first attempt with a directory.
    {
        QTemporaryFile tmpFile("testFile1.XXXXXX");
        QVERIFY(tmpFile.open());
        QString uniqueFileName = tmpFile.fileName();
        QVERIFY(QFileInfo(uniqueFileName).isFile());
        QVERIFY(uniqueFileName != takenFileName);
    }

    QVERIFY(dir.rmdir(takenFileName));
}

void tst_QTemporaryFile::stdfilesystem()
{
#if !QT_CONFIG(cxx17_filesystem)
    QSKIP("std::filesystem not available");
#else
    // ctor
    {
        std::filesystem::path testFile("testFile1.XXXXXX");
        QTemporaryFile file(testFile);
        QCOMPARE(file.fileTemplate(), QtPrivate::fromFilesystemPath(testFile));
    }
    // rename
    {
        QTemporaryFile file("testFile1.XXXXXX");
        QVERIFY(file.open());
        QByteArray payload = "abc123 I am a string";
        file.write(payload);
        QVERIFY(file.rename(std::filesystem::path("./test")));
        file.close();

        QFile f(u"./test"_s);
        QVERIFY(f.exists());
        QVERIFY(f.open(QFile::ReadOnly));
        QCOMPARE(f.readAll(), payload);
    }
    // createNativeFile
    {
        std::filesystem::path resource(":/resources/test.txt");
        std::unique_ptr<QTemporaryFile> tmp(QTemporaryFile::createNativeFile(resource));
        QVERIFY(tmp);
        QFile file(resource);
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(tmp->readAll(), file.readAll());
    }
    // setFileTemplate
    {
        QTemporaryFile file;
        std::filesystem::path testFile("testFile1.XXXXXX");
        file.setFileTemplate(testFile);
        QCOMPARE(file.fileTemplate(), QtPrivate::fromFilesystemPath(testFile));
    }
#endif
}

QTEST_MAIN(tst_QTemporaryFile)
#include "tst_qtemporaryfile.moc"
