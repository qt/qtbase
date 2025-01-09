// Copyright (C) 2012 David Faure <faure@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSaveFile>
#include <qcoreapplication.h>
#include <qstring.h>
#include <qsystemdetection.h>
#include <qtemporaryfile.h>
#include <qfile.h>
#include <qdir.h>
#include <qset.h>

#if defined(Q_OS_UNIX)
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h> // for geteuid
#endif

#if defined(Q_OS_WIN)
# include <qt_windows.h>
#endif

#ifdef Q_OS_INTEGRITY
#include "qplatformdefs.h"
#endif

// Restore permissions so that the QTemporaryDir cleanup can happen
class PermissionRestorer
{
    Q_DISABLE_COPY(PermissionRestorer)
public:
    explicit PermissionRestorer(const QString& path) : m_path(path) {}
    ~PermissionRestorer()  { restore(); }

    inline void restore()
    {
        QFile file(m_path);
#ifdef Q_OS_UNIX
        file.setPermissions(QFile::Permissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
#else
        file.setPermissions(QFile::WriteOwner);
        file.remove();
#endif
    }

private:
    const QString m_path;
};

class tst_QSaveFile : public QObject
{
    Q_OBJECT
public slots:

private slots:
    void transactionalWrite();
    void retryTransactionalWrite();
    void textStreamManualFlush();
    void textStreamAutoFlush();
    void saveTwice();
    void transactionalWriteNoPermissionsOnDir_data();
    void transactionalWriteNoPermissionsOnDir();
    void transactionalWriteNoPermissionsOnFile();
    void transactionalWriteCanceled();
    void transactionalWritePermissionsErrorRenaming();
    void transactionalWriteTypeErrorRenaming();
    void symlink();
    void directory();

#ifdef Q_OS_UNIX
    void writeFailToDevFull_data();
    void writeFailToDevFull();
#endif
#if defined(RLIMIT_FSIZE) && (defined(Q_OS_LINUX) || defined(Q_OS_MACOS))
    void writeFailResourceLimit_data();
    void writeFailResourceLimit();
#endif

#ifdef Q_OS_WIN
    void alternateDataStream_data();
    void alternateDataStream();
#endif
};

static inline QByteArray msgCannotOpen(const QFileDevice &f)
{
    QString result = QStringLiteral("Cannot open ") + QDir::toNativeSeparators(f.fileName())
        + QStringLiteral(": ") + f.errorString();
    return result.toLocal8Bit();
}

void tst_QSaveFile::transactionalWrite()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QFile::remove(targetFile);
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QVERIFY(file.isOpen());
    QCOMPARE(file.fileName(), targetFile);
    QVERIFY(!QFile::exists(targetFile));

    const char *data = "Hello";
    QCOMPARE(file.write(data), qint64(strlen(data)));
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(!QFile::exists(targetFile));
    QVERIFY(file.fileTime(QFile::FileModificationTime).isValid());

    QVERIFY(file.commit());
    QVERIFY(QFile::exists(targetFile));
    QCOMPARE(file.fileName(), targetFile);
#if defined(Q_OS_WIN)
    // Without this delay, file.fileTime() and file.size() tests fail to
    // pass on Windows in the CI. It passes locally in a VM, so it looks like
    // it depends on how often different filesystems on different OSes, update
    // their metadata.
    // Interestingly, this delay is enough to fix similar tests in the rest
    // of tst_QSaveFile's functions.
    QTRY_VERIFY(file.fileTime(QFile::FileModificationTime).isValid());
#else
    QVERIFY(file.fileTime(QFile::FileModificationTime).isValid());
#endif

    QCOMPARE(file.size(), qint64(strlen(data)));

    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1(data));
    QCOMPARE(file.fileTime(QFile::FileModificationTime),
             reader.fileTime(QFile::FileModificationTime));

    // check that permissions are the same as for QFile
    const QString otherFile = dir.path() + QString::fromLatin1("/otherfile");
    QFile::remove(otherFile);
    QFile other(otherFile);
    QVERIFY(other.open(QIODevice::WriteOnly));
    other.close();
    QCOMPARE(QFile::permissions(targetFile), QFile::permissions(otherFile));
}

