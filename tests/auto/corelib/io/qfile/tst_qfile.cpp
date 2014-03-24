/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <qplatformdefs.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <private/qabstractfileengine_p.h>
#include <private/qfsfileengine_p.h>
#include <private/qfilesystemengine_p.h>

#ifdef Q_OS_WIN
QT_BEGIN_NAMESPACE
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
QT_END_NAMESPACE
#endif

#if !defined(Q_OS_WINCE) && !defined(QT_NO_NETWORK)
#include <QHostInfo>
#endif
#include <QProcess>
#ifdef Q_OS_WIN
# include <qt_windows.h>
#else
# include <sys/types.h>
# include <unistd.h>
#endif
#ifdef Q_OS_MAC
# include <sys/mount.h>
#elif defined(Q_OS_LINUX)
# include <sys/vfs.h>
#elif defined(Q_OS_FREEBSD)
# include <sys/param.h>
# include <sys/mount.h>
#elif defined(Q_OS_IRIX)
# include <sys/statfs.h>
#elif defined(Q_OS_WINCE)
# include <qplatformdefs.h>
#elif defined(Q_OS_VXWORKS)
# include <fcntl.h>
#if defined(_WRS_KERNEL)
#undef QT_OPEN
#define QT_OPEN(path, oflag) ::open(path, oflag, 0)
#endif
#endif

#ifdef Q_OS_QNX
#ifdef open
#undef open
#endif
#endif

#include <stdio.h>
#include <errno.h>

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
#include "../../../network-settings.h"
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef QT_OPEN_BINARY
#define QT_OPEN_BINARY 0
#endif

Q_DECLARE_METATYPE(QFile::FileError)

class tst_QFile : public QObject
{
    Q_OBJECT
public:
    tst_QFile();

private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();
    void exists();
    void open_data();
    void open();
    void openUnbuffered();
    void size_data();
    void size();
    void sizeNoExist();
    void seek();
    void setSize();
    void setSizeSeek();
    void atEnd();
    void readLine();
    void readLine2();
    void readLineNullInLine();
    void readAll_data();
    void readAll();
    void readAllBuffer();
#if !defined(Q_OS_WINCE) && !defined(QT_NO_PROCESS)
    void readAllStdin();
    void readLineStdin();
    void readLineStdin_lineByLine();
#endif
    void text();
    void missingEndOfLine();
    void readBlock();
    void getch();
    void ungetChar();
    void createFile();
    void append();
    void permissions_data();
    void permissions();
#ifdef Q_OS_WIN
    void permissionsNtfs_data();
    void permissionsNtfs();
#endif
    void setPermissions();
    void copy();
    void copyAfterFail();
    void copyRemovesTemporaryFile() const;
    void copyShouldntOverwrite();
    void copyFallback();
    void link();
    void linkToDir();
    void absolutePathLinkToRelativePath();
    void readBrokenLink();
    void readTextFile_data();
    void readTextFile();
    void readTextFile2();
    void writeTextFile_data();
    void writeTextFile();
    /* void largeFileSupport(); */
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    void largeUncFileSupport();
#endif
    void flush();
    void bufferedRead();
#ifdef Q_OS_UNIX
    void isSequential();
#endif
    void encodeName();
    void truncate();
    void seekToPos();
    void seekAfterEndOfFile();
    void FILEReadWrite();
    void i18nFileName_data();
    void i18nFileName();
    void longFileName_data();
    void longFileName();
    void fileEngineHandler();
#ifdef QT_BUILD_INTERNAL
    void useQFileInAFileHandler();
#endif
    void getCharFF();
    void remove_and_exists();
    void removeOpenFile();
    void fullDisk();
    void writeLargeDataBlock_data();
    void writeLargeDataBlock();
    void readFromWriteOnlyFile();
    void writeToReadOnlyFile();
#if defined(Q_OS_LINUX) || defined(Q_OS_AIX) || defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
    void virtualFile();
#endif
    void textFile();
    void rename_data();
    void rename();
    void renameWithAtEndSpecialFile() const;
    void renameFallback();
    void renameMultiple();
    void appendAndRead();
    void miscWithUncPathAsCurrentDir();
    void standarderror();
    void handle();
    void nativeHandleLeaks();

    void readEof_data();
    void readEof();

    void map_data();
    void map();
    void mapResource_data();
    void mapResource();
    void mapOpenMode_data();
    void mapOpenMode();

#ifndef Q_OS_WINCE
    void openStandardStreamsFileDescriptors();
    void openStandardStreamsBufferedStreams();
#endif

    void resize_data();
    void resize();

    void objectConstructors();

    void caseSensitivity();

    void autocloseHandle();

    void posAfterFailedStat();

    void openDirectory();
    void writeNothing();

    void invalidFile_data();
    void invalidFile();

private:
    enum FileType {
        OpenQFile,
        OpenFd,
        OpenStream,
        NumberOfFileTypes
    };

    bool openFd(QFile &file, QIODevice::OpenMode mode, QFile::FileHandleFlags handleFlags)
    {
        int fdMode = QT_OPEN_LARGEFILE | QT_OPEN_BINARY;

        // File will be truncated if in Write mode.
        if (mode & QIODevice::WriteOnly)
            fdMode |= QT_OPEN_WRONLY | QT_OPEN_TRUNC;
        if (mode & QIODevice::ReadOnly)
            fdMode |= QT_OPEN_RDONLY;

        fd_ = QT_OPEN(qPrintable(file.fileName()), fdMode);

        return (-1 != fd_) && file.open(fd_, mode, handleFlags);
    }

    bool openStream(QFile &file, QIODevice::OpenMode mode, QFile::FileHandleFlags handleFlags)
    {
        char const *streamMode = "";

        // File will be truncated if in Write mode.
        if (mode & QIODevice::WriteOnly)
            streamMode = "wb+";
        else if (mode & QIODevice::ReadOnly)
            streamMode = "rb";

        stream_ = QT_FOPEN(qPrintable(file.fileName()), streamMode);

        return stream_ && file.open(stream_, mode, handleFlags);
    }

    bool openFile(QFile &file, QIODevice::OpenMode mode, FileType type = OpenQFile, QFile::FileHandleFlags handleFlags = QFile::DontCloseHandle)
    {
        if (mode & QIODevice::WriteOnly && !file.exists())
        {
            // Make sure the file exists
            QFile createFile(file.fileName());
            if (!createFile.open(QIODevice::ReadWrite))
                return false;
        }

        // Note: openFd and openStream will truncate the file if write mode.
        switch (type)
        {
            case OpenQFile:
                return file.open(mode);

            case OpenFd:
                return openFd(file, mode, handleFlags);

            case OpenStream:
                return openStream(file, mode, handleFlags);

            case NumberOfFileTypes:
                break;
        }

        return false;
    }

    void closeFile(QFile &file)
    {
        file.close();

        if (-1 != fd_)
            QT_CLOSE(fd_);
        if (stream_)
            ::fclose(stream_);

        fd_ = -1;
        stream_ = 0;
    }

    int fd_;
    FILE *stream_;

    QTemporaryDir m_temporaryDir;
    const QString m_oldDir;
    QString m_stdinProcessDir;
    QString m_testSourceFile;
    QString m_testLogFile;
    QString m_dosFile;
    QString m_forCopyingFile;
    QString m_forRenamingFile;
    QString m_twoDotsFile;
    QString m_testFile;
    QString m_resourcesDir;
    QString m_noEndOfLineFile;
};

static const char noReadFile[] = "noreadfile";
static const char readOnlyFile[] = "readonlyfile";

void tst_QFile::init()
{
    fd_ = -1;
    stream_ = 0;
}

void tst_QFile::cleanup()
{
    if (-1 != fd_)
        QT_CLOSE(fd_);
    fd_ = -1;
    if (stream_)
        ::fclose(stream_);
    stream_ = 0;

    // Windows UNC tests set a different working directory which might not be restored on failures.
    if (QDir::currentPath() != m_temporaryDir.path())
        QVERIFY(QDir::setCurrent(m_temporaryDir.path()));

    // Clean out everything except the readonly-files.
    const QDir dir(m_temporaryDir.path());
    foreach (const QFileInfo &fi, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        const QString fileName = fi.fileName();
        if (fileName != QLatin1String(noReadFile) && fileName != QLatin1String(readOnlyFile)) {
            const QString absoluteFilePath = fi.absoluteFilePath();
            if (fi.isDir() && !fi.isSymLink()) {
                QDir remainingDir(absoluteFilePath);
                QVERIFY2(remainingDir.removeRecursively(), qPrintable(absoluteFilePath));
            } else {
                QVERIFY2(QFile::remove(absoluteFilePath), qPrintable(absoluteFilePath));
            }
        }
    }
}

tst_QFile::tst_QFile() : m_oldDir(QDir::currentPath())
{
}

void tst_QFile::initTestCase()
{
    QVERIFY(m_temporaryDir.isValid());
    m_stdinProcessDir = QFINDTESTDATA("stdinprocess");
    QVERIFY(!m_stdinProcessDir.isEmpty());
    m_testSourceFile = QFINDTESTDATA("tst_qfile.cpp");
    QVERIFY(!m_testSourceFile.isEmpty());
    m_testLogFile = QFINDTESTDATA("testlog.txt");
    QVERIFY(!m_testLogFile.isEmpty());
    m_dosFile = QFINDTESTDATA("dosfile.txt");
    QVERIFY(!m_dosFile.isEmpty());
    m_forCopyingFile = QFINDTESTDATA("forCopying.txt");
    QVERIFY(!m_forCopyingFile .isEmpty());
    m_forRenamingFile = QFINDTESTDATA("forRenaming.txt");
    QVERIFY(!m_forRenamingFile.isEmpty());
    m_twoDotsFile = QFINDTESTDATA("two.dots.file");
    QVERIFY(!m_twoDotsFile.isEmpty());
    m_testFile = QFINDTESTDATA("testfile.txt");
    QVERIFY(!m_testFile.isEmpty());
    m_resourcesDir = QFINDTESTDATA("resources");
    QVERIFY(!m_resourcesDir.isEmpty());
    m_noEndOfLineFile = QFINDTESTDATA("noendofline.txt");
    QVERIFY(!m_noEndOfLineFile.isEmpty());

    QVERIFY(QDir::setCurrent(m_temporaryDir.path()));

    // create a file and make it read-only
    QFile file(QString::fromLatin1(readOnlyFile));
    QVERIFY2(file.open(QFile::WriteOnly), qPrintable(file.errorString()));
    file.write("a", 1);
    file.close();
    QVERIFY2(file.setPermissions(QFile::ReadOwner), qPrintable(file.errorString()));
    // create another file and make it not readable
    file.setFileName(QString::fromLatin1(noReadFile));
    QVERIFY2(file.open(QFile::WriteOnly), qPrintable(file.errorString()));
    file.write("b", 1);
    file.close();
#ifndef Q_OS_WIN // Not supported on Windows.
    QVERIFY2(file.setPermissions(0), qPrintable(file.errorString()));
#else
    QVERIFY2(file.open(QFile::WriteOnly), qPrintable(file.errorString()));
#endif
}

void tst_QFile::cleanupTestCase()
{
    QFile file(QString::fromLatin1(readOnlyFile));
    QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner));
    file.setFileName(QString::fromLatin1(noReadFile));
    QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner));
    QVERIFY(QDir::setCurrent(m_oldDir)); //release test directory for removal
}

//------------------------------------------
// The 'testfile' is currently just a
// testfile. The path of this file, the
// attributes and the contents itself
// will be changed as far as we have a
// proper way to handle files in the
// testing environment.
//------------------------------------------

void tst_QFile::exists()
{
    QFile f( m_testFile );
    QVERIFY(f.exists());

    QFile file("nobodyhassuchafile");
    file.remove();
    QVERIFY(!file.exists());

    QFile file2("nobodyhassuchafile");
    QVERIFY(file2.open(QIODevice::WriteOnly));
    file2.close();

    QVERIFY(file.exists());

    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QVERIFY(file.exists());

    file.remove();
    QVERIFY(!file.exists());

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    QFile unc("//" + QtNetworkSettings::winServerName() + "/testshare/readme.txt");
    QVERIFY(unc.exists());
#endif
}

