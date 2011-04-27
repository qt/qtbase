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

#include <qglyphs.h>
#include <qpainter.h>
#include <qtextlayout.h>
#include <qfontdatabase.h>

// #define DEBUG_SAVE_IMAGE

class tst_QGlyphs: public QObject
{
    Q_OBJECT

#if !defined(QT_NO_RAWFONT)
private slots:
    void initTestCase();
    void cleanupTestCase();

    void constructionAndDestruction();
    void copyConstructor();
    void assignment();
    void equalsOperator_data();
    void equalsOperator();
    void textLayoutGlyphIndexes();
    void drawExistingGlyphs();
    void drawNonExistentGlyphs();
    void drawMultiScriptText1();
    void drawMultiScriptText2();
    void drawStruckOutText();
    void drawOverlinedText();
    void drawUnderlinedText();
    void drawRightToLeft();
    void detach();

private:
    int m_testFontId;
    QFont m_testFont;
#endif // QT_NO_RAWFONT

};

#if !defined(QT_NO_RAWFONT)

Q_DECLARE_METATYPE(QGlyphs);

void tst_QGlyphs::initTestCase()
{
    m_testFontId = QFontDatabase::addApplicationFont(SRCDIR "test.ttf");
    QVERIFY(m_testFontId >= 0);

    m_testFont = QFont("QtsSpecialTestFont");

    QCOMPARE(QFontInfo(m_testFont).family(), QString::fromLatin1("QtsSpecialTestFont"));
}

void tst_QGlyphs::cleanupTestCase()
{
    QFontDatabase::removeApplicationFont(m_testFontId);
}

void tst_QGlyphs::constructionAndDestruction()
{
    QGlyphs glyphIndexes;
}

static QGlyphs make_dummy_indexes()
{
    QGlyphs glyphs;

    QVector<quint32> glyphIndexes;
    QVector<QPointF> positions;
    QFont font;
    font.setPointSize(18);

    glyphIndexes.append(1);
    glyphIndexes.append(2);
    glyphIndexes.append(3);

    positions.append(QPointF(1, 2));
    positions.append(QPointF(3, 4));
    positions.append(QPointF(5, 6));

    glyphs.setFont(QRawFont::fromFont(font));
    glyphs.setGlyphIndexes(glyphIndexes);
    glyphs.setPositions(positions);

    return glyphs;
}

void tst_QGlyphs::copyConstructor()
{
    QGlyphs glyphs;

    {
        QVector<quint32> glyphIndexes;
        QVector<QPointF> positions;
        QFont font;
        font.setPointSize(18);

        glyphIndexes.append(1);
        glyphIndexes.append(2);
        glyphIndexes.append(3);

        positions.append(QPointF(1, 2));
        positions.append(QPointF(3, 4));
        positions.append(QPointF(5, 6));

        glyphs.setFont(QRawFont::fromFont(font));
        glyphs.setGlyphIndexes(glyphIndexes);
        glyphs.setPositions(positions);
    }

    QGlyphs otherGlyphs(glyphs);
    QCOMPARE(otherGlyphs.font(), glyphs.font());
    QCOMPARE(glyphs.glyphIndexes(), otherGlyphs.glyphIndexes());
    QCOMPARE(glyphs.positions(), otherGlyphs.positions());
}

void tst_QGlyphs::assignment()
{
    QGlyphs glyphs(make_dummy_indexes());

    QGlyphs otherGlyphs = glyphs;
    QCOMPARE(otherGlyphs.font(), glyphs.font());
    QCOMPARE(glyphs.glyphIndexes(), otherGlyphs.glyphIndexes());
    QCOMPARE(glyphs.positions(), otherGlyphs.positions());
}

void tst_QGlyphs::equalsOperator_data()
{
    QTest::addColumn<QGlyphs>("one");
    QTest::addColumn<QGlyphs>("two");
    QTest::addColumn<bool>("equals");

    QGlyphs one(make_dummy_indexes());
    QGlyphs two(make_dummy_indexes());

    QTest::newRow("Identical") << one << two << true;

    {
        QGlyphs busted(two);

        QVector<QPointF> positions = busted.positions();
        positions[2] += QPointF(1, 1);
        busted.setPositions(positions);


        QTest::newRow("Different positions") << one << busted << false;
    }

    {
        QGlyphs busted(two);

        QFont font;
        font.setPixelSize(busted.font().pixelSize() * 2);
        busted.setFont(QRawFont::fromFont(font));

        QTest::newRow("Different fonts") << one << busted << false;
    }

    {
        QGlyphs busted(two);

        QVector<quint32> glyphIndexes = busted.glyphIndexes();
        glyphIndexes[2] += 1;
        busted.setGlyphIndexes(glyphIndexes);

        QTest::newRow("Different glyph indexes") << one << busted << false;
    }

}

