// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
#include <qpa/qplatformintegration.h>

#include <QtGui/private/qguiapplication_p.h>

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

    void variableFont_data();
    void variableFont();

#ifdef Q_OS_WIN
    void findCourier();
#endif

    void addApplicationFontFallback();
    void addApplicationEmojiFontFamily();

private:
    QString m_ledFont;
    QString m_testFont;
    QString m_testFontCondensed;
    QString m_testFontItalic;
    QString m_testFontVariable;
    QString m_limitedFont;
    QString m_fallbackFont;
    QString m_emojiFont;
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
    m_testFontVariable = QFINDTESTDATA("testfont_variable.ttf");
    m_limitedFont = QFINDTESTDATA("QtTestLimitedFont-Regular.ttf");
    m_fallbackFont = QFINDTESTDATA("QtTestFallbackFont-Regular.ttf");
    m_emojiFont = QFINDTESTDATA("QtEmojiTestFont-Regular.ttf");
    QVERIFY(!m_ledFont.isEmpty());
    QVERIFY(!m_testFont.isEmpty());
    QVERIFY(!m_testFontCondensed.isEmpty());
    QVERIFY(!m_testFontItalic.isEmpty());
    QVERIFY(!m_testFontVariable.isEmpty());
    QVERIFY(!m_limitedFont.isEmpty());
    QVERIFY(!m_fallbackFont.isEmpty());
    QVERIFY(!m_emojiFont.isEmpty());
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
#if defined(Q_OS_VXWORKS)
    QSKIP("QTBUG-130071: VxWorks doesn't support fixed system font out of the box");
#endif
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
        QVERIFY(fontfile.open(QIODevice::ReadOnly));
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
    int testFontId = QFontDatabase::addApplicationFont(m_testFont);
    int testFontCondensedId = QFontDatabase::addApplicationFont(m_testFontCondensed);
    auto cleanup = qScopeGuard([&testFontId, &testFontCondensedId] {
        if (testFontId >= 0)
            QFontDatabase::removeApplicationFont(testFontId);
        if (testFontCondensedId >= 0)
            QFontDatabase::removeApplicationFont(testFontCondensedId);
    });

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
    int testFontCondensedId = QFontDatabase::addApplicationFont(m_testFontCondensed);
    if (!QFontDatabase::hasFamily("QtBidiTestFont"))
        QSKIP("This platform doesn't support preferred font family names (QTBUG-53478)");
    int testFontId = QFontDatabase::addApplicationFont(m_testFont);
    auto cleanup = qScopeGuard([&testFontId, &testFontCondensedId] {
        if (testFontId >= 0)
            QFontDatabase::removeApplicationFont(testFontId);
        if (testFontCondensedId >= 0)
            QFontDatabase::removeApplicationFont(testFontCondensedId);
    });

    // Test we correctly get the condensed font using different font matching methods:
    QFont tfcByStretch("QtBidiTestFont");
    tfcByStretch.setStretch(QFont::Condensed);
    QFont tfcByStyleName("QtBidiTestFont");
    tfcByStyleName.setStyleName("Condensed");

#ifdef Q_OS_WIN
    QFont f;
    f.setStyleStrategy(QFont::NoFontMerging);
    QFontPrivate *font_d = QFontPrivate::get(f);
    if (font_d->engineForScript(QChar::Script_Common)->type() != QFontEngine::Freetype
        && font_d->engineForScript(QChar::Script_Common)->type() != QFontEngine::DirectWrite) {
        QEXPECT_FAIL("","No matching of sub-family by stretch on Windows", Continue);
    }
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

void tst_QFontDatabase::variableFont_data()
{
    QTest::addColumn<bool>("loadFromData");

    QTest::newRow( "Load from file" ) << false;
    QTest::newRow( "Load from data" ) << true;
}

