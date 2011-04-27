/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QStandardItemModel>
#include <qdebug.h>
#include <qdesktopservices.h>

//#define RUN_MANUAL_TESTS
//TESTED_CLASS=
//TESTED_FILES=

class tst_qdesktopservices : public QObject {
  Q_OBJECT

public:
    tst_qdesktopservices();
    virtual ~tst_qdesktopservices();

private slots:
    void init();
    void cleanup();
    void openUrl();
#ifdef Q_OS_SYMBIAN
    // These test are manual ones, you need to check from  device that
    // correct system application is started with correct content
    // When you want to run these test, uncomment //#define RUN_MANUAL_TESTS
    void openHttpUrl_data();
    void openHttpUrl();
    void openMailtoUrl_data();
    void openMailtoUrl();
    void openFileUrl_data();
    void openFileUrl();
    void openMultipleFileUrls();
#endif
    void handlers();
    void storageLocation_data();
    void storageLocation();

    void storageLocationDoesNotEndWithSlash_data();
    void storageLocationDoesNotEndWithSlash();

protected:
};

tst_qdesktopservices::tst_qdesktopservices()
{
    QCoreApplication::setOrganizationName("Nokia");
    QCoreApplication::setApplicationName("tst_qdesktopservices");
}

tst_qdesktopservices::~tst_qdesktopservices()
{
}

void tst_qdesktopservices::init()
{
}

void tst_qdesktopservices::cleanup()
{
}

void tst_qdesktopservices::openUrl()
{
    // At the bare minimum check that they return false for invalid url's
    QCOMPARE(QDesktopServices::openUrl(QUrl()), false);
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    // this test is only valid on windows on other systems it might mean open a new document in the application handling .file
    QCOMPARE(QDesktopServices::openUrl(QUrl("file://invalid.file")), false);
#endif
}

#ifdef Q_OS_SYMBIAN
void tst_qdesktopservices::openHttpUrl_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("result");
    QTest::newRow("BasicWithHttp") << QUrl("http://www.google.fi") << true;
    QTest::newRow("BasicWithoutHttp") << QUrl("www.nokia.fi") << true;
    QTest::newRow("BasicWithUserAndPw") << QUrl("http://s60prereleases:oslofjord@pepper.troll.no/s60prereleases/patches/") << true;
    QTest::newRow("URL with space") << QUrl("http://www.manataka.org/Contents Page.html") << true;

}

void tst_qdesktopservices::openHttpUrl()
{
#ifndef RUN_MANUAL_TESTS
    QSKIP("Test disabled -- only for manual purposes", SkipAll);
#endif

    QFETCH(QUrl, url);
    QFETCH(bool, result);
    QCOMPARE(QDesktopServices::openUrl(url), result);
    QTest::qWait(30000);
}

