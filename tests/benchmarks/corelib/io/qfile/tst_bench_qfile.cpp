// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDebug>
#include <QTemporaryFile>
#include <QString>
#include <QDirIterator>

#include <private/qfsfileengine_p.h>

#include <qtest.h>

#include <stdio.h>

#ifdef Q_OS_WIN
# include <qt_windows.h>
#endif

#if defined(Q_OS_QNX) && defined(open)
#undef open
#endif

#define BUFSIZE 1024*512
#define FACTOR 1024*512
#define TF_SIZE FACTOR*81

// 10 predefined (but random() seek positions
// hardcoded to be comparable over several runs
const int seekpos[] = {int(TF_SIZE*0.52),
                       int(TF_SIZE*0.23),
                       int(TF_SIZE*0.73),
                       int(TF_SIZE*0.77),
                       int(TF_SIZE*0.80),
                       int(TF_SIZE*0.12),
                       int(TF_SIZE*0.53),
                       int(TF_SIZE*0.21),
                       int(TF_SIZE*0.27),
                       int(TF_SIZE*0.78)};

const int sp_size = sizeof(seekpos)/sizeof(int);

class tst_qfile: public QObject
{
Q_ENUMS(BenchmarkType)
Q_OBJECT
public:
    enum BenchmarkType {
        QFileBenchmark = 1,
#ifdef QT_BUILD_INTERNAL
        QFSFileEngineBenchmark,
#endif
        Win32Benchmark,
        PosixBenchmark,
        QFileFromPosixBenchmark
    };
private slots:
    void initTestCase();

    void open_data();
    void open();
    void seek_data();
    void seek();

    void readSmallFiles_QFile() { readSmallFiles(); }
    void readSmallFiles_QFSFileEngine() { readSmallFiles(); }
    void readSmallFiles_posix() { readSmallFiles(); }
    void readSmallFiles_Win32() { readSmallFiles(); }

    void readSmallFiles_QFile_data();
    void readSmallFiles_QFSFileEngine_data();
    void readSmallFiles_posix_data();
    void readSmallFiles_Win32_data();

    void readBigFile_QFile_data();
    void readBigFile_QFSFileEngine_data();
    void readBigFile_posix_data();
    void readBigFile_Win32_data();

    void readBigFile_QFile() { readBigFile(); }
    void readBigFile_QFSFileEngine() { readBigFile(); }
    void readBigFile_posix() { readBigFile(); }
    void readBigFile_Win32() { readBigFile(); }

private:
    void readFile_data(BenchmarkType type, QIODevice::OpenModeFlag t, QIODevice::OpenModeFlag b);
    void readBigFile();
    void readSmallFiles();

    class TestDataDir : public QTemporaryDir
    {
        void createFile();
        void createSmallFiles();
    public:
        TestDataDir() : QTemporaryDir(), fail(errorString().toLocal8Bit())
        {
            if (fail.isEmpty() && !QTemporaryDir::isValid())
                fail = "Failed to create temporary directory for data";
            if (isValid())
                createSmallFiles();
            if (isValid())
                createFile();
            if (isValid())
                QTest::qSleep(2000); // let IO settle
        }
        bool isValid() { return QTemporaryDir::isValid() && fail.isEmpty(); }
        QByteArray fail;
        QString filename;
    } tempDir;
};

Q_DECLARE_METATYPE(tst_qfile::BenchmarkType)
Q_DECLARE_METATYPE(QIODevice::OpenMode)
Q_DECLARE_METATYPE(QIODevice::OpenModeFlag)

/* None of the tests modify the test data in tempDir, so it's OK to only create
 * and tear down the directory once.
 */
