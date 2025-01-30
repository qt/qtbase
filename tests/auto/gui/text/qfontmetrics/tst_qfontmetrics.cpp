// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qfontdatabase.h>
#include <private/qfontengine_p.h>
#include <qstringlist.h>
#include <qlist.h>

class tst_QFontMetrics : public QObject
{
Q_OBJECT

private slots:
    void same();
    void metrics();
    void boundingRect();
    void boundingRect2();
    void elidedText_data();
    void elidedText();
    void veryNarrowElidedText();
    void averageCharWidth();
    void bypassShaping();
    void elidedMultiLength();
    void elidedMultiLengthF();
    void inFontUcs4();
    void lineWidth();
    void mnemonicTextWidth();
    void leadingBelowLine();
    void elidedMetrics();
    void zeroWidthMetrics();
    void verticalMetrics_data();
    void verticalMetrics();
    void largeText_data();
    void largeText(); // QTBUG-123339
    void typoLineMetrics();
    void defaultIgnorableHorizontalAdvance_data();
    void defaultIgnorableHorizontalAdvance();
    void hugeFontMetrics();
};

void tst_QFontMetrics::same()
{
    QFont font;
    font.setBold(true);
    QFontMetrics fm(font);
    const QString text = QLatin1String("Some stupid STRING");
    QCOMPARE(fm.size(0, text), fm.size(0, text)) ;

    for (int i = 10; i <= 32; ++i) {
        font.setPixelSize(i);
        QFontMetrics fm1(font);
        QCOMPARE(fm1.size(0, text), fm1.size(0, text));
    }

    {
        QImage image;
        QFontMetrics fm2(font, &image);
        QString text2 =  QLatin1String("Foo Foo");
        QCOMPARE(fm2.size(0, text2), fm2.size(0, text2));  //used to crash
    }

    {
        QImage image;
        QFontMetricsF fm3(font, &image);
        QString text2 =  QLatin1String("Foo Foo");
        QCOMPARE(fm3.size(0, text2), fm3.size(0, text2));  //used to crash
    }
}


void tst_QFontMetrics::metrics()
{
    QFont font;

    // Query the QFontDatabase for a specific font, store the
    // result in family, style and size.
    QStringList families = QFontDatabase::families();
    if (families.isEmpty())
        return;

    QStringList::ConstIterator f_it, f_end = families.end();
    for (f_it = families.begin(); f_it != f_end; ++f_it) {
        const QString &family = *f_it;

        QStringList styles = QFontDatabase::styles(family);
        QStringList::ConstIterator s_it, s_end = styles.end();
        for (s_it = styles.begin(); s_it != s_end; ++s_it) {
            const QString &style = *s_it;

            if (QFontDatabase::isSmoothlyScalable(family, style)) {
                // smoothly scalable font... don't need to load every pointsize
                font = QFontDatabase::font(family, style, 12);

                QFontMetrics fontmetrics(font);
                QCOMPARE(fontmetrics.ascent() + fontmetrics.descent(),
                        fontmetrics.height());

                QCOMPARE(fontmetrics.height() + fontmetrics.leading(),
                        fontmetrics.lineSpacing());
            } else {
                QList<int> sizes = QFontDatabase::pointSizes(family, style);
                QVERIFY(!sizes.isEmpty());
                QList<int>::ConstIterator z_it, z_end = sizes.end();
                for (z_it = sizes.begin(); z_it != z_end; ++z_it) {
                    const int size = *z_it;

                    // Initialize the font, and check if it is an exact match
                    font = QFontDatabase::font(family, style, size);

                    QFontMetrics fontmetrics(font);
                    QCOMPARE(fontmetrics.ascent() + fontmetrics.descent(),
                            fontmetrics.height());
                    QCOMPARE(fontmetrics.height() + fontmetrics.leading(),
                            fontmetrics.lineSpacing());
                }
            }
        }
    }
}

void tst_QFontMetrics::boundingRect()
{
    QFont f;
    f.setPointSize(24);
    QFontMetrics fm(f);
    QRect r = fm.boundingRect(QChar('Y'));
    QVERIFY(r.top() < 0);
    r = fm.boundingRect(QString("Y"));
    QVERIFY(r.top() < 0);
}