void tst_QFile::open_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<int>("mode");
    QTest::addColumn<bool>("ok");
    QTest::addColumn<QFile::FileError>("status");

    QTest::newRow( "exist_readOnly"  )
        << m_testFile << int(QIODevice::ReadOnly)
        << true << QFile::NoError;

    QTest::newRow( "exist_writeOnly" )
        << QString::fromLatin1(readOnlyFile)
        << int(QIODevice::WriteOnly)
        << false
        << QFile::OpenError;

    QTest::newRow( "exist_append"    )
        << QString::fromLatin1(readOnlyFile) << int(QIODevice::Append)
        << false << QFile::OpenError;

    QTest::newRow( "nonexist_readOnly"  )
        << QString("nonExist.txt") << int(QIODevice::ReadOnly)
        << false << QFile::OpenError;

    QTest::newRow("emptyfile")
        << QString("")
        << int(QIODevice::ReadOnly)
        << false
        << QFile::OpenError;

    QTest::newRow("nullfile") << QString() << int(QIODevice::ReadOnly) << false
        << QFile::OpenError;

    QTest::newRow("two-dots") << m_twoDotsFile << int(QIODevice::ReadOnly) << true
        << QFile::NoError;

    QTest::newRow("readonlyfile") << QString::fromLatin1(readOnlyFile) << int(QIODevice::WriteOnly)
                                  << false << QFile::OpenError;
    QTest::newRow("noreadfile") << QString::fromLatin1(noReadFile) << int(QIODevice::ReadOnly)
                                << false << QFile::OpenError;
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    //opening devices requires administrative privileges (and elevation).
    HANDLE hTest = CreateFile(_T("\\\\.\\PhysicalDrive0"), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hTest != INVALID_HANDLE_VALUE) {
        CloseHandle(hTest);
        QTest::newRow("//./PhysicalDrive0") << QString("//./PhysicalDrive0") << int(QIODevice::ReadOnly)
                                            << true << QFile::NoError;
    } else {
        QTest::newRow("//./PhysicalDrive0") << QString("//./PhysicalDrive0") << int(QIODevice::ReadOnly)
                                            << false << QFile::OpenError;
    }
    QTest::newRow("uncFile") << "//" + QtNetworkSettings::winServerName() + "/testshare/test.pri" << int(QIODevice::ReadOnly)
                             << true << QFile::NoError;
#endif
}

void tst_QFile::open()
{
    QFETCH( QString, filename );
    QFETCH( int, mode );

    QFile f( filename );

    QFETCH( bool, ok );

#if defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
    if (::getuid() == 0)
        // root and Chuck Norris don't care for file permissions. Skip.
        QSKIP("Running this test as root doesn't make sense");
#endif

#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    QEXPECT_FAIL("noreadfile", "Windows does not currently support non-readable files.", Abort);
#endif
    if (filename.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "QFSFileEngine::open: No file name specified");

    QCOMPARE(f.open( QIODevice::OpenMode(mode) ), ok);

    QTEST( f.error(), "status" );
}

void tst_QFile::openUnbuffered()
{
    QFile file(m_testFile);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Unbuffered));
    char c = '\0';
    QVERIFY(file.seek(1));
    QCOMPARE(file.pos(), qint64(1));
    QVERIFY(file.getChar(&c));
    QCOMPARE(file.pos(), qint64(2));
    char d = '\0';
    QVERIFY(file.seek(3));
    QCOMPARE(file.pos(), qint64(3));
    QVERIFY(file.getChar(&d));
    QCOMPARE(file.pos(), qint64(4));
    QVERIFY(file.seek(1));
    QCOMPARE(file.pos(), qint64(1));
    char c2 = '\0';
    QVERIFY(file.getChar(&c2));
    QCOMPARE(file.pos(), qint64(2));
    QVERIFY(file.seek(3));
    QCOMPARE(file.pos(), qint64(3));
    char d2 = '\0';
    QVERIFY(file.getChar(&d2));
    QCOMPARE(file.pos(), qint64(4));
    QCOMPARE(c, c2);
    QCOMPARE(d, d2);
    QCOMPARE(c, '-');
    QCOMPARE(d, '-');
}

void tst_QFile::size_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<qint64>("size");

    QTest::newRow( "exist01" ) << m_testFile << (qint64)245;
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    // Only test UNC on Windows./
    QTest::newRow("unc") << "//" + QString(QtNetworkSettings::winServerName() + "/testshare/test.pri") << (qint64)34;
#endif
}

void tst_QFile::size()
{
    QFETCH( QString, filename );
    QFETCH( qint64, size );

#ifdef Q_OS_WINCE
        filename = QFileInfo(filename).absoluteFilePath();
#endif

    {
        QFile f( filename );
        QCOMPARE( f.size(), size );

        QVERIFY( f.open(QIODevice::ReadOnly) );
        QCOMPARE( f.size(), size );
    }

    {
        QFile f;
        FILE* stream = QT_FOPEN(filename.toLocal8Bit().constData(), "rb");
        QVERIFY( stream );
        QVERIFY( f.open(stream, QIODevice::ReadOnly) );
        QCOMPARE( f.size(), size );

        f.close();
        fclose(stream);
    }

    // Currently low level file I/O is not well supported on Windows CE, so
    // skip this part of the test.
#ifndef Q_OS_WINCE
    {
        QFile f;

        int fd = QT_OPEN(filename.toLocal8Bit().constData(), QT_OPEN_RDONLY);

        QVERIFY( fd != -1 );
        QVERIFY( f.open(fd, QIODevice::ReadOnly) );
        QCOMPARE( f.size(), size );

        f.close();
        QT_CLOSE(fd);
    }
#endif
}

void tst_QFile::sizeNoExist()
{
    QFile file("nonexist01");
    QVERIFY( !file.exists() );
    QCOMPARE( file.size(), (qint64)0 );
    QVERIFY( !file.open(QIODevice::ReadOnly) );
}

void tst_QFile::seek()
{
    QFile file("newfile.txt");
    file.open(QIODevice::WriteOnly);
    QCOMPARE(file.size(), qint64(0));
    QCOMPARE(file.pos(), qint64(0));
    QVERIFY(file.seek(10));
    QCOMPARE(file.pos(), qint64(10));
    QCOMPARE(file.size(), qint64(0));
    file.close();
}

void tst_QFile::setSize()
{
    QFile f("createme.txt");
    QVERIFY(f.open(QIODevice::Truncate | QIODevice::ReadWrite));
    f.putChar('a');

    f.seek(0);
    char c = '\0';
    f.getChar(&c);
    QCOMPARE(c, 'a');

    QCOMPARE(f.size(), (qlonglong)1);
    bool ok = f.resize(99);
    QVERIFY(ok);
    QCOMPARE(f.size(), (qlonglong)99);

    f.seek(0);
    c = '\0';
    f.getChar(&c);
    QCOMPARE(c, 'a');

    QVERIFY(f.resize(1));
    QCOMPARE(f.size(), (qlonglong)1);

    f.seek(0);
    c = '\0';
    f.getChar(&c);
    QCOMPARE(c, 'a');

    f.close();

    QCOMPARE(f.size(), (qlonglong)1);
    QVERIFY(f.resize(100));
    QCOMPARE(f.size(), (qlonglong)100);
    QVERIFY(f.resize(50));
    QCOMPARE(f.size(), (qlonglong)50);
}

void tst_QFile::setSizeSeek()
{
    QFile f("setsizeseek.txt");
    QVERIFY(f.open(QFile::WriteOnly));
    f.write("ABCD");

    QCOMPARE(f.pos(), qint64(4));
    f.resize(2);
    QCOMPARE(f.pos(), qint64(2));
    f.resize(4);
    QCOMPARE(f.pos(), qint64(2));
    f.resize(0);
    QCOMPARE(f.pos(), qint64(0));
    f.resize(4);
    QCOMPARE(f.pos(), qint64(0));

    f.seek(3);
    QCOMPARE(f.pos(), qint64(3));
    f.resize(2);
    QCOMPARE(f.pos(), qint64(2));
}

void tst_QFile::atEnd()
{
    QFile f( m_testFile );
    QVERIFY(f.open( QIODevice::ReadOnly ));

    int size = f.size();
    f.seek( size );

    bool end = f.atEnd();
    f.close();
    QVERIFY(end);
}

void tst_QFile::readLine()
{
    QFile f( m_testFile );
    QVERIFY(f.open( QIODevice::ReadOnly ));

    int i = 0;
    char p[128];
    int foo;
    while ( (foo=f.readLine( p, 128 )) > 0 ) {
        ++i;
        if ( i == 5 ) {
            QCOMPARE( p[0], 'T' );
            QCOMPARE( p[3], 's' );
            QCOMPARE( p[11], 'i' );
        }
    }
    f.close();
    QCOMPARE( i, 6 );
}

void tst_QFile::readLine2()
{
    QFile f( m_testFile );
    f.open( QIODevice::ReadOnly );

    char p[128];
    QCOMPARE(f.readLine(p, 60), qlonglong(59));
    QCOMPARE(f.readLine(p, 60), qlonglong(59));
    memset(p, '@', sizeof(p));
    QCOMPARE(f.readLine(p, 60), qlonglong(59));

    QCOMPARE(p[57], '-');
    QCOMPARE(p[58], '\n');
    QCOMPARE(p[59], '\0');
    QCOMPARE(p[60], '@');
}

void tst_QFile::readLineNullInLine()
{
    QFile::remove("nullinline.txt");
    QFile file("nullinline.txt");
    QVERIFY(file.open(QIODevice::ReadWrite));
    QVERIFY(file.write("linewith\0null\nanotherline\0withnull\n\0\nnull\0", 42) > 0);
    QVERIFY(file.flush());
    file.reset();

    QCOMPARE(file.readLine(), QByteArray("linewith\0null\n", 14));
    QCOMPARE(file.readLine(), QByteArray("anotherline\0withnull\n", 21));
    QCOMPARE(file.readLine(), QByteArray("\0\n", 2));
    QCOMPARE(file.readLine(), QByteArray("null\0", 5));
    QCOMPARE(file.readLine(), QByteArray());
}

void tst_QFile::readAll_data()
{
    QTest::addColumn<bool>("textMode");
    QTest::addColumn<QString>("fileName");
    QTest::newRow( "TextMode unixfile" ) <<  true << m_testFile;
    QTest::newRow( "BinaryMode unixfile" ) <<  false << m_testFile;
    QTest::newRow( "TextMode dosfile" ) <<  true << m_dosFile;
    QTest::newRow( "BinaryMode dosfile" ) <<  false << m_dosFile;
    QTest::newRow( "TextMode bigfile" ) <<  true << m_testSourceFile;
    QTest::newRow( "BinaryMode  bigfile" ) <<  false << m_testSourceFile;
    QVERIFY(QFile(m_testSourceFile).size() > 64*1024);
}

void tst_QFile::readAll()
{
    QFETCH( bool, textMode );
    QFETCH( QString, fileName );

    QFile file(fileName);
    if (textMode)
        QVERIFY(file.open(QFile::Text | QFile::ReadOnly));
    else
        QVERIFY(file.open(QFile::ReadOnly));

    QByteArray a = file.readAll();
    file.reset();
    QVERIFY(file.pos() == 0);

    QVERIFY(file.bytesAvailable() > 7);
    QByteArray b = file.read(1);
    char x;
    file.getChar(&x);
    b.append(x);
    b.append(file.read(5));
    b.append(file.readAll());

    QCOMPARE(a, b);
}

void tst_QFile::readAllBuffer()
{
    QString fileName = QLatin1String("readAllBuffer.txt");

    QFile::remove(fileName);

    QFile writer(fileName);
    QFile reader(fileName);

    QByteArray data1("This is arguably a very simple text.");
    QByteArray data2("This is surely not as simple a test.");

    QVERIFY( writer.open(QIODevice::ReadWrite | QIODevice::Unbuffered) );
    QVERIFY( reader.open(QIODevice::ReadOnly) );

    QCOMPARE( writer.write(data1), qint64(data1.size()) );
    QVERIFY( writer.seek(0) );

    QByteArray result;
    result = reader.read(18);
    QCOMPARE( result.size(), 18 );

    QCOMPARE( writer.write(data2), qint64(data2.size()) ); // new data, old version buffered in reader
    QCOMPARE( writer.write(data2), qint64(data2.size()) ); // new data, unbuffered in reader

    result += reader.readAll();

    QCOMPARE( result, data1 + data2 );

    QFile::remove(fileName);
}

#if !defined(Q_OS_WINCE) && !defined(QT_NO_PROCESS)
void tst_QFile::readAllStdin()
{
    QByteArray lotsOfData(1024, '@'); // 10 megs

    QProcess process;
    process.start(m_stdinProcessDir + QStringLiteral("/stdinprocess"), QStringList(QStringLiteral("all")));
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    for (int i = 0; i < 5; ++i) {
        QTest::qWait(1000);
        process.write(lotsOfData);
        while (process.bytesToWrite() > 0) {
            QVERIFY(process.waitForBytesWritten());
        }
    }

    process.closeWriteChannel();
    process.waitForFinished();
    QCOMPARE(process.readAll().size(), lotsOfData.size() * 5);
}

void tst_QFile::readLineStdin()
{

    QByteArray lotsOfData(1024, '@'); // 10 megs
    for (int i = 0; i < lotsOfData.size(); ++i) {
        if ((i % 32) == 31)
            lotsOfData[i] = '\n';
        else
            lotsOfData[i] = char('0' + i % 32);
    }

    for (int i = 0; i < 2; ++i) {
        QProcess process;
        process.start(m_stdinProcessDir + QStringLiteral("/stdinprocess"),
                      QStringList() << QStringLiteral("line") << QString::number(i),
                      QIODevice::Text | QIODevice::ReadWrite);
        QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
        for (int i = 0; i < 5; ++i) {
            QTest::qWait(1000);
            process.write(lotsOfData);
            while (process.bytesToWrite() > 0) {
                QVERIFY(process.waitForBytesWritten());
            }
        }

        process.closeWriteChannel();
        QVERIFY(process.waitForFinished(5000));

        QByteArray array = process.readAll();
        QCOMPARE(array.size(), lotsOfData.size() * 5);
        for (int i = 0; i < array.size(); ++i) {
            if ((i % 32) == 31)
                QCOMPARE(char(array[i]), '\n');
            else
                QCOMPARE(char(array[i]), char('0' + i % 32));
        }
    }
}

