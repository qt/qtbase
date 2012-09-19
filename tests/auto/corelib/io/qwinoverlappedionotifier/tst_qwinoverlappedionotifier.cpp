/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include <private/qwinoverlappedionotifier_p.h>
#include <qbytearray.h>

class tst_QWinOverlappedIoNotifier : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void readFile_data();
    void readFile();
    void waitForNotified_data();
    void waitForNotified();
    void brokenPipe();

private:
    QFileInfo sourceFileInfo;
    DWORD notifiedBytesRead;
    DWORD notifiedErrorCode;
};

class NotifierSink : public QObject
{
    Q_OBJECT
public:
    NotifierSink(QWinOverlappedIoNotifier *notifier)
        : QObject(notifier),
          notifications(0),
          notifiedBytesRead(0),
          notifiedErrorCode(ERROR_SUCCESS)
    {
        connect(notifier, &QWinOverlappedIoNotifier::notified, this, &NotifierSink::notified);
    }

protected slots:
    void notified(DWORD bytesRead, DWORD errorCode)
    {
        notifications++;
        notifiedBytesRead = bytesRead;
        notifiedErrorCode = errorCode;
        emit notificationReceived();
    }

signals:
    void notificationReceived();

public:
    int notifications;
    DWORD notifiedBytesRead;
    DWORD notifiedErrorCode;
};

void tst_QWinOverlappedIoNotifier::initTestCase()
{
    sourceFileInfo.setFile(QFINDTESTDATA("tst_qwinoverlappedionotifier.cpp"));
    QVERIFY2(sourceFileInfo.exists(), "File tst_qwinoverlappedionotifier.cpp not found.");
}

void tst_QWinOverlappedIoNotifier::readFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("readBufferSize");
    QTest::addColumn<DWORD>("expectedBytesRead");

    QString sourceFileName = QDir::toNativeSeparators(sourceFileInfo.absoluteFilePath());
    int sourceFileSize = sourceFileInfo.size();

    QTest::newRow("read file, less than available")
        << sourceFileName << sourceFileSize / 2 << DWORD(sourceFileSize / 2);
    QTest::newRow("read file, more than available")
        << sourceFileName << sourceFileSize * 2 << DWORD(sourceFileSize);
}

void tst_QWinOverlappedIoNotifier::readFile()
{
    QFETCH(QString, fileName);
    QFETCH(int, readBufferSize);
    QFETCH(DWORD, expectedBytesRead);

    QWinOverlappedIoNotifier notifier;
    NotifierSink sink(&notifier);
    connect(&sink, &NotifierSink::notificationReceived, &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);

    HANDLE hFile = CreateFile(reinterpret_cast<const wchar_t*>(fileName.utf16()),
                              GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    notifier.setHandle(hFile);
    notifier.setEnabled(true);

    OVERLAPPED overlapped = {0};
    QByteArray buffer(readBufferSize, 0);
    BOOL readSuccess = ReadFile(hFile, buffer.data(), buffer.size(), NULL, &overlapped);
    QVERIFY(readSuccess || GetLastError() == ERROR_IO_PENDING);

    QTestEventLoop::instance().enterLoop(3);
    CloseHandle(hFile);
    QCOMPARE(sink.notifications, 1);
    QCOMPARE(sink.notifiedBytesRead, expectedBytesRead);
    QCOMPARE(sink.notifiedErrorCode, DWORD(ERROR_SUCCESS));
}

void tst_QWinOverlappedIoNotifier::waitForNotified_data()
{
    readFile_data();
}

void tst_QWinOverlappedIoNotifier::waitForNotified()
{
    QFETCH(QString, fileName);
    QFETCH(int, readBufferSize);
    QFETCH(DWORD, expectedBytesRead);

    QWinOverlappedIoNotifier notifier;
    NotifierSink sink(&notifier);
    HANDLE hFile = CreateFile(reinterpret_cast<const wchar_t*>(fileName.utf16()),
                              GENERIC_READ, FILE_SHARE_READ,
                              NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    QCOMPARE(notifier.waitForNotified(0), false);
    notifier.setHandle(hFile);
    notifier.setEnabled(true);
    QCOMPARE(notifier.waitForNotified(100), false);

    OVERLAPPED overlapped = {0};
    QByteArray buffer(readBufferSize, 0);
    BOOL readSuccess = ReadFile(hFile, buffer.data(), buffer.size(), NULL, &overlapped);
    QVERIFY(readSuccess || GetLastError() == ERROR_IO_PENDING);

    QCOMPARE(notifier.waitForNotified(3000), true);
    CloseHandle(hFile);
    QCOMPARE(sink.notifications, 1);
    QCOMPARE(sink.notifiedBytesRead, expectedBytesRead);
    QCOMPARE(sink.notifiedErrorCode, DWORD(ERROR_SUCCESS));
    QCOMPARE(notifier.waitForNotified(100), false);
}

void tst_QWinOverlappedIoNotifier::brokenPipe()
{
    QWinOverlappedIoNotifier notifier;
    NotifierSink sink(&notifier);
    connect(&sink, &NotifierSink::notificationReceived, &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);

    wchar_t pipeName[] = L"\\\\.\\pipe\\tst_QWinOverlappedIoNotifier_brokenPipe";
    HANDLE hPipe = CreateNamedPipe(pipeName,
                                   PIPE_ACCESS_DUPLEX,
                                   PIPE_TYPE_BYTE | PIPE_NOWAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                   1, 0, 0, 0, NULL);
    QVERIFY(hPipe != INVALID_HANDLE_VALUE);
    HANDLE hReadEnd = CreateFile(pipeName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    QVERIFY(hReadEnd != INVALID_HANDLE_VALUE);
    notifier.setHandle(hReadEnd);
    notifier.setEnabled(true);

    OVERLAPPED overlapped = {0};
    QByteArray buffer(1024, 0);
    BOOL readSuccess = ReadFile(hReadEnd, buffer.data(), buffer.size(), NULL, &overlapped);
    QVERIFY(readSuccess || GetLastError() == ERROR_IO_PENDING);

    // close the write end of the pipe
    CloseHandle(hPipe);

    QTestEventLoop::instance().enterLoop(3);
    CloseHandle(hReadEnd);
    QCOMPARE(sink.notifications, 1);
    QCOMPARE(sink.notifiedBytesRead, DWORD(0));
    QCOMPARE(sink.notifiedErrorCode, DWORD(ERROR_BROKEN_PIPE));
}

QTEST_MAIN(tst_QWinOverlappedIoNotifier)

#include "tst_qwinoverlappedionotifier.moc"
