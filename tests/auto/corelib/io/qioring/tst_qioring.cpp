// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtCore/private/qioring_p.h>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#include <io.h>
#else
#include <QtCore/private/qcore_unix_p.h>
#endif

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

class tst_QIORing : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void construct();
    void sharedInstance();
    void open();
    void read();
    void write();
    void stat();
    void fiveGiBReadWrite();

private:
    static void closeFile(qintptr fd);
    static qintptr openHelper(QIORing *ring, const QString &path, QIODevice::OpenMode flags);
};

void tst_QIORing::initTestCase()
{
    if (QIORing::sharedInstance() == nullptr)
        QSKIP("QIORing wasn't able to initialize on this platform. Test will be skipped.");
}

void tst_QIORing::closeFile(qintptr fd)
{
#ifdef Q_OS_WIN
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    HANDLE h = HANDLE(fd);
    CloseHandle(h);
#else
    QT_CLOSE(fd);
#endif
}

qintptr tst_QIORing::openHelper(QIORing *ring, const QString &path, QIODevice::OpenMode flags)
{
    QIORingRequest<QIORing::Operation::Open> request;
    request.path = QtPrivate::toFilesystemPath(path);
    request.flags = flags;
    qintptr fd = -1;
    request.setCallback([&fd](const QIORingRequest<QIORing::Operation::Open> &request) {
        const auto *result = std::get_if<QIORingResult<QIORing::Operation::Open>>(&request.result);
        QVERIFY(result);
        fd = result->fd;
    });

    QIORing::RequestHandle handle = ring->queueRequest(std::move(request));
    ring->waitForRequest(handle, 500ms);
    QCOMPARE_GE(fd, 0);
    return fd;
}

void tst_QIORing::construct()
{
    QIORing ring(1, 2);
    QVERIFY(ring.ensureInitialized());

    // Everything must supports the basics:
    QVERIFY(ring.supportsOperation(QIORing::Operation::Read));
    QVERIFY(ring.supportsOperation(QIORing::Operation::Write));
    QVERIFY(ring.supportsOperation(QIORing::Operation::Close));
    QVERIFY(ring.supportsOperation(QIORing::Operation::Open));
    QVERIFY(ring.supportsOperation(QIORing::Operation::Flush));
    QVERIFY(ring.supportsOperation(QIORing::Operation::Cancel));
    QVERIFY(ring.supportsOperation(QIORing::Operation::VectoredRead));
    QVERIFY(ring.supportsOperation(QIORing::Operation::VectoredWrite));

    QCOMPARE_GE(ring.submissionQueueSize(), 1u);
    QCOMPARE_GE(ring.completionQueueSize(), 2u);
}

void tst_QIORing::sharedInstance()
{
    QIORing *shared = QIORing::sharedInstance();
    QVERIFY(shared);
    QCOMPARE_GE(shared->submissionQueueSize(), QIORing::DefaultSubmissionQueueSize);
    QCOMPARE_GE(shared->completionQueueSize(), QIORing::DefaultCompletionQueueSize);
}

void tst_QIORing::open()
{
    QString sourceDir = QFINDTESTDATA("data");
    QIORing ring;
    QVERIFY(ring.ensureInitialized());

    QIORingRequest<QIORing::Operation::Open> openRequest;
    openRequest.path = QtPrivate::toFilesystemPath(sourceDir + "/input.txt"_L1);
    openRequest.flags = QIODevice::ReadOnly | QIODevice::ExistingOnly;
    qintptr fd = -1;
    openRequest.setCallback([&fd](const QIORingRequest<QIORing::Operation::Open> &request) {
        if (request.result.index() == 1) {
            const auto &result = std::get<QIORingResult<QIORing::Operation::Open>>(request.result);
            QCOMPARE_GE(result.fd, 0);
            fd = result.fd;
        } else {
            const auto &error = std::get<QFileDevice::FileError>(request.result);
            QFAIL(qPrintable("Failed to open file: %1"_L1.arg(QString::number(int(error)))));
        }
    });
    QIORing::RequestHandle handle = ring.queueRequest(std::move(openRequest));
    ring.waitForRequest(handle, 500ms);
    QVERIFY(fd >= 0);
    closeFile(fd);
}