void tst_QFile::readLineStdin_lineByLine()
{
    for (int i = 0; i < 2; ++i) {
        QProcess process;
        process.start(m_stdinProcessDir + QStringLiteral("/stdinprocess"),
                      QStringList() << QStringLiteral("line") << QString::number(i),
                      QIODevice::Text | QIODevice::ReadWrite);
        QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));

        for (int j = 0; j < 3; ++j) {
            QByteArray line = "line " + QByteArray::number(j) + "\n";
            QCOMPARE(process.write(line), qint64(line.size()));
            QVERIFY(process.waitForBytesWritten(2000));
            if (process.bytesAvailable() == 0)
                QVERIFY(process.waitForReadyRead(2000));
            QCOMPARE(process.readAll(), line);
        }

        process.closeWriteChannel();
        QVERIFY(process.waitForFinished(5000));
    }
}
#endif

void tst_QFile::text()
{
    // dosfile.txt is a binary CRLF file
    QFile file(m_dosFile);
    QVERIFY(file.open(QFile::Text | QFile::ReadOnly));
    QCOMPARE(file.readLine(),
            QByteArray("/dev/system/root     /                    reiserfs   acl,user_xattr        1 1\n"));
    QCOMPARE(file.readLine(),
            QByteArray("/dev/sda1            /boot                ext3       acl,user_xattr        1 2\n"));
    file.ungetChar('\n');
    file.ungetChar('2');
    QCOMPARE(file.readLine().constData(), QByteArray("2\n").constData());
}

void tst_QFile::missingEndOfLine()
{
    QFile file(m_noEndOfLineFile);
    QVERIFY(file.open(QFile::ReadOnly));

    int nlines = 0;
    while (!file.atEnd()) {
        ++nlines;
        file.readLine();
    }

    QCOMPARE(nlines, 3);
}

void tst_QFile::readBlock()
{
    QFile f( m_testFile );
    f.open( QIODevice::ReadOnly );

    int length = 0;
    char p[256];
    length = f.read( p, 256 );
    f.close();
    QCOMPARE( length, 245 );
    QCOMPARE( p[59], 'D' );
    QCOMPARE( p[178], 'T' );
    QCOMPARE( p[199], 'l' );
}

void tst_QFile::getch()
{
    QFile f( m_testFile );
    f.open( QIODevice::ReadOnly );

    char c;
    int i = 0;
    while (f.getChar(&c)) {
        QCOMPARE(f.pos(), qint64(i + 1));
        if ( i == 59 )
            QCOMPARE( c, 'D' );
        ++i;
    }
    f.close();
    QCOMPARE( i, 245 );
}

void tst_QFile::ungetChar()
{
    QFile f(m_testFile);
    QVERIFY(f.open(QIODevice::ReadOnly));

    QByteArray array = f.readLine();
    QCOMPARE(array.constData(), "----------------------------------------------------------\n");
    f.ungetChar('\n');

    array = f.readLine();
    QCOMPARE(array.constData(), "\n");

    f.ungetChar('\n');
    f.ungetChar('-');
    f.ungetChar('-');

    array = f.readLine();
    QCOMPARE(array.constData(), "--\n");

    QFile::remove("genfile.txt");
    QFile out("genfile.txt");
    QVERIFY(out.open(QIODevice::ReadWrite));
    out.write("123");
    out.seek(0);
    QCOMPARE(out.readAll().constData(), "123");
    out.ungetChar('3');
    out.write("4");
    out.seek(0);
    QCOMPARE(out.readAll().constData(), "124");
    out.ungetChar('4');
    out.ungetChar('2');
    out.ungetChar('1');
    char buf[3];
    QCOMPARE(out.read(buf, sizeof(buf)), qint64(3));
    QCOMPARE(buf[0], '1');
    QCOMPARE(buf[1], '2');
    QCOMPARE(buf[2], '4');
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
QString driveLetters()
{
    wchar_t volumeName[MAX_PATH];
    wchar_t path[MAX_PATH];
    const HANDLE h = FindFirstVolumeW(volumeName, MAX_PATH);
    if (h == INVALID_HANDLE_VALUE)
        return QString();
    QString result;
    do {
        if (GetVolumePathNamesForVolumeNameW(volumeName, path, MAX_PATH, NULL)) {
            if (path[1] == L':')
                result.append(QChar(path[0]));
        }
    } while (FindNextVolumeW(h, volumeName, MAX_PATH));
    FindVolumeClose(h);
    return result;
}

static inline QChar invalidDriveLetter()
{
    const QString drives = driveLetters().toLower();
    for (char c = 'a'; c <= 'z'; ++c)
        if (!drives.contains(QLatin1Char(c)))
            return QLatin1Char(c);
    Q_ASSERT(false); // All drive letters used?!
    return QChar();
}

#endif // Q_OS_WIN

void tst_QFile::invalidFile_data()
{
    QTest::addColumn<QString>("fileName");
#if !defined(Q_OS_WIN)
    QTest::newRow( "x11" ) << QString( "qwe//" );
#else
#if !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    QTest::newRow( "colon2" ) << invalidDriveLetter() + QString::fromLatin1(":ail:invalid");
#endif
    QTest::newRow( "colon3" ) << QString( ":failinvalid" );
    QTest::newRow( "forwardslash" ) << QString( "fail/invalid" );
    QTest::newRow( "asterisk" ) << QString( "fail*invalid" );
    QTest::newRow( "questionmark" ) << QString( "fail?invalid" );
    QTest::newRow( "quote" ) << QString( "fail\"invalid" );
    QTest::newRow( "lt" ) << QString( "fail<invalid" );
    QTest::newRow( "gt" ) << QString( "fail>invalid" );
    QTest::newRow( "pipe" ) << QString( "fail|invalid" );
#endif
}

void tst_QFile::invalidFile()
{
    QFETCH( QString, fileName );
    QFile f( fileName );
    QVERIFY2( !f.open( QIODevice::ReadWrite ), qPrintable(fileName) );
}

void tst_QFile::createFile()
{
    if ( QFile::exists( "createme.txt" ) )
        QFile::remove( "createme.txt" );
    QVERIFY( !QFile::exists( "createme.txt" ) );

    QFile f( "createme.txt" );
    QVERIFY( f.open( QIODevice::WriteOnly ) );
    f.close();
    QVERIFY( QFile::exists( "createme.txt" ) );
}

void tst_QFile::append()
{
    const QString name("appendme.txt");
    if (QFile::exists(name))
        QFile::remove(name);
    QVERIFY(!QFile::exists(name));

    QFile f(name);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.putChar('a');
    f.close();

    QVERIFY(f.open(QIODevice::Append));
    QVERIFY(f.pos() == 1);
    f.putChar('a');
    f.close();
    QCOMPARE(int(f.size()), 2);
}

void tst_QFile::permissions_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<uint>("perms");
    QTest::addColumn<bool>("expected");
    QTest::addColumn<bool>("create");

    QTest::newRow("data0") << QCoreApplication::instance()->applicationFilePath() << uint(QFile::ExeUser) << true << false;
    QTest::newRow("data1") << m_testSourceFile << uint(QFile::ReadUser) << true << false;
    QTest::newRow("readonly") << QString::fromLatin1("readonlyfile") << uint(QFile::WriteUser) << false << false;
#ifndef Q_OS_WINCE
    QTest::newRow("longfile") << QString::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                                    "longFileNamelongFileNamelongFileNamelongFileName"
                                                    "longFileNamelongFileNamelongFileNamelongFileName"
                                                    "longFileNamelongFileNamelongFileNamelongFileName"
                                                    "longFileNamelongFileNamelongFileNamelongFileName.txt") << uint(QFile::ReadUser) << true << true;
#endif
    QTest::newRow("resource1") << ":/tst_qfileinfo/resources/file1.ext1" << uint(QFile::ReadUser) << true << false;
    QTest::newRow("resource2") << ":/tst_qfileinfo/resources/file1.ext1" << uint(QFile::WriteUser) << false << false;
    QTest::newRow("resource3") << ":/tst_qfileinfo/resources/file1.ext1" << uint(QFile::ExeUser) << false << false;
}

void tst_QFile::permissions()
{
    QFETCH(QString, file);
    QFETCH(uint, perms);
    QFETCH(bool, expected);
    QFETCH(bool, create);
    if (create) {
        QFile fc(file);
        QVERIFY(fc.open(QFile::WriteOnly));
        QVERIFY(fc.write("hello\n"));
        fc.close();
    }

    QFile f(file);
    QFile::Permissions memberResult = f.permissions() & perms;
    QFile::Permissions staticResult = QFile::permissions(file) & perms;

    if (create) {
        QFile::remove(file);
    }

#ifdef Q_OS_WIN
    if (qt_ntfs_permission_lookup)
        QEXPECT_FAIL("readonly", "QTBUG-25630", Abort);
#endif
#ifdef Q_OS_UNIX
    if (strcmp(QTest::currentDataTag(), "readonly") == 0) {
        // in case accidentally run as root
        if (::getuid() == 0)
            QSKIP("Running this test as root doesn't make sense");
    }
#endif
    QCOMPARE((memberResult == QFile::Permissions(perms)), expected);
    QCOMPARE((staticResult == QFile::Permissions(perms)), expected);
}

#ifdef Q_OS_WIN
void tst_QFile::permissionsNtfs_data()
{
    permissions_data();
}

void tst_QFile::permissionsNtfs()
{
    QScopedValueRollback<int> ntfsMode(qt_ntfs_permission_lookup);
    qt_ntfs_permission_lookup++;
    permissions();
}
#endif

void tst_QFile::setPermissions()
{
    if ( QFile::exists( "createme.txt" ) )
        QFile::remove( "createme.txt" );
    QVERIFY( !QFile::exists( "createme.txt" ) );

    QFile f("createme.txt");
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.putChar('a');
    f.close();

    QFile::Permissions perms(QFile::WriteUser | QFile::ReadUser);
    QVERIFY(f.setPermissions(perms));
    QVERIFY((f.permissions() & perms) == perms);

}

void tst_QFile::copy()
{
    QFile::setPermissions("tst_qfile_copy.cpp", QFile::WriteUser);
    QFile::remove("tst_qfile_copy.cpp");
    QFile::remove("test2");
    QVERIFY(QFile::copy(m_testSourceFile, "tst_qfile_copy.cpp"));
    QFile in1(m_testSourceFile), in2("tst_qfile_copy.cpp");
    QVERIFY(in1.open(QFile::ReadOnly));
    QVERIFY(in2.open(QFile::ReadOnly));
    QByteArray data1 = in1.readAll(), data2 = in2.readAll();
    QCOMPARE(data1, data2);
    QFile::remove( "main_copy.cpp" );

    QFile::copy(QDir::currentPath(), QDir::currentPath() + QLatin1String("/test2"));
}

void tst_QFile::copyAfterFail()
{
    QFile file1("file-to-be-copied.txt");
    QFile file2("existing-file.txt");

    QVERIFY(file1.open(QIODevice::ReadWrite) && "(test-precondition)");
    QVERIFY(file2.open(QIODevice::ReadWrite) && "(test-precondition)");
    file2.close();
    QVERIFY(!QFile::exists("copied-file-1.txt") && "(test-precondition)");
    QVERIFY(!QFile::exists("copied-file-2.txt") && "(test-precondition)");

    QVERIFY(!file1.copy("existing-file.txt"));
    QCOMPARE(file1.error(), QFile::CopyError);

    QVERIFY(file1.copy("copied-file-1.txt"));
    QVERIFY(!file1.isOpen());
    QCOMPARE(file1.error(), QFile::NoError);

    QVERIFY(!file1.copy("existing-file.txt"));
    QCOMPARE(file1.error(), QFile::CopyError);

    QVERIFY(file1.copy("copied-file-2.txt"));
    QVERIFY(!file1.isOpen());
    QCOMPARE(file1.error(), QFile::NoError);

    QVERIFY(QFile::exists("copied-file-1.txt"));
    QVERIFY(QFile::exists("copied-file-2.txt"));
}

void tst_QFile::copyRemovesTemporaryFile() const
{
    const QString newName(QLatin1String("copyRemovesTemporaryFile"));
    QVERIFY(QFile::copy(m_forCopyingFile, newName));

    QVERIFY(!QFile::exists(QStringLiteral("qt_temp.XXXXXX")));
}

void tst_QFile::copyShouldntOverwrite()
{
    // Copy should not overwrite existing files.
    QFile::remove("tst_qfile.cpy");
    QFile file(m_testSourceFile);
    QVERIFY(file.copy("tst_qfile.cpy"));

    bool ok = QFile::setPermissions("tst_qfile.cpy", QFile::WriteOther);
    QVERIFY(ok);
    QVERIFY(!file.copy("tst_qfile.cpy"));
}