// QTBUG-77007: Simulate the case of an application with a loop prompting
// to retry saving on failure. Create a read-only file first (Unix only)
void tst_QSaveFile::retryTransactionalWrite()
{
#ifndef Q_OS_UNIX
    QSKIP("This test is Unix only");
#else
    // root can open the read-only file for writing...
    if (geteuid() == 0)
        QSKIP("This test does not work as the root user");
#endif //Q_OS_UNIX
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    const char *data = "Hello";
    QString targetFile = dir.path() + QLatin1String("/outfile");
    const QString readOnlyName = targetFile + QLatin1String(".ro");
    {
        QFile readOnlyFile(readOnlyName);
        QVERIFY2(readOnlyFile.open(QIODevice::WriteOnly), msgCannotOpen(readOnlyFile).constData());
        readOnlyFile.write(data);
        readOnlyFile.close();
        auto permissions = readOnlyFile.permissions();
        permissions &= ~(QFileDevice::WriteOwner | QFileDevice::WriteGroup | QFileDevice::WriteUser);
        QVERIFY(readOnlyFile.setPermissions(permissions));
    }

    QSaveFile file(readOnlyName);
    QVERIFY(!file.open(QIODevice::WriteOnly));

    file.setFileName(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QVERIFY(file.isOpen());
    QCOMPARE(file.write(data), qint64(strlen(data)));
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(file.commit());
    QCOMPARE(file.size(), qint64(strlen(data)));
}

void tst_QSaveFile::saveTwice()
{
    const char *hello = "Hello";
    // Check that we can reuse a QSaveFile object
    // (and test the case of an existing target file)
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QCOMPARE(file.write(hello), qint64(strlen(hello)));
    QVERIFY2(file.commit(), qPrintable(file.errorString()));
    QCOMPARE(file.size(), qint64(strlen(hello)));

    const char *world = "World";
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QCOMPARE(file.write(world), qint64(strlen(world)));
    QVERIFY2(file.commit(), qPrintable(file.errorString()));
    QCOMPARE(file.size(), qint64(strlen(world)));

    QFile reader(targetFile);
    QVERIFY2(reader.open(QIODevice::ReadOnly), msgCannotOpen(reader).constData());
    QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1(world));
}

void tst_QSaveFile::textStreamManualFlush()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());

    const char *data = "Manual flush";
    QTextStream ts(&file);
    ts << data;
    ts.flush();
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(!QFile::exists(targetFile));

    QVERIFY(file.commit());
    QCOMPARE(file.size(), qint64(strlen(data)));
    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll().constData()), QString::fromLatin1(data));
    QFile::remove(targetFile);
}

void tst_QSaveFile::textStreamAutoFlush()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());

    QTextStream ts(&file);
    ts << "Auto-flush.";
    // no flush
    QVERIFY(file.commit()); // QIODevice::close will emit aboutToClose, which will flush the stream
    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll().constData()), QString::fromLatin1("Auto-flush."));
    QFile::remove(targetFile);
}

void tst_QSaveFile::transactionalWriteNoPermissionsOnDir_data()
{
    QTest::addColumn<bool>("directWriteFallback");

    QTest::newRow("default") << false;
    QTest::newRow("directWriteFallback") << true;
}