void tst_QFontDatabase::variableFont()
{
    QFETCH(bool, loadFromData);

    {
        QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        if (!pfdb->supportsVariableApplicationFonts())
            QSKIP("Variable application fonts not supported on this platform");
    }

    int id = -1;
    if (loadFromData) {
        QFile file(m_testFontVariable);
        QVERIFY(file.open(QIODevice::ReadOnly));

        QByteArray data = file.readAll();
        id = QFontDatabase::addApplicationFontFromData(data);
    } else {
        id = QFontDatabase::addApplicationFont(m_testFontVariable);
    }
    if (id == -1)
        QSKIP("Skip the test since app fonts are not supported on this system");

    auto cleanup = qScopeGuard([&id] {
        if (id >= 0)
            QFontDatabase::removeApplicationFont(id);
    });

    QString family = QFontDatabase::applicationFontFamilies(id).first();
    {
        QFont font(family);
        QCOMPARE(QFontInfo(font).styleName(), u"Regular"_s);
        QCOMPARE(QFontInfo(font).weight(), QFont::Normal);
    }

    {
        QFont font(family);
        font.setWeight(QFont::Black);
        QCOMPARE(QFontInfo(font).styleName(), u"QtExtraBold"_s);
        QCOMPARE(QFontInfo(font).weight(), int(QFont::Black));
    }

    {
        QFont regularFont(family);
        QFont extraBoldFont(family);
        extraBoldFont.setStyleName(u"QtExtraBold"_s);

        QFontMetricsF regularFm(regularFont);
        QFontMetricsF extraBoldFm(extraBoldFont);

        QVERIFY(regularFm.horizontalAdvance(QLatin1Char('1')) < extraBoldFm.horizontalAdvance(QLatin1Char('1')));
    }

    {
        QFont regularFont(family);
        QFont extraBoldFont(family);
        extraBoldFont.setStyleName(u"QtExtraBold"_s);
        extraBoldFont.setVariableAxis("wght", 400);

        QFontMetricsF regularFm(regularFont);
        QFontMetricsF extraBoldFm(extraBoldFont);

        QCOMPARE(extraBoldFm.horizontalAdvance(QLatin1Char('1')), regularFm.horizontalAdvance(QLatin1Char('1')));
    }
}

void tst_QFontDatabase::addApplicationFontFallback()
{
    int ledId = -1;
    int id = -1;
    int limitedId = -1;
    int fallbackId = -1;
    auto cleanup = qScopeGuard([&id, &ledId, &limitedId, &fallbackId] {
        if (id >= 0)
            QFontDatabase::removeApplicationFont(id);
        if (ledId >= 0)
            QFontDatabase::removeApplicationFont(ledId);
        if (limitedId >= 0)
            QFontDatabase::removeApplicationFont(limitedId);
        if (fallbackId >= 0)
            QFontDatabase::removeApplicationFont(fallbackId);
    });

    const QChar hebrewChar(0x05D0); // Hebrew 'aleph'

    ledId = QFontDatabase::addApplicationFont(m_ledFont);
    if (ledId < 0)
        QSKIP("Skip the test since app fonts are not supported on this system");

    auto getHebrewFont = [&]() {
        QTextLayout layout;
        layout.setText(hebrewChar);
        layout.setFont(QFont(u"LED Real"_s));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        if (glyphRuns.isEmpty())
            return QString{};

        return glyphRuns.first().rawFont().familyName();
    };

    QString defaultHebrewFont = getHebrewFont();
    if (defaultHebrewFont.isEmpty())
        QSKIP("Skip the test since Hebrew is not supported on this system");

    QVERIFY(QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Hebrew).isEmpty());
    QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Hebrew, u"QtBidiTestFont"_s);

    QCOMPARE(QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Hebrew).size(), 1);
    QCOMPARE(QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Hebrew).first(), u"QtBidiTestFont"_s);

    {
        QString hebrewFontNow = getHebrewFont();
        QCOMPARE(hebrewFontNow, defaultHebrewFont);
    }

    id = QFontDatabase::addApplicationFont(m_testFont);
    QVERIFY(id >= 0);

    {
        QString hebrewFontNow = getHebrewFont();
        QCOMPARE(hebrewFontNow, u"QtBidiTestFont"_s);
    }

    QFontDatabase::removeApplicationFallbackFontFamily(QChar::Script_Hebrew, u"QtBidiTestFont"_s);

    {
        QString hebrewFontNow = getHebrewFont();
        QCOMPARE(hebrewFontNow, defaultHebrewFont);
    }

    QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Hebrew, QStringList(u"QtBidiTestFont"_s));

    {
        QString hebrewFontNow = getHebrewFont();
        QCOMPARE(hebrewFontNow, u"QtBidiTestFont"_s);
    }

    QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Hebrew, QStringList{});

    {
        QString hebrewFontNow = getHebrewFont();
        QCOMPARE(hebrewFontNow, defaultHebrewFont);
    }

    limitedId = QFontDatabase::addApplicationFont(m_limitedFont);
    QVERIFY(limitedId >= 0);

    fallbackId = QFontDatabase::addApplicationFont(m_fallbackFont);
    QVERIFY(fallbackId >= 0);

    QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Common, u"QtTestFallbackFont"_s);

    // The fallback for Common will be used also for Latin, because Latin and Common are
    // considered the same script by the font matching engine.
    {
        QTextLayout layout;
        layout.setText(u"A'B,"_s);
        layout.setFont(QFont(u"QtTestLimitedFont"_s));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        QVERIFY(glyphRuns.size() > 1);
        for (int i = 0; i < glyphRuns.size(); ++i) {
            QVERIFY(glyphRuns.at(i).rawFont().familyName() == u"QtTestFallbackFont"_s
                    || glyphRuns.at(i).rawFont().familyName() == u"QtTestLimitedFont"_s);
        }
    }

    // When the text only consists of common script characters, the fallback font will also be used.
    {
        QTextLayout layout;
        layout.setText(u"',"_s);
        layout.setFont(QFont(u"QtTestLimitedFont"_s));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        QCOMPARE(glyphRuns.size(), 2);
        for (int i = 0; i < glyphRuns.size(); ++i) {
            QVERIFY(glyphRuns.at(i).rawFont().familyName() == u"QtTestFallbackFont"_s
                    || glyphRuns.at(i).rawFont().familyName() == u"QtTestLimitedFont"_s);
        }
    }

    QVERIFY(QFontDatabase::removeApplicationFallbackFontFamily(QChar::Script_Common, u"QtTestFallbackFont"_s));
    QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Latin, u"QtTestFallbackFont"_s);

    // Latin fallback works just the same as Common fallback
    {
        QTextLayout layout;
        layout.setText(u"A'B,"_s);
        layout.setFont(QFont(u"QtTestLimitedFont"_s));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        QCOMPARE(glyphRuns.size(), 2);
        for (int i = 0; i < glyphRuns.size(); ++i) {
            QVERIFY(glyphRuns.at(i).rawFont().familyName() == u"QtTestFallbackFont"_s
                    || glyphRuns.at(i).rawFont().familyName() == u"QtTestLimitedFont"_s);
        }
    }

    // When the common character is placed next to a Cyrillic characters, it gets adapted to this,
    // so the fallback font will not be selected, even if it supports the character in question
    {
        QTextLayout layout;
        layout.setText(u"A'Б,"_s);
        layout.setFont(QFont(u"QtTestLimitedFont"_s));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        QCOMPARE(glyphRuns.size(), 2);
        for (int i = 0; i < glyphRuns.size(); ++i) {
            QVERIFY(glyphRuns.at(i).rawFont().familyName() != u"QtTestFallbackFont"_s);
        }
    }

    QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Cyrillic, u"QtTestFallbackFont"_s);

    // When we set the fallback font for Cyrillic as well, it gets selected
    {
        QTextLayout layout;
        layout.setText(u"A'Б,"_s);
        layout.setFont(QFont(u"QtTestLimitedFont"_s));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        QCOMPARE(glyphRuns.size(), 2);
        for (int i = 0; i < glyphRuns.size(); ++i) {
            QVERIFY(glyphRuns.at(i).rawFont().familyName() == u"QtTestFallbackFont"_s
                    || glyphRuns.at(i).rawFont().familyName() == u"QtTestLimitedFont"_s);
        }
    }

    QVERIFY(QFontDatabase::removeApplicationFallbackFontFamily(QChar::Script_Cyrillic, u"QtTestFallbackFont"_s));
    QVERIFY(QFontDatabase::removeApplicationFallbackFontFamily(QChar::Script_Latin, u"QtTestFallbackFont"_s));
}

