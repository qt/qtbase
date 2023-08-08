// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QSignalSpy>

#include <qfontdatabase.h>
#include <qfontinfo.h>
#include <qfontmetrics.h>
#include <qtextlayout.h>
#include <private/qrawfont_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <qpa/qplatformfontdatabase.h>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcTests, "qt.text.tests")

class tst_QFontDatabase : public QObject
{
Q_OBJECT

public:
    tst_QFontDatabase();

private slots:
    void initTestCase();
    void styles_data();
    void styles();

    void fixedPitch_data();
    void fixedPitch();
    void systemFixedFont();

#ifdef Q_OS_MAC
    void trickyFonts_data();
    void trickyFonts();
#endif

    void widthTwoTimes_data();
    void widthTwoTimes();

    void addAppFont_data();
    void addAppFont();

    void addTwoAppFontsFromFamily();

    void aliases();
    void fallbackFonts();

    void condensedFontWidth();
    void condensedFontWidthNoFontMerging();
    void condensedFontMatching();

    void rasterFonts();
    void smoothFonts();

    void registerOpenTypePreferredNamesSystem();
    void registerOpenTypePreferredNamesApplication();

    void stretchRespected();

#ifdef Q_OS_WIN
    void findCourier();
#endif

private:
    QString m_ledFont;
    QString m_testFont;
    QString m_testFontCondensed;
    QString m_testFontItalic;
};

tst_QFontDatabase::tst_QFontDatabase()
{
}

void tst_QFontDatabase::initTestCase()
{
    m_ledFont = QFINDTESTDATA("LED_REAL.TTF");
    m_testFont = QFINDTESTDATA("testfont.ttf");
    m_testFontCondensed = QFINDTESTDATA("testfont_condensed.ttf");
    m_testFontItalic = QFINDTESTDATA("testfont_italic.ttf");
    QVERIFY(!m_ledFont.isEmpty());
    QVERIFY(!m_testFont.isEmpty());
    QVERIFY(!m_testFontCondensed.isEmpty());
    QVERIFY(!m_testFontItalic.isEmpty());
}

void tst_QFontDatabase::styles_data()
{
    QTest::addColumn<QString>("font");

    QTest::newRow( "data0" ) << QString( "Times New Roman" );
}

void tst_QFontDatabase::styles()
{
    QFETCH( QString, font );

    QStringList styles = QFontDatabase::styles( font );
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

    if (!QFontDatabase::families().contains(font))
        QSKIP("Font not installed");

    QCOMPARE(QFontDatabase::isFixedPitch(font), fixedPitch);

    QFont qfont(font);
    QFontInfo fi(qfont);
    QCOMPARE(fi.fixedPitch(), fixedPitch);
}

void tst_QFontDatabase::systemFixedFont() // QTBUG-54623
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QFontInfo fontInfo(font);
    bool fdbSaysFixed = QFontDatabase::isFixedPitch(fontInfo.family(), fontInfo.styleName());
    qCDebug(lcTests) << "system fixed font is" << font << "really fixed?" << fdbSaysFixed << fontInfo.fixedPitch();
    QVERIFY(fdbSaysFixed);
    QVERIFY(fontInfo.fixedPitch());
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

    if (!QFontDatabase::families().contains(font))
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
    int w1 = fm.horizontalAdvance(text, 0);
    int w2 = fm.horizontalAdvance(text, 0);

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

    const QStringList oldFamilies = QFontDatabase::families();
    QVERIFY(!oldFamilies.isEmpty());

    fontDbChangedSpy.clear();

    int id;
    if (useMemoryFont) {
        QFile fontfile(m_ledFont);
        fontfile.open(QIODevice::ReadOnly);
        QByteArray fontdata = fontfile.readAll();
        QVERIFY(!fontdata.isEmpty());
        id = QFontDatabase::addApplicationFontFromData(fontdata);
    } else {
        id = QFontDatabase::addApplicationFont(m_ledFont);
    }
#if defined(Q_OS_HPUX) && defined(QT_NO_FONTCONFIG)
    // Documentation says that X11 systems that don't have fontconfig
    // don't support application fonts.
    QCOMPARE(id, -1);
    return;
#endif
    QCOMPARE(fontDbChangedSpy.size(), 1);
    if (id == -1)
        QSKIP("Skip the test since app fonts are not supported on this system");

    const QStringList addedFamilies = QFontDatabase::applicationFontFamilies(id);
    QVERIFY(!addedFamilies.isEmpty());

    const QStringList newFamilies = QFontDatabase::families();
    QVERIFY(!newFamilies.isEmpty());
    QVERIFY(newFamilies.size() >= oldFamilies.size());

    for (int i = 0; i < addedFamilies.size(); ++i) {
        QString family = addedFamilies.at(i);
        QVERIFY(newFamilies.contains(family));
        QFont qfont(family);
        QFontInfo fi(qfont);
        QCOMPARE(fi.family(), family);
    }

    QVERIFY(QFontDatabase::removeApplicationFont(id));
    QCOMPARE(fontDbChangedSpy.size(), 2);

    QVERIFY(QFontDatabase::families().size() <= oldFamilies.size());
}