void tst_qfile::TestDataDir::createFile()
{
    QFile tmpFile(filePath("testFile"));
    if (!tmpFile.open(QIODevice::WriteOnly)) {
        fail = "Unable to prepare files for test";
        return;
    }
#if 0 // Varied data, rather than filling with '\0' bytes:
    for (int row = 0; row < FACTOR; ++row) {
        tmpFile.write(QByteArray().fill('0' + row % ('0' - 'z'), 80));
        tmpFile.write("\n");
    }
#else
    tmpFile.seek(FACTOR * 80);
    tmpFile.putChar('\n');
#endif
    filename = tmpFile.fileName();
    tmpFile.close();
}

void tst_qfile::TestDataDir::createSmallFiles()
{
    for (int i = 0; i < 1000; ++i) {
        QFile f(filePath(QString::number(i)));
        if (!f.open(QIODevice::WriteOnly)) {
            fail = "Unable to prepare small files for test";
            return;
        }
        f.seek(511);
        f.putChar('\n');
        f.close();
    }
}

void tst_qfile::initTestCase()
{
    QVERIFY2(tempDir.isValid(), tempDir.fail.constData());
}

void tst_qfile::readFile_data(BenchmarkType type, QIODevice::OpenModeFlag t,
                              QIODevice::OpenModeFlag b)
{
    QTest::addColumn<tst_qfile::BenchmarkType>("testType");
    QTest::addColumn<int>("blockSize");
    QTest::addColumn<QFile::OpenModeFlag>("textMode");
    QTest::addColumn<QFile::OpenModeFlag>("bufferedMode");

    QByteArray flagstring;
    if (t & QIODevice::Text)
        flagstring += "textMode";
    if (b & QIODevice::Unbuffered) {
        if (flagstring.size())
            flagstring += ' ';
        flagstring += "unbuffered";
    }
    if (flagstring.isEmpty())
        flagstring = "none";

    const int kbs[] = {1, 2, 8, 16, 32, 512};
    for (int kb : kbs) {
        const int size = 1024 * kb;
        QTest::addRow("BS: %d, Flags: %s", size, flagstring.constData())
            << type << size << t << b;
    }
}

void tst_qfile::readBigFile_QFile_data()
{
    readFile_data(QFileBenchmark, QIODevice::NotOpen, QIODevice::NotOpen);
    readFile_data(QFileBenchmark, QIODevice::NotOpen, QIODevice::Unbuffered);
    readFile_data(QFileBenchmark, QIODevice::Text, QIODevice::NotOpen);
    readFile_data(QFileBenchmark, QIODevice::Text, QIODevice::Unbuffered);

}

void tst_qfile::readBigFile_QFSFileEngine_data()
{
#ifdef QT_BUILD_INTERNAL
    // Support for buffering dropped at 5.10, so only test Unbuffered
    readFile_data(QFSFileEngineBenchmark, QIODevice::NotOpen, QIODevice::Unbuffered);
    readFile_data(QFSFileEngineBenchmark, QIODevice::Text, QIODevice::Unbuffered);
#else
    QSKIP("This test requires -developer-build.");
#endif
}

void tst_qfile::readBigFile_posix_data()
{
    readFile_data(PosixBenchmark, QIODevice::NotOpen, QIODevice::NotOpen);
}

void tst_qfile::readBigFile_Win32_data()
{
#ifdef Q_OS_WIN
    readFile_data(Win32Benchmark, QIODevice::NotOpen, QIODevice::NotOpen);
#else
    QSKIP("This is Windows only benchmark.");
#endif
}

