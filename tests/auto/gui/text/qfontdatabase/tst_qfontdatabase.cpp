/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qfontdatabase.h>
#include <qfontinfo.h>
#include <qfontmetrics.h>
#include <qtextlayout.h>
#include <private/qrawfont_p.h>
#include <qpa/qplatformfontdatabase.h>

class tst_QFontDatabase : public QObject
{
Q_OBJECT

public:
    tst_QFontDatabase();
    virtual ~tst_QFontDatabase();

public slots:
    void initTestCase();
    void init();
    void cleanup();
private slots:
    void styles_data();
    void styles();

    void fixedPitch_data();
    void fixedPitch();

#ifdef Q_OS_MAC
    void trickyFonts_data();
    void trickyFonts();
#endif

    void widthTwoTimes_data();
    void widthTwoTimes();

    void addAppFont_data();
    void addAppFont();

    void aliases();
    void fallbackFonts();

private:
    const QString m_testFont;
};

tst_QFontDatabase::tst_QFontDatabase()
    : m_testFont(QFINDTESTDATA("LED_REAL.TTF"))
{
}

tst_QFontDatabase::~tst_QFontDatabase()
{

}

void tst_QFontDatabase::initTestCase()
{
    QVERIFY(!m_testFont.isEmpty());
}

void tst_QFontDatabase::init()
{
// TODO: Add initialization code here.
// This will be executed immediately before each test is run.
}

void tst_QFontDatabase::cleanup()
{
// TODO: Add cleanup code here.
// This will be executed immediately after each test is run.
}

void tst_QFontDatabase::styles_data()
{
    QTest::addColumn<QString>("font");

    QTest::newRow( "data0" ) << QString( "Times New Roman" );
}

void tst_QFontDatabase::styles()
{
    QFETCH( QString, font );

    QFontDatabase fdb;
    QStringList styles = fdb.styles( font );
    QStringList::Iterator it = styles.begin();
    while ( it != styles.end() ) {
        QString style = *it;
        QString trimmed = style.trimmed();
        ++it;

        QCOMPARE(style, trimmed);
    }
}

void tst_QFontDatabase::fixedPitch_data()
{
    QTest::addColumn<QString>("font");
    QTest::addColumn<bool>("fixedPitch");

    QTest::newRow( "Times New Roman" ) << QString( "Times New Roman" ) << false;
    QTest::newRow( "Arial" ) << QString( "Arial" ) << false;
    QTest::newRow( "Andale Mono" ) << QString( "Andale Mono" ) << true;
    QTest::newRow( "Courier" ) << QString( "Courier" ) << true;
    QTest::newRow( "Courier New" ) << QString( "Courier New" ) << true;
#ifndef Q_OS_MAC
    QTest::newRow( "Script" ) << QString( "Script" ) << false;
    QTest::newRow( "Lucida Console" ) << QString( "Lucida Console" ) << true;
    QTest::newRow( "DejaVu Sans" ) << QString( "DejaVu Sans" ) << false;
    QTest::newRow( "DejaVu Sans Mono" ) << QString( "DejaVu Sans Mono" ) << true;
#else
    QTest::newRow( "Menlo" ) << QString( "Menlo" ) << true;
    QTest::newRow( "Monaco" ) << QString( "Monaco" ) << true;
#endif
}

void tst_QFontDatabase::fixedPitch()
{
    QFETCH(QString, font);
    QFETCH(bool, fixedPitch);

    QFontDatabase fdb;
    if (!fdb.families().contains(font))
        QSKIP("Font not installed");

    QCOMPARE(fdb.isFixedPitch(font), fixedPitch);

    QFont qfont(font);
    QFontInfo fi(qfont);
    QCOMPARE(fi.fixedPitch(), fixedPitch);
}

#ifdef Q_OS_MAC
void tst_QFontDatabase::trickyFonts_data()
{
    QTest::addColumn<QString>("font");

    QTest::newRow( "Geeza Pro" ) << QString( "Geeza Pro" );
}