void tst_QFontDatabase::addTwoAppFontsFromFamily()
{
    int regularId = QFontDatabase::addApplicationFont(m_testFont);
    if (regularId == -1)
        QSKIP("Skip the test since app fonts are not supported on this system");

    int italicId = QFontDatabase::addApplicationFont(m_testFontItalic);
    QVERIFY(italicId != -1);

    QVERIFY(!QFontDatabase::applicationFontFamilies(regularId).isEmpty());
    QVERIFY(!QFontDatabase::applicationFontFamilies(italicId).isEmpty());

    QString regularFontName = QFontDatabase::applicationFontFamilies(regularId).first();
    QString italicFontName = QFontDatabase::applicationFontFamilies(italicId).first();
    QCOMPARE(regularFontName, italicFontName);

    QFont italicFont = QFontDatabase::font(italicFontName,
                                           QString::fromLatin1("Italic"), 14);
    QVERIFY(italicFont.italic());

    QFontDatabase::removeApplicationFont(regularId);
    QFontDatabase::removeApplicationFont(italicId);
}

void tst_QFontDatabase::aliases()
{
    const QStringList families = QFontDatabase::families();
    QVERIFY(!families.isEmpty());

    QString firstFont;
    for (int i = 0; i < families.size(); ++i) {
        if (!families.at(i).contains('[')) {
            firstFont = families.at(i);
            break;
        }
    }

    if (firstFont.isEmpty())
        QSKIP("Skipped because there are no unambiguous font families on the system.");

    QVERIFY(QFontDatabase::hasFamily(firstFont));
    const QString alias = QStringLiteral("AliasToFirstFont") + firstFont;
    QVERIFY(!QFontDatabase::hasFamily(alias));
    QPlatformFontDatabase::registerAliasToFontFamily(firstFont, alias);
    QVERIFY(QFontDatabase::hasFamily(alias));
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

    const QList<QGlyphRun> runs = layout.glyphRuns(0, 1);
    for (QGlyphRun run : runs) {
        QRawFont rawFont = run.rawFont();
        QVERIFY(rawFont.isValid());

        QCOMPARE(run.glyphIndexes().size(), 1);
        QVERIFY(run.glyphIndexes().at(0) != 0);
    }
}

static QString testString()
{
    return QStringLiteral("foo bar");
}

void tst_QFontDatabase::stretchRespected()
{
    int italicId = QFontDatabase::addApplicationFont(m_testFontItalic);
    QVERIFY(italicId != -1);

    QVERIFY(!QFontDatabase::applicationFontFamilies(italicId).isEmpty());

    QString italicFontName = QFontDatabase::applicationFontFamilies(italicId).first();

    QFont italicFont = QFontDatabase::font(italicFontName,
                                           QString::fromLatin1("Italic"), 14);
    QVERIFY(italicFont.italic());

    QFont italicStretchedFont = italicFont;
    italicStretchedFont.setStretch( 400 );

    QVERIFY(QFontMetricsF(italicFont).horizontalAdvance(QStringLiteral("foobar")) <
            QFontMetricsF(italicStretchedFont).horizontalAdvance(QStringLiteral("foobar")));

    QFontDatabase::removeApplicationFont(italicId);
}

void tst_QFontDatabase::condensedFontWidthNoFontMerging()
{
    int regularFontId = QFontDatabase::addApplicationFont(m_testFont);
    int condensedFontId = QFontDatabase::addApplicationFont(m_testFontCondensed);

    QVERIFY(!QFontDatabase::applicationFontFamilies(regularFontId).isEmpty());
    QVERIFY(!QFontDatabase::applicationFontFamilies(condensedFontId).isEmpty());

    QString regularFontName = QFontDatabase::applicationFontFamilies(regularFontId).first();
    QString condensedFontName = QFontDatabase::applicationFontFamilies(condensedFontId).first();

    QFont condensedFont1(condensedFontName);
    if (regularFontName == condensedFontName)
        condensedFont1.setStyleName(QStringLiteral("Condensed"));
    condensedFont1.setStyleStrategy(QFont::PreferMatch);

    QFont condensedFont2 = condensedFont1;
    condensedFont2.setStyleStrategy(QFont::StyleStrategy(QFont::NoFontMerging | QFont::PreferMatch));

    QCOMPARE(QFontMetricsF(condensedFont2).horizontalAdvance(QStringLiteral("foobar")),
             QFontMetricsF(condensedFont1).horizontalAdvance(QStringLiteral("foobar")));
 }

void tst_QFontDatabase::condensedFontWidth()
{
    QFontDatabase::addApplicationFont(m_testFont);
    QFontDatabase::addApplicationFont(m_testFontCondensed);

    QVERIFY(QFontDatabase::hasFamily("QtBidiTestFont"));
    if (!QFontDatabase::hasFamily("QtBidiTestFontCondensed"))
        QSKIP("This platform doesn't support font sub-family names (QTBUG-55625)");

    // Test we really get a condensed font, and a not renormalized one (QTBUG-48043):
    QFont testFont("QtBidiTestFont");
    QFont testFontCondensed("QtBidiTestFontCondensed");
    QFontMetrics fmTF(testFont);
    QFontMetrics fmTFC(testFontCondensed);
    QVERIFY(fmTF.horizontalAdvance(testString()) > fmTFC.horizontalAdvance(testString()));

}

