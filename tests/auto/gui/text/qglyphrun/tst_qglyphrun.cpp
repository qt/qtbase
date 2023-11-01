// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <qglyphrun.h>
#include <qpainter.h>
#include <qtextlayout.h>
#include <qfontdatabase.h>

// #define DEBUG_SAVE_IMAGE

class tst_QGlyphRun: public QObject
{
    Q_OBJECT

#if !defined(QT_NO_RAWFONT)
private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void constructionAndDestruction();
    void copyConstructor();
    void assignment();
    void equalsOperator_data();
    void equalsOperator();
    void isEmpty();
    void textLayoutGlyphIndexes();
    void drawExistingGlyphs();
    void drawNonExistentGlyphs();
    void drawMultiScriptText1();
    void drawMultiScriptText2();
    void drawRightToLeft();
    void detach();
    void setRawData();
    void setRawDataAndGetAsVector();
    void boundingRect();
    void mixedScripts();
    void multiLineBoundingRect();
    void defaultIgnorables();
    void stringIndexes();
    void retrievalFlags_data();
    void retrievalFlags();
    void objectReplacementCharacter();

private:
    int m_testFontId;
    QFont m_testFont;
    bool m_testFont_ok;
#endif // QT_NO_RAWFONT

};

#if !defined(QT_NO_RAWFONT)

Q_DECLARE_METATYPE(QGlyphRun);

void tst_QGlyphRun::initTestCase()
{
    m_testFont_ok = false;

    m_testFontId = QFontDatabase::addApplicationFont(QFINDTESTDATA("test.ttf"));
    QVERIFY(m_testFontId >= 0);

    m_testFont = QFont("QtsSpecialTestFont");

    QCOMPARE(QFontInfo(m_testFont).family(), QString::fromLatin1("QtsSpecialTestFont"));

    m_testFont_ok = true;
}

void tst_QGlyphRun::init()
{
    if (!m_testFont_ok)
        QSKIP("Test font is not working correctly");
}

void tst_QGlyphRun::cleanupTestCase()
{
    QFontDatabase::removeApplicationFont(m_testFontId);
}

void tst_QGlyphRun::constructionAndDestruction()
{
    QGlyphRun glyphIndexes;
}

static QGlyphRun make_dummy_indexes()
{
    QGlyphRun glyphs;

    QList<quint32> glyphIndexes;
    QList<QPointF> positions;
    QFont font;
    font.setPointSize(18);

    glyphIndexes.append(1);
    glyphIndexes.append(2);
    glyphIndexes.append(3);

    positions.append(QPointF(1, 2));
    positions.append(QPointF(3, 4));
    positions.append(QPointF(5, 6));

    glyphs.setRawFont(QRawFont::fromFont(font));
    glyphs.setGlyphIndexes(glyphIndexes);
    glyphs.setPositions(positions);

    return glyphs;
}

void tst_QGlyphRun::copyConstructor()
{
    QGlyphRun glyphs;

    {
        QList<quint32> glyphIndexes;
        QList<QPointF> positions;
        QFont font;
        font.setPointSize(18);

        glyphIndexes.append(1);
        glyphIndexes.append(2);
        glyphIndexes.append(3);

        positions.append(QPointF(1, 2));
        positions.append(QPointF(3, 4));
        positions.append(QPointF(5, 6));

        glyphs.setRawFont(QRawFont::fromFont(font));
        glyphs.setGlyphIndexes(glyphIndexes);
        glyphs.setPositions(positions);
    }

    QGlyphRun otherGlyphs(glyphs);
    QCOMPARE(otherGlyphs.rawFont(), glyphs.rawFont());
    QCOMPARE(glyphs.glyphIndexes(), otherGlyphs.glyphIndexes());
    QCOMPARE(glyphs.positions(), otherGlyphs.positions());
}

void tst_QGlyphRun::assignment()
{
    QGlyphRun glyphs(make_dummy_indexes());

    QGlyphRun otherGlyphs = glyphs;
    QCOMPARE(otherGlyphs.rawFont(), glyphs.rawFont());
    QCOMPARE(glyphs.glyphIndexes(), otherGlyphs.glyphIndexes());
    QCOMPARE(glyphs.positions(), otherGlyphs.positions());
}

