/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QEvent>

#ifdef Q_OS_SYMBIAN
#include <apgcli.h>
#include "private/qcore_symbian_p.h"
#endif

class tst_qfileopenevent : public QObject
{
    Q_OBJECT
public:
    tst_qfileopenevent(){}
    ~tst_qfileopenevent();

public slots:
    void initTestCase();

private slots:
    void constructor();
    void fileOpen();
    void handleLifetime();
    void multiOpen();
    void sendAndReceive();
    void external_data();
    void external();

private:
#ifdef Q_OS_SYMBIAN
    RFile createRFile(const TDesC& filename, const TDesC8& content);
#else
    void createFile(const QString &filename, const QByteArray &content);
#endif
    QFileOpenEvent * createFileAndEvent(const QString &filename, const QByteArray &content);
    void checkReadAndWrite(QFileOpenEvent& event, const QByteArray& readContent, const QByteArray& writeContent, bool writeOk);
    QByteArray readFileContent(QFileOpenEvent& event);
    bool appendFileContent(QFileOpenEvent& event, const QByteArray& writeContent);

    bool event(QEvent *);

private:
#ifdef Q_OS_SYMBIAN
    struct AutoRFs : public RFs
    {
        AutoRFs()
        {
            qt_symbian_throwIfError(Connect());
            qt_symbian_throwIfError(ShareProtected());
        }

        ~AutoRFs()
        {
            Close();
        }
    };
    AutoRFs fsSession;
#endif
};

tst_qfileopenevent::~tst_qfileopenevent()
{
};

void tst_qfileopenevent::initTestCase()
{
}

#ifdef Q_OS_SYMBIAN
RFile tst_qfileopenevent::createRFile(const TDesC& filename, const TDesC8& content)
{
    RFile file;
    qt_symbian_throwIfError(file.Replace(fsSession, filename, EFileWrite));
    qt_symbian_throwIfError(file.Write(content));
    return file;
}
#else
void tst_qfileopenevent::createFile(const QString &filename, const QByteArray &content)
{
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();
}
#endif

QFileOpenEvent * tst_qfileopenevent::createFileAndEvent(const QString &filename, const QByteArray &content)
{
#ifdef Q_OS_SYMBIAN
    RFile rFile = createRFile(qt_QString2TPtrC(filename), TPtrC8((TText8*)content.constData(), content.size()));
    QScopedPointer<RFile, QScopedPointerRCloser<RFile> > closeRFile(&rFile);
    return new QFileOpenEvent(rFile);
#else
    createFile(filename, content);
    return new QFileOpenEvent(filename);
#endif
}

void tst_qfileopenevent::constructor()
{
    // check that filename get/set works
    QFileOpenEvent nameTest(QLatin1String("fileNameTest"));
    QCOMPARE(nameTest.file(), QLatin1String("fileNameTest"));

    // check that url get/set works
    QFileOpenEvent urlTest(QUrl(QLatin1String("file:///urlNameTest")));
    QCOMPARE(urlTest.url().toString(), QLatin1String("file:///urlNameTest"));

#ifdef Q_OS_SYMBIAN
    // check that RFile construction works
    RFile rFile = createRFile(_L("testRFile"), _L8("test content"));
    QFileOpenEvent rFileTest(rFile);
    QString targetName(QLatin1String("testRFile"));
    QCOMPARE(rFileTest.file().right(targetName.size()), targetName);
    QCOMPARE(rFileTest.url().toString().right(targetName.size()), targetName);
    rFile.Close();
#endif
}

QByteArray tst_qfileopenevent::readFileContent(QFileOpenEvent& event)
{
    QFile file;
    event.openFile(file, QFile::ReadOnly);
    file.seek(0);
    QByteArray data = file.readAll();
    return data;
}