void tst_QFile::copyFallback()
{
    // Using a resource file to trigger QFile::copy's fallback handling
    QFile file(":/copy-fallback.qrc");
    QFile::remove("file-copy-destination.txt");

    QVERIFY2(file.exists(), "test precondition");
    QVERIFY2(!QFile::exists("file-copy-destination.txt"), "test precondition");

    // Fallback copy of closed file.
    QVERIFY(file.copy("file-copy-destination.txt"));
    QVERIFY(QFile::exists("file-copy-destination.txt"));
    QVERIFY(!file.isOpen());

#ifdef Q_OS_WINCE
    // Need to reset permissions on Windows to be able to delete
    QVERIFY(QFile::setPermissions("file-copy-destination.txt",
            QFile::WriteOther));
#else
     // Need to reset permissions on Windows to be able to delete
    QVERIFY(QFile::setPermissions("file-copy-destination.txt",
           QFile::ReadOwner | QFile::WriteOwner));
#endif
    QVERIFY(QFile::remove("file-copy-destination.txt"));

    // Fallback copy of open file.
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.copy("file-copy-destination.txt"));
    QVERIFY(QFile::exists("file-copy-destination.txt"));
    QVERIFY(!file.isOpen());

    file.close();
    QFile::setPermissions("file-copy-destination.txt",
            QFile::ReadOwner | QFile::WriteOwner);
}

#ifdef Q_OS_WIN
#include <objbase.h>
#ifndef Q_OS_WINPHONE
#include <shlobj.h>
#endif
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
static QString getWorkingDirectoryForLink(const QString &linkFileName)
{
    bool neededCoInit = false;
    QString ret;

    IShellLink *psl;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
    if (hres == CO_E_NOTINITIALIZED) { // COM was not initialized
        neededCoInit = true;
        CoInitialize(NULL);
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
    }

    if (SUCCEEDED(hres)) {    // Get pointer to the IPersistFile interface.
        IPersistFile *ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
        if (SUCCEEDED(hres))  {
            hres = ppf->Load((LPOLESTR)linkFileName.utf16(), STGM_READ);
            //The original path of the link is retrieved. If the file/folder
            //was moved, the return value still have the old path.
            if(SUCCEEDED(hres)) {
                wchar_t szGotPath[MAX_PATH];
                if (psl->GetWorkingDirectory(szGotPath, MAX_PATH) == NOERROR)
                    ret = QString::fromWCharArray(szGotPath);
            }
            ppf->Release();
        }
        psl->Release();
    }

    if (neededCoInit) {
        CoUninitialize();
    }

    return ret;
}
#endif

void tst_QFile::link()
{
    QFile::remove("myLink.lnk");

    QFileInfo info1(m_testSourceFile);
    QString referenceTarget = QDir::cleanPath(info1.absoluteFilePath());

    QVERIFY(QFile::link(m_testSourceFile, "myLink.lnk"));

    QFileInfo info2("myLink.lnk");
    QVERIFY(info2.isSymLink());
    QCOMPARE(info2.symLinkTarget(), referenceTarget);

    QFile link("myLink.lnk");
    QVERIFY(link.open(QIODevice::ReadOnly));
    QCOMPARE(link.symLinkTarget(), referenceTarget);
    link.close();

    QCOMPARE(QFile::symLinkTarget("myLink.lnk"), referenceTarget);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    QString wd = getWorkingDirectoryForLink(info2.absoluteFilePath());
    QCOMPARE(QDir::fromNativeSeparators(wd), QDir::cleanPath(info1.absolutePath()));
#endif
}

void tst_QFile::linkToDir()
{
    QFile::remove("myLinkToDir.lnk");
    QDir dir;
    dir.mkdir("myDir");
    QFileInfo info1("myDir");
    QVERIFY(QFile::link("myDir", "myLinkToDir.lnk"));
    QFileInfo info2("myLinkToDir.lnk");
#if !(defined Q_OS_HPUX && defined(__ia64))
    // absurd HP-UX filesystem bug on gravlaks - checking if a symlink
    // resolves or not alters the file system to make the broken symlink
    // later fail...
    QVERIFY(info2.isSymLink());
#endif
    QCOMPARE(info2.symLinkTarget(), info1.absoluteFilePath());
    QVERIFY(QFile::remove(info2.absoluteFilePath()));
}

void tst_QFile::absolutePathLinkToRelativePath()
{
    QFile::remove("myDir/test.txt");
    QFile::remove("myDir/myLink.lnk");
    QDir dir;
    dir.mkdir("myDir");
    QFile("myDir/test.txt").open(QFile::WriteOnly);

#ifdef Q_OS_WIN
    QVERIFY(QFile::link("test.txt", "myDir/myLink.lnk"));
#else
    QVERIFY(QFile::link("myDir/test.txt", "myDir/myLink.lnk"));
#endif
    QEXPECT_FAIL("", "Symlinking using relative paths is currently different on Windows and Unix", Continue);
    QCOMPARE(QFileInfo(QFile(QFileInfo("myDir/myLink.lnk").absoluteFilePath()).symLinkTarget()).absoluteFilePath(),
             QFileInfo("myDir/test.txt").absoluteFilePath());
}

void tst_QFile::readBrokenLink()
{
    QFile::remove("myLink2.lnk");
    QFileInfo info1("file12");
    QVERIFY(QFile::link("file12", "myLink2.lnk"));
    QFileInfo info2("myLink2.lnk");
    QVERIFY(info2.isSymLink());
    QCOMPARE(info2.symLinkTarget(), info1.absoluteFilePath());
    QVERIFY(QFile::remove(info2.absoluteFilePath()));
    QVERIFY(QFile::link("ole/..", "myLink2.lnk"));
    QCOMPARE(QFileInfo("myLink2.lnk").symLinkTarget(), QDir::currentPath());
}

void tst_QFile::readTextFile_data()
{
    QTest::addColumn<QByteArray>("in");
    QTest::addColumn<QByteArray>("out");

    QTest::newRow("empty") << QByteArray() << QByteArray();
    QTest::newRow("a") << QByteArray("a") << QByteArray("a");
    QTest::newRow("a\\rb") << QByteArray("a\rb") << QByteArray("ab");
    QTest::newRow("\\n") << QByteArray("\n") << QByteArray("\n");
    QTest::newRow("\\r\\n") << QByteArray("\r\n") << QByteArray("\n");
    QTest::newRow("\\r") << QByteArray("\r") << QByteArray();
    QTest::newRow("twolines") << QByteArray("Hello\r\nWorld\r\n") << QByteArray("Hello\nWorld\n");
    QTest::newRow("twolines no endline") << QByteArray("Hello\r\nWorld") << QByteArray("Hello\nWorld");
}

void tst_QFile::readTextFile()
{
    QFETCH(QByteArray, in);
    QFETCH(QByteArray, out);

    QFile winfile("winfile.txt");
    QVERIFY(winfile.open(QFile::WriteOnly | QFile::Truncate));
    winfile.write(in);
    winfile.close();

    QVERIFY(winfile.open(QFile::ReadOnly));
    QCOMPARE(winfile.readAll(), in);
    winfile.close();

    QVERIFY(winfile.open(QFile::ReadOnly | QFile::Text));
    QCOMPARE(winfile.readAll(), out);
}

void tst_QFile::readTextFile2()
{
    {
        QFile file(m_testLogFile);
        QVERIFY(file.open(QIODevice::ReadOnly));
        file.read(4097);
    }

    {
        QFile file(m_testLogFile);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        file.read(4097);
    }
}

void tst_QFile::writeTextFile_data()
{
    QTest::addColumn<QByteArray>("in");

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("a") << QByteArray("a");
    QTest::newRow("a\\rb") << QByteArray("a\rb");
    QTest::newRow("\\n") << QByteArray("\n");
    QTest::newRow("\\r\\n") << QByteArray("\r\n");
    QTest::newRow("\\r") << QByteArray("\r");
    QTest::newRow("twolines crlf") << QByteArray("Hello\r\nWorld\r\n");
    QTest::newRow("twolines crlf no endline") << QByteArray("Hello\r\nWorld");
    QTest::newRow("twolines lf") << QByteArray("Hello\nWorld\n");
    QTest::newRow("twolines lf no endline") << QByteArray("Hello\nWorld");
    QTest::newRow("mixed") << QByteArray("this\nis\r\na\nmixed\r\nfile\n");
}

void tst_QFile::writeTextFile()
{
    QFETCH(QByteArray, in);

    QFile file("textfile.txt");
    QVERIFY(file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text));
    QByteArray out = in;
#ifdef Q_OS_WIN
    out.replace('\n', "\r\n");
#endif
    QCOMPARE(file.write(in), qlonglong(in.size()));
    file.close();

    file.open(QFile::ReadOnly);
    QCOMPARE(file.readAll(), out);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
void tst_QFile::largeUncFileSupport()
{
    qint64 size = Q_INT64_C(8589934592);
    qint64 dataOffset = Q_INT64_C(8589914592);
    QByteArray knownData("LargeFile content at offset 8589914592");
    QString largeFile("//" + QtNetworkSettings::winServerName() + "/testsharelargefile/file.bin");

    {
        // 1) Native file handling.
        QFile file(largeFile);
        QCOMPARE(file.size(), size);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.size(), size);
        QVERIFY(file.seek(dataOffset));
        QCOMPARE(file.read(knownData.size()), knownData);
    }
    {
        // 2) stdlib file handling.
        QFile file;
        FILE *fh = fopen(QFile::encodeName(largeFile).data(), "rb");
        QVERIFY(file.open(fh, QIODevice::ReadOnly));
        QCOMPARE(file.size(), size);
        QVERIFY(file.seek(dataOffset));
        QCOMPARE(file.read(knownData.size()), knownData);
        fclose(fh);
    }
    {
        // 3) stdio file handling.
        QFile file;
        FILE *fh = fopen(QFile::encodeName(largeFile).data(), "rb");
        int fd = int(_fileno(fh));
        QVERIFY(file.open(fd, QIODevice::ReadOnly));
        QCOMPARE(file.size(), size);
        QVERIFY(file.seek(dataOffset));
        QCOMPARE(file.read(knownData.size()), knownData);
        fclose(fh);
    }
}
#endif

void tst_QFile::flush()
{
    QString fileName("stdfile.txt");

    QFile::remove(fileName);

    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly));
        QCOMPARE(file.write("abc", 3),qint64(3));
    }

    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly | QFile::Append));
        QCOMPARE(file.pos(), qlonglong(3));
        QCOMPARE(file.write("def", 3), qlonglong(3));
        QCOMPARE(file.pos(), qlonglong(6));
    }

    {
        QFile file("stdfile.txt");
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("abcdef"));
    }
}

void tst_QFile::bufferedRead()
{
    QFile::remove("stdfile.txt");

    QFile file("stdfile.txt");
    QVERIFY(file.open(QFile::WriteOnly));
    file.write("abcdef");
    file.close();

#if defined(Q_OS_WINCE)
    FILE *stdFile = fopen((QCoreApplication::applicationDirPath() + "/stdfile.txt").toLatin1() , "r");
#else
    FILE *stdFile = fopen("stdfile.txt", "r");
#endif
    QVERIFY(stdFile);
    char c;
    QCOMPARE(int(fread(&c, 1, 1, stdFile)), 1);
    QCOMPARE(c, 'a');
    QCOMPARE(int(ftell(stdFile)), 1);

    {
        QFile file;
        QVERIFY(file.open(stdFile, QFile::ReadOnly));
        QCOMPARE(file.pos(), qlonglong(1));
        QCOMPARE(file.read(&c, 1), qlonglong(1));
        QCOMPARE(c, 'b');
        QCOMPARE(file.pos(), qlonglong(2));
    }

    fclose(stdFile);
}

#ifdef Q_OS_UNIX
void tst_QFile::isSequential()
{
    QFile zero("/dev/null");
    QVERIFY(zero.open(QFile::ReadOnly));
    QVERIFY(zero.isSequential());
}
#endif

void tst_QFile::encodeName()
{
    QCOMPARE(QFile::encodeName(QString::null), QByteArray());
}

void tst_QFile::truncate()
{
    for (int i = 0; i < 2; ++i) {
        QFile file("truncate.txt");
        QVERIFY(file.open(QFile::WriteOnly));
        file.write(QByteArray(200, '@'));
        file.close();

        QVERIFY(file.open((i ? QFile::WriteOnly : QFile::ReadWrite) | QFile::Truncate));
        file.write(QByteArray(100, '$'));
        file.close();

        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray(100, '$'));
    }
}

void tst_QFile::seekToPos()
{
    {
        QFile file("seekToPos.txt");
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("a\r\nb\r\nc\r\n");
        file.flush();
    }

    QFile file("seekToPos.txt");
    QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
    file.seek(1);
    char c;
    QVERIFY(file.getChar(&c));
    QCOMPARE(c, '\n');

    QCOMPARE(file.pos(), qint64(3));
    file.seek(file.pos());
    QCOMPARE(file.pos(), qint64(3));

    file.seek(1);
    file.seek(file.pos());
    QCOMPARE(file.pos(), qint64(1));

}

void tst_QFile::seekAfterEndOfFile()
{
    QLatin1String filename("seekAfterEof.dat");
    QFile::remove(filename);
    {
        QFile file(filename);
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("abcd");
        QCOMPARE(file.size(), qint64(4));
        file.seek(8);
        file.write("ijkl");
        QCOMPARE(file.size(), qint64(12));
        file.seek(4);
        file.write("efgh");
        QCOMPARE(file.size(), qint64(12));
        file.seek(16);
        file.write("----");
        QCOMPARE(file.size(), qint64(20));
        file.flush();
    }

    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray contents = file.readAll();
    QCOMPARE(contents.left(12), QByteArray("abcdefghijkl", 12));
    //bytes 12-15 are uninitialised so we don't care what they read as.
    QCOMPARE(contents.mid(16), QByteArray("----", 4));
    file.close();
}