void tst_QSaveFile::transactionalWriteNoPermissionsOnDir()
{
#ifdef Q_OS_UNIX
#if !defined(Q_OS_VXWORKS)
    if (::geteuid() == 0)
        QSKIP("Test is not applicable with root privileges");
#endif
    QFETCH(bool, directWriteFallback);
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    QVERIFY(QFile(dir.path()).setPermissions(QFile::ReadOwner | QFile::ExeOwner));
    PermissionRestorer permissionRestorer(dir.path());

    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile firstTry(targetFile);
    QVERIFY(!firstTry.open(QIODevice::WriteOnly));
    QCOMPARE((int)firstTry.error(), (int)QFile::OpenError);
    QVERIFY(!firstTry.commit());

    // Now make an existing writable file
    permissionRestorer.restore();
    QFile f(targetFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    QCOMPARE(f.write("Hello"), Q_INT64_C(5));
    f.close();

    // Make the directory non-writable again
    QVERIFY(QFile(dir.path()).setPermissions(QFile::ReadOwner | QFile::ExeOwner));

    // And write to it again using QSaveFile; only works if directWriteFallback is enabled
    QSaveFile file(targetFile);
    file.setDirectWriteFallback(directWriteFallback);
    QCOMPARE(file.directWriteFallback(), directWriteFallback);
    if (directWriteFallback) {
        QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
        QCOMPARE((int)file.error(), (int)QFile::NoError);
        QCOMPARE(file.write("World"), Q_INT64_C(5));
        QVERIFY(file.commit());

        QFile reader(targetFile);
        QVERIFY(reader.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1("World"));
        reader.close();

        QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
        QCOMPARE((int)file.error(), (int)QFile::NoError);
        QCOMPARE(file.write("W"), Q_INT64_C(1));
        file.cancelWriting();
        QVERIFY(!file.commit());

        QVERIFY(reader.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1("W"));
    } else {
        QVERIFY(!file.open(QIODevice::WriteOnly));
        QCOMPARE((int)file.error(), (int)QFile::OpenError);
    }
#endif
}

void tst_QSaveFile::transactionalWriteNoPermissionsOnFile()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
    if (::geteuid() == 0)
        QSKIP("Test is not applicable with root privileges");
#endif
    // Setup an existing but readonly file
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QFile file(targetFile);
    PermissionRestorer permissionRestorer(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QCOMPARE(file.write("Hello"), Q_INT64_C(5));
    file.close();
    file.setPermissions(QFile::ReadOwner);
    QVERIFY(!file.open(QIODevice::WriteOnly));

    // Try saving into it
    {
        QSaveFile saveFile(targetFile);
        QVERIFY(!saveFile.open(QIODevice::WriteOnly)); // just like QFile
    }
    QVERIFY(file.exists());
}

void tst_QSaveFile::transactionalWriteCanceled()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QFile::remove(targetFile);
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());

    QTextStream ts(&file);
    ts << "This writing operation will soon be canceled.\n";
    ts.flush();
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(!QFile::exists(targetFile));

    // We change our mind, let's abort writing
    file.cancelWriting();

    QVERIFY(!file.commit());

    QVERIFY(!QFile::exists(targetFile)); // temp file was discarded
    QCOMPARE(file.fileName(), targetFile);
}

void tst_QSaveFile::transactionalWritePermissionsErrorRenaming()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
    if (::geteuid() == 0)
        QSKIP("Test is not applicable with root privileges");
#endif
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QCOMPARE(file.write("Hello"), qint64(5));
    QVERIFY(!QFile::exists(targetFile));
#ifdef Q_OS_UNIX
    // Make rename() fail for lack of permissions in the directory
    QFile dirAsFile(dir.path()); // yay, I have to use QFile to change a dir's permissions...
    QVERIFY(dirAsFile.setPermissions(QFile::Permissions{})); // no permissions
    PermissionRestorer permissionRestorer(dir.path());
#else
    // Windows: Make rename() fail for lack of permissions on an existing target file
    QFile existingTargetFile(targetFile);
    QVERIFY2(existingTargetFile.open(QIODevice::WriteOnly), msgCannotOpen(existingTargetFile).constData());
    QCOMPARE(file.write("Target"), qint64(6));
    existingTargetFile.close();
    QVERIFY(existingTargetFile.setPermissions(QFile::ReadOwner));
    PermissionRestorer permissionRestorer(targetFile);
#endif

    // The saving should fail.
    QVERIFY(!file.commit());
#ifdef Q_OS_UNIX
    QVERIFY(!QFile::exists(targetFile)); // renaming failed
