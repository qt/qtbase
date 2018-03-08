/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
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
    void elidedText_data();
    void elidedText();
    void veryNarrowElidedText();
    void averageCharWidth();

#if QT_DEPRECATED_SINCE(5, 11) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void bypassShaping();
#endif

    void elidedMultiLength();
    void elidedMultiLengthF();
    void inFontUcs4();
    void lineWidth();
    void mnemonicTextWidth();
    void leadingBelowLine();
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
    QFontDatabase fdb;

    // Query the QFontDatabase for a specific font, store the
    // result in family, style and size.
    QStringList families = fdb.families();
    if (families.isEmpty())
        return;

    QStringList::ConstIterator f_it, f_end = families.end();
    for (f_it = families.begin(); f_it != f_end; ++f_it) {
        const QString &family = *f_it;

        QStringList styles = fdb.styles(family);
        QStringList::ConstIterator s_it, s_end = styles.end();
        for (s_it = styles.begin(); s_it != s_end; ++s_it) {
            const QString &style = *s_it;

            if (fdb.isSmoothlyScalable(family, style)) {
                // smoothly scalable font... don't need to load every pointsize
                font = fdb.font(family, style, 12);

                QFontMetrics fontmetrics(font);
                QCOMPARE(fontmetrics.ascent() + fontmetrics.descent(),
                        fontmetrics.height());

                QCOMPARE(fontmetrics.height() + fontmetrics.leading(),
                        fontmetrics.lineSpacing());
            } else {
                QList<int> sizes = fdb.pointSizes(family, style);
                QVERIFY(!sizes.isEmpty());
                QList<int>::ConstIterator z_it, z_end = sizes.end();
                for (z_it = sizes.begin(); z_it != z_end; ++z_it) {
                    const int size = *z_it;

                    // Initialize the font, and check if it is an exact match
                    font = fdb.font(family, style, size);

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

#if QT_DEPRECATED_SINCE(5, 11) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void tst_QFontMetrics::bypassShaping()
{
    QFont f;
    f.setStyleStrategy(QFont::ForceIntegerMetrics);
    QFontMetrics fm(f);
    QString text = " A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z";
    int textWidth = fm.width(text, -1, Qt::TextBypassShaping);
    QVERIFY(textWidth != 0);
    int charsWidth = 0;
    for (int i = 0; i < text.size(); ++i)
        charsWidth += fm.horizontalAdvance(text[i]);
    // This assertion is needed in Qt WebKit's WebCore::Font::offsetForPositionForSimpleText
    QCOMPARE(textWidth, charsWidth);
}
#endif

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
                                         QFontEngine::GlyphIndicesOnly));
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
                                         QFontEngine::GlyphIndicesOnly));
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

QTEST_MAIN(tst_QFontMetrics)
#include "tst_qfontmetrics.moc"
