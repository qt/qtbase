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
#include <qcoreapplication.h>
#include <qstring.h>
#include <qtemporarydir.h>
#include <qtemporaryfile.h>
#include <qfile.h>
#include <qdir.h>
#include <qset.h>
#include <qtextcodec.h>

#if defined(Q_OS_WIN)
# include <windows.h>
#endif
#if defined(Q_OS_UNIX)
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <fcntl.h>             // open(2)
# include <unistd.h>            // close(2)
#endif

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
    void write();
    void openCloseOpenClose();
    void removeAndReOpen();
    void size();
    void resize();
    void openOnRootDrives();
    void stressTest();
    void rename();
    void renameFdLeak();
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
private:
    QTemporaryDir m_temporaryDir;
    QString m_previousCurrent;
};

void tst_QTemporaryFile::initTestCase()
{
    QVERIFY2(m_temporaryDir.isValid(), qPrintable(m_temporaryDir.errorString()));
    m_previousCurrent = QDir::currentPath();
    QVERIFY(QDir::setCurrent(m_temporaryDir.path()));

    // For QTBUG_4796
    QVERIFY(QDir("test-XXXXXX").exists() || QDir().mkdir("test-XXXXXX"));
    QCoreApplication::setApplicationName("tst_qtemporaryfile");

#if defined(Q_OS_ANDROID)
    QString sourceDir(":/android_testdata/");
    QDirIterator it(sourceDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();

        QFileInfo sourceFileInfo = it.fileInfo();
        if (!sourceFileInfo.isDir()) {
            QFileInfo destinationFileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') + sourceFileInfo.filePath().mid(sourceDir.length()));

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

static inline bool canHandleUnicodeFileNames()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    return true;
#else
    // Check for UTF-8 by converting the Euro symbol (see tst_utf8)
    return QFile::encodeName(QString(QChar(0x20AC))) == QByteArrayLiteral("\342\202\254");
#endif
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

    QTest::newRow("set template, no suffix") << "" << "foo" << "" << "foo";
    QTest::newRow("set template, with lowercase XXXXXX") << "" << "qt_" << "xxxxxx" << "qt_XXXXXXxxxxxx";
    QTest::newRow("set template, with xxx") << "" << "qt_" << ".xxx" << "qt_XXXXXX.xxx";
    QTest::newRow("set template, with >6 X's") << "" << "qt_" << ".xxx" << "qt_XXXXXXXXXXXXXX.xxx";
    QTest::newRow("set template, with >6 X's, no suffix") << "" << "qt_" << "" << "qt_XXXXXXXXXXXXXX";
    if (canHandleUnicodeFileNames()) {
        // Test Umlauts (contained in Latin1)
        QString prefix = "qt_" + umlautTestText();
        QTest::newRow("Umlauts") << (prefix + "XXXXXX") << prefix << QString() << QString();
        // Test Chinese
        prefix = "qt_" + hanTestText();
        QTest::newRow("Chinese characters") << (prefix + "XXXXXX") << prefix << QString() << QString();
    }
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

    QCOMPARE(file.open(), true);

    QString fileName = QFileInfo(file).fileName();
    if (prefix.length())
        QCOMPARE(fileName.left(prefix.length()), prefix);

    if (suffix.length())
        QCOMPARE(fileName.right(suffix.length()), suffix);
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
    file.open();
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
    QVERIFY(!QFile::exists(fileName));

    // Test if disabling auto remove works.
    {
        QTemporaryFile file("tempXXXXXX");
        file.setAutoRemove(false);
        QVERIFY(file.open());
        fileName = file.fileName();
        file.close();
    }
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

#if defined(Q_OS_ANDROID)
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

void tst_QTemporaryFile::write()
{
    QByteArray data("OLE\nOLE\nOLE");
    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE((int)file.write(data), data.size());
    file.reset();
    QFile compare(file.fileName());
    compare.open(QIODevice::ReadOnly);
    QCOMPARE(compare.readAll() , data);
    file.close();
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
        file.open();
        fileName = file.fileName();
        QVERIFY(QFile::exists(fileName));

        file.remove();
        QVERIFY(!QFile::exists(fileName));

        QVERIFY(file.open());
        QCOMPARE(QFileInfo(file.fileName()).path(), QFileInfo(fileName).path());
        fileName = file.fileName();
        QVERIFY(QFile::exists(fileName));
    }
    QVERIFY(!QFile::exists(fileName));
}

void tst_QTemporaryFile::size()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    QVERIFY(file.exists());
    QVERIFY(!file.isSequential());
    QByteArray str("foobar");
    file.write(str);
    QVERIFY(QFile::exists(file.fileName()));
    // On CE it takes more time for the filesystem to update
    // the information. Usually you have to close it or seek
    // to get latest information. flush() does not help either.
    QCOMPARE(file.size(), qint64(6));
    file.seek(0);
    QCOMPARE(file.size(), qint64(6));
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
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    unsigned int lastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
    // If it's possible to create a file in the root directory, it
    // must be possible to create a temp file there too.
    foreach (QFileInfo driveInfo, QDir::drives()) {
        QFile testFile(driveInfo.filePath() + "XXXXXX.txt");
        if (testFile.open(QIODevice::ReadWrite)) {
            testFile.remove();
            QTemporaryFile file(driveInfo.filePath() + "XXXXXX.txt");
            file.setAutoRemove(true);
            QVERIFY(file.open());
        }
    }
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    SetErrorMode(lastErrorMode);
#endif
}

void tst_QTemporaryFile::stressTest()
{
    const int iterations = 1000;

    QSet<QString> names;
    for (int i = 0; i < iterations; ++i) {
        QTemporaryFile file;
        file.setAutoRemove(false);
        QVERIFY2(file.open(), qPrintable(file.errorString()));
        QVERIFY(!names.contains(file.fileName()));
        names.insert(file.fileName());
    }
    for (QSet<QString>::const_iterator it = names.constBegin(); it != names.constEnd(); ++it) {
        QFile::remove(*it);
    }
}

void tst_QTemporaryFile::rename()
{
    // This test checks that the temporary file is deleted, even after a
    // rename.

    QDir dir;
    QVERIFY(!dir.exists("temporary-file.txt"));

    QString tempname;
    {
        QTemporaryFile file(dir.filePath("temporary-file.XXXXXX"));

        QVERIFY(file.open());
        tempname = file.fileName();
        QVERIFY(dir.exists(tempname));

        QVERIFY(file.rename("temporary-file.txt"));
        QVERIFY(!dir.exists(tempname));
        QVERIFY(dir.exists("temporary-file.txt"));
        QCOMPARE(file.fileName(), QString("temporary-file.txt"));
    }

    QVERIFY(!dir.exists(tempname));
    QVERIFY(!dir.exists("temporary-file.txt"));
}

void tst_QTemporaryFile::renameFdLeak()
{
#ifdef Q_OS_UNIX

#  if defined(Q_OS_ANDROID)
    ChdirOnReturn cor(QDir::currentPath());
    QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
#  endif

    const QByteArray sourceFile = QFile::encodeName(QFINDTESTDATA(__FILE__));
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
    struct CleanOnReturn
    {
        ~CleanOnReturn()
        {
            if (!tempName.isEmpty())
                QFile::remove(tempName);
        }

        void reset()
        {
            tempName.clear();
        }

        QString tempName;
    };

    CleanOnReturn cleaner;

    {
        QTemporaryFile file;
        QVERIFY( file.open() );
        cleaner.tempName = file.fileName();

        QVERIFY( QFile::exists(cleaner.tempName) );
        QVERIFY( !QFileInfo("i-do-not-exist").isDir() );
        QVERIFY( !file.rename("i-do-not-exist/file.txt") );
        QVERIFY( QFile::exists(cleaner.tempName) );
    }

    QVERIFY( !QFile::exists(cleaner.tempName) );
    cleaner.reset();
}

void tst_QTemporaryFile::createNativeFile_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<qint64>("currentPos");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("content");

#if defined(Q_OS_ANDROID)
    const QString nativeFilePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/resources/test.txt");
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
        f.open(QIODevice::ReadOnly);
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
    QTest::newRow("XXXXXX<unicode>") << QString() << unicode << true;
    QTest::newRow("<unicode>XXXXXX") << unicode << QString() << true;
    QTest::newRow("<unicode>XXXXXX<unicode>") << unicode << unicode << true;
}