#endif
    QCOMPARE(file.error(), QFile::RenameError);
}

void tst_QSaveFile::transactionalWriteTypeErrorRenaming()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QCOMPARE(file.write("Hello"), qint64(5));

    QFileInfo target(targetFile);
    QVERIFY(!target.exists(targetFile));

    // create a directory with the target file name: one can't replace a
    // directory with a file through a rename
    QVERIFY(QDir(dir.path()).mkdir("outfile"));

    // The saving should fail.
    QVERIFY(!file.commit());
    QCOMPARE(file.error(), QFile::RenameError);
#ifdef Q_OS_UNIX
    QCOMPARE(file.errorString(), qt_error_string(EISDIR));
#endif

    target.refresh();
    QVERIFY(target.isDir());
}

void tst_QSaveFile::symlink()
{
#ifdef Q_OS_UNIX
    QByteArray someData = "some data";
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    const QString targetFile = dir.path() + QLatin1String("/outfile");
    const QString linkFile = dir.path() + QLatin1String("/linkfile");
    {
        QFile file(targetFile);
        QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
        QCOMPARE(file.write("Hello"), Q_INT64_C(5));
        file.close();
    }

    QVERIFY(QFile::link(targetFile, linkFile));

    const QString canonical = QFileInfo(linkFile).canonicalFilePath();
    QCOMPARE(canonical, QFileInfo(targetFile).canonicalFilePath());

    // Try saving into it
    {
        QSaveFile saveFile(linkFile);
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        QCOMPARE(saveFile.write(someData), someData.size());
        QVERIFY(saveFile.commit());

        //Check that the linkFile is still a link and still has the same canonical path
        QFileInfo info(linkFile);
        QVERIFY(info.isSymLink());
        QCOMPARE(QFileInfo(linkFile).canonicalFilePath(), canonical);

        QFile file(targetFile);
        QVERIFY2(file.open(QIODevice::ReadOnly), msgCannotOpen(file).constData());
        QCOMPARE(file.readAll(), someData);
        file.remove();
    }

    // Save into a symbolic link that point to a removed file
    someData = "more stuff";
    {
        QSaveFile saveFile(linkFile);
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        QCOMPARE(saveFile.write(someData), someData.size());
        QVERIFY(saveFile.commit());

        QFileInfo info(linkFile);
        QVERIFY(info.isSymLink());
        QCOMPARE(QFileInfo(linkFile).canonicalFilePath(), canonical);

        QFile file(targetFile);
        QVERIFY2(file.open(QIODevice::ReadOnly), msgCannotOpen(file).constData());
        QCOMPARE(file.readAll(), someData);
    }

    // link to a link in another directory
    QTemporaryDir dir2;
    QVERIFY2(dir2.isValid(), qPrintable(dir2.errorString()));

    const QString linkFile2 = dir2.path() + QLatin1String("/linkfile");
    QVERIFY(QFile::link(linkFile, linkFile2));
    QCOMPARE(QFileInfo(linkFile2).canonicalFilePath(), canonical);


    someData = "hello everyone";

    {
        QSaveFile saveFile(linkFile2);
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        QCOMPARE(saveFile.write(someData), someData.size());
        QVERIFY(saveFile.commit());
        QCOMPARE(saveFile.size(), someData.size());

        QFile file(targetFile);
        QVERIFY2(file.open(QIODevice::ReadOnly), msgCannotOpen(file).constData());
        QCOMPARE(file.readAll(), someData);
    }

    //cyclic link
    const QString cyclicLink = dir.path() + QLatin1String("/cyclic");
    QVERIFY(QFile::link(cyclicLink, cyclicLink));
    {
        QSaveFile saveFile(cyclicLink);
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        QCOMPARE(saveFile.write(someData), someData.size());
        QVERIFY(saveFile.commit());

        QFile file(cyclicLink);
        QVERIFY2(file.open(QIODevice::ReadOnly), msgCannotOpen(file).constData());
        QCOMPARE(file.readAll(), someData);
    }

    //cyclic link2
    QVERIFY(QFile::link(cyclicLink + QLatin1Char('1'), cyclicLink + QLatin1Char('2')));
    QVERIFY(QFile::link(cyclicLink + QLatin1Char('2'), cyclicLink + QLatin1Char('1')));

    {
        QSaveFile saveFile(cyclicLink + QLatin1Char('1'));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        QCOMPARE(saveFile.write(someData), someData.size());
        QVERIFY(saveFile.commit());
        QCOMPARE(saveFile.size(), someData.size());

        // the explicit file becomes a file instead of a link
        QVERIFY(!QFileInfo(cyclicLink + QLatin1Char('1')).isSymLink());
        QVERIFY(QFileInfo(cyclicLink + QLatin1Char('2')).isSymLink());

        QFile file(cyclicLink + QLatin1Char('1'));
        QVERIFY2(file.open(QIODevice::ReadOnly), msgCannotOpen(file).constData());
        QCOMPARE(file.readAll(), someData);
    }
#endif
}