void tst_QIORing::read()
{
    QFile file(QFINDTESTDATA("data/input.txt"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    int fd = file.handle();
#ifdef Q_OS_WIN
    qintptr nativeFd = _get_osfhandle(fd);
#else
    qintptr nativeFd = fd;
#endif

    QIORing ring;
    QVERIFY(ring.ensureInitialized());
    QIORingRequest<QIORing::Operation::Read> readRequest;
    readRequest.fd = nativeFd;
    readRequest.offset = sizeof("lorem ") - 1;
    std::array<std::byte, sizeof("ipsum") - 1> buffer{};
    readRequest.destination = buffer;
    qint64 bytesRead = 0;
    readRequest.setCallback([&bytesRead](const QIORingRequest<QIORing::Operation::Read> &request) {
        const auto *result = std::get_if<QIORingResult<QIORing::Operation::Read>>(&request.result);
        QVERIFY(result);
        bytesRead = result->bytesRead;
    });
    QIORing::RequestHandle handle = ring.queueRequest(std::move(readRequest));
    QVERIFY(ring.waitForRequest(handle));
    QCOMPARE(bytesRead, sizeof("ipsum") - 1);
    QCOMPARE(QLatin1StringView(buffer), "ipsum");
}

void tst_QIORing::write()
{
    QIORing ring;
    QVERIFY(ring.ensureInitialized());

    QTemporaryDir dir;
    auto path = dir.filePath("out");

    auto fd = openHelper(&ring, path, QIODevice::ReadWrite);
    auto cleanup = qScopeGuard([fd](){
        closeFile(fd);
    });

    QIORingRequest<QIORing::Operation::Write> writeRequest;
    writeRequest.fd = fd;
    writeRequest.offset = 0;
    QByteArray buffer(1024 * 1024 * 10, 'a');
    QSpan span = buffer;
    writeRequest.source = as_bytes(span);

    qint64 bytesWritten = 0;
    writeRequest.setCallback( //
            [&bytesWritten](const QIORingRequest<QIORing::Operation::Write> &request) {
                const auto *result = std::get_if<QIORingResult<QIORing::Operation::Write>>(
                        &request.result);
                QVERIFY(result);
                bytesWritten = result->bytesWritten;
            });
    QIORing::RequestHandle handle = ring.queueRequest(std::move(writeRequest));
    QVERIFY(ring.waitForRequest(handle));
    QCOMPARE(bytesWritten, buffer.size());

    // And read back again:
    QIORingRequest<QIORing::Operation::Read> readRequest;
    readRequest.fd = fd;
    readRequest.offset = 0;
    std::fill(buffer.begin(), buffer.end(), '\0');
    readRequest.destination = as_writable_bytes(span);

    qint64 bytesRead = 0;
    readRequest.setCallback([&bytesRead](const QIORingRequest<QIORing::Operation::Read> &request) {
        const auto *result = std::get_if<QIORingResult<QIORing::Operation::Read>>(&request.result);
        QVERIFY(result);
        bytesRead = result->bytesRead;
    });
    handle = ring.queueRequest(std::move(readRequest));
    QVERIFY(ring.waitForRequest(handle));
    QCOMPARE(bytesRead, buffer.size());
    QVERIFY(std::all_of(buffer.begin(), buffer.end(), [](char ch) { return ch == 'a'; }));
}

void tst_QIORing::stat()
{
    QIORing ring;
    auto fd = openHelper(&ring, QFINDTESTDATA("data/input.txt"), QIODevice::ReadOnly);

    QVERIFY(ring.ensureInitialized());
    QIORingRequest<QIORing::Operation::Stat> statRequest;
    statRequest.fd = fd;
    quint64 size = 0;
    statRequest.setCallback([&size](const QIORingRequest<QIORing::Operation::Stat> &request) {
        const auto *result = std::get_if<QIORingResult<QIORing::Operation::Stat>>(&request.result);
        QVERIFY(result);
        size = result->size;
    });
    QIORing::RequestHandle handle = ring.queueRequest(std::move(statRequest));
    QVERIFY(ring.waitForRequest(handle));
    QCOMPARE(size, 11);
}

void tst_QIORing::fiveGiBReadWrite()
{
#if Q_PROCESSOR_WORDSIZE < 8
    QSKIP("Can't test this on 32-bit.");
#else
    static constexpr qsizetype Size = 5ll * 1024 * 1024 * 1024;
    std::unique_ptr<std::byte[]> bytes(new (std::nothrow) std::byte[Size]);
    if (!bytes)
        QSKIP("Failed to allocate the buffer (not enough memory?)");
    std::fill_n(bytes.get(), Size, std::byte(242));

    QIORing ring;
    QVERIFY(ring.ensureInitialized());

    QTemporaryDir dir;
    auto path = dir.filePath("largefile");

    auto fd = openHelper(&ring, path, QIODevice::ReadWrite);
    auto cleanup = qScopeGuard([fd]() { closeFile(fd); });

    QIORingRequest<QIORing::Operation::Write> writeRequest;
    writeRequest.fd = fd;
    writeRequest.offset = 0;
    QSpan span = QSpan(bytes.get(), Size);
    writeRequest.source = span;

    quint64 bytesWritten = 0;
    writeRequest.setCallback( //
            [&bytesWritten](const QIORingRequest<QIORing::Operation::Write> &request) {
                auto *result = std::get_if<QIORingResult<QIORing::Operation::Write>>(
                        &request.result);
                QVERIFY(result);
                bytesWritten = result->bytesWritten;
                QCOMPARE(bytesWritten, Size);
            });
    QIORing::RequestHandle handle = ring.queueRequest(std::move(writeRequest));
    QVERIFY(ring.waitForRequest(handle));

    // And read back again:
    QIORingRequest<QIORing::Operation::Read> readRequest;
    readRequest.fd = fd;
    readRequest.offset = 0;
    std::fill_n(bytes.get(), Size, std::byte('\0'));
    readRequest.destination = span;

    quint64 bytesRead = 0;
    readRequest.setCallback([&bytesRead](const QIORingRequest<QIORing::Operation::Read> &request) {
        const auto *result = std::get_if<QIORingResult<QIORing::Operation::Read>>(&request.result);
        QVERIFY(result);
        bytesRead = result->bytesRead;
        QCOMPARE(bytesRead, Size);
    });
    handle = ring.queueRequest(std::move(readRequest));
    QVERIFY(ring.waitForRequest(handle));
    QVERIFY(std::all_of(bytes.get(), bytes.get() + Size,
                        [](std::byte ch) { return ch == std::byte(242); }));
#endif
}

QTEST_MAIN(tst_QIORing)
#include <tst_qioring.moc>