void tst_QGlyphRun::equalsOperator_data()
{
    QTest::addColumn<QGlyphRun>("one");
    QTest::addColumn<QGlyphRun>("two");
    QTest::addColumn<bool>("equals");

    QGlyphRun one(make_dummy_indexes());
    QGlyphRun two(make_dummy_indexes());

    QTest::newRow("Identical") << one << two << true;

    {
        QGlyphRun busted(two);

        QList<QPointF> positions = busted.positions();
        positions[2] += QPointF(1, 1);
        busted.setPositions(positions);


        QTest::newRow("Different positions") << one << busted << false;
    }

    {
        QGlyphRun busted(two);

        QFont font;
        font.setPixelSize(busted.rawFont().pixelSize() * 2);
        busted.setRawFont(QRawFont::fromFont(font));

        QTest::newRow("Different fonts") << one << busted << false;
    }

    {
        QGlyphRun busted(two);

        QList<quint32> glyphIndexes = busted.glyphIndexes();
        glyphIndexes[2] += 1;
        busted.setGlyphIndexes(glyphIndexes);

        QTest::newRow("Different glyph indexes") << one << busted << false;
    }

}

void tst_QGlyphRun::equalsOperator()
{
    QFETCH(QGlyphRun, one);
    QFETCH(QGlyphRun, two);
    QFETCH(bool, equals);

    QCOMPARE(one == two, equals);
    QCOMPARE(one != two, !equals);
}

void tst_QGlyphRun::isEmpty()
{
    QGlyphRun glyphs;
    QVERIFY(glyphs.isEmpty());

    glyphs.setGlyphIndexes(QList<quint32>() << 1 << 2 << 3);
    QVERIFY(!glyphs.isEmpty());

    glyphs.clear();
    QVERIFY(glyphs.isEmpty());

    QList<quint32> glyphIndexes = QList<quint32>() << 1 << 2 << 3;
    QList<QPointF> positions = QList<QPointF>() << QPointF(0, 0) << QPointF(0, 0) << QPointF(0, 0);
    glyphs.setRawData(glyphIndexes.constData(), positions.constData(), glyphIndexes.size());
    QVERIFY(!glyphs.isEmpty());
}

void tst_QGlyphRun::textLayoutGlyphIndexes()
{
    QString s;
    s.append(QLatin1Char('A'));
    s.append(QChar(0xe000));

    QTextLayout layout(s);
    layout.setFont(m_testFont);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> listOfGlyphs = layout.glyphRuns();

    QCOMPARE(listOfGlyphs.size(), 1);

    QGlyphRun glyphs = listOfGlyphs.at(0);

    QCOMPARE(glyphs.glyphIndexes().size(), 2);
    QCOMPARE(glyphs.glyphIndexes().at(0), quint32(2));
    QCOMPARE(glyphs.glyphIndexes().at(1), quint32(1));
}