void tst_QFontMetrics::boundingRect2()
{
    QFont f;
    f.setPixelSize(16);
    QFontMetricsF fm(f);
    QString str("AVAVAVA vvvvvvvvvv fffffffff file");
    QRectF br = fm.boundingRect(str);
    QRectF tbr = fm.tightBoundingRect(str);
    qreal advance = fm.horizontalAdvance(str);
    // Bounding rect plus bearings should be similar to advance
    qreal bearings = fm.leftBearing(QChar('A')) + fm.rightBearing(QChar('e'));
    QVERIFY(qAbs(br.width() + bearings - advance) < fm.averageCharWidth()/2.0);
    QVERIFY(qAbs(tbr.width() + bearings - advance) < fm.averageCharWidth()/2.0);
}

void tst_QFontMetrics::elidedText_data()
{
    QTest::addColumn<QFont>("font");
    QTest::addColumn<QString>("text");

    QTest::newRow("helvetica hello") << QFont("helvetica",10) << QString("hello") ;
    QTest::newRow("helvetica hello &Bye") << QFont("helvetica",10) << QString("hello&Bye") ;
}


void tst_QFontMetrics::elidedText()
{
    QFETCH(QFont, font);
    QFETCH(QString, text);
    QFontMetrics fm(font);
    int w = fm.horizontalAdvance(text);
    QString newtext = fm.elidedText(text,Qt::ElideRight,w+1, 0);
    QCOMPARE(text,newtext); // should not elide
    newtext = fm.elidedText(text,Qt::ElideRight,w-1, 0);
    QVERIFY(text != newtext); // should elide
}

void tst_QFontMetrics::veryNarrowElidedText()
{
    QFont f;
    QFontMetrics fm(f);
    QString text("hello");
    QCOMPARE(fm.elidedText(text, Qt::ElideRight, 0), QString());
}

void tst_QFontMetrics::averageCharWidth()
{
    QFont f;
    QFontMetrics fm(f);
    QVERIFY(fm.averageCharWidth() != 0);
    QFontMetricsF fmf(f);
    QVERIFY(fmf.averageCharWidth() != 0);
}

void tst_QFontMetrics::bypassShaping()
{
    QFont f;
    f.setStyleStrategy(QFont::PreferNoShaping);
    f.setKerning(false);

    QFontMetricsF fm(f);
    QString text = " A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z";
    qreal textWidth = fm.horizontalAdvance(text);
    QVERIFY(textWidth != 0);
    qreal charsWidth = 0;
    for (int i = 0; i < text.size(); ++i)
        charsWidth += fm.horizontalAdvance(text[i]);
    QCOMPARE(textWidth, charsWidth);
}

template<class FontMetrics, typename PrimitiveType> void elidedMultiLength_helper()
{
    QString text1 = QLatin1String("Long Text 1\x9cShorter\x9csmall");
    QString text1_long = "Long Text 1";
    QString text1_short = "Shorter";
    QString text1_small = "small";
    FontMetrics fm = FontMetrics(QFont());
    PrimitiveType width_long = fm.size(0, text1_long).width();
    QCOMPARE(fm.elidedText(text1,Qt::ElideRight, 8000), text1_long);
    QCOMPARE(fm.elidedText(text1,Qt::ElideRight, width_long + 1), text1_long);
    QCOMPARE(fm.elidedText(text1,Qt::ElideRight, width_long - 1), text1_short);
    PrimitiveType width_short = fm.size(0, text1_short).width();
    QCOMPARE(fm.elidedText(text1,Qt::ElideRight, width_short + 1), text1_short);
    QCOMPARE(fm.elidedText(text1,Qt::ElideRight, width_short - 1), text1_small);

    // Not even wide enough for "small" - should use ellipsis
    QChar ellipsisChar(0x2026);
    QString text1_el = QString::fromLatin1("s") + ellipsisChar;
    PrimitiveType width_small = fm.horizontalAdvance(text1_el);
    QCOMPARE(fm.elidedText(text1,Qt::ElideRight, width_small + 1), text1_el);
}