bool tst_qfileopenevent::appendFileContent(QFileOpenEvent& event, const QByteArray& writeContent)
{
    QFile file;
    bool ok = event.openFile(file, QFile::Append | QFile::Unbuffered);
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
#ifdef Q_OS_SYMBIAN
    // create writeable file
    {
        RFile rFile = createRFile(_L("testFileOpen"), _L8("test content"));
        QFileOpenEvent rFileTest(rFile);
        checkReadAndWrite(rFileTest, QByteArray("test content"), QByteArray("+RFileWrite"), true);
        rFile.Close();
    }

    // open read-only RFile
    {
        RFile rFile;
        int err = rFile.Open(fsSession, _L("testFileOpen"), EFileRead);
        QFileOpenEvent rFileTest(rFile);
        checkReadAndWrite(rFileTest, QByteArray("test content+RFileWrite"), QByteArray("+RFileRead"), false);
        rFile.Close();
    }
#else
    createFile(QLatin1String("testFileOpen"), QByteArray("test content+RFileWrite"));
#endif

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
    QFile qFile;
    QCOMPARE(event->openFile(qFile, QFile::Append | QFile::Unbuffered), true);
    event.reset(0);

    // write to the QFile after the event is closed
    QString writeContent(QLatin1String("+closed original handles"));
    QCOMPARE(int(qFile.write(writeContent.toUtf8())), writeContent.size());
    qFile.close();

    // check the content
    QFile check("testHandleLifetime");
    check.open(QFile::ReadOnly);
    QString content(check.readAll());
    QCOMPARE(content, QLatin1String("test content+closed original handles"));
    check.close();

    QFile::remove(QLatin1String("testHandleLifetime"));
}

void tst_qfileopenevent::multiOpen()
{
    QScopedPointer<QFileOpenEvent> event(createFileAndEvent(QLatin1String("testMultiOpen"), QByteArray("itlum")));

    QFile files[5];
    for (int i=0; i<5; i++) {
        QCOMPARE(event->openFile(files[i], QFile::ReadOnly), true);
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
    QScopedPointer<QFileOpenEvent> event(createFileAndEvent(QLatin1String("testSendAndReceive"), QByteArray("sending")));

    QCoreApplication::instance()->postEvent(this, event.take());
    QCoreApplication::instance()->processEvents();

    // QTBUG-17468: On Mac, processEvents doesn't always process posted events
    QCoreApplication::instance()->sendPostedEvents();

    // check the content
    QFile check("testSendAndReceive");
    QCOMPARE(check.open(QFile::ReadOnly), true);
    QString content(check.readAll());
    QCOMPARE(content, QLatin1String("sending+received"));
    check.close();

    QFile::remove(QLatin1String("testSendAndReceive"));
}

void tst_qfileopenevent::external_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QByteArray>("targetContent");
    QTest::addColumn<bool>("sendHandle");

    QString privateName(QLatin1String("tst_qfileopenevent_external"));
    QString publicName(QLatin1String("C:\\Data\\tst_qfileopenevent_external"));
    QByteArray writeSuccess("original+external");
    QByteArray writeFail("original");
    QTest::newRow("public name") << publicName << writeSuccess << false;
    QTest::newRow("data caged name") << privateName << writeFail << false;
    QTest::newRow("public handle") << publicName << writeSuccess << true;
    QTest::newRow("data caged handle") << privateName << writeSuccess << true;
}

void tst_qfileopenevent::external()
{
#ifndef Q_OS_SYMBIAN
    QSKIP("external app file open test only valid in Symbian", SkipAll);
#else

    QFETCH(QString, filename);
    QFETCH(QByteArray, targetContent);
    QFETCH(bool, sendHandle);

    RFile rFile = createRFile(qt_QString2TPtrC(filename), _L8("original"));

    // launch app with the file
    RApaLsSession apa;
    QCOMPARE(apa.Connect(), KErrNone);
    TThreadId threadId;
    TDataType type(_L8("application/x-tst_qfileopenevent"));
    if (sendHandle) {
        QCOMPARE(apa.StartDocument(rFile, type, threadId), KErrNone);
        rFile.Close();
    } else {
        TFileName fullName;
        rFile.FullName(fullName);
        rFile.Close();
        QCOMPARE(apa.StartDocument(fullName, type, threadId), KErrNone);
    }

    // wait for app exit
    RThread appThread;
    if (appThread.Open(threadId) == KErrNone) {
        TRequestStatus status;
        appThread.Logon(status);
        User::WaitForRequest(status);
    }

    // check the contents
    QFile check(filename);
    QCOMPARE(check.open(QFile::ReadOnly), true);
    QCOMPARE(check.readAll(), targetContent);
    bool ok = check.remove();

    QFile::remove(filename);
#endif
}

QTEST_MAIN(tst_qfileopenevent)
#include "tst_qfileopenevent.moc"
