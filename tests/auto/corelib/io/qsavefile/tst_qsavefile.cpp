/****************************************************************************
**
** Copyright (C) 2012 David Faure <faure@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <qcoreapplication.h>
#include <qstring.h>
#include <qtemporaryfile.h>
#include <qfile.h>
#include <qdir.h>
#include <qset.h>

#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
#include <unistd.h> // for geteuid
#endif

#if defined(Q_OS_WIN)
# include <windows.h>
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
    void textStreamManualFlush();
    void textStreamAutoFlush();
    void saveTwice();
    void transactionalWriteNoPermissionsOnDir_data();
    void transactionalWriteNoPermissionsOnDir();
    void transactionalWriteNoPermissionsOnFile();
    void transactionalWriteCanceled();
    void transactionalWriteErrorRenaming();
    void symlink();
    void directory();
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

    QCOMPARE(file.write("Hello"), Q_INT64_C(5));
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(!QFile::exists(targetFile));

    QVERIFY(file.commit());
    QVERIFY(QFile::exists(targetFile));
    QCOMPARE(file.fileName(), targetFile);

    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1("Hello"));

    // check that permissions are the same as for QFile
    const QString otherFile = dir.path() + QString::fromLatin1("/otherfile");
    QFile::remove(otherFile);
    QFile other(otherFile);
    other.open(QIODevice::WriteOnly);
    other.close();
    QCOMPARE(QFile::permissions(targetFile), QFile::permissions(otherFile));
}

void tst_QSaveFile::saveTwice()
{
    // Check that we can reuse a QSaveFile object
    // (and test the case of an existing target file)
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QCOMPARE(file.write("Hello"), Q_INT64_C(5));
    QVERIFY2(file.commit(), qPrintable(file.errorString()));

    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());
    QCOMPARE(file.write("World"), Q_INT64_C(5));
    QVERIFY2(file.commit(), qPrintable(file.errorString()));

    QFile reader(targetFile);
    QVERIFY2(reader.open(QIODevice::ReadOnly), msgCannotOpen(reader).constData());
    QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1("World"));
}

void tst_QSaveFile::textStreamManualFlush()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), qPrintable(dir.errorString()));
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY2(file.open(QIODevice::WriteOnly), msgCannotOpen(file).constData());

    QTextStream ts(&file);
    ts << "Manual flush";
    ts.flush();
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(!QFile::exists(targetFile));

    QVERIFY(file.commit());
    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll().constData()), QString::fromLatin1("Manual flush"));
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
        file.cancelWriting(); // no effect, as per the documentation
        QVERIFY(file.commit());

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

void tst_QSaveFile::transactionalWriteErrorRenaming()
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
    QVERIFY(dirAsFile.setPermissions(QFile::Permissions(0))); // no permissions
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

    QString canonical = QFileInfo(linkFile).canonicalFilePath();
    QCOMPARE(canonical, QFileInfo(targetFile).canonicalFilePath());

    // Try saving into it
    {
        QSaveFile saveFile(linkFile);
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        QCOMPARE(saveFile.write(someData), someData.size());
        saveFile.commit();

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
        saveFile.commit();

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
        saveFile.commit();

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
        saveFile.commit();

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
        saveFile.commit();

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

QTEST_MAIN(tst_QSaveFile)
#include "tst_qsavefile.moc"
