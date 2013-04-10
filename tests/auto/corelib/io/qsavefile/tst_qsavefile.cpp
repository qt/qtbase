/****************************************************************************
**
** Copyright (C) 2012 David Faure <faure@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#if defined(Q_OS_UNIX)
# include <unistd.h> // for geteuid
# include <sys/types.h>
#endif

#if defined(Q_OS_WIN)
# include <windows.h>
#endif

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
};

void tst_QSaveFile::transactionalWrite()
{
    QTemporaryDir dir;
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QFile::remove(targetFile);
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
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
}

void tst_QSaveFile::saveTwice()
{
    // Check that we can reuse a QSaveFile object
    // (and test the case of an existing target file)
    QTemporaryDir dir;
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("Hello"), Q_INT64_C(5));
    QVERIFY2(file.commit(), qPrintable(file.errorString()));

    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("World"), Q_INT64_C(5));
    QVERIFY2(file.commit(), qPrintable(file.errorString()));

    QFile reader(targetFile);
    QVERIFY(reader.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1("World"));
}

void tst_QSaveFile::textStreamManualFlush()
{
    QTemporaryDir dir;
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));

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
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));

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
    QFETCH(bool, directWriteFallback);
    // Restore permissions so that the QTemporaryDir cleanup can happen
    class PermissionRestorer
    {
        QString m_path;
    public:
        PermissionRestorer(const QString& path)
            : m_path(path)
        {}

        ~PermissionRestorer()
        {
            restore();
        }
        void restore()
        {
            QFile file(m_path);
            file.setPermissions(QFile::Permissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        }
    };


    QTemporaryDir dir;
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
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE((int)file.error(), (int)QFile::NoError);
        QCOMPARE(file.write("World"), Q_INT64_C(5));
        QVERIFY(file.commit());

        QFile reader(targetFile);
        QVERIFY(reader.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromLatin1(reader.readAll()), QString::fromLatin1("World"));
        reader.close();

        QVERIFY(file.open(QIODevice::WriteOnly));
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
    // Setup an existing but readonly file
    QTemporaryDir dir;
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
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
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QFile::remove(targetFile);
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));

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
    QTemporaryDir dir;
    const QString targetFile = dir.path() + QString::fromLatin1("/outfile");
    QSaveFile file(targetFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("Hello"), qint64(5));
    QVERIFY(!QFile::exists(targetFile));

    // Restore permissions so that the QTemporaryDir cleanup can happen
    class PermissionRestorer
    {
    public:
        PermissionRestorer(const QString& path)
            : m_path(path)
        {}

        ~PermissionRestorer()
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
        QString m_path;
    };

#ifdef Q_OS_UNIX
    // Make rename() fail for lack of permissions in the directory
    QFile dirAsFile(dir.path()); // yay, I have to use QFile to change a dir's permissions...
    QVERIFY(dirAsFile.setPermissions(QFile::Permissions(0))); // no permissions
    PermissionRestorer permissionRestorer(dir.path());
#else
    // Windows: Make rename() fail for lack of permissions on an existing target file
    QFile existingTargetFile(targetFile);
    QVERIFY(existingTargetFile.open(QIODevice::WriteOnly));
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

QTEST_MAIN(tst_QSaveFile)
#include "tst_qsavefile.moc"