void tst_QFile::FILEReadWrite()
{
    // Tests modifying a file. First creates it then reads in 4 bytes and then overwrites these
    // 4 bytes with new values. At the end check to see the file contains the new values.

    QFile::remove("FILEReadWrite.txt");

    // create test file
    {
        QFile f("FILEReadWrite.txt");
        QVERIFY(f.open(QFile::WriteOnly));
        QDataStream ds(&f);
        qint8 c = 0;
        ds << c;
        c = 1;
        ds << c;
        c = 2;
        ds << c;
        c = 3;
        ds << c;
        c = 4;
        ds << c;
        c = 5;
        ds << c;
        c = 6;
        ds << c;
        c = 7;
        ds << c;
        c = 8;
        ds << c;
        c = 9;
        ds << c;
        c = 10;
        ds << c;
        c = 11;
        ds << c;
        f.close();
    }

#ifdef Q_OS_WINCE
    FILE *fp = fopen(qPrintable(QCoreApplication::applicationDirPath() + "\\FILEReadWrite.txt"), "r+b");
#else
    FILE *fp = fopen("FILEReadWrite.txt", "r+b");
#endif
    QVERIFY(fp);
    QFile file;
    QVERIFY(file.open(fp, QFile::ReadWrite));
    QDataStream sfile(&file) ;

    qint8 var1,var2,var3,var4;
    while (!sfile.atEnd())
    {
        qint64 base = file.pos();

        QCOMPARE(file.pos(), base + 0);
        sfile >> var1;
        QCOMPARE(file.pos(), base + 1);
        file.flush(); // flushing should not change the base
        QCOMPARE(file.pos(), base + 1);
        sfile >> var2;
        QCOMPARE(file.pos(), base + 2);
        sfile >> var3;
        QCOMPARE(file.pos(), base + 3);
        sfile >> var4;
        QCOMPARE(file.pos(), base + 4);
        file.seek(file.pos() - 4) ;   // Move it back 4, for we are going to write new values based on old ones
        QCOMPARE(file.pos(), base + 0);
        sfile << qint8(var1 + 5);
        QCOMPARE(file.pos(), base + 1);
        sfile << qint8(var2 + 5);
        QCOMPARE(file.pos(), base + 2);
        sfile << qint8(var3 + 5);
        QCOMPARE(file.pos(), base + 3);
        sfile << qint8(var4 + 5);
        QCOMPARE(file.pos(), base + 4);

    }
    file.close();
    fclose(fp);

    // check modified file
    {
        QFile f("FILEReadWrite.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        QDataStream ds(&f);
        qint8 c = 0;
        ds >> c;
        QCOMPARE(c, (qint8)5);
        ds >> c;
        QCOMPARE(c, (qint8)6);
        ds >> c;
        QCOMPARE(c, (qint8)7);
        ds >> c;
        QCOMPARE(c, (qint8)8);
        ds >> c;
        QCOMPARE(c, (qint8)9);
        ds >> c;
        QCOMPARE(c, (qint8)10);
        ds >> c;
        QCOMPARE(c, (qint8)11);
        ds >> c;
        QCOMPARE(c, (qint8)12);
        ds >> c;
        QCOMPARE(c, (qint8)13);
        ds >> c;
        QCOMPARE(c, (qint8)14);
        ds >> c;
        QCOMPARE(c, (qint8)15);
        ds >> c;
        QCOMPARE(c, (qint8)16);
        f.close();
    }
}


/*
#include <qglobal.h>
#define BUFFSIZE 1
#define FILESIZE   0x10000000f
void tst_QFile::largeFileSupport()
{
#ifdef Q_OS_SOLARIS
    QSKIP("Solaris does not support statfs");
#else
    qlonglong sizeNeeded = 2147483647;
    sizeNeeded *= 2;
    sizeNeeded += 1024;
    qlonglong freespace = qlonglong(0);
#ifdef Q_OS_WIN
    _ULARGE_INTEGER free;
    if (::GetDiskFreeSpaceEx((wchar_t*)QDir::currentPath().utf16(), &free, 0, 0))
        freespace = free.QuadPart;
    if (freespace != 0) {
#elif defined(Q_OS_IRIX)
    struct statfs info;
    if (statfs(QDir::currentPath().local8Bit(), &info, sizeof(struct statfs), 0) == 0) {
        freespace = qlonglong(info.f_bfree * info.f_bsize);
#else
    struct statfs info;
    if (statfs(const_cast<char *>(QDir::currentPath().toLocal8Bit().constData()), &info) == 0) {
        freespace = qlonglong(info.f_bavail * info.f_bsize);
#endif
        if (freespace > sizeNeeded) {
            QFile bigFile("bigfile");
            if (bigFile.open(QFile::ReadWrite)) {
                char c[BUFFSIZE] = {'a'};
                QVERIFY(bigFile.write(c, BUFFSIZE) == BUFFSIZE);
                qlonglong oldPos = bigFile.pos();
                QVERIFY(bigFile.resize(sizeNeeded));
                QCOMPARE(oldPos, bigFile.pos());
                QVERIFY(bigFile.seek(sizeNeeded - BUFFSIZE));
                QVERIFY(bigFile.write(c, BUFFSIZE) == BUFFSIZE);

                bigFile.close();
                if (bigFile.open(QFile::ReadOnly)) {
                    QVERIFY(bigFile.read(c, BUFFSIZE) == BUFFSIZE);
                    int i = 0;
                    for (i=0; i<BUFFSIZE; i++)
                        QCOMPARE(c[i], 'a');
                    QVERIFY(bigFile.seek(sizeNeeded - BUFFSIZE));
                    QVERIFY(bigFile.read(c, BUFFSIZE) == BUFFSIZE);
                    for (i=0; i<BUFFSIZE; i++)
                        QCOMPARE(c[i], 'a');
                    bigFile.close();
                    QVERIFY(bigFile.remove());
                } else {
                    QVERIFY(bigFile.remove());
                    QFAIL("Could not reopen file");
                }
            } else {
                QFAIL("Could not open file");
            }
        } else {
            QSKIP("Not enough space to run test");
        }
    } else {
        QFAIL("Could not determin disk space");
    }
#endif
}
*/

void tst_QFile::i18nFileName_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow( "01" ) << QString::fromUtf8("xxxxxxx.txt");
}

void tst_QFile::i18nFileName()
{
     QFETCH(QString, fileName);
     if (QFile::exists(fileName)) {
         QVERIFY(QFile::remove(fileName));
     }
     {
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly | QFile::Text));
        QTextStream ts(&file);
        ts.setCodec("UTF-8");
        ts << fileName << endl;
     }
     {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        QTextStream ts(&file);
        ts.setCodec("UTF-8");
        QString line = ts.readLine();
        QCOMPARE(line, fileName);
     }
}


void tst_QFile::longFileName_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow( "16 chars" ) << QString::fromLatin1("longFileName.txt");
    QTest::newRow( "52 chars" ) << QString::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName.txt");
    QTest::newRow( "148 chars" ) << QString::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName.txt");
    QTest::newRow( "244 chars" ) << QString::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName.txt");
    QTest::newRow( "244 chars to absolutepath" ) << QFileInfo(QString::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName.txt")).absoluteFilePath();
  /* needs to be put on a windows 2000 > test machine
  QTest::newRow( "244 chars on UNC" ) <<  QString::fromLatin1("//arsia/D/troll/tmp/longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName.txt");*/
}

void tst_QFile::longFileName()
{
    QFETCH(QString, fileName);
    if (QFile::exists(fileName)) {
        QVERIFY(QFile::remove(fileName));
    }
    {
        QFile file(fileName);
#if defined(Q_OS_WINCE)
        QEXPECT_FAIL("244 chars", "Full pathname must be less than 260 chars", Abort);
        QEXPECT_FAIL("244 chars to absolutepath", "Full pathname must be less than 260 chars", Abort);
#endif
        QVERIFY(file.open(QFile::WriteOnly | QFile::Text));
        QTextStream ts(&file);
        ts << fileName << endl;
    }
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        QTextStream ts(&file);
        QString line = ts.readLine();
        QCOMPARE(line, fileName);
    }
    QString newName = fileName + QLatin1String("1");
    {
        QVERIFY(QFile::copy(fileName, newName));
        QFile file(newName);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        QTextStream ts(&file);
        QString line = ts.readLine();
        QCOMPARE(line, fileName);

    }
    QVERIFY(QFile::remove(newName));
    {
        QVERIFY(QFile::rename(fileName, newName));
        QFile file(newName);
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        QTextStream ts(&file);
        QString line = ts.readLine();
        QCOMPARE(line, fileName);
    }
    QVERIFY(QFile::exists(newName));
}

#ifdef QT_BUILD_INTERNAL
class MyEngine : public QAbstractFileEngine
{
public:
    MyEngine(int n) { number = n; }
    virtual ~MyEngine() {}

    void setFileName(const QString &) {}
    bool open(QIODevice::OpenMode) { return false; }
    bool close() { return false; }
    bool flush() { return false; }
    qint64 size() const { return 123 + number; }
    qint64 at() const { return -1; }
    bool seek(qint64) { return false; }
    bool isSequential() const { return false; }
    qint64 read(char *, qint64) { return -1; }
    qint64 write(const char *, qint64) { return -1; }
    bool remove() { return false; }
    bool copy(const QString &) { return false; }
    bool rename(const QString &) { return false; }
    bool link(const QString &) { return false; }
    bool mkdir(const QString &, bool) const { return false; }
    bool rmdir(const QString &, bool) const { return false; }
    bool setSize(qint64) { return false; }
    QStringList entryList(QDir::Filters, const QStringList &) const { return QStringList(); }
    bool caseSensitive() const { return false; }
    bool isRelativePath() const { return false; }
    FileFlags fileFlags(FileFlags) const { return 0; }
    bool chmod(uint) { return false; }
    QString fileName(FileName) const { return name; }
    uint ownerId(FileOwner) const { return 0; }
    QString owner(FileOwner) const { return QString(); }
    QDateTime fileTime(FileTime) const { return QDateTime(); }

private:
    int number;
    QString name;
};

class MyHandler : public QAbstractFileEngineHandler
{
public:
    inline QAbstractFileEngine *create(const QString &) const
    {
        return new MyEngine(1);
    }
};

class MyHandler2 : public QAbstractFileEngineHandler
{
public:
    inline QAbstractFileEngine *create(const QString &) const
    {
        return new MyEngine(2);
    }
};
#endif

void tst_QFile::fileEngineHandler()
{
    // A file that does not exist has a size of 0.
    QFile::remove("ole.bull");
    QFile file("ole.bull");
    QCOMPARE(file.size(), qint64(0));

#ifdef QT_BUILD_INTERNAL
    // Instantiating our handler will enable the new engine.
    MyHandler handler;
    file.setFileName("ole.bull");
    QCOMPARE(file.size(), qint64(124));

    // A new, identical handler should take preference over the last one.
    MyHandler2 handler2;
    file.setFileName("ole.bull");
    QCOMPARE(file.size(), qint64(125));
#endif
}

#ifdef QT_BUILD_INTERNAL
class MyRecursiveHandler : public QAbstractFileEngineHandler
{
public:
    inline QAbstractFileEngine *create(const QString &fileName) const
    {
        if (fileName.startsWith(":!")) {
            QDir dir;

            QString realFile = QFINDTESTDATA(fileName.mid(2));
            if (dir.exists(realFile))
                return new QFSFileEngine(realFile);
        }
        return 0;
    }
};
#endif

#ifdef QT_BUILD_INTERNAL
void tst_QFile::useQFileInAFileHandler()
{
    // This test should not dead-lock
    MyRecursiveHandler handler;
    QFile file(":!tst_qfile.cpp");
    QVERIFY(file.exists());
}
#endif

void tst_QFile::getCharFF()
{
    QFile file("file.txt");
    file.open(QFile::ReadWrite);
    file.write("\xff\xff\xff");
    file.flush();
    file.seek(0);

    char c;
    QVERIFY(file.getChar(&c));
    QVERIFY(file.getChar(&c));
    QVERIFY(file.getChar(&c));
}

void tst_QFile::remove_and_exists()
{
    QFile::remove("tull_i_grunn.txt");
    QFile f("tull_i_grunn.txt");

    QVERIFY(!f.exists());

    bool opened = f.open(QIODevice::WriteOnly);
    QVERIFY(opened);

    f.write(QString("testing that remove/exists work...").toLatin1());
    f.close();

    QVERIFY(f.exists());

    f.remove();
    QVERIFY(!f.exists());
}

void tst_QFile::removeOpenFile()
{
    {
        // remove an opened, write-only file
        QFile::remove("remove_unclosed.txt");
        QFile f("remove_unclosed.txt");

        QVERIFY(!f.exists());
        bool opened = f.open(QIODevice::WriteOnly);
        QVERIFY(opened);
        f.write(QString("testing that remove closes the file first...").toLatin1());

        bool removed = f.remove(); // remove should both close and remove the file
        QVERIFY(removed);
        QVERIFY(!f.isOpen());
        QVERIFY(!f.exists());
        QVERIFY(f.error() == QFile::NoError);
    }

    {
        // remove an opened, read-only file
        QFile::remove("remove_unclosed.txt");

        // first, write a file that we can remove
        {
            QFile f("remove_unclosed.txt");
            QVERIFY(!f.exists());
            bool opened = f.open(QIODevice::WriteOnly);
            QVERIFY(opened);
            f.write(QString("testing that remove closes the file first...").toLatin1());
            f.close();
        }

        QFile f("remove_unclosed.txt");
        bool opened = f.open(QIODevice::ReadOnly);
        QVERIFY(opened);
        f.readAll();
        // this used to only fail on FreeBSD (and Mac OS X)
        QVERIFY(f.flush());
        bool removed = f.remove(); // remove should both close and remove the file
        QVERIFY(removed);
        QVERIFY(!f.isOpen());
        QVERIFY(!f.exists());
        QVERIFY(f.error() == QFile::NoError);
    }
}