void tst_qdesktopservices::openMailtoUrl_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("result");

    // http://en.wikipedia.org/wiki/E-mail_address
    // RFC Valid e-mail addresses
    QTest::newRow("Wiki valid email 1") << QUrl("mailto:abc@example.com") << true;
    QTest::newRow("Wiki valid email 2") << QUrl("mailto:Abc@example.com") << true;
    QTest::newRow("Wiki valid email 3") << QUrl("mailto:aBC@example.com") << true;
    QTest::newRow("Wiki valid email 4") << QUrl("mailto:abc.123@example.com") << true;
    QTest::newRow("Wiki valid email 5") << QUrl("mailto:1234567890@example.com") << true;
    QTest::newRow("Wiki valid email 6") << QUrl("mailto:_______@example.com") << true;
    QTest::newRow("Wiki valid email 7") << QUrl("mailto:abc+mailbox/department=shipping@example.com") << true;
    // S60 email client considers the next URL invalid, even ity should be valid
    QTest::newRow("Wiki valid email 8") << QUrl("mailto:!#$%&'*+-/=?^_`.{|}~@example.com") << true; // all of these characters are allowed
    QTest::newRow("Wiki valid email 9") << QUrl("mailto:\"abc@def\"@example.com") << true; // anything goes inside quotation marks
    QTest::newRow("Wiki valid email 10") << QUrl("mailto:\"Fred \\\"quota\\\" Bloggs\"@example.com") << true; // however, quotes need escaping

    // RFC invalid e-mail addresses
    // These return true even though they are invalid, but check that user is notified about invalid URL in mail application
    QTest::newRow("Wiki invalid email 1") << QUrl("mailto:Abc.example.com") << true;        // character @ is missing
    QTest::newRow("Wiki invalid email 2") << QUrl("mailto:Abc.@example.com") << true;        // character dot(.) is last in local part
    QTest::newRow("Wiki invalid email 3") << QUrl("mailto:Abc..123@example.com") << true;    // character dot(.) is double
    QTest::newRow("Wiki invalid email 4") << QUrl("mailto:A@b@c@example.com") << true;        // only one @ is allowed outside quotations marks
    QTest::newRow("Wiki invalid email 5") << QUrl("mailto:()[]\\;:,<>@example.com") << true;    // none of the characters before the @ is allowed outside quotation marks

    QTest::newRow("Basic") << QUrl("mailto:test@nokia.com") << true;
    QTest::newRow("BasicSeveralAddr") << QUrl("mailto:test@nokia.com,test2@nokia.com,test3@nokia.com") << true;
    QTest::newRow("BasicAndSubject") << QUrl("mailto:test@nokia.com?subject=hello nokia") << true;
    QTest::newRow("BasicAndTo") << QUrl("mailto:test@nokia.com?to=test2@nokia.com") << true;

    QTest::newRow("BasicAndCc") << QUrl("mailto:test@nokia.com?cc=mycc@address.com") << true;
    QTest::newRow("BasicAndBcc") << QUrl("mailto:test@nokia.com?bcc=mybcc@address.com") << true;
    QTest::newRow("BasicAndBody") << QUrl("mailto:test@nokia.com?body=Test email message body") << true;

    // RFC examples, these are actually invalid because there is not host defined
    // Check that user is notified about invalid URL in mail application
    QTest::newRow("RFC2368 Example 1") << QUrl::fromEncoded("mailto:addr1%2C%20addr2") << true;
    QTest::newRow("RFC2368 Example 2") << QUrl::fromEncoded("mailto:?to=addr1%2C%20addr2") << true;
    QTest::newRow("RFC2368 Example 3") << QUrl("mailto:addr1?to=addr2") << true;

    QTest::newRow("RFC2368 Example 4") << QUrl("mailto:joe@example.com?cc=bob@example.com&body=hello") << true;
    QTest::newRow("RFC2368 Example 5") << QUrl("mailto:?to=joe@example.com&cc=bob@example.com&body=hello") << true;
    QTest::newRow("RFC2368 Example 6") << QUrl("mailto:foobar@example.com?In-Reply-To=%3c3469A91.D10AF4C@example.com") << true; // OpaqueData
    QTest::newRow("RFC2368 Example 7") << QUrl::fromEncoded("mailto:infobot@example.com?body=send%20current-issue%0D%0Asend%20index") << true;
    QTest::newRow("RFC2368 Example 8") << QUrl::fromEncoded("mailto:infobot@example.com?body=send%20current-issue") << true;
    QTest::newRow("RFC2368 Example 9") << QUrl("mailto:infobot@example.com?subject=current-issue") << true;
    QTest::newRow("RFC2368 Example 10") << QUrl("mailto:chris@example.com") << true;

    //QTest::newRow("RFC2368 Example 11 - illegal chars") << QUrl("mailto:joe@example.com?cc=bob@example.com?body=hello") << false;
    QTest::newRow("RFC2368 Example 12") << QUrl::fromEncoded("mailto:gorby%25kremvax@example.com") << true; // encoded reserved chars '%'
    QTest::newRow("RFC2368 Example 13") << QUrl::fromEncoded("mailto:unlikely%3Faddress@example.com?blat=foop") << true;    // encoded reserved chars  `?'
}