void tst_qfile::readBigFile()
{
    QFETCH(tst_qfile::BenchmarkType, testType);
    QFETCH(int, blockSize);
    QFETCH(QFile::OpenModeFlag, textMode);
    QFETCH(QFile::OpenModeFlag, bufferedMode);

    char buffer[BUFSIZE];
    switch (testType) {
        case QFileBenchmark: {
            QFile file(tempDir.filename);
            Q_UNUSED(file.open(QIODevice::ReadOnly|textMode|bufferedMode));
            QBENCHMARK {
                while(!file.atEnd())
                    file.read(blockSize);
                file.reset();
            }
            file.close();
        }
        break;
#ifdef QT_BUILD_INTERNAL
        case QFSFileEngineBenchmark: {
            QFSFileEngine fse(tempDir.filename);
            Q_UNUSED(fse.open(QIODevice::ReadOnly | textMode | bufferedMode, std::nullopt));
            QBENCHMARK {
                //qWarning() << fse.supportsExtension(QAbstractFileEngine::AtEndExtension);
                while (fse.read(buffer, blockSize)) {}
                fse.seek(0);
            }
            fse.close();
        }
        break;
#endif
        case PosixBenchmark: {
            QByteArray data = tempDir.filename.toLocal8Bit();
            const char* cfilename = data.constData();
            FILE* cfile = ::fopen(cfilename, "rb");
            QBENCHMARK {
                while(!feof(cfile))
                    [[maybe_unused]] auto r = ::fread(buffer, blockSize, 1, cfile);
                ::fseek(cfile, 0, SEEK_SET);
            }
            ::fclose(cfile);
        }
        break;
        case QFileFromPosixBenchmark: {
            // No gain in benchmarking this case
        }
        break;
        case Win32Benchmark: {
#ifdef Q_OS_WIN
            HANDLE hndl;

            // ensure we don't account string conversion
            const wchar_t *cfilename = reinterpret_cast<const wchar_t *>(tempDir.filename.utf16());

            hndl = CreateFile(cfilename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
            Q_ASSERT(hndl);
            wchar_t* nativeBuffer = new wchar_t[BUFSIZE];
            DWORD numberOfBytesRead;

            QBENCHMARK {
                do {
                   ReadFile(hndl, nativeBuffer, blockSize, &numberOfBytesRead, NULL);
                } while(numberOfBytesRead != 0);
                SetFilePointer(hndl, 0, NULL, FILE_BEGIN);
            }
            delete[] nativeBuffer;
            CloseHandle(hndl);
#else
            QFAIL("Not running on a non-Windows platform!");
#endif
        }
        break;
    }
}

void tst_qfile::seek_data()
{
    QTest::addColumn<tst_qfile::BenchmarkType>("testType");
    QTest::newRow("QFile") << QFileBenchmark;
#ifdef QT_BUILD_INTERNAL
    QTest::newRow("QFSFileEngine") << QFSFileEngineBenchmark;
#endif
    QTest::newRow("Posix FILE*") << PosixBenchmark;
#ifdef Q_OS_WIN
    QTest::newRow("Win32 API") << Win32Benchmark;
#endif
}

void tst_qfile::seek()
{
    QFETCH(tst_qfile::BenchmarkType, testType);
    int i = 0;

    switch (testType) {
        case QFileBenchmark: {
            QFile file(tempDir.filename);
            Q_UNUSED(file.open(QIODevice::ReadOnly));
            QBENCHMARK {
                i=(i+1)%sp_size;
                file.seek(seekpos[i]);
            }
            file.close();
        }
        break;
#ifdef QT_BUILD_INTERNAL
        case QFSFileEngineBenchmark: {
            QFSFileEngine fse(tempDir.filename);
            Q_UNUSED(fse.open(QIODevice::ReadOnly | QIODevice::Unbuffered, std::nullopt));
            QBENCHMARK {
                i=(i+1)%sp_size;
                fse.seek(seekpos[i]);
            }
            fse.close();
        }
        break;
#endif
        case PosixBenchmark: {
            QByteArray data = tempDir.filename.toLocal8Bit();
            const char* cfilename = data.constData();
            FILE* cfile = ::fopen(cfilename, "rb");
            QBENCHMARK {
                i=(i+1)%sp_size;
                ::fseek(cfile, seekpos[i], SEEK_SET);
            }
            ::fclose(cfile);
        }
        break;
        case QFileFromPosixBenchmark: {
            // No gain in benchmarking this case
        }
        break;
        case Win32Benchmark: {
#ifdef Q_OS_WIN
            HANDLE hndl;

            // ensure we don't account string conversion
            const wchar_t *cfilename = reinterpret_cast<const wchar_t *>(tempDir.filename.utf16());

            hndl = CreateFile(cfilename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
            Q_ASSERT(hndl);
            QBENCHMARK {
                i=(i+1)%sp_size;
                SetFilePointer(hndl, seekpos[i], NULL, 0);
            }
            CloseHandle(hndl);
#else
            QFAIL("Not running on a Windows platform!");
#endif
        }
        break;
    }
}

void tst_qfile::open_data()
{
    QTest::addColumn<tst_qfile::BenchmarkType>("testType");
    QTest::newRow("QFile") << QFileBenchmark;
#ifdef QT_BUILD_INTERNAL
    QTest::newRow("QFSFileEngine") << QFSFileEngineBenchmark;
#endif
    QTest::newRow("Posix FILE*") << PosixBenchmark;
    QTest::newRow("QFile from FILE*") << QFileFromPosixBenchmark;
#ifdef Q_OS_WIN
    QTest::newRow("Win32 API") << Win32Benchmark;
#endif
}

void tst_qfile::open()
{
    QFETCH(tst_qfile::BenchmarkType, testType);

    switch (testType) {
        case QFileBenchmark: {
            QBENCHMARK {
                QFile file(tempDir.filename);
                Q_UNUSED(file.open(QIODevice::ReadOnly));
                file.close();
            }
        }
        break;
#ifdef QT_BUILD_INTERNAL
        case QFSFileEngineBenchmark: {
            QBENCHMARK {
                QFSFileEngine fse(tempDir.filename);
                Q_UNUSED(fse.open(QIODevice::ReadOnly | QIODevice::Unbuffered, std::nullopt));
                fse.close();
            }
        }
        break;
#endif
        case PosixBenchmark: {
            // ensure we don't account toLocal8Bit()
            QByteArray data = tempDir.filename.toLocal8Bit();
            const char* cfilename = data.constData();

            QBENCHMARK {
                FILE* cfile = ::fopen(cfilename, "rb");
                ::fclose(cfile);
            }
        }
        break;
        case QFileFromPosixBenchmark: {
            // ensure we don't account toLocal8Bit()
            QByteArray data = tempDir.filename.toLocal8Bit();
            const char* cfilename = data.constData();
            FILE* cfile = ::fopen(cfilename, "rb");

            QBENCHMARK {
                QFile file;
                [[maybe_unused]] const auto r =
                file.open(cfile, QIODevice::ReadOnly);
                file.close();
            }
            ::fclose(cfile);
        }
        break;
        case Win32Benchmark: {
#ifdef Q_OS_WIN
            HANDLE hndl;

            // ensure we don't account string conversion
            const wchar_t *cfilename = reinterpret_cast<const wchar_t *>(tempDir.filename.utf16());

            QBENCHMARK {
                hndl = CreateFile(cfilename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
                Q_ASSERT(hndl);
                CloseHandle(hndl);
            }
#else
        QFAIL("Not running on a non-Windows platform!");
#endif
        }
        break;
    }
}

void tst_qfile::readSmallFiles_QFile_data()
{
    readFile_data(QFileBenchmark, QIODevice::NotOpen, QIODevice::NotOpen);
    readFile_data(QFileBenchmark, QIODevice::NotOpen, QIODevice::Unbuffered);
    readFile_data(QFileBenchmark, QIODevice::Text, QIODevice::NotOpen);
    readFile_data(QFileBenchmark, QIODevice::Text, QIODevice::Unbuffered);

}

void tst_qfile::readSmallFiles_QFSFileEngine_data()
{
#ifdef QT_BUILD_INTERNAL
    // Support for buffering dropped at 5.10, so only test Unbuffered
    readFile_data(QFSFileEngineBenchmark, QIODevice::NotOpen, QIODevice::Unbuffered);
    readFile_data(QFSFileEngineBenchmark, QIODevice::Text, QIODevice::Unbuffered);
#else
    QSKIP("This test requires -developer-build.");
#endif
}

void tst_qfile::readSmallFiles_posix_data()
{
    readFile_data(PosixBenchmark, QIODevice::NotOpen, QIODevice::NotOpen);
}

void tst_qfile::readSmallFiles_Win32_data()
{

#ifdef Q_OS_WIN
    readFile_data(Win32Benchmark, QIODevice::NotOpen, QIODevice::NotOpen);
#else
    QSKIP("This is Windows only benchmark.");
#endif
}

void tst_qfile::readSmallFiles()
{
    QFETCH(tst_qfile::BenchmarkType, testType);
    QFETCH(int, blockSize);
    QFETCH(QFile::OpenModeFlag, textMode);
    QFETCH(QFile::OpenModeFlag, bufferedMode);

    QDir dir(tempDir.path());
    const QStringList files = dir.entryList(QDir::NoDotAndDotDot|QDir::NoSymLinks|QDir::Files);
    char buffer[BUFSIZE];

    switch (testType) {
        case QFileBenchmark: {
            QList<QFile*> fileList;
            for (const QString &file : files) {
                QFile *f = new QFile(tempDir.filePath(file));
                Q_UNUSED(f->open(QIODevice::ReadOnly|textMode|bufferedMode));
                fileList.append(f);
            }

            QBENCHMARK {
                for (QFile *const file : std::as_const(fileList)) {
                    while (!file->atEnd())
                       file->read(buffer, blockSize);
                }
            }

            for (QFile *const file : std::as_const(fileList)) {
                file->close();
                delete file;
            }
        }
        break;
#ifdef QT_BUILD_INTERNAL
        case QFSFileEngineBenchmark: {
            QList<QFSFileEngine*> fileList;
            for (const QString &file : files) {
                QFSFileEngine *fse = new QFSFileEngine(tempDir.filePath(file));
                Q_UNUSED(fse->open(QIODevice::ReadOnly | textMode | bufferedMode, std::nullopt));
                fileList.append(fse);
            }

            QBENCHMARK {
                for (QFSFileEngine *const fse : std::as_const(fileList))
                    while (fse->read(buffer, blockSize)) {}
            }

            for (QFSFileEngine *const fse : std::as_const(fileList)) {
                fse->close();
                delete fse;
            }
        }
        break;
#endif
        case PosixBenchmark: {
            QList<FILE *> fileList;
            for (const QString &file : files)
                fileList.append(::fopen(QFile::encodeName(tempDir.filePath(file)).constData(), "rb"));

            QBENCHMARK {
                for (FILE *const cfile : std::as_const(fileList)) {
                    while (!feof(cfile))
                        [[maybe_unused]] auto f = ::fread(buffer, blockSize, 1, cfile);
                    ::fseek(cfile, 0, SEEK_SET);
                }
            }

            for (FILE *const cfile : std::as_const(fileList))
                ::fclose(cfile);
        }
        break;
        case QFileFromPosixBenchmark: {
            // No gain in benchmarking this case
        }
        break;
        case Win32Benchmark: {
#ifdef Q_OS_WIN
            HANDLE hndl;

            // ensure we don't account string conversion
            const wchar_t *cfilename = reinterpret_cast<const wchar_t *>(tempDir.filename.utf16());

            hndl = CreateFile(cfilename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
            Q_ASSERT(hndl);
            wchar_t* nativeBuffer = new wchar_t[BUFSIZE];
            DWORD numberOfBytesRead;
            QBENCHMARK {
                do {
                   ReadFile(hndl, nativeBuffer, blockSize, &numberOfBytesRead, NULL);
                } while(numberOfBytesRead != 0);
            }
            delete nativeBuffer;
            CloseHandle(hndl);
#else
            QFAIL("Not running on a non-Windows platform!");
#endif
        }
        break;
    }
}

QTEST_MAIN(tst_qfile)

#include "tst_bench_qfile.moc"