void tst_QFile::fullDisk()
{
    QFile file("/dev/full");
    if (!file.exists())
        QSKIP("/dev/full doesn't exist on this system");

    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("foobar", 6);

    QVERIFY(!file.flush());
    QCOMPARE(file.error(), QFile::ResourceError);
    QVERIFY(!file.flush());
    QCOMPARE(file.error(), QFile::ResourceError);

    char c = 0;
    file.write(&c, 0);
    QVERIFY(!file.flush());
    QCOMPARE(file.error(), QFile::ResourceError);
    QCOMPARE(file.write(&c, 1), qint64(1));
    QVERIFY(!file.flush());
    QCOMPARE(file.error(), QFile::ResourceError);

    file.close();
    QVERIFY(!file.isOpen());
    QCOMPARE(file.error(), QFile::ResourceError);

    file.open(QIODevice::WriteOnly);
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(file.flush()); // Shouldn't inherit write buffer
    file.close();
    QCOMPARE(file.error(), QFile::NoError);

    // try again without flush:
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("foobar", 6);
    file.close();
    QVERIFY(file.error() != QFile::NoError);
}

void tst_QFile::writeLargeDataBlock_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("type");

    QTest::newRow("localfile-QFile")  << "./largeblockfile.txt" << (int)OpenQFile;
    QTest::newRow("localfile-Fd")     << "./largeblockfile.txt" << (int)OpenFd;
    QTest::newRow("localfile-Stream") << "./largeblockfile.txt" << (int)OpenStream;

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT) && !defined(QT_NO_NETWORK)
    // Some semi-randomness to avoid collisions.
    QTest::newRow("unc file")
        << QString("//" + QtNetworkSettings::winServerName() + "/TESTSHAREWRITABLE/largefile-%1-%2.txt")
        .arg(QHostInfo::localHostName())
        .arg(QTime::currentTime().msec()) << (int)OpenQFile;
#endif
}

static QByteArray getLargeDataBlock()
{
    static QByteArray array;

    if (array.isNull())
    {
#if defined(Q_OS_WINCE) || defined(Q_OS_VXWORKS)
        int resizeSize = 1024 * 1024; // WinCE does not have much space
#else
        int resizeSize = 64 * 1024 * 1024;
#endif
        array.resize(resizeSize);
        for (int i = 0; i < array.size(); ++i)
            array[i] = uchar(i);
    }

    return array;
}

void tst_QFile::writeLargeDataBlock()
{
    QFETCH(QString, fileName);
    QFETCH( int, type );

    QByteArray const originalData = getLargeDataBlock();

    {
        QFile file(fileName);

        QVERIFY2( openFile(file, QIODevice::WriteOnly, (FileType)type),
            qPrintable(QString("Couldn't open file for writing: [%1]").arg(fileName)) );
        qint64 fileWriteOriginalData = file.write(originalData);
        qint64 originalDataSize      = (qint64)originalData.size();
#if defined(Q_OS_WIN)
        if (fileWriteOriginalData != originalDataSize) {
            qWarning() << qPrintable(QString("Error writing a large data block to [%1]: %2")
                .arg(fileName)
                .arg(file.errorString()));
            QEXPECT_FAIL("unc file", "QTBUG-26906 writing", Abort);
        }
#endif
        QCOMPARE( fileWriteOriginalData, originalDataSize );
        QVERIFY( file.flush() );

        closeFile(file);
    }

    QByteArray readData;

    {
        QFile file(fileName);

        QVERIFY2( openFile(file, QIODevice::ReadOnly, (FileType)type),
            qPrintable(QString("Couldn't open file for reading: [%1]").arg(fileName)) );
        readData = file.readAll();

#if defined(Q_OS_WIN)
        if (readData != originalData) {
            qWarning() << qPrintable(QString("Error reading a large data block from [%1]: %2")
                .arg(fileName)
                .arg(file.errorString()));
            QEXPECT_FAIL("unc file", "QTBUG-26906 reading", Abort);
        }
#endif
        closeFile(file);
    }
    QCOMPARE( readData, originalData );
    QVERIFY( QFile::remove(fileName) );
}

void tst_QFile::readFromWriteOnlyFile()
{
    QFile file("writeonlyfile");
    QVERIFY(file.open(QFile::WriteOnly));
    char c;
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::read: WriteOnly device");
    QCOMPARE(file.read(&c, 1), qint64(-1));
}

void tst_QFile::writeToReadOnlyFile()
{
    QFile file("readonlyfile");
    QVERIFY(file.open(QFile::ReadOnly));
    char c = 0;
    QTest::ignoreMessage(QtWarningMsg, "QIODevice::write: ReadOnly device");
    QCOMPARE(file.write(&c, 1), qint64(-1));
}

#if defined(Q_OS_LINUX) || defined(Q_OS_AIX) || defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
// This platform have 0-sized virtual files
void tst_QFile::virtualFile()
{
    // test if QFile works with virtual files
    QString fname;
#if defined(Q_OS_LINUX)
    fname = "/proc/self/maps";
#elif defined(Q_OS_AIX)
    fname = QString("/proc/%1/map").arg(getpid());
#else // defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
    fname = "/proc/curproc/map";
#endif

    // consistency check
    QFileInfo fi(fname);
    QVERIFY(fi.exists());
    QVERIFY(fi.isFile());
    QCOMPARE(fi.size(), Q_INT64_C(0));

    // open the file
    QFile f(fname);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.size(), Q_INT64_C(0));
    QVERIFY(f.atEnd());

    // read data
    QByteArray data = f.read(16);
    QCOMPARE(data.size(), 16);
    QCOMPARE(f.pos(), Q_INT64_C(16));

    // line-reading
    data = f.readLine();
    QVERIFY(!data.isEmpty());

    // read all:
    data = f.readAll();
    QVERIFY(f.pos() != 0);
    QVERIFY(!data.isEmpty());

    // seeking
    QVERIFY(f.seek(1));
    QCOMPARE(f.pos(), Q_INT64_C(1));
}
#endif

void tst_QFile::textFile()
{
#if defined(Q_OS_WINCE)
    FILE *fs = ::fopen((QCoreApplication::applicationDirPath() + "/writeabletextfile").toLatin1() , "wt");
#elif defined(Q_OS_WIN)
    FILE *fs = ::fopen("writeabletextfile", "wt");
#else
    FILE *fs = ::fopen("writeabletextfile", "w");
#endif
    QFile f;
    QByteArray part1("This\nis\na\nfile\nwith\nnewlines\n");
    QByteArray part2("Add\nsome\nmore\nnewlines\n");

    QVERIFY(f.open(fs, QIODevice::WriteOnly));
    f.write(part1);
    f.write(part2);
    f.close();
    ::fclose(fs);

    QFile file("writeabletextfile");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray data = file.readAll();

    QByteArray expected = part1 + part2;
#ifdef Q_OS_WIN
    expected.replace("\n", "\015\012");
#endif
    QCOMPARE(data, expected);
    file.close();
}

static const char renameSourceFile[] = "renamefile";

void tst_QFile::rename_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("destination");
    QTest::addColumn<bool>("result");

    QTest::newRow("a -> b") << QString("a") << QString("b") << false;
    QTest::newRow("a -> .") << QString("a") << QString(".") << false;
    QTest::newRow("renamefile -> renamefile") << QString::fromLatin1(renameSourceFile) << QString::fromLatin1(renameSourceFile) << false;
    QTest::newRow("renamefile -> noreadfile") << QString::fromLatin1(renameSourceFile) << QString::fromLatin1(noReadFile) << false;
#if defined(Q_OS_UNIX)
    QTest::newRow("renamefile -> /etc/renamefile") << QString::fromLatin1(renameSourceFile) << QString("/etc/renamefile") << false;
#endif
    QTest::newRow("renamefile -> renamedfile") << QString::fromLatin1(renameSourceFile) << QString("renamedfile") << true;
    QTest::newRow("renamefile -> ..") << QString::fromLatin1(renameSourceFile) << QString("..") << false;
    QTest::newRow("renamefile -> rEnAmEfIlE") << QString::fromLatin1(renameSourceFile) << QStringLiteral("rEnAmEfIlE") << true;
}

void tst_QFile::rename()
{
    QFETCH(QString, source);
    QFETCH(QString, destination);
    QFETCH(bool, result);

    const QByteArray content = QByteArrayLiteral("testdatacontent") + QTime::currentTime().toString().toLatin1();

#if defined(Q_OS_UNIX)
    if (strcmp(QTest::currentDataTag(), "renamefile -> /etc/renamefile") == 0) {
#if !defined(Q_OS_VXWORKS)
        if (::getuid() == 0)
#endif
            QSKIP("Running this test as root doesn't make sense");
    }
#endif

    const QString sourceFileName = QString::fromLatin1(renameSourceFile);
    QFile sourceFile(sourceFileName);
    QVERIFY2(sourceFile.open(QFile::WriteOnly | QFile::Text), qPrintable(sourceFile.errorString()));
    QVERIFY2(sourceFile.write(content), qPrintable(sourceFile.errorString()));
    sourceFile.close();

    QFile file(source);
    const bool success = file.rename(destination);
    if (result) {
        QVERIFY2(success, qPrintable(file.errorString()));
        QCOMPARE(file.error(), QFile::NoError);
        // This will report the source file still existing for a rename changing the case
        // on Windows, Mac.
        if (sourceFileName.compare(destination, Qt::CaseInsensitive))
            QVERIFY(!sourceFile.exists());
        QFile destinationFile(destination);
        QVERIFY2(destinationFile.open(QFile::ReadOnly | QFile::Text), qPrintable(destinationFile.errorString()));
        QCOMPARE(destinationFile.readAll(), content);
        destinationFile.close();
    } else {
        QVERIFY(!success);
        QCOMPARE(file.error(), QFile::RenameError);
    }
}

/*!
 \since 4.5

 Some special files have QFile::atEnd() returning true, even though there is
 more data available. True for corner cases, as well as some mounts on OS X.

 Here, we reproduce that condition by having a QFile sub-class with this
 peculiar atEnd() behavior.
*/
void tst_QFile::renameWithAtEndSpecialFile() const
{
    class PeculiarAtEnd : public QFile
    {
    public:
        virtual bool atEnd() const
        {
            return true;
        }
    };

    const QString newName(QLatin1String("newName.txt"));
    /* Cleanup, so we're a bit more robust. */
    QFile::remove(newName);

    const QString originalName = QStringLiteral("forRenaming.txt");
    // Copy from source tree
    if (!QFile::exists(originalName))
        QVERIFY(QFile::copy(m_forRenamingFile, originalName));

    PeculiarAtEnd file;
    file.setFileName(originalName);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

    QVERIFY(file.rename(newName));

    file.close();
}

void tst_QFile::renameFallback()
{
    // Using a resource file both to trigger QFile::rename's fallback handling
    // and as a *read-only* source whose move should fail.
    QFile file(":/rename-fallback.qrc");
    QVERIFY(file.exists() && "(test-precondition)");
    QFile::remove("file-rename-destination.txt");

    QVERIFY(!file.rename("file-rename-destination.txt"));
    QVERIFY(!QFile::exists("file-rename-destination.txt"));
    QVERIFY(!file.isOpen());
}

void tst_QFile::renameMultiple()
{
    // create the file if it doesn't exist
    QFile file("file-to-be-renamed.txt");
    QFile file2("existing-file.txt");
    QVERIFY(file.open(QIODevice::ReadWrite) && "(test-precondition)");
    QVERIFY(file2.open(QIODevice::ReadWrite) && "(test-precondition)");

    // any stale files from previous test failures?
    QFile::remove("file-renamed-once.txt");
    QFile::remove("file-renamed-twice.txt");

    // begin testing
    QVERIFY(QFile::exists("existing-file.txt"));
    QVERIFY(!file.rename("existing-file.txt"));
    QCOMPARE(file.error(), QFile::RenameError);
    QCOMPARE(file.fileName(), QString("file-to-be-renamed.txt"));

    QVERIFY(file.rename("file-renamed-once.txt"));
    QVERIFY(!file.isOpen());
    QCOMPARE(file.fileName(), QString("file-renamed-once.txt"));

    QVERIFY(QFile::exists("existing-file.txt"));
    QVERIFY(!file.rename("existing-file.txt"));
    QCOMPARE(file.error(), QFile::RenameError);
    QCOMPARE(file.fileName(), QString("file-renamed-once.txt"));

    QVERIFY(file.rename("file-renamed-twice.txt"));
    QVERIFY(!file.isOpen());
    QCOMPARE(file.fileName(), QString("file-renamed-twice.txt"));

    QVERIFY(QFile::exists("existing-file.txt"));
    QVERIFY(!QFile::exists("file-to-be-renamed.txt"));
    QVERIFY(!QFile::exists("file-renamed-once.txt"));
    QVERIFY(QFile::exists("file-renamed-twice.txt"));

    file.remove();
    file2.remove();
    QVERIFY(!QFile::exists("file-renamed-twice.txt"));
    QVERIFY(!QFile::exists("existing-file.txt"));
}