void tst_QGlyphRun::drawExistingGlyphs()
{
    QPixmap textLayoutDraw(1000, 1000);
    QPixmap drawGlyphs(1000, 1000);

    textLayoutDraw.fill(Qt::white);
    drawGlyphs.fill(Qt::white);

    QString s;
    s.append(QLatin1Char('A'));
    s.append(QChar(0xe000));

    QTextLayout layout(s);
    layout.setFont(m_testFont);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    {
        QPainter p(&textLayoutDraw);
        layout.draw(&p, QPointF(50, 50));
    }

    QGlyphRun glyphs = layout.glyphRuns().size() > 0
                                 ? layout.glyphRuns().at(0)
                                 : QGlyphRun();

    {
        QPainter p(&drawGlyphs);
        p.drawGlyphRun(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawExistingGlyphs_textLayoutDraw.png");
    drawGlyphs.save("drawExistingGlyphs_drawGlyphIndexes.png");
#endif

    QCOMPARE(textLayoutDraw, drawGlyphs);
}

void tst_QGlyphRun::setRawData()
{
    QGlyphRun glyphRun;
    glyphRun.setRawFont(QRawFont::fromFont(m_testFont));
    glyphRun.setGlyphIndexes(QList<quint32>() << 2 << 2 << 2);
    glyphRun.setPositions(QList<QPointF>() << QPointF(2, 3) << QPointF(20, 3) << QPointF(10, 20));

    QPixmap baseline(100, 50);
    baseline.fill(Qt::white);
    {
        QPainter p(&baseline);
        p.drawGlyphRun(QPointF(3, 2), glyphRun);
    }

    QGlyphRun baselineCopied = glyphRun;

    quint32 glyphIndexArray[3] = { 2, 2, 2 };
    QPointF glyphPositionArray[3] = { QPointF(2, 3), QPointF(20, 3), QPointF(10, 20) };

    glyphRun.setRawData(glyphIndexArray, glyphPositionArray, 3);

    QPixmap rawDataGlyphs(100, 50);
    rawDataGlyphs.fill(Qt::white);
    {
        QPainter p(&rawDataGlyphs);
        p.drawGlyphRun(QPointF(3, 2), glyphRun);
    }

    quint32 otherGlyphIndexArray[1] = { 2 };
    QPointF otherGlyphPositionArray[1] = { QPointF(2, 3) };

    glyphRun.setRawData(otherGlyphIndexArray, otherGlyphPositionArray, 1);

    QPixmap baselineCopiedPixmap(100, 50);
    baselineCopiedPixmap.fill(Qt::white);
    {
        QPainter p(&baselineCopiedPixmap);
        p.drawGlyphRun(QPointF(3, 2), baselineCopied);
    }

#if defined(DEBUG_SAVE_IMAGE)
    baseline.save("setRawData_baseline.png");
    rawDataGlyphs.save("setRawData_rawDataGlyphs.png");
    baselineCopiedPixmap.save("setRawData_baselineCopiedPixmap.png");
#endif

    QCOMPARE(rawDataGlyphs, baseline);
    QCOMPARE(baselineCopiedPixmap, baseline);
}

void tst_QGlyphRun::setRawDataAndGetAsVector()
{
    QList<quint32> glyphIndexArray;
    glyphIndexArray << 3 << 2 << 1 << 4;

    QList<QPointF> glyphPositionArray;
    glyphPositionArray << QPointF(1, 2) << QPointF(3, 4) << QPointF(5, 6) << QPointF(7, 8);

    QGlyphRun glyphRun;
    glyphRun.setRawData(glyphIndexArray.constData(), glyphPositionArray.constData(), 4);

    QList<quint32> glyphIndexes = glyphRun.glyphIndexes();
    QList<QPointF> glyphPositions = glyphRun.positions();

    QCOMPARE(glyphIndexes.size(), 4);
    QCOMPARE(glyphPositions.size(), 4);

    QCOMPARE(glyphIndexes, glyphIndexArray);
    QCOMPARE(glyphPositions, glyphPositionArray);

    QGlyphRun otherGlyphRun;
    otherGlyphRun.setGlyphIndexes(glyphIndexArray);
    otherGlyphRun.setPositions(glyphPositionArray);

    QCOMPARE(glyphRun, otherGlyphRun);
}

void tst_QGlyphRun::drawNonExistentGlyphs()
{
    QList<quint32> glyphIndexes;
    glyphIndexes.append(4);

    QList<QPointF> glyphPositions;
    glyphPositions.append(QPointF(0, 0));

    QGlyphRun glyphs;
    glyphs.setGlyphIndexes(glyphIndexes);
    glyphs.setPositions(glyphPositions);
    glyphs.setRawFont(QRawFont::fromFont(m_testFont));

    QPixmap image(1000, 1000);
    image.fill(Qt::white);

    QPixmap imageBefore = image;
    {
        QPainter p(&image);
        p.drawGlyphRun(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    image.save("drawNonExistentGlyphs.png");
#endif

    QCOMPARE(image, imageBefore); // Should be unchanged
}

void tst_QGlyphRun::drawMultiScriptText1()
{
    QString text;
    text += QChar(0x03D0); // Greek, beta

    QTextLayout textLayout(text);
    textLayout.setCacheEnabled(true);
    textLayout.beginLayout();
    textLayout.createLine();
    textLayout.endLayout();

    QPixmap textLayoutDraw(1000, 1000);
    textLayoutDraw.fill(Qt::white);

    QPixmap drawGlyphs(1000, 1000);
    drawGlyphs.fill(Qt::white);

    QList<QGlyphRun> glyphsList = textLayout.glyphRuns();
    QCOMPARE(glyphsList.size(), 1);

    {
        QPainter p(&textLayoutDraw);
        textLayout.draw(&p, QPointF(50, 50));
    }

    {
        QPainter p(&drawGlyphs);
        foreach (QGlyphRun glyphs, glyphsList)
            p.drawGlyphRun(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawMultiScriptText1_textLayoutDraw.png");
    drawGlyphs.save("drawMultiScriptText1_drawGlyphIndexes.png");
#endif

    QCOMPARE(drawGlyphs, textLayoutDraw);
}


void tst_QGlyphRun::drawMultiScriptText2()
{
    QString text;
    text += QChar(0x0621); // Arabic, Hamza
    text += QChar(0x03D0); // Greek, beta

    QTextLayout textLayout(text);
    textLayout.setCacheEnabled(true);
    textLayout.beginLayout();
    textLayout.createLine();
    textLayout.endLayout();

    QPixmap textLayoutDraw(1000, 1000);
    textLayoutDraw.fill(Qt::white);

    QPixmap drawGlyphs(1000, 1000);
    drawGlyphs.fill(Qt::white);

    QList<QGlyphRun> glyphsList = textLayout.glyphRuns();
    QCOMPARE(glyphsList.size(), 2);

    {
        QPainter p(&textLayoutDraw);
        textLayout.draw(&p, QPointF(50, 50));
    }

    {
        QPainter p(&drawGlyphs);
        foreach (QGlyphRun glyphs, glyphsList)
            p.drawGlyphRun(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawMultiScriptText2_textLayoutDraw.png");
    drawGlyphs.save("drawMultiScriptText2_drawGlyphIndexes.png");
#endif

    QCOMPARE(drawGlyphs, textLayoutDraw);
}

void tst_QGlyphRun::detach()
{

#define ARG(...) __VA_ARGS__

    QGlyphRun glyphs;

    glyphs.setGlyphIndexes(QList<quint32>() << 1 << 2 << 3);

    QGlyphRun otherGlyphs;
    otherGlyphs = glyphs;

    QCOMPARE(otherGlyphs.glyphIndexes(), glyphs.glyphIndexes());

    otherGlyphs.setGlyphIndexes(QList<quint32>() << 4 << 5 << 6);

    QCOMPARE(otherGlyphs.glyphIndexes(), ARG({4, 5, 6}));
    QCOMPARE(glyphs.glyphIndexes(), ARG({1, 2, 3}));

#undef ARG
}

void tst_QGlyphRun::drawRightToLeft()
{
    QString s;
    s.append(QChar(1575));
    s.append(QChar(1578));

    QPixmap textLayoutDraw(1000, 1000);
    QPixmap drawGlyphs(1000, 1000);

    textLayoutDraw.fill(Qt::white);
    drawGlyphs.fill(Qt::white);

    QTextLayout layout(s);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    {
        QPainter p(&textLayoutDraw);
        layout.draw(&p, QPointF(50, 50));
    }

    QGlyphRun glyphs = layout.glyphRuns().size() > 0
                                 ? layout.glyphRuns().at(0)
                                 : QGlyphRun();

    {
        QPainter p(&drawGlyphs);
        p.drawGlyphRun(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawRightToLeft_textLayoutDraw.png");
    drawGlyphs.save("drawRightToLeft_drawGlyphIndexes.png");
#endif

    QCOMPARE(textLayoutDraw, drawGlyphs);

}

void tst_QGlyphRun::boundingRect()
{
    QString s(QLatin1String("AbCdE"));

    QRawFont rawFont(QRawFont::fromFont(QFont()));
    QVERIFY(rawFont.isValid());
    QList<quint32> glyphIndexes = rawFont.glyphIndexesForString(s);
    QList<QPointF> positions = rawFont.advancesForGlyphIndexes(glyphIndexes);
    QCOMPARE(glyphIndexes.size(), s.size());
    QCOMPARE(positions.size(), glyphIndexes.size());

    QGlyphRun glyphs;
    glyphs.setRawFont(rawFont);
    glyphs.setGlyphIndexes(glyphIndexes);
    glyphs.setPositions(positions);

    QRectF boundingRect = glyphs.boundingRect();

    glyphs.clear();
    glyphs.setRawFont(rawFont);
    glyphs.setRawData(glyphIndexes.constData(), positions.constData(), glyphIndexes.size());
    QCOMPARE(glyphs.boundingRect(), boundingRect);

    boundingRect = QRectF(0, 0, 1, 1);
    glyphs.setBoundingRect(boundingRect);
    QCOMPARE(glyphs.boundingRect(), boundingRect);
}

void tst_QGlyphRun::mixedScripts()
{
    if (QFontDatabase::families(QFontDatabase::Korean).isEmpty())
        QSKIP("This test requires support for Hangul text");

    QString s;
    s += QChar(0x31); // The character '1'
    s += QChar(0xbc14); // Hangul character

    QTextLayout layout;
    layout.setFont(m_testFont);
    layout.setText(s);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> glyphRuns = layout.glyphRuns();
    QCOMPARE(glyphRuns.size(), 2);
}

void tst_QGlyphRun::multiLineBoundingRect()
{
    QTextLayout layout;
    layout.setText("Foo Bar");
    layout.beginLayout();

    QTextLine line = layout.createLine();
    line.setNumColumns(4);
    line.setPosition(QPointF(0, 0));

    line = layout.createLine();
    line.setPosition(QPointF(0, 10));

    layout.endLayout();

    QCOMPARE(layout.lineCount(), 2);

    QList<QGlyphRun> firstLineGlyphRuns = layout.lineAt(0).glyphRuns();
    QList<QGlyphRun> allGlyphRuns = layout.glyphRuns();
    QCOMPARE(firstLineGlyphRuns.size(), 1);
    QCOMPARE(allGlyphRuns.size(), 1);

    QGlyphRun firstLineGlyphRun = firstLineGlyphRuns.first();
    QGlyphRun allGlyphRun = allGlyphRuns.first();

    QVERIFY(firstLineGlyphRun.boundingRect().height() < allGlyphRun.boundingRect().height());
}

void tst_QGlyphRun::defaultIgnorables()
{
    {
        QTextLayout layout;
        layout.setFont(QFont("QtsSpecialTestFont"));
        layout.setText(QChar(0x200D));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> runs = layout.glyphRuns();
        QCOMPARE(runs.size(), 0);
    }

    {
        QTextLayout layout;
        layout.setFont(QFont("QtsSpecialTestFont"));
        layout.setText(QStringLiteral("AAA") + QChar(0xFE0F) + QStringLiteral("111"));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> runs = layout.glyphRuns();
        QVERIFY(!runs.isEmpty());

        bool hasFullMainFontRun = false;
        for (const QGlyphRun &run : runs) {
            // QtsSpecialFont will be used for at least five characters: AA[...]111
            // Depending on the font selected for the 0xFE0F variant selector, the
            // third 'A' may be in QtsSpecialFont or in the fallback. This is platform-specific,
            // so we accept either.
            if (run.rawFont().familyName() == QStringLiteral("QtsSpecialTestFont")
                    && run.glyphIndexes().size() >= 5) {
                hasFullMainFontRun = true;
                break;
            }
        }
        QVERIFY(hasFullMainFontRun);
    }
}

void tst_QGlyphRun::stringIndexes()
{
    int ligatureFontId = QFontDatabase::addApplicationFont(QFINDTESTDATA("Ligatures.otf"));
    QVERIFY(ligatureFontId >= 0);

    QFont ligatureFont = QFont("QtLigatures");
    QCOMPARE(QFontInfo(ligatureFont).family(), QString::fromLatin1("QtLigatures"));

    QTextLayout::GlyphRunRetrievalFlags retrievalFlags
            = QTextLayout::RetrieveGlyphIndexes | QTextLayout::RetrieveStringIndexes;

    // Three characters -> three glyphs
    {
        QTextLayout layout;
        layout.setText("f i");
        layout.setFont(ligatureFont);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(-1, -1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 3);
            QCOMPARE(stringIndexes.at(0), 0);
            QCOMPARE(stringIndexes.at(1), 1);
            QCOMPARE(stringIndexes.at(2), 2);
        }

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(2, -1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 1);
            QCOMPARE(stringIndexes.at(0), 2);
        }
    }

    // Two characters -> one glyph
    {
        QTextLayout layout;
        layout.setText("fi");
        layout.setFont(ligatureFont);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(-1, -1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 1);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(233));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 1);
            QCOMPARE(stringIndexes.at(0), 0);
        }

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(1, -1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 1);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(233));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 1);
            QCOMPARE(stringIndexes.at(0), 1);
        }
    }

    // Four characters -> three glyphs
    {
        QTextLayout layout;
        layout.setText("ffii");
        layout.setFont(ligatureFont);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(-1, -1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 3);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(71));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(1), uint(233));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(2), uint(74));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 3);
            QCOMPARE(stringIndexes.at(0), uint(0));
            QCOMPARE(stringIndexes.at(1), uint(1));
            QCOMPARE(stringIndexes.at(2), uint(3));
        }

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(1, 1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 1);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(233));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 1);
            QCOMPARE(stringIndexes.at(0), uint(1));
        }

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(1, 2, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 1);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(233));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 1);
            QCOMPARE(stringIndexes.at(0), uint(1));
        }

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(1, 3, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 2);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(233));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(1), uint(74));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 2);
            QCOMPARE(stringIndexes.at(0), 1);
            QCOMPARE(stringIndexes.at(1), 3);
        }

    }

    // One character -> two glyphs
    {
        QTextLayout layout;
        layout.setText(QChar(0xe6)); // LATIN SMALL LETTER AE
        layout.setFont(ligatureFont);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns(-1, -1, retrievalFlags);
        QCOMPARE(glyphRuns.size(), 1);

        QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 2);
        QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(66));
        QCOMPARE(glyphRuns.at(0).glyphIndexes().at(1), uint(70));

        QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
        QCOMPARE(stringIndexes.size(), 2);
        QCOMPARE(stringIndexes.at(0), uint(0));
        QCOMPARE(stringIndexes.at(1), uint(0));
    }

    // Three characters -> four glyphs
    {
        QTextLayout layout;
        layout.setText(QString('f') + QChar(0xe6) + QChar('i'));
        layout.setFont(ligatureFont);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(-1, -1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 4);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(71));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(1), uint(66));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(2), uint(70));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(3), uint(74));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 4);
            QCOMPARE(stringIndexes.at(0), 0);
            QCOMPARE(stringIndexes.at(1), 1);
            QCOMPARE(stringIndexes.at(2), 1);
            QCOMPARE(stringIndexes.at(3), 2);
        }

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(1, -1, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 3);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(66));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(1), uint(70));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(2), uint(74));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 3);
            QCOMPARE(stringIndexes.at(0), 1);
            QCOMPARE(stringIndexes.at(1), 1);
            QCOMPARE(stringIndexes.at(2), 2);
        }

        {
            QList<QGlyphRun> glyphRuns = layout.glyphRuns(0, 2, retrievalFlags);
            QCOMPARE(glyphRuns.size(), 1);

            QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 3);
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(71));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(1), uint(66));
            QCOMPARE(glyphRuns.at(0).glyphIndexes().at(2), uint(70));

            QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
            QCOMPARE(stringIndexes.size(), 3);
            QCOMPARE(stringIndexes.at(0), 0);
            QCOMPARE(stringIndexes.at(1), 1);
            QCOMPARE(stringIndexes.at(2), 1);
        }


    }

    // Five characters -> five glyphs
    {
        QTextLayout layout;
        layout.setText(QLatin1String("ffi") + QChar(0xe6) + QLatin1Char('i'));
        layout.setFont(ligatureFont);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QList<QGlyphRun> glyphRuns = layout.glyphRuns(-1, -1, retrievalFlags);
        QCOMPARE(glyphRuns.size(), 1);

        QCOMPARE(glyphRuns.at(0).glyphIndexes().size(), 5);
        QCOMPARE(glyphRuns.at(0).glyphIndexes().at(0), uint(71));
        QCOMPARE(glyphRuns.at(0).glyphIndexes().at(1), uint(233));
        QCOMPARE(glyphRuns.at(0).glyphIndexes().at(2), uint(66));
        QCOMPARE(glyphRuns.at(0).glyphIndexes().at(3), uint(70));
        QCOMPARE(glyphRuns.at(0).glyphIndexes().at(4), uint(74));

        QList<qsizetype> stringIndexes = glyphRuns.at(0).stringIndexes();
        QCOMPARE(stringIndexes.size(), 5);
        QCOMPARE(stringIndexes.at(0), 0);
        QCOMPARE(stringIndexes.at(1), 1);
        QCOMPARE(stringIndexes.at(2), 3);
        QCOMPARE(stringIndexes.at(3), 3);
        QCOMPARE(stringIndexes.at(4), 4);
    }

}