void tst_QFontDatabase::trickyFonts()
{
    QFETCH(QString, font);

    QFontDatabase fdb;
    if (!fdb.families().contains(font))
        QSKIP( "Font not installed");

    QFont qfont(font);
    QFontInfo fi(qfont);
    QCOMPARE(fi.family(), font);
}
#endif

void tst_QFontDatabase::widthTwoTimes_data()
{
    QTest::addColumn<QString>("font");
    QTest::addColumn<int>("pixelSize");
    QTest::addColumn<QString>("text");

    QTest::newRow("Arial") << QString("Arial") << 1000 << QString("Some text");
}

void tst_QFontDatabase::widthTwoTimes()
{
    QFETCH(QString, font);
    QFETCH(int, pixelSize);
    QFETCH(QString, text);

    QFont f;
    f.setFamily(font);
    f.setPixelSize(pixelSize);

    QFontMetrics fm(f);
    int w1 = fm.charWidth(text, 0);
    int w2 = fm.charWidth(text, 0);

    QCOMPARE(w1, w2);
}

void tst_QFontDatabase::addAppFont_data()
{
    QTest::addColumn<bool>("useMemoryFont");
    QTest::newRow("font file") << false;
    QTest::newRow("memory font") << true;
}

void tst_QFontDatabase::addAppFont()
{
    QFETCH(bool, useMemoryFont);
    QSignalSpy fontDbChangedSpy(QGuiApplication::instance(), SIGNAL(fontDatabaseChanged()));

    QFontDatabase db;

    const QStringList oldFamilies = db.families();
    QVERIFY(!oldFamilies.isEmpty());

    fontDbChangedSpy.clear();

    int id;
    if (useMemoryFont) {
        QFile fontfile(m_testFont);
        fontfile.open(QIODevice::ReadOnly);
        QByteArray fontdata = fontfile.readAll();
        QVERIFY(!fontdata.isEmpty());
        id = QFontDatabase::addApplicationFontFromData(fontdata);
    } else {
        id = QFontDatabase::addApplicationFont(m_testFont);
    }
#if defined(Q_OS_HPUX) && defined(QT_NO_FONTCONFIG)
    // Documentation says that X11 systems that don't have fontconfig
    // don't support application fonts.
    QCOMPARE(id, -1);
    return;
#endif
    QCOMPARE(fontDbChangedSpy.count(), 1);
    if (id == -1)
        QSKIP("Skip the test since app fonts are not supported on this system");

    const QStringList addedFamilies = QFontDatabase::applicationFontFamilies(id);
    QVERIFY(!addedFamilies.isEmpty());

    const QStringList newFamilies = db.families();
    QVERIFY(!newFamilies.isEmpty());
    QVERIFY(newFamilies.count() >= oldFamilies.count());

    for (int i = 0; i < addedFamilies.count(); ++i)
        QVERIFY(newFamilies.contains(addedFamilies.at(i)));

    QVERIFY(QFontDatabase::removeApplicationFont(id));
    QCOMPARE(fontDbChangedSpy.count(), 2);

    QCOMPARE(db.families(), oldFamilies);
}

void tst_QFontDatabase::aliases()
{
    QFontDatabase db;
    const QStringList families = db.families();
    QVERIFY(!families.isEmpty());
    const QString firstFont = families.front();
    QVERIFY(db.hasFamily(firstFont));
    const QString alias = QStringLiteral("AliasToFirstFont") + firstFont;
    QVERIFY(!db.hasFamily(alias));
    QPlatformFontDatabase::registerAliasToFontFamily(firstFont, alias);
    QVERIFY(db.hasFamily(alias));
}

void tst_QFontDatabase::fallbackFonts()
{
    QTextLayout layout;
    QString s;
    s.append(QChar(0x31));
    s.append(QChar(0x05D0));
    layout.setText(s);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> runs = layout.glyphRuns(0, 1);
    foreach (QGlyphRun run, runs) {
        QRawFont rawFont = run.rawFont();
        QVERIFY(rawFont.isValid());

        QCOMPARE(run.glyphIndexes().size(), 1);
        QVERIFY(run.glyphIndexes().at(0) != 0);
    }
}

QTEST_MAIN(tst_QFontDatabase)
#include "tst_qfontdatabase.moc"