void tst_QTemporaryFile::QTBUG_4796()
{
    QVERIFY(QDir("test-XXXXXX").exists());

    struct CleanOnReturn
    {
        ~CleanOnReturn()
        {
            Q_FOREACH(QString tempName, tempNames)
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

    Q_FOREACH(QString const &tempName, cleaner.tempNames)
        QVERIFY( !QFile::exists(tempName) );

    cleaner.reset();
}

void tst_QTemporaryFile::guaranteeUnique()
{
    QDir dir(QDir::tempPath());
    QString takenFileName;

    // First pass. See which filename QTemporaryFile will try first.
    {
        // Fix the random seed.
        qsrand(1135);
        QTemporaryFile tmpFile("testFile1.XXXXXX");
        tmpFile.open();
        takenFileName = tmpFile.fileName();
        QVERIFY(QFile::exists(takenFileName));
    }

    QVERIFY(!QFile::exists(takenFileName));

    // Create a directory with same name.
    QVERIFY(dir.mkdir(takenFileName));

    // Second pass, now we have blocked its first attempt with a directory.
    {
        // Fix the random seed.
        qsrand(1135);
        QTemporaryFile tmpFile("testFile1.XXXXXX");
        QVERIFY(tmpFile.open());
        QString uniqueFileName = tmpFile.fileName();
        QVERIFY(QFileInfo(uniqueFileName).isFile());
        QVERIFY(uniqueFileName != takenFileName);
    }

    QVERIFY(dir.rmdir(takenFileName));
}

QTEST_MAIN(tst_QTemporaryFile)
#include "tst_qtemporaryfile.moc"