void tst_QFile::appendAndRead()
{
    QFile writeFile(QLatin1String("appendfile.txt"));
    QVERIFY(writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate));

    QFile readFile(QLatin1String("appendfile.txt"));
    QVERIFY(readFile.open(QIODevice::ReadOnly));

    // Write to the end of the file, then read that character back, and so on.
    for (int i = 0; i < 100; ++i) {
        char c = '\0';
        writeFile.putChar(char(i % 256));
        writeFile.flush();
        QVERIFY(readFile.getChar(&c));
        QCOMPARE(c, char(i % 256));
        QCOMPARE(readFile.pos(), writeFile.pos());
    }

    // Write blocks and read them back
    for (int j = 0; j < 18; ++j) {
        writeFile.write(QByteArray(1 << j, '@'));
        writeFile.flush();
        QCOMPARE(readFile.read(1 << j).size(), 1 << j);
    }

    readFile.close();
}

void tst_QFile::miscWithUncPathAsCurrentDir()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    QString current = QDir::currentPath();
    QVERIFY(QDir::setCurrent("//" + QtNetworkSettings::winServerName() + "/testshare"));
    QFile file("test.pri");
    QVERIFY(file.exists());
    QCOMPARE(int(file.size()), 34);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(QDir::setCurrent(current));
#endif
}

void tst_QFile::standarderror()
{
    QFile f;
    bool ok = f.open(stderr, QFile::WriteOnly);
    QVERIFY(ok);
    f.close();
}

void tst_QFile::handle()
{
    int fd;
#if !defined(Q_OS_WINCE)
    QFile file(m_testSourceFile);
    QVERIFY(file.open(QIODevice::ReadOnly));
    fd = int(file.handle());
    QVERIFY(fd > 2);
    QCOMPARE(int(file.handle()), fd);
    char c = '\0';
    QT_READ(int(file.handle()), &c, 1);
    QCOMPARE(c, '/');

    // test if the QFile and the handle remain in sync
    QVERIFY(file.getChar(&c));
    QCOMPARE(c, '*');

    // same, but read from QFile first now
    file.close();
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Unbuffered));
    fd = int(file.handle());
    QVERIFY(fd > 2);
    QVERIFY(file.getChar(&c));
    QCOMPARE(c, '/');
#ifdef Q_OS_UNIX
    QCOMPARE(QT_READ(fd, &c, 1), ssize_t(1));
#else
    QCOMPARE(QT_READ(fd, &c, 1), 1);
#endif

    QCOMPARE(c, '*');
#endif

    //test round trip of adopted stdio file handle
    QFile file2;
    FILE *fp = fopen(qPrintable(m_testSourceFile), "r");
    file2.open(fp, QIODevice::ReadOnly);
    QCOMPARE(int(file2.handle()), int(fileno(fp)));
    QCOMPARE(int(file2.handle()), int(fileno(fp)));
    fclose(fp);

    //test round trip of adopted posix file handle
#ifdef Q_OS_UNIX
    QFile file3;
    fd = QT_OPEN(qPrintable(m_testSourceFile), QT_OPEN_RDONLY);
    file3.open(fd, QIODevice::ReadOnly);
    QCOMPARE(int(file3.handle()), fd);
    QT_CLOSE(fd);
#endif
}

void tst_QFile::nativeHandleLeaks()
{
    int fd1, fd2;

#ifdef Q_OS_WIN
    HANDLE handle1, handle2;
#endif

    {
        QFile file("qt_file.tmp");
        QVERIFY( file.open(QIODevice::ReadWrite) );

        fd1 = file.handle();
        QVERIFY( -1 != fd1 );
    }

#ifdef Q_OS_WIN
# ifndef Q_OS_WINRT
    handle1 = ::CreateFileA("qt_file.tmp", GENERIC_READ, 0, NULL,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
# else
    handle1 = ::CreateFile2(L"qt_file.tmp", GENERIC_READ, 0, OPEN_ALWAYS, NULL);
# endif
    QVERIFY( INVALID_HANDLE_VALUE != handle1 );
    QVERIFY( ::CloseHandle(handle1) );
#endif

    {
        QFile file("qt_file.tmp");
        QVERIFY( file.open(QIODevice::ReadOnly) );

        fd2 = file.handle();
        QVERIFY( -1 != fd2 );
    }

#ifdef Q_OS_WIN
# ifndef Q_OS_WINRT
    handle2 = ::CreateFileA("qt_file.tmp", GENERIC_READ, 0, NULL,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
# else
    handle2 = ::CreateFile2(L"qt_file.tmp", GENERIC_READ, 0, OPEN_ALWAYS, NULL);
# endif
    QVERIFY( INVALID_HANDLE_VALUE != handle2 );
    QVERIFY( ::CloseHandle(handle2) );
#endif

    QCOMPARE( fd2, fd1 );

#ifdef Q_OS_WIN
    QCOMPARE( handle2, handle1 );
#endif
}

void tst_QFile::readEof_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<int>("imode");

    QTest::newRow("buffered") << m_testFile << 0;
    QTest::newRow("unbuffered") << m_testFile << int(QIODevice::Unbuffered);

#if defined(Q_OS_UNIX)
    QTest::newRow("sequential,buffered") << "/dev/null" << 0;
    QTest::newRow("sequential,unbuffered") << "/dev/null" << int(QIODevice::Unbuffered);
#endif
}

void tst_QFile::readEof()
{
    QFETCH(QString, filename);
    QFETCH(int, imode);
    QIODevice::OpenMode mode = QIODevice::OpenMode(imode);

    {
        QFile file(filename);
        QVERIFY(file.open(QIODevice::ReadOnly | mode));
        bool isSequential = file.isSequential();
        if (!isSequential) {
            QVERIFY(file.seek(245));
            QVERIFY(file.atEnd());
        }

        char buf[10];
        int ret = file.read(buf, sizeof buf);
        QCOMPARE(ret, 0);
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());

        // Do it again to ensure that we get the same result
        ret = file.read(buf, sizeof buf);
        QCOMPARE(ret, 0);
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());
    }

    {
        QFile file(filename);
        QVERIFY(file.open(QIODevice::ReadOnly | mode));
        bool isSequential = file.isSequential();
        if (!isSequential) {
            QVERIFY(file.seek(245));
            QVERIFY(file.atEnd());
        }

        QByteArray ret = file.read(10);
        QVERIFY(ret.isEmpty());
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());

        // Do it again to ensure that we get the same result
        ret = file.read(10);
        QVERIFY(ret.isEmpty());
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());
    }

    {
        QFile file(filename);
        QVERIFY(file.open(QIODevice::ReadOnly | mode));
        bool isSequential = file.isSequential();
        if (!isSequential) {
            QVERIFY(file.seek(245));
            QVERIFY(file.atEnd());
        }

        char buf[10];
        int ret = file.readLine(buf, sizeof buf);
        QCOMPARE(ret, -1);
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());

        // Do it again to ensure that we get the same result
        ret = file.readLine(buf, sizeof buf);
        QCOMPARE(ret, -1);
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());
    }

    {
        QFile file(filename);
        QVERIFY(file.open(QIODevice::ReadOnly | mode));
        bool isSequential = file.isSequential();
        if (!isSequential) {
            QVERIFY(file.seek(245));
            QVERIFY(file.atEnd());
        }

        QByteArray ret = file.readLine();
        QVERIFY(ret.isNull());
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());

        // Do it again to ensure that we get the same result
        ret = file.readLine();
        QVERIFY(ret.isNull());
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());
    }

    {
        QFile file(filename);
        QVERIFY(file.open(QIODevice::ReadOnly | mode));
        bool isSequential = file.isSequential();
        if (!isSequential) {
            QVERIFY(file.seek(245));
            QVERIFY(file.atEnd());
        }

        char c;
        QVERIFY(!file.getChar(&c));
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());

        // Do it again to ensure that we get the same result
        QVERIFY(!file.getChar(&c));
        QVERIFY(file.error() == QFile::NoError);
        QVERIFY(file.atEnd());
    }
}

void tst_QFile::posAfterFailedStat()
{
    // Regression test for a bug introduced in 4.3.0; after a failed stat,
    // pos() could no longer be calculated correctly.
    QFile::remove("tmp.txt");
    QFile file("tmp.txt");
    QVERIFY(!file.exists());
    QVERIFY(file.open(QIODevice::Append));
    QVERIFY(file.exists());
    file.write("qt430", 5);
    QVERIFY(!file.isSequential());
    QCOMPARE(file.pos(), qint64(5));
    file.remove();
}

#define FILESIZE 65536 * 3

void tst_QFile::map_data()
{
    QTest::addColumn<int>("fileSize");
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("size");
    QTest::addColumn<QFile::FileError>("error");

    QTest::newRow("zero")         << FILESIZE << 0     << FILESIZE         << QFile::NoError;
    QTest::newRow("small, but 0") << FILESIZE << 30    << FILESIZE - 30    << QFile::NoError;
    QTest::newRow("a page")       << FILESIZE << 4096  << FILESIZE - 4096  << QFile::NoError;
    QTest::newRow("+page")        << FILESIZE << 5000  << FILESIZE - 5000  << QFile::NoError;
    QTest::newRow("++page")       << FILESIZE << 65576 << FILESIZE - 65576 << QFile::NoError;
    QTest::newRow("bad size")     << FILESIZE << 0     << -1               << QFile::ResourceError;
    QTest::newRow("bad offset")   << FILESIZE << -1    << 1                << QFile::UnspecifiedError;
    QTest::newRow("zerozero")     << FILESIZE << 0     << 0                << QFile::UnspecifiedError;
}

void tst_QFile::map()
{
    QFETCH(int, fileSize);
    QFETCH(int, offset);
    QFETCH(int, size);
    QFETCH(QFile::FileError, error);

    QString fileName = QDir::currentPath() + '/' + "qfile_map_testfile";

#ifdef Q_OS_WINCE
     fileName = QFileInfo(fileName).absoluteFilePath();
#endif

    if (QFile::exists(fileName)) {
        QVERIFY(QFile::setPermissions(fileName,
            QFile::WriteOwner | QFile::ReadOwner | QFile::WriteUser | QFile::ReadUser));
        QFile::remove(fileName);
    }
    QFile file(fileName);

    // invalid, not open
    uchar *memory = file.map(0, size);
    QVERIFY(!memory);
    QCOMPARE(file.error(), QFile::PermissionsError);
    QVERIFY(!file.unmap(memory));
    QCOMPARE(file.error(), QFile::PermissionsError);

    // make a file
    QVERIFY(file.open(QFile::ReadWrite));
    QVERIFY(file.resize(fileSize));
    QVERIFY(file.flush());
    file.close();
    QVERIFY(file.open(QFile::ReadWrite));
    memory = file.map(offset, size);
    if (error != QFile::NoError) {
        QVERIFY(file.error() != QFile::NoError);
        return;
    }

    QCOMPARE(file.error(), error);
    QVERIFY(memory);
    memory[0] = 'Q';
    QVERIFY(file.unmap(memory));
    QCOMPARE(file.error(), QFile::NoError);

    // Verify changes were saved
    memory = file.map(offset, size);
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(memory);
    QVERIFY(memory[0] == 'Q');
    QVERIFY(file.unmap(memory));
    QCOMPARE(file.error(), QFile::NoError);

    // hpux won't let you map multiple times.
#if !defined(Q_OS_HPUX) && !defined(Q_USE_DEPRECATED_MAP_API) && !defined(Q_OS_WINCE)
    // exotic test to make sure that multiple maps work

    // note: windows ce does not reference count mutliple maps
    // it's essentially just the same reference but it
    // cause a resource lock on the file which prevents it
    // from being removed    uchar *memory1 = file.map(0, file.size());
    uchar *memory1 = file.map(0, file.size());
    QCOMPARE(file.error(), QFile::NoError);
    uchar *memory2 = file.map(0, file.size());
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(memory1);
    QVERIFY(memory2);
    QVERIFY(file.unmap(memory1));
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(file.unmap(memory2));
    QCOMPARE(file.error(), QFile::NoError);
    memory1 = file.map(0, file.size());
    QCOMPARE(file.error(), QFile::NoError);
    QVERIFY(memory1);
    QVERIFY(file.unmap(memory1));
    QCOMPARE(file.error(), QFile::NoError);
#endif

    file.close();

#if !defined(Q_OS_VXWORKS)
#if defined(Q_OS_UNIX)
    if (::getuid() != 0)
        // root always has permissions
#endif
    {
        // Change permissions on a file, just to confirm it would fail
        QFile::Permissions originalPermissions = file.permissions();
        QVERIFY(file.setPermissions(QFile::ReadOther));
        QVERIFY(!file.open(QFile::ReadWrite));
        memory = file.map(offset, size);
        QCOMPARE(file.error(), QFile::PermissionsError);
        QVERIFY(!memory);
        QVERIFY(file.setPermissions(originalPermissions));
    }
#endif
    QVERIFY(file.remove());
}