void tst_QFontDatabase::addApplicationEmojiFontFamily()
{
    {
        QPlatformFontDatabase *pfdb = QGuiApplicationPrivate::platformIntegration()->fontDatabase();
        if (!pfdb->supportsColrv0Fonts())
            QSKIP("This test depends on COLRv0 support.");
    }

    int id = -1;
    auto cleanup = qScopeGuard([&id] {
        if (id >= 0)
            QFontDatabase::removeApplicationFont(id);
    });

    id = QFontDatabase::addApplicationFont(m_emojiFont);
    QVERIFY(id >= 0);

    QStringList families = QFontDatabase::applicationFontFamilies(id);
    QVERIFY(families.size() > 0);

    const QChar airplane(0x2708);
    const QChar vs16(0xfe0f);

    QFontDatabase::addApplicationEmojiFontFamily(families.first());

    // Get emoji version of regular airplane symbol
    {
        QTextLayout layout;
        layout.setText(QString(airplane) + vs16);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        QCOMPARE(glyphRuns.size(), 1);

        QGlyphRun glyphRun = glyphRuns.first();
        QList<quint32> glyphIndexes = glyphRun.glyphIndexes();

        QCOMPARE(glyphIndexes.size(), 1);
        QCOMPARE(glyphIndexes.at(0), 237);
    }

    const QChar asterisk('*');
    const QChar enclosingKeyCap(0x20e3);

    // Get emoji keycap ligature (vs16 should be ignored when evaluating ligature substitution)
    {
        QTextLayout layout;
        layout.setText(QString(asterisk) + vs16 + enclosingKeyCap);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns();
        QCOMPARE(glyphRuns.size(), 1);

        QGlyphRun glyphRun = glyphRuns.first();
        QList<quint32> glyphIndexes = glyphRun.glyphIndexes();

        QCOMPARE(glyphIndexes.size(), 1);
        QCOMPARE(glyphIndexes.at(0), 238);
    }

}

QTEST_MAIN(tst_QFontDatabase)
#include "tst_qfontdatabase.moc"