void tst_qdesktopservices::openMailtoUrl()
{
#ifndef RUN_MANUAL_TESTS
    QSKIP("Test disabled -- only for manual purposes", SkipAll);
#endif

    QFETCH(QUrl, url);
    QFETCH(bool, result);
    QCOMPARE(QDesktopServices::openUrl(url), result);
    QTest::qWait(5000);
}

void tst_qdesktopservices::openFileUrl_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("result");

    // Text files
    QTest::newRow("DOS text file") << QUrl("file:///c:/data/others/dosfile.txt") << true;
    QTest::newRow("No EOF text file") << QUrl("file:///c:/data/others/noendofline.txt") << true;
    QTest::newRow("text file") << QUrl("file:///c:/data/others/testfile.txt") << true;
    QTest::newRow("text file with space") << QUrl("file:///c:/data/others/test file.txt") << true;

    // Images
    QTest::newRow("BMP image") << QUrl("file:///c:/data/images/image.bmp") << true;
    QTest::newRow("GIF image") << QUrl("file:///c:/data/images/image.gif") << true;
    QTest::newRow("JPG image") << QUrl("file:///c:/data/images/image.jpg") << true;
    QTest::newRow("PNG image") << QUrl("file:///c:/data/images/image.png") << true;

    // Audio
    QTest::newRow("MP4 audio") << QUrl("file:///c:/data/sounds/aac-only.mp4") << true;
    QTest::newRow("3GP audio") << QUrl("file:///c:/data/sounds/audio_3gpp.3gp") << true;

    // Video
    QTest::newRow("MP4 video") << QUrl("file:///c:/data/videos/vid-mpeg4-22k.mp4") << true;

    // Installs
    QTest::newRow("SISX") << QUrl("file:///c:/data/installs/ErrRd.sisx") << true;

    // Errors
    QTest::newRow("File does not exist") << QUrl("file:///c:/thisfileneverexists.txt") << false;
}

void tst_qdesktopservices::openFileUrl()
{
#ifndef RUN_MANUAL_TESTS
    QSKIP("Test disabled -- only for manual purposes", SkipAll);
#endif

    QFETCH(QUrl, url);
    QFETCH(bool, result);
    QCOMPARE(QDesktopServices::openUrl(url), result);
    QTest::qWait(5000);
}

void tst_qdesktopservices::openMultipleFileUrls()
{
#ifndef RUN_MANUAL_TESTS
    QSKIP("Test disabled -- only for manual purposes", SkipAll);
#endif

    QCOMPARE(QDesktopServices::openUrl(QUrl("file:///c:/data/images/image.bmp")), true);
    QCOMPARE(QDesktopServices::openUrl(QUrl("file:///c:/data/images/image.png")), true);
    QCOMPARE(QDesktopServices::openUrl(QUrl("file:///c:/data/others/noendofline.txt")), true); 
    QCOMPARE(QDesktopServices::openUrl(QUrl("file:///c:/data/installs/ErrRd.sisx")), true);      
}
#endif


class MyUrlHandler : public QObject
{
    Q_OBJECT
public:
    QUrl lastHandledUrl;

public slots:
    inline void handle(const QUrl &url) {
        lastHandledUrl = url;
    }
};

void tst_qdesktopservices::handlers()
{
    MyUrlHandler fooHandler;
    MyUrlHandler barHandler;

    QDesktopServices::setUrlHandler(QString("foo"), &fooHandler, "handle");
    QDesktopServices::setUrlHandler(QString("bar"), &barHandler, "handle");

    QUrl fooUrl("foo://blub/meh");
    QUrl barUrl("bar://hmm/hmmmm");

    QVERIFY(QDesktopServices::openUrl(fooUrl));
    QVERIFY(QDesktopServices::openUrl(barUrl));

    QCOMPARE(fooHandler.lastHandledUrl.toString(), fooUrl.toString());
    QCOMPARE(barHandler.lastHandledUrl.toString(), barUrl.toString());
}