void tst_QSaveFile::directory()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));

    const QString subdir = dir.path() + QLatin1String("/subdir");
    QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("subdir")));
    {
        QFile sf(subdir);
        QVERIFY(!sf.open(QIODevice::WriteOnly));
    }

#ifdef Q_OS_UNIX
    //link to a directory
    const QString linkToDir = dir.path() + QLatin1String("/linkToDir");
    QVERIFY(QFile::link(subdir, linkToDir));

    {
        QFile sf(linkToDir);
        QVERIFY(!sf.open(QIODevice::WriteOnly));
    }
#endif
}

[[maybe_unused]] static void bufferedAndUnbuffered()
{
    QTest::addColumn<QIODevice::OpenMode>("mode");
    QTest::newRow("unbuffered") << QIODevice::OpenMode(QIODevice::Unbuffered);
    QTest::newRow("buffered") << QIODevice::OpenMode();
}

#ifdef Q_OS_UNIX
void tst_QSaveFile::writeFailToDevFull_data()
{
    // check if /dev/full exists and is writable
    if (access("/dev/full", W_OK) != 0)
        QSKIP("/dev/full either does not exist or is not writable");
    if (access("/dev", W_OK) == 0)
        QSKIP("/dev is writable (running as root?): this test would replace /dev/full");

    bufferedAndUnbuffered();
}

void tst_QSaveFile::writeFailToDevFull()
{
    QFETCH(QIODevice::OpenMode, mode);
    mode |= QIODevice::WriteOnly;

    QSaveFile saveFile("/dev/full");
    saveFile.setDirectWriteFallback(true);

    QVERIFY2(saveFile.open(mode), msgCannotOpen(saveFile).constData());

    QByteArray data("abc");
    qint64 written = saveFile.write(data);
    if (mode & QIODevice::Unbuffered) {
        // error reported immediately
        QCOMPARE(written, -1);
        QCOMPARE(saveFile.error(), QFile::ResourceError);
        QCOMPARE(saveFile.errorString(), qt_error_string(ENOSPC));
    } else {
        // error reported only on .commit()
        QCOMPARE(written, data.size());
        QCOMPARE(saveFile.error(), QFile::NoError);
    }
    QVERIFY(!saveFile.commit());
    QCOMPARE(saveFile.error(), QFile::ResourceError);
}
#endif // Q_OS_UNIX
#if defined(RLIMIT_FSIZE) && (defined(Q_OS_LINUX) || defined(Q_OS_MACOS))
// This test is only enabled on Linux and on macOS because we can verify that
// those OSes do respect RLIMIT_FSIZE. We can also verify that some other Unix
// OSes do not.

void tst_QSaveFile::writeFailResourceLimit_data()
{
    bufferedAndUnbuffered();
}