void tst_QFontDatabase::condensedFontMatching()
{
    QFontDatabase::removeAllApplicationFonts();
    QFontDatabase::addApplicationFont(m_testFontCondensed);
    if (!QFontDatabase::hasFamily("QtBidiTestFont"))
        QSKIP("This platform doesn't support preferred font family names (QTBUG-53478)");
    QFontDatabase::addApplicationFont(m_testFont);

    // Test we correctly get the condensed font using different font matching methods:
    QFont tfcByStretch("QtBidiTestFont");
    tfcByStretch.setStretch(QFont::Condensed);
    QFont tfcByStyleName("QtBidiTestFont");
    tfcByStyleName.setStyleName("Condensed");

#ifdef Q_OS_WIN
    QFont f;
    f.setStyleStrategy(QFont::NoFontMerging);
    QFontPrivate *font_d = QFontPrivate::get(f);
    if (font_d->engineForScript(QChar::Script_Common)->type() != QFontEngine::Freetype)
        QEXPECT_FAIL("","No matching of sub-family by stretch on Windows", Continue);
#endif

    QCOMPARE(QFontMetrics(tfcByStretch).horizontalAdvance(testString()),
             QFontMetrics(tfcByStyleName).horizontalAdvance(testString()));

    if (!QFontDatabase::hasFamily("QtBidiTestFontCondensed"))
        QSKIP("This platform doesn't support font sub-family names (QTBUG-55625)");

    QFont tfcBySubfamilyName("QtBidiTestFontCondensed");
    QCOMPARE(QFontMetrics(tfcByStyleName).horizontalAdvance(testString()),
             QFontMetrics(tfcBySubfamilyName).horizontalAdvance(testString()));
}

void tst_QFontDatabase::rasterFonts()
{
    QFont font(QLatin1String("Fixedsys"), 1000);
    QFontInfo fontInfo(font);

    if (fontInfo.family() != font.family())
        QSKIP("Fixedsys font not available.");

    QVERIFY(!QFontDatabase::isSmoothlyScalable(font.family()));
    QVERIFY(fontInfo.pointSize() != font.pointSize());
}

void tst_QFontDatabase::smoothFonts()
{
    QFont font(QLatin1String("Arial"), 1000);
    QFontInfo fontInfo(font);

    if (fontInfo.family() != font.family())
        QSKIP("Arial font not available.");

    // Smooth and bitmap scaling are mutually exclusive
    QVERIFY(QFontDatabase::isSmoothlyScalable(font.family()));
    QVERIFY(!QFontDatabase::isBitmapScalable(font.family()));
}

void tst_QFontDatabase::registerOpenTypePreferredNamesSystem()
{
    // This font family was picked because it was the only one I had installed which showcased the
    // problem
    if (!QFontDatabase::hasFamily(QString::fromLatin1("Source Code Pro ExtraLight")))
        QSKIP("Source Code Pro ExtraLight is not installed");

    QStringList styles = QFontDatabase::styles(QString::fromLatin1("Source Code Pro"));
    QVERIFY(styles.contains(QLatin1String("ExtraLight")));
}

void tst_QFontDatabase::registerOpenTypePreferredNamesApplication()
{
    int id = QFontDatabase::addApplicationFont(QString::fromLatin1(":/testfont_open.otf"));
    if (id == -1)
        QSKIP("Skip the test since app fonts are not supported on this system");

    QStringList styles = QFontDatabase::styles(QString::fromLatin1("QtBidiTestFont"));
    QVERIFY(styles.contains(QLatin1String("Open")));

    QFontDatabase::removeApplicationFont(id);
}

#ifdef Q_OS_WIN
void tst_QFontDatabase::findCourier()
{
    QFont font = QFontDatabase::font(u"Courier"_s, u""_s, 16);
    QFontInfo info(font);
    QCOMPARE(info.family(), u"Courier New"_s);
    QCOMPARE(info.pointSize(), 16);

    font = QFontDatabase::font("Courier", "", 64);
    info = font;
    QCOMPARE(info.family(), u"Courier New"_s);
    QCOMPARE(info.pointSize(), 64);

    // By setting "PreferBitmap" we should get Courier itself.
    font.setStyleStrategy(QFont::PreferBitmap);
    info = font;
    QCOMPARE(info.family(), u"Courier"_s);
    // Which has an upper bound on point size
    QCOMPARE(info.pointSize(), 19);

    font.setStyleStrategy(QFont::PreferDefault);
    info = font;
    QCOMPARE(info.family(), u"Courier New"_s);
    QCOMPARE(info.pointSize(), 64);
}
#endif

QTEST_MAIN(tst_QFontDatabase)
#include "tst_qfontdatabase.moc"