void tst_QFontMetrics::elidedMultiLength()
{
    elidedMultiLength_helper<QFontMetrics, int>();
}

void tst_QFontMetrics::elidedMultiLengthF()
{
    elidedMultiLength_helper<QFontMetricsF, qreal>();
}

void tst_QFontMetrics::inFontUcs4()
{
    int id = QFontDatabase::addApplicationFont(":/fonts/ucs4font.ttf");
    QVERIFY(id >= 0);

    QFont font("QtTestUcs4");
    {
        QFontMetrics fm(font);
        QVERIFY(fm.inFontUcs4(0x1D7FF));
    }

    {
        QFontMetricsF fm(font);
        QVERIFY(fm.inFontUcs4(0x1D7FF));
    }

    {
        QFontEngine *engine = QFontPrivate::get(font)->engineForScript(QChar::Script_Common);
        QGlyphLayout glyphs;
        glyphs.numGlyphs = 3;
        uint buf[3];
        glyphs.glyphs = buf;

        QString string;
        {
            string.append(QChar::highSurrogate(0x1D7FF));
            string.append(QChar::lowSurrogate(0x1D7FF));

            glyphs.numGlyphs = 3;
            glyphs.glyphs[0] = 0;
            QVERIFY(engine->stringToCMap(string.constData(), string.size(),
                                         &glyphs, &glyphs.numGlyphs,
                                         QFontEngine::GlyphIndicesOnly) > 0);
            QCOMPARE(glyphs.numGlyphs, 1);
            QCOMPARE(glyphs.glyphs[0], uint(1));
        }
        {
            string.clear();
            string.append(QChar::ObjectReplacementCharacter);

            glyphs.numGlyphs = 3;
            glyphs.glyphs[0] = 0;
            QVERIFY(engine->stringToCMap(string.constData(), string.size(),
                                         &glyphs, &glyphs.numGlyphs,
                                         QFontEngine::GlyphIndicesOnly) >= 0);
            QVERIFY(glyphs.glyphs[0] != 1);
        }
    }

    QFontDatabase::removeApplicationFont(id);
}

void tst_QFontMetrics::lineWidth()
{
    // QTBUG-13009, QTBUG-13011
    QFont smallFont;
    smallFont.setPointSize(8);
    smallFont.setWeight(QFont::Light);
    const QFontMetrics smallFontMetrics(smallFont);

    QFont bigFont;
    bigFont.setPointSize(40);
    bigFont.setWeight(QFont::Black);
    const QFontMetrics bigFontMetrics(bigFont);

    QVERIFY(smallFontMetrics.lineWidth() >= 1);
    QVERIFY(smallFontMetrics.lineWidth() < bigFontMetrics.lineWidth());
}

void tst_QFontMetrics::mnemonicTextWidth()
{
    // QTBUG-41593
    QFont f;
    QFontMetrics fm(f);
    const QString f1  = "File";
    const QString f2  = "&File";

    QCOMPARE(fm.size(Qt::TextShowMnemonic, f1), fm.size(Qt::TextShowMnemonic, f2));
    QCOMPARE(fm.size(Qt::TextHideMnemonic, f1), fm.size(Qt::TextHideMnemonic, f2));
}

void tst_QFontMetrics::leadingBelowLine()
{
    QScriptLine line;
    line.leading = 10;
    line.leadingIncluded = true;
    line.ascent = 5;
    QCOMPARE(line.height(), line.ascent + line.descent + line.leading);
    QCOMPARE(line.base(), line.ascent);
}