void tst_QGlyphs::equalsOperator()
{
    QFETCH(QGlyphs, one);
    QFETCH(QGlyphs, two);
    QFETCH(bool, equals);

    QCOMPARE(one == two, equals);
    QCOMPARE(one != two, !equals);
}


void tst_QGlyphs::textLayoutGlyphIndexes()
{
    QString s;
    s.append(QLatin1Char('A'));
    s.append(QChar(0xe000));

    QTextLayout layout(s);
    layout.setFont(m_testFont);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QList<QGlyphs> listOfGlyphs = layout.glyphs();
    QCOMPARE(listOfGlyphs.size(), 1);

    QGlyphs glyphs = listOfGlyphs.at(0);

    QCOMPARE(glyphs.glyphIndexes().size(), 2);
    QCOMPARE(glyphs.glyphIndexes().at(0), quint32(2));
    QCOMPARE(glyphs.glyphIndexes().at(1), quint32(1));
}

void tst_QGlyphs::drawExistingGlyphs()
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
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    {
        QPainter p(&textLayoutDraw);
        layout.draw(&p, QPointF(50, 50));
    }

    QGlyphs glyphs = layout.glyphs().size() > 0
                                 ? layout.glyphs().at(0)
                                 : QGlyphs();

    {
        QPainter p(&drawGlyphs);
        p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawExistingGlyphs_textLayoutDraw.png");
    drawGlyphs.save("drawExistingGlyphs_drawGlyphIndexes.png");
#endif

    QCOMPARE(textLayoutDraw, drawGlyphs);
}

