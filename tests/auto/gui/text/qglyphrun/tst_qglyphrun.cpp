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
    QTextLayout layout;
    layout.setFont(QFont("QtsSpecialTestFont"));
    layout.setText(QChar(0x200D));
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphRun> runs = layout.glyphRuns();
    QCOMPARE(runs.size(), 1);
    QCOMPARE(runs.at(0).glyphIndexes().size(), 1);
    QCOMPARE(runs.at(0).glyphIndexes()[0], uint(0));
}

#endif // QT_NO_RAWFONT

QTEST_MAIN(tst_QGlyphRun)
#include "tst_qglyphrun.moc"