void tst_QFontMetrics::elidedMetrics()
{
    QString testFont = QFINDTESTDATA("fonts/testfont.ttf");
    QVERIFY(!testFont.isEmpty());

    int id = QFontDatabase::addApplicationFont(testFont);
    QVERIFY(id >= 0);

    QFont font(QFontDatabase::applicationFontFamilies(id).at(0));
    font.setPixelSize(12.0);

    QFontMetricsF metrics(font);
    QString s = QStringLiteral("VeryLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongText");

    QRectF boundingRect = metrics.boundingRect(s);

    QString elided = metrics.elidedText(s, Qt::ElideRight, boundingRect.width() / 2.0);

    QRectF elidedBoundingRect = metrics.boundingRect(elided);

    QCOMPARE(boundingRect.height(), elidedBoundingRect.height());
    QCOMPARE(boundingRect.y(), elidedBoundingRect.y());

    QFontDatabase::removeApplicationFont(id);
}

void tst_QFontMetrics::zeroWidthMetrics()
{
    QString zwnj(QChar(0x200c));
    QString zwsp(QChar(0x200b));

    QFont font;
    QFontMetricsF fm(font);
    QCOMPARE(fm.horizontalAdvance(zwnj), 0);
    QCOMPARE(fm.horizontalAdvance(zwsp), 0);
    QCOMPARE(fm.boundingRect(zwnj).width(), 0);
    QCOMPARE(fm.boundingRect(zwsp).width(), 0);

    QString string1 = QStringLiteral("(") + zwnj + QStringLiteral(")");
    QString string2 = QStringLiteral("(") + zwnj + zwnj + QStringLiteral(")");
    QString string3 = QStringLiteral("(") + zwsp + QStringLiteral(")");
    QString string4 = QStringLiteral("(") + zwsp + zwsp + QStringLiteral(")");

    QCOMPARE(fm.horizontalAdvance(string1), fm.horizontalAdvance(string2));
    QCOMPARE(fm.horizontalAdvance(string3), fm.horizontalAdvance(string4));
    QCOMPARE(fm.boundingRect(string1).width(), fm.boundingRect(string2).width());
    QCOMPARE(fm.boundingRect(string3).width(), fm.boundingRect(string4).width());
    QCOMPARE(fm.tightBoundingRect(string1).width(), fm.tightBoundingRect(string2).width());
    QCOMPARE(fm.tightBoundingRect(string3).width(), fm.tightBoundingRect(string4).width());
}

void tst_QFontMetrics::verticalMetrics_data()
{
    QTest::addColumn<QFont>("font");
    QStringList families = QFontDatabase::families();
    for (const QString &family : families) {
        QFont font(family);
        QTest::newRow(family.toUtf8()) << font;
    }
}

void tst_QFontMetrics::verticalMetrics()
{
    QFETCH(QFont, font);
    QFontMetrics fm(font);
    QVERIFY(fm.ascent() != 0 || fm.descent() != 0);
}

void tst_QFontMetrics::largeText_data()
{
    QTest::addColumn<qsizetype>("size");
    for (int i = 1; i < 20; ++i) {
        qsizetype size = qsizetype(1) << i;
        QByteArray rowText = QByteArray::number(size);
        QTest::newRow(rowText.constData()) << size;
    }
}

void tst_QFontMetrics::largeText()
{
    QFont font;
    QFontMetrics fm(font);
    QFETCH(qsizetype, size);
    QString string(size, QLatin1Char('A'));
    QRect boundingRect = fm.boundingRect(string);
    QVERIFY(boundingRect.isValid());
}

void tst_QFontMetrics::typoLineMetrics()
{
    QString testFont = QFINDTESTDATA("fonts/testfont_linemetrics.otf");
    QVERIFY(!testFont.isEmpty());

    int id = QFontDatabase::addApplicationFont(testFont);
    QVERIFY(id >= 0);

    {
        auto cleanup = qScopeGuard([&id] {
            if (id >= 0)
                QFontDatabase::removeApplicationFont(id);
        });

        QImage img(100, 100, QImage::Format_ARGB32);
        img.setDevicePixelRatio(1.0);
        QFont font(QFontDatabase::applicationFontFamilies(id).at(0), &img);
        font.setPixelSize(18);

        const qreal unitsPerEm = 1000.0;

        QFontMetrics defaultFm(font);
        const int defaultAscent = defaultFm.ascent();
        const int defaultDescent = defaultFm.descent();
        const int defaultLeading = defaultFm.leading();

        QCOMPARE(defaultAscent, qRound(1234.0 / unitsPerEm * font.pixelSize()));
        QCOMPARE(defaultDescent, qRound(5678.0 / unitsPerEm * font.pixelSize()));
        QCOMPARE(defaultLeading, 0.0);

        font.setStyleStrategy(QFont::PreferTypoLineMetrics);
        const QFontMetrics typoFm(font);

        const int typoAscent = typoFm.ascent();
        const int typoDescent = typoFm.descent();
        const int typoLeading = typoFm.leading();

        QCOMPARE(typoAscent, qRound(2000.0 / unitsPerEm * font.pixelSize()));
        QCOMPARE(typoDescent, qRound(3000.0 / unitsPerEm * font.pixelSize()));
        QCOMPARE(typoLeading, qRound(1000.0 / unitsPerEm * font.pixelSize()));
    }
}