void tst_QSaveFile::writeFailResourceLimit()
{
    // don't make it too small because stdout may be a log file!
    static constexpr qint64 FileSizeLimit = 1024 * 1024;
    struct RlimitChanger {
        struct rlimit old;
        RlimitChanger()
        {
            getrlimit(RLIMIT_FSIZE, &old);
            struct rlimit newLimit = {};
            newLimit.rlim_cur = FileSizeLimit;
            newLimit.rlim_max = old.rlim_max;
            if (setrlimit(RLIMIT_FSIZE, &newLimit) != 0)
                old.rlim_cur = 0;

            // ignore SIGXFSZ so we get EF2BIG when writing the file
            signal(SIGXFSZ, SIG_IGN);
        }
        ~RlimitChanger()
        {
            if (old.rlim_cur)
                setrlimit(RLIMIT_FSIZE, &old);
        }
    };

    QFETCH(QIODevice::OpenMode, mode);
    mode |= QIODevice::WriteOnly;

    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QFile::remove(targetFile);
    QSaveFile saveFile(targetFile);
    QVERIFY2(saveFile.open(mode), msgCannotOpen(saveFile).constData());

    RlimitChanger changer;
    if (changer.old.rlim_cur == 0)
        QSKIP("Could not set the file size resource limit");

    QByteArray data(FileSizeLimit + 16, 'a');
    qint64 written = 0, lastWrite;
    do {
        lastWrite = saveFile.write(data.constData() + written, data.size() - written);
        if (lastWrite > 0)
            written += lastWrite;
    } while (lastWrite > 0 && written < data.size());
    if (mode & QIODevice::Unbuffered) {
        // error reported immediately
        QCOMPARE_LT(written, data.size());
        QCOMPARE(lastWrite, -1);
        QCOMPARE(saveFile.error(), QFile::WriteError);
        QCOMPARE(saveFile.errorString(), qt_error_string(EFBIG));
    } else {
        // error reported only on .commit()
        QCOMPARE(written, data.size());
        QCOMPARE(saveFile.error(), QFile::NoError);
    }
    QVERIFY(!saveFile.commit());
    QCOMPARE(saveFile.error(), QFile::WriteError);
}
#endif // RLIMIT_FSIZE && (Linux or macOS)

#ifdef Q_OS_WIN
void tst_QSaveFile::alternateDataStream_data()
{
    QTest::addColumn<bool>("directWriteFallback");
    QTest::addColumn<bool>("success");

    QTest::newRow("default") << false << false;
    QTest::newRow("directWriteFallback") << true << true;
}

void tst_QSaveFile::alternateDataStream()
{
    QFETCH(bool, directWriteFallback);
    QFETCH(bool, success);
    static const char newContent[] = "New content\r\n";

    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    QString baseName = dir.path() + QLatin1String("/base");
    {
        QFile baseFile(baseName);
        QVERIFY2(baseFile.open(QIODevice::ReadWrite), qPrintable(baseFile.errorString()));
    }

    // First, create a file with old content
    QString adsName = baseName + QLatin1String(":outfile");
    {
        QFile targetFile(adsName);
        if (!targetFile.open(QIODevice::ReadWrite))
            QSKIP("Failed to ceate ADS file (" + targetFile.errorString().toUtf8()
                  + "). Temp dir is FAT?");
        targetFile.write("Old content\r\n");
    }

    // And write to it again using QSaveFile; only works if directWriteFallback is enabled
    QSaveFile file(adsName);
    file.setDirectWriteFallback(directWriteFallback);
    QCOMPARE(file.directWriteFallback(), directWriteFallback);

    if (success) {
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
        file.write(newContent);
        QVERIFY2(file.commit(), qPrintable(file.errorString()));
        QCOMPARE(file.size(), qint64(strlen(newContent)));

        // check the contents
        QFile targetFile(adsName);
        QVERIFY2(targetFile.open(QIODevice::ReadOnly), qPrintable(targetFile.errorString()));
        QByteArray contents = targetFile.readAll();
        QCOMPARE(contents, QByteArray(newContent));
    } else {
        QVERIFY(!file.open(QIODevice::WriteOnly));
    }
}
#endif

QTEST_MAIN(tst_QSaveFile)
#include "tst_qsavefile.moc"
