// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QTemporaryDir>
#include <QTest>
#include <QtGui/qevent.h>

class tst_qfileopenevent : public QObject
{
    Q_OBJECT
public:
    tst_qfileopenevent(){}
    ~tst_qfileopenevent();

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void constructor();
    void fileOpen();
    void handleLifetime();
    void multiOpen();
    void sendAndReceive();

private:
    void createFile(const QString &filename, const QByteArray &content);
    QFileOpenEvent * createFileAndEvent(const QString &filename, const QByteArray &content);
    void checkReadAndWrite(QFileOpenEvent& event, const QByteArray& readContent, const QByteArray& writeContent, bool writeOk);
    QByteArray readFileContent(QFileOpenEvent& event);
    bool appendFileContent(QFileOpenEvent& event, const QByteArray& writeContent);

    bool event(QEvent *) override;

    QTemporaryDir m_temporaryDir;
    QString m_originalCurrent;
};

tst_qfileopenevent::~tst_qfileopenevent()
{
};

void tst_qfileopenevent::initTestCase()
{
    m_originalCurrent = QDir::currentPath();
    QDir::setCurrent(m_temporaryDir.path());
}

void tst_qfileopenevent::cleanupTestCase()
{
    QDir::setCurrent(m_originalCurrent);
}

void tst_qfileopenevent::createFile(const QString &filename, const QByteArray &content)
{
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();
}

QFileOpenEvent * tst_qfileopenevent::createFileAndEvent(const QString &filename, const QByteArray &content)
{
    createFile(filename, content);
    return new QFileOpenEvent(filename);
}

void tst_qfileopenevent::constructor()
{
    // check that filename get/set works
    QFileOpenEvent nameTest(QLatin1String("fileNameTest"));
    QCOMPARE(nameTest.file(), QLatin1String("fileNameTest"));

    // check that url get/set works
    QFileOpenEvent urlTest(QUrl(QLatin1String("file:///urlNameTest")));
    QCOMPARE(urlTest.url().toString(), QLatin1String("file:///urlNameTest"));
}

QByteArray tst_qfileopenevent::readFileContent(QFileOpenEvent& event)
{
    QFile file(event.file());
    file.open(QFile::ReadOnly);
    file.seek(0);
    QByteArray data = file.readAll();
    return data;
}

bool tst_qfileopenevent::appendFileContent(QFileOpenEvent& event, const QByteArray& writeContent)
{
    QFile file(event.file());
    bool ok = file.open(QFile::Append | QFile::Unbuffered);
    if (ok)
        ok = file.write(writeContent) == writeContent.size();
    return ok;
}

void tst_qfileopenevent::checkReadAndWrite(QFileOpenEvent& event, const QByteArray& readContent, const QByteArray& writeContent, bool writeOk)
{
    QCOMPARE(readFileContent(event), readContent);
    QCOMPARE(appendFileContent(event, writeContent), writeOk);
    QCOMPARE(readFileContent(event), writeOk ? readContent+writeContent : readContent);
}

void tst_qfileopenevent::fileOpen()
{
    createFile(QLatin1String("testFileOpen"), QByteArray("test content+RFileWrite"));

    // filename event
    QUrl fileUrl;   // need to get the URL during the file test, for use in the URL test
    {
        QFileOpenEvent nameTest(QLatin1String("testFileOpen"));
        fileUrl = nameTest.url();
        checkReadAndWrite(nameTest, QByteArray("test content+RFileWrite"), QByteArray("+nameWrite"), true);
    }

    // url event
    {
        QFileOpenEvent urlTest(fileUrl);
        checkReadAndWrite(urlTest, QByteArray("test content+RFileWrite+nameWrite"), QByteArray("+urlWrite"), true);
    }

    QFile::remove(QLatin1String("testFileOpen"));
}

void tst_qfileopenevent::handleLifetime()
{
    QScopedPointer<QFileOpenEvent> event(createFileAndEvent(QLatin1String("testHandleLifetime"), QByteArray("test content")));

    // open a QFile after the original RFile is closed
    QFile qFile(event->file());
    QVERIFY(qFile.open(QFile::Append | QFile::Unbuffered));
    event.reset(0);

    // write to the QFile after the event is closed
    QString writeContent(QLatin1String("+closed original handles"));
    QCOMPARE(int(qFile.write(writeContent.toUtf8())), writeContent.size());
    qFile.close();

    // check the content
    QFile checkContent("testHandleLifetime");
    checkContent.open(QFile::ReadOnly);
    QString content(checkContent.readAll());
    QCOMPARE(content, QLatin1String("test content+closed original handles"));
    checkContent.close();

    QFile::remove(QLatin1String("testHandleLifetime"));
}

void tst_qfileopenevent::multiOpen()
{
    QScopedPointer<QFileOpenEvent> event(createFileAndEvent(QLatin1String("testMultiOpen"), QByteArray("itlum")));

    QFile files[5];
    for (int i=0; i<5; i++) {
        files[i].setFileName(event->file());
        QVERIFY(files[i].open(QFile::ReadOnly));
    }
    for (int i=0; i<5; i++)
        files[i].seek(i);
    QString str;
    for (int i=4; i>=0; i--) {
        char c;
        files[i].getChar(&c);
        str.append(c);
        files[i].close();
    }
    QCOMPARE(str, QLatin1String("multi"));

    QFile::remove(QLatin1String("testMultiOpen"));
}

bool tst_qfileopenevent::event(QEvent *event)
{
    if (event->type() != QEvent::FileOpen)
        return QObject::event(event);
    QFileOpenEvent* fileOpenEvent = static_cast<QFileOpenEvent *>(event);
    appendFileContent(*fileOpenEvent, "+received");
    return true;
}

void tst_qfileopenevent::sendAndReceive()
{
    std::unique_ptr<QFileOpenEvent> event(createFileAndEvent(QLatin1String("testSendAndReceive"), QByteArray("sending")));

    QCoreApplication::instance()->postEvent(this, event.release());
    QCoreApplication::instance()->processEvents();

    // QTBUG-17468: On Mac, processEvents doesn't always process posted events
    QCoreApplication::instance()->sendPostedEvents();

    // check the content
    QFile checkContent("testSendAndReceive");
    QCOMPARE(checkContent.open(QFile::ReadOnly), true);
    QString content(checkContent.readAll());
    QCOMPARE(content, QLatin1String("sending+received"));
    checkContent.close();

    QFile::remove(QLatin1String("testSendAndReceive"));
}

QTEST_MAIN(tst_qfileopenevent)
#include "tst_qfileopenevent.moc"