void tst_QFontMetrics::defaultIgnorableHorizontalAdvance_data()
{
    QTest::addColumn<bool>("withShaping");

    QTest::newRow("With Text Shaping") << true;
    QTest::newRow("Without Text Shaping") << false;
}

void tst_QFontMetrics::defaultIgnorableHorizontalAdvance()
{
    QFETCH(bool, withShaping);

    // testfont_bidimarks.ttf is a version of testfont.ttf with additional
    // glyphs for U+200E (LRM) and U+200F (RLM)
    QString testFont = QFINDTESTDATA("fonts/testfont_bidimarks.ttf");
    QVERIFY(!testFont.isEmpty());

    int id = QFontDatabase::addApplicationFont(testFont);
    QVERIFY(id >= 0);

    auto cleanup = qScopeGuard([&id] {
        if (id >= 0)
            QFontDatabase::removeApplicationFont(id);
    });

    QFont withoutSupport;
    QFont withSupport(QFontDatabase::applicationFontFamilies(id).at(0));

    if (!withShaping) {
        withoutSupport.setStyleStrategy(QFont::PreferNoShaping);
        withSupport.setStyleStrategy(QFont::PreferNoShaping);
    }

    QFontMetrics withoutSupportMetrics(withoutSupport);
    QFontMetrics withSupportMetrics(withSupport);

    QTextOption opt;
    opt.setFlags(QTextOption::ShowDefaultIgnorables);

    const QChar LRM = (ushort)0x200e;
    const QString str = QStringLiteral("[") + LRM + QStringLiteral("]");

    int withoutSupportWithoutIgnorablesAdvance = withoutSupportMetrics.horizontalAdvance(str);
    int withoutSupportWithIgnorablesAdvance = withoutSupportMetrics.horizontalAdvance(str, opt);

    QCOMPARE_GT(withoutSupportWithoutIgnorablesAdvance, 0);
    QCOMPARE_EQ(withoutSupportWithIgnorablesAdvance, withoutSupportWithoutIgnorablesAdvance);

    int withSupportWithoutIgnorablesAdvance = withSupportMetrics.horizontalAdvance(str);
    int withSupportWithIgnorablesAdvance = withSupportMetrics.horizontalAdvance(str, opt);

    QCOMPARE_GT(withSupportWithoutIgnorablesAdvance, 0);
    QCOMPARE_GT(withSupportWithIgnorablesAdvance, withSupportWithoutIgnorablesAdvance);
}

void tst_QFontMetrics::hugeFontMetrics()
{
    QFont bigFont;
    bigFont.setPixelSize(10000);

    QFont hugeFont;
    hugeFont.setPixelSize(32000);

    QFont hugeFontWithTypoLineMetrics;
    hugeFontWithTypoLineMetrics.setStyleStrategy(QFont::PreferTypoLineMetrics);
    hugeFontWithTypoLineMetrics.setPixelSize(30000);

    QVERIFY(QFontMetricsF(bigFont).height() < QFontMetricsF(hugeFont).height());
    QVERIFY(QFontMetricsF(bigFont).height() < QFontMetricsF(hugeFontWithTypoLineMetrics).height());
}

QTEST_MAIN(tst_QFontMetrics)
#include "tst_qfontmetrics.moc"