void tst_QGlyphRun::retrievalFlags_data()
{
    QTest::addColumn<QTextLayout::GlyphRunRetrievalFlags>("flags");
    QTest::addColumn<bool>("expectedGlyphIndexes");
    QTest::addColumn<bool>("expectedStringIndexes");
    QTest::addColumn<bool>("expectedString");
    QTest::addColumn<bool>("expectedGlyphPositions");

    QTest::newRow("Glyph indexes")
            << QTextLayout::GlyphRunRetrievalFlags(QTextLayout::RetrieveGlyphIndexes)
            << true << false << false << false;
    QTest::newRow("Glyph Positions")
            << QTextLayout::GlyphRunRetrievalFlags(QTextLayout::RetrieveGlyphPositions)
            << false << false << false << true;
    QTest::newRow("String indexes")
            << QTextLayout::GlyphRunRetrievalFlags(QTextLayout::RetrieveStringIndexes)
            << false << true << false << false;
    QTest::newRow("String")
            << QTextLayout::GlyphRunRetrievalFlags(QTextLayout::RetrieveString)
            << false << false << true << false;

    QTest::newRow("Default")
            << QTextLayout::GlyphRunRetrievalFlags(QTextLayout::DefaultRetrievalFlags)
            << true << false << false << true;
    QTest::newRow("All")
            << QTextLayout::GlyphRunRetrievalFlags(QTextLayout::RetrieveAll)
            << true << true << true << true;
}