Q_DECLARE_METATYPE(QDesktopServices::StandardLocation)
void tst_qdesktopservices::storageLocation_data()
{
    QTest::addColumn<QDesktopServices::StandardLocation>("location");
    QTest::newRow("DesktopLocation") << QDesktopServices::DesktopLocation;
    QTest::newRow("DocumentsLocation") << QDesktopServices::DocumentsLocation;
    QTest::newRow("FontsLocation") << QDesktopServices::FontsLocation;
    QTest::newRow("ApplicationsLocation") << QDesktopServices::ApplicationsLocation;
    QTest::newRow("MusicLocation") << QDesktopServices::MusicLocation;
    QTest::newRow("MoviesLocation") << QDesktopServices::MoviesLocation;
    QTest::newRow("PicturesLocation") << QDesktopServices::PicturesLocation;
    QTest::newRow("TempLocation") << QDesktopServices::TempLocation;
    QTest::newRow("HomeLocation") << QDesktopServices::HomeLocation;
    QTest::newRow("DataLocation") << QDesktopServices::DataLocation;
}

void tst_qdesktopservices::storageLocation()
{
    QFETCH(QDesktopServices::StandardLocation, location);
#ifdef Q_OS_SYMBIAN
    QString storageLocation = QDesktopServices::storageLocation(location);
    QString displayName = QDesktopServices::displayName(location);
    //qDebug( "displayName: %s",  displayName );

    storageLocation = storageLocation.toLower();
    displayName = displayName.toLower();

    QString drive = QDir::currentPath().left(2).toLower();
    if( drive == "z:" )
        drive = "c:";

    switch(location) {
    case QDesktopServices::DesktopLocation:
        QCOMPARE( storageLocation, drive + QString("/data") );
        break;
    case QDesktopServices::DocumentsLocation:
        QCOMPARE( storageLocation, drive + QString("/data") );
        break;
    case QDesktopServices::FontsLocation:
        // Currently point always to ROM
        QCOMPARE( storageLocation, QString("z:/resource/fonts") );
        break;
    case QDesktopServices::ApplicationsLocation:
#ifdef Q_CC_NOKIAX86
        QCOMPARE( storageLocation, QString("z:/sys/bin") );
#else
        QCOMPARE( storageLocation, drive + QString("/sys/bin") );
#endif
        break;
    case QDesktopServices::MusicLocation:
        QCOMPARE( storageLocation, drive + QString("/data/sounds") );
        break;
    case QDesktopServices::MoviesLocation:
        QCOMPARE( storageLocation, drive + QString("/data/videos") );
        break;
    case QDesktopServices::PicturesLocation:
        QCOMPARE( storageLocation, drive + QString("/data/images") );
        break;
    case QDesktopServices::TempLocation:
        QCOMPARE( storageLocation, QDir::tempPath().toLower());
        break;
    case QDesktopServices::HomeLocation:
        QCOMPARE( storageLocation, QDir::homePath().toLower());
        break;
    case QDesktopServices::DataLocation:
        // Just check the folder not the drive
        QCOMPARE( storageLocation.mid(2), QDir::currentPath().mid(2).toLower());
        break;
    default:
        QCOMPARE( storageLocation, QString() );
        break;
    }

#else
    QDesktopServices::storageLocation(location);
    QDesktopServices::displayName(location);
#endif
}


void tst_qdesktopservices::storageLocationDoesNotEndWithSlash_data()
{
    storageLocation_data();
}

void tst_qdesktopservices::storageLocationDoesNotEndWithSlash()
{
    // Currently all desktop locations return their storage location
    // with "Unix-style" paths (i.e. they use a slash, not backslash).
    QFETCH(QDesktopServices::StandardLocation, location);
    QString loc = QDesktopServices::storageLocation(location);
    if (loc.size() > 1)  // workaround for unlikely case of locations that return '/'
        QCOMPARE(loc.endsWith(QLatin1Char('/')), false);
}


QTEST_MAIN(tst_qdesktopservices)
#include "tst_qdesktopservices.moc"