void tst_QGlyphs::drawNonExistentGlyphs()
{
    QVector<quint32> glyphIndexes;
    glyphIndexes.append(3);

    QVector<QPointF> glyphPositions;
    glyphPositions.append(QPointF(0, 0));

    QGlyphs glyphs;
    glyphs.setGlyphIndexes(glyphIndexes);
    glyphs.setPositions(glyphPositions);
    glyphs.setFont(QRawFont::fromFont(m_testFont));

    QPixmap image(1000, 1000);
    image.fill(Qt::white);

    QPixmap imageBefore = image;
    {
        QPainter p(&image);
        p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    image.save("drawNonExistentGlyphs.png");
#endif

    QCOMPARE(image, imageBefore); // Should be unchanged
}

void tst_QGlyphs::drawMultiScriptText1()
{
    QString text;
    text += QChar(0x03D0); // Greek, beta

    QTextLayout textLayout(text);
    textLayout.beginLayout();
    textLayout.createLine();
    textLayout.endLayout();

    QPixmap textLayoutDraw(1000, 1000);
    textLayoutDraw.fill(Qt::white);

    QPixmap drawGlyphs(1000, 1000);
    drawGlyphs.fill(Qt::white);

    QList<QGlyphs> glyphsList = textLayout.glyphs();
    QCOMPARE(glyphsList.size(), 1);

    {
        QPainter p(&textLayoutDraw);
        textLayout.draw(&p, QPointF(50, 50));
    }

    {
        QPainter p(&drawGlyphs);
        foreach (QGlyphs glyphs, glyphsList)
            p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawMultiScriptText1_textLayoutDraw.png");
    drawGlyphs.save("drawMultiScriptText1_drawGlyphIndexes.png");
#endif

    QCOMPARE(drawGlyphs, textLayoutDraw);
}


void tst_QGlyphs::drawMultiScriptText2()
{
    QString text;
    text += QChar(0x0621); // Arabic, Hamza
    text += QChar(0x03D0); // Greek, beta

    QTextLayout textLayout(text);
    textLayout.beginLayout();
    textLayout.createLine();
    textLayout.endLayout();

    QPixmap textLayoutDraw(1000, 1000);
    textLayoutDraw.fill(Qt::white);

    QPixmap drawGlyphs(1000, 1000);
    drawGlyphs.fill(Qt::white);

    QList<QGlyphs> glyphsList = textLayout.glyphs();
    QCOMPARE(glyphsList.size(), 2);

    {
        QPainter p(&textLayoutDraw);
        textLayout.draw(&p, QPointF(50, 50));
    }

    {
        QPainter p(&drawGlyphs);
        foreach (QGlyphs glyphs, glyphsList)
            p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawMultiScriptText2_textLayoutDraw.png");
    drawGlyphs.save("drawMultiScriptText2_drawGlyphIndexes.png");
#endif

    QCOMPARE(drawGlyphs, textLayoutDraw);
}

void tst_QGlyphs::detach()
{
    QGlyphs glyphs;

    glyphs.setGlyphIndexes(QVector<quint32>() << 1 << 2 << 3);

    QGlyphs otherGlyphs;
    otherGlyphs = glyphs;

    QCOMPARE(otherGlyphs.glyphIndexes(), glyphs.glyphIndexes());

    otherGlyphs.setGlyphIndexes(QVector<quint32>() << 4 << 5 << 6);

    QCOMPARE(otherGlyphs.glyphIndexes(), QVector<quint32>() << 4 << 5 << 6);
    QCOMPARE(glyphs.glyphIndexes(), QVector<quint32>() << 1 << 2 << 3);
}

void tst_QGlyphs::drawStruckOutText()
{
    QPixmap textLayoutDraw(1000, 1000);
    QPixmap drawGlyphs(1000, 1000);

    textLayoutDraw.fill(Qt::white);
    drawGlyphs.fill(Qt::white);

    QString s = QString::fromLatin1("Foobar");

    QFont font;
    font.setStrikeOut(true);

    QTextLayout layout(s);
    layout.setFont(font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    {
        QPainter p(&textLayoutDraw);
        layout.draw(&p, QPointF(50, 50));
    }

    QGlyphs glyphs = layout.glyphs().size() > 0
                                 ? layout.glyphs().at(0)
                                 : QGlyphs();

    {
        QPainter p(&drawGlyphs);
        p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawStruckOutText_textLayoutDraw.png");
    drawGlyphs.save("drawStruckOutText_drawGlyphIndexes.png");
#endif

    QCOMPARE(textLayoutDraw, drawGlyphs);
}

void tst_QGlyphs::drawOverlinedText()
{
    QPixmap textLayoutDraw(1000, 1000);
    QPixmap drawGlyphs(1000, 1000);

    textLayoutDraw.fill(Qt::white);
    drawGlyphs.fill(Qt::white);

    QString s = QString::fromLatin1("Foobar");

    QFont font;
    font.setOverline(true);

    QTextLayout layout(s);
    layout.setFont(font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    {
        QPainter p(&textLayoutDraw);
        layout.draw(&p, QPointF(50, 50));
    }

    QGlyphs glyphs = layout.glyphs().size() > 0
                                 ? layout.glyphs().at(0)
                                 : QGlyphs();

    {
        QPainter p(&drawGlyphs);
        p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawOverlineText_textLayoutDraw.png");
    drawGlyphs.save("drawOverlineText_drawGlyphIndexes.png");
#endif

    QCOMPARE(textLayoutDraw, drawGlyphs);
}

void tst_QGlyphs::drawUnderlinedText()
{
    QPixmap textLayoutDraw(1000, 1000);
    QPixmap drawGlyphs(1000, 1000);

    textLayoutDraw.fill(Qt::white);
    drawGlyphs.fill(Qt::white);

    QString s = QString::fromLatin1("Foobar");

    QFont font;
    font.setUnderline(true);

    QTextLayout layout(s);
    layout.setFont(font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    {
        QPainter p(&textLayoutDraw);
        layout.draw(&p, QPointF(50, 50));
    }

    QGlyphs glyphs = layout.glyphs().size() > 0
                                 ? layout.glyphs().at(0)
                                 : QGlyphs();

    {
        QPainter p(&drawGlyphs);
        p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawUnderlineText_textLayoutDraw.png");
    drawGlyphs.save("drawUnderlineText_drawGlyphIndexes.png");
#endif

    QCOMPARE(textLayoutDraw, drawGlyphs);
}

void tst_QGlyphs::drawRightToLeft()
{
    QString s;
    s.append(QChar(1575));
    s.append(QChar(1578));

    QPixmap textLayoutDraw(1000, 1000);
    QPixmap drawGlyphs(1000, 1000);

    textLayoutDraw.fill(Qt::white);
    drawGlyphs.fill(Qt::white);

    QFont font;
    font.setUnderline(true);

    QTextLayout layout(s);
    layout.setFont(font);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    {
        QPainter p(&textLayoutDraw);
        layout.draw(&p, QPointF(50, 50));
    }

    QGlyphs glyphs = layout.glyphs().size() > 0
                                 ? layout.glyphs().at(0)
                                 : QGlyphs();

    {
        QPainter p(&drawGlyphs);
        p.drawGlyphs(QPointF(50, 50), glyphs);
    }

#if defined(DEBUG_SAVE_IMAGE)
    textLayoutDraw.save("drawRightToLeft_textLayoutDraw.png");
    drawGlyphs.save("drawRightToLeft_drawGlyphIndexes.png");
#endif

    QCOMPARE(textLayoutDraw, drawGlyphs);

}

#endif // QT_NO_RAWFONT

QTEST_MAIN(tst_QGlyphs)
#include "tst_qglyphs.moc"