void tst_QGlyphRun::retrievalFlags()
{
    QFETCH(QTextLayout::GlyphRunRetrievalFlags, flags);
    QFETCH(bool, expectedGlyphIndexes);
    QFETCH(bool, expectedStringIndexes);
    QFETCH(bool, expectedString);
    QFETCH(bool, expectedGlyphPositions);

    QTextLayout layout;
    layout.setText(QLatin1String("abc"));
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> glyphRuns = layout.glyphRuns(-1, -1, flags);
    QVERIFY(!glyphRuns.isEmpty());

    QGlyphRun firstGlyphRun = glyphRuns.first();
    QCOMPARE(firstGlyphRun.glyphIndexes().isEmpty(), !expectedGlyphIndexes);
    QCOMPARE(firstGlyphRun.stringIndexes().isEmpty(), !expectedStringIndexes);
    QCOMPARE(firstGlyphRun.sourceString().isEmpty(), !expectedString);
    QCOMPARE(firstGlyphRun.positions().isEmpty(), !expectedGlyphPositions);
}

void tst_QGlyphRun::objectReplacementCharacter()
{
    QTextLayout layout;
    layout.setFont(m_testFont);
    layout.setText(QStringLiteral("\uFFFC"));
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> glyphRuns = layout.glyphRuns();
    QCOMPARE(glyphRuns.size(), 1);
    QCOMPARE(glyphRuns.first().glyphIndexes().size(), 1);
    QCOMPARE(glyphRuns.first().glyphIndexes().first(), uint(5));
}

#endif // QT_NO_RAWFONT

QTEST_MAIN(tst_QGlyphRun)
#include "tst_qglyphrun.moc"