void tst_QFile::mapResource_data()
{
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("size");
    QTest::addColumn<QFile::FileError>("error");
    QTest::addColumn<QString>("fileName");

    QString validFile = ":/tst_qfileinfo/resources/file1.ext1";
    QString invalidFile = ":/tst_qfileinfo/resources/filefoo.ext1";

    for (int i = 0; i < 2; ++i) {
        QString file = (i == 0) ? validFile : invalidFile;
        QTest::newRow("0, 0") << 0 << 0 << QFile::UnspecifiedError << file;
        QTest::newRow("0, BIG") << 0 << 4096 << QFile::UnspecifiedError << file;
        QTest::newRow("-1, 0") << -1 << 0 << QFile::UnspecifiedError << file;
        QTest::newRow("0, -1") << 0 << -1 << QFile::UnspecifiedError << file;
    }

    QTest::newRow("0, 1") << 0 << 1 << QFile::NoError << validFile;
}

void tst_QFile::mapResource()
{
    QFETCH(QString, fileName);
    QFETCH(int, offset);
    QFETCH(int, size);
    QFETCH(QFile::FileError, error);

    QFile file(fileName);
    uchar *memory = file.map(offset, size);
    QCOMPARE(file.error(), error);
    QVERIFY((error == QFile::NoError) ? (memory != 0) : (memory == 0));
    if (error == QFile::NoError)
        QCOMPARE(QString(memory[0]), QString::number(offset + 1));
    QVERIFY(file.unmap(memory));
}

void tst_QFile::mapOpenMode_data()
{
    QTest::addColumn<int>("openMode");

    QTest::newRow("ReadOnly") << int(QIODevice::ReadOnly);
    //QTest::newRow("WriteOnly") << int(QIODevice::WriteOnly); // this doesn't make sense
    QTest::newRow("ReadWrite") << int(QIODevice::ReadWrite);
    QTest::newRow("ReadOnly,Unbuffered") << int(QIODevice::ReadOnly | QIODevice::Unbuffered);
    QTest::newRow("ReadWrite,Unbuffered") << int(QIODevice::ReadWrite | QIODevice::Unbuffered);
}

void tst_QFile::mapOpenMode()
{
    QFETCH(int, openMode);
    static const qint64 fileSize = 4096;

    QByteArray pattern(fileSize, 'A');

    QString fileName = QDir::currentPath() + '/' + "qfile_map_testfile";
    if (QFile::exists(fileName)) {
        QVERIFY(QFile::setPermissions(fileName,
            QFile::WriteOwner | QFile::ReadOwner | QFile::WriteUser | QFile::ReadUser));
        QFile::remove(fileName);
    }
    QFile file(fileName);

    // make a file
    QVERIFY(file.open(QFile::ReadWrite));
    QVERIFY(file.write(pattern));
    QVERIFY(file.flush());
    file.close();

    // open according to our mode
    QVERIFY(file.open(QIODevice::OpenMode(openMode)));

    uchar *memory = file.map(0, fileSize);
    QVERIFY(memory);
    QVERIFY(memcmp(memory, pattern, fileSize) == 0);

    if (openMode & QIODevice::WriteOnly) {
        // try to write to the file
        *memory = 'a';
        file.unmap(memory);
        file.close();
        file.open(QIODevice::OpenMode(openMode));
        file.seek(0);
        char c;
        QVERIFY(file.getChar(&c));
        QCOMPARE(c, 'a');
    }

    file.close();
}

void tst_QFile::openDirectory()
{
    QFile f1(m_resourcesDir);
    // it's a directory, it must exist
    QVERIFY(f1.exists());

    // ...but not be openable
    QVERIFY(!f1.open(QIODevice::ReadOnly));
    f1.close();
    QVERIFY(!f1.open(QIODevice::ReadOnly|QIODevice::Unbuffered));
    f1.close();
    QVERIFY(!f1.open(QIODevice::ReadWrite));
    f1.close();
    QVERIFY(!f1.open(QIODevice::WriteOnly));
    f1.close();
    QVERIFY(!f1.open(QIODevice::WriteOnly|QIODevice::Unbuffered));
    f1.close();
}

static qint64 streamExpectedSize(int fd)
{
    QT_STATBUF sb;
    if (QT_FSTAT(fd, &sb) != -1)
        return sb.st_size;
    return 0;
}

static qint64 streamCurrentPosition(int fd)
{
    QT_OFF_T pos = QT_LSEEK(fd, 0, SEEK_CUR);
    if (pos != -1)
        return pos;
    return 0;
}

static qint64 streamCurrentPosition(FILE *f)
{
    QT_OFF_T pos = QT_FTELL(f);
    if (pos != -1)
        return pos;
    return 0;
}

class MessageHandler {
public:
    MessageHandler(QtMessageHandler messageHandler = handler)
    {
        ok = true;
        oldMessageHandler = qInstallMessageHandler(messageHandler);
    }

    ~MessageHandler()
    {
        qInstallMessageHandler(oldMessageHandler);
    }

    static bool testPassed()
    {
        return ok;
    }
protected:
    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if (msg == QString::fromLatin1("QIODevice::seek: Cannot call seek on a sequential device"))
            ok = false;
        // Defer to old message handler.
        if (oldMessageHandler)
            oldMessageHandler(type, context, msg);
    }

    static QtMessageHandler oldMessageHandler;
    static bool ok;
};

bool MessageHandler::ok = true;
QtMessageHandler MessageHandler::oldMessageHandler = 0;

    //allthough Windows CE (not mobile!) has functions that allow redirecting
    //the standard file descriptors to a file (see SetStdioPathW/GetStdioPathW)
    //it does not have functions to simply open them like below .
#ifndef Q_OS_WINCE
void tst_QFile::openStandardStreamsFileDescriptors()
{

    // Check that QIODevice::seek() isn't called when opening a sequential device (QFile).
    MessageHandler msgHandler;

    {
        QFile in;
        in.open(STDIN_FILENO, QIODevice::ReadOnly);
        QCOMPARE( in.pos(), streamCurrentPosition(STDIN_FILENO) );
        QCOMPARE( in.size(), streamExpectedSize(STDIN_FILENO) );
    }

    {
        QFile out;
        QVERIFY(out.open(STDOUT_FILENO, QIODevice::WriteOnly));
        QCOMPARE( out.pos(), streamCurrentPosition(STDOUT_FILENO) );
        QCOMPARE( out.size(), streamExpectedSize(STDOUT_FILENO) );
    }

    {
        QFile err;
        err.open(STDERR_FILENO, QIODevice::WriteOnly);
        QCOMPARE( err.pos(), streamCurrentPosition(STDERR_FILENO) );
        QCOMPARE( err.size(), streamExpectedSize(STDERR_FILENO) );
    }

    QVERIFY(msgHandler.testPassed());
}

void tst_QFile::openStandardStreamsBufferedStreams()
{
    // Check that QIODevice::seek() isn't called when opening a sequential device (QFile).
    MessageHandler msgHandler;

    // Using streams
    {
        QFile in;
        in.open(stdin, QIODevice::ReadOnly);
        QCOMPARE( in.pos(), streamCurrentPosition(stdin) );
        QCOMPARE( in.size(), streamExpectedSize(QT_FILENO(stdin)) );
    }

    {
        QFile out;
        out.open(stdout, QIODevice::WriteOnly);
        QCOMPARE( out.pos(), streamCurrentPosition(stdout) );
        QCOMPARE( out.size(), streamExpectedSize(QT_FILENO(stdout)) );
    }

    {
        QFile err;
        err.open(stderr, QIODevice::WriteOnly);
        QCOMPARE( err.pos(), streamCurrentPosition(stderr) );
        QCOMPARE( err.size(), streamExpectedSize(QT_FILENO(stderr)) );
    }

    QVERIFY(msgHandler.testPassed());
}
#endif

void tst_QFile::writeNothing()
{
    for (int i = 0; i < NumberOfFileTypes; ++i) {
        QFile file("file.txt");
        QVERIFY( openFile(file, QIODevice::WriteOnly | QIODevice::Unbuffered, FileType(i)) );
        QVERIFY( 0 == file.write((char *)0, 0) );
        QCOMPARE( file.error(), QFile::NoError );
        closeFile(file);
    }
}

void tst_QFile::resize_data()
{
    QTest::addColumn<int>("filetype");

    QTest::newRow("native") << int(OpenQFile);
    QTest::newRow("fileno") << int(OpenFd);
    QTest::newRow("stream") << int(OpenStream);
}

void tst_QFile::resize()
{
    QFETCH(int, filetype);
    QString filename(QLatin1String("file.txt"));
    QFile file(filename);
    QVERIFY(openFile(file, QIODevice::ReadWrite, FileType(filetype)));
    QVERIFY(file.resize(8));
    QCOMPARE(file.size(), qint64(8));
    closeFile(file);
    QFile::resize(filename, 4);
    QCOMPARE(QFileInfo(filename).size(), qint64(4));
}

void tst_QFile::objectConstructors()
{
    QObject ob;
    QFile* file1 = new QFile(m_testFile, &ob);
    QFile* file2 = new QFile(&ob);
    QVERIFY(file1->exists());
    QVERIFY(!file2->exists());
}

void tst_QFile::caseSensitivity()
{
#if defined(Q_OS_WIN)
    const bool caseSensitive = false;
#elif defined(Q_OS_MAC)
     const bool caseSensitive = pathconf(QDir::currentPath().toLatin1().constData(), _PC_CASE_SENSITIVE);
#else
    const bool caseSensitive = true;
#endif

    QByteArray testData("a little test");
    QString filename("File.txt");
    {
        QFile f(filename);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QVERIFY(f.write(testData));
        f.close();
    }
    QStringList alternates;
    QFileInfo fi(filename);
    QVERIFY(fi.exists());
    alternates << "file.txt" << "File.TXT" << "fIlE.TxT" << fi.absoluteFilePath().toUpper() << fi.absoluteFilePath().toLower();
    foreach (QString alt, alternates) {
        QFileInfo fi2(alt);
        QCOMPARE(fi2.exists(), !caseSensitive);
        QCOMPARE(fi.size() == fi2.size(), !caseSensitive);
        QFile f2(alt);
        QCOMPARE(f2.open(QIODevice::ReadOnly), !caseSensitive);
        if (!caseSensitive)
            QCOMPARE(f2.readAll(), testData);
    }
}

//MSVCRT asserts when any function is called with a closed file handle.
//This replaces the default crashing error handler with one that ignores the error (allowing EBADF to be returned)
class AutoIgnoreInvalidParameter
{
public:
#if defined(Q_OS_WIN) && defined (Q_CC_MSVC)
    static void ignore_invalid_parameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {}
    AutoIgnoreInvalidParameter()
    {
        oldHandler = _set_invalid_parameter_handler(ignore_invalid_parameter);
        //also disable the abort/retry/ignore popup
        oldReportMode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    }
    ~AutoIgnoreInvalidParameter()
    {
        //restore previous settings
        _set_invalid_parameter_handler(oldHandler);
        _CrtSetReportMode(_CRT_ASSERT, oldReportMode);
    }
    _invalid_parameter_handler oldHandler;
    int oldReportMode;
#endif
};

void tst_QFile::autocloseHandle()
{
    {
        QFile file("readonlyfile");
        QVERIFY(openFile(file, QIODevice::ReadOnly, OpenFd, QFile::AutoCloseHandle));
        int fd = fd_;
        QCOMPARE(file.handle(), fd);
        file.close();
        fd_ = -1;
        QCOMPARE(file.handle(), -1);
        AutoIgnoreInvalidParameter a;
        Q_UNUSED(a);
        //file is closed, read should fail
        char buf;
        QCOMPARE((int)QT_READ(fd, &buf, 1), -1);
        QVERIFY(errno = EBADF);
    }

    {
        QFile file("readonlyfile");
        QVERIFY(openFile(file, QIODevice::ReadOnly, OpenFd, QFile::DontCloseHandle));
        QCOMPARE(file.handle(), fd_);
        file.close();
        QCOMPARE(file.handle(), -1);
        //file is not closed, read should succeed
        char buf;
        QCOMPARE((int)QT_READ(fd_, &buf, 1), 1);
        QT_CLOSE(fd_);
        fd_ = -1;
    }

    {
        QFile file("readonlyfile");
        QVERIFY(openFile(file, QIODevice::ReadOnly, OpenStream, QFile::AutoCloseHandle));
        int fd = QT_FILENO(stream_);
        QCOMPARE(file.handle(), fd);
        file.close();
        stream_ = 0;
        QCOMPARE(file.handle(), -1);
        AutoIgnoreInvalidParameter a;
        Q_UNUSED(a);
        //file is closed, read should fail
        char buf;
        QCOMPARE((int)QT_READ(fd, &buf, 1), -1); //not using fread because the FILE* was freed by fclose
    }

    {
        QFile file("readonlyfile");
        QVERIFY(openFile(file, QIODevice::ReadOnly, OpenStream, QFile::DontCloseHandle));
        QCOMPARE(file.handle(), int(QT_FILENO(stream_)));
        file.close();
        QCOMPARE(file.handle(), -1);
        //file is not closed, read should succeed
        char buf;
        QCOMPARE(int(::fread(&buf, 1, 1, stream_)), 1);
        ::fclose(stream_);
        stream_ = 0;
    }
}

QTEST_MAIN(tst_QFile)
#include "tst_qfile.moc"
