// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


/*
    !!!!!! Warning !!!!!
    Please don't save this file in emacs. It contains utf8 text sequences emacs will
    silently convert to a series of question marks.
 */
#include <QTest>



#include <private/qtextengine_p.h>
#include <qtextlayout.h>

#include <qdebug.h>


#define TESTFONT_SIZE 12

Q_DECLARE_METATYPE(QTextOption::WrapMode)
Q_DECLARE_METATYPE(Qt::LayoutDirection)
Q_DECLARE_METATYPE(Qt::AlignmentFlag)

class tst_QTextLayout : public QObject
{
    Q_OBJECT

public:
    tst_QTextLayout();

private slots:
    void init();
    void cleanup();
    void getSetCheck();
    void lineBreaking();
#ifdef QT_BUILD_INTERNAL
    void simpleBoundingRect();
    void threeLineBoundingRect_data();
    void threeLineBoundingRect();
    void boundingRectWithLongLineAndNoWrap();
    void forcedBreaks();
    void breakAny();
    void noWrap();

    void cursorToXForInlineObjects();
    void cursorToXForSetColumns();
    void cursorToXForTrailingSpaces_data();
    void cursorToXForTrailingSpaces();
    void cursorToXInvalidInput();
    void cursorToXForBidiBoundaries_data();
    void cursorToXForBidiBoundaries();

    void horizontalAlignment_data();
    void horizontalAlignment();
    void horizontalAlignmentMultiline_data();
    void horizontalAlignmentMultiline();
#endif
    void defaultWordSeparators_data();
    void defaultWordSeparators();
    void cursorMovementFromInvalidPositions();
    void cursorMovementInsideSpaces();
    void charWordStopOnLineSeparator();
#ifdef QT_BUILD_INTERNAL
    void xToCursorAtEndOfLine();
#endif
    void xToCursorForBidiEnds_data();
    void xToCursorForBidiEnds();
    void boundingRectTopLeft();
    void graphemeBoundaryForSurrogatePairs();
    void tabStops();
    void integerOverflow();
#ifdef QT_BUILD_INTERNAL
    void testDefaultTabs();
    void testTabs();
    void testMultilineTab();
    void testRightTab();
    void testTabsInAlignedParag();
    void testCenteredTab();
    void testDelimiterTab();
    void testMultiTab();
    void testTabDPIScale();
    void tabsForRtl();
#endif
    void tabHeight();
    void capitalization_allUpperCase();
    void capitalization_allUpperCase_newline();
    void capitalization_allLowerCase();
    void capitalization_smallCaps();
    void capitalization_capitalize();
    void longText();
    void widthOfTabs();
    void columnWrapWithTabs();
    void boundingRectForUnsetLineWidth();
    void boundingRectForSetLineWidth();
    void glyphLessItems();
    void justifyTrailingSpaces();
    void layoutWithCustomTabStops();

    // QTextLine stuff
#ifdef QT_BUILD_INTERNAL
    void setNumColumnsWrapAtWordBoundaryOrAnywhere();
    void setNumColumnsWordWrap();
    void smallTextLengthNoWrap();
    void smallTextLengthWordWrap();
    void smallTextLengthWrapAtWordBoundaryOrAnywhere();
    void testLineBreakingAllSpaces();
#endif
    void lineWidthFromBOM();
    void textWidthVsWIdth();
    void textWithSurrogates_qtbug15679();
    void textWidthWithStackedTextEngine();
    void textWidthWithLineSeparator();
    void cursorInLigatureWithMultipleLines();
    void xToCursorForLigatures();
    void cursorInNonStopChars();
    void nbsp();
    void nbspWithFormat();
    void noModificationOfInputString();
    void superscriptCrash_qtbug53911();
    void showLineAndParagraphSeparatorsCrash();
    void koreanWordWrap();
    void tooManyDirectionalCharctersCrash_qtbug77819();
    void softHyphens_data();
    void softHyphens();
    void min_maximumWidth_data();
    void min_maximumWidth();
    void negativeLineWidth();

private:
    QFont testFont;
};

// Testing get/set functions
void tst_QTextLayout::getSetCheck()
{
    QString str("Bogus text");
    QTextLayout layout(str);
    layout.beginLayout();
    QTextEngine *engine = layout.engine();
    QTextInlineObject obj1(0, engine);
    // qreal QTextInlineObject::width()
    // void QTextInlineObject::setWidth(qreal)
    obj1.setWidth(0.0);
    QCOMPARE(0.0, obj1.width());
    obj1.setWidth(1.2);
    QVERIFY(1.0 < obj1.width());

    // qreal QTextInlineObject::ascent()
    // void QTextInlineObject::setAscent(qreal)
    obj1.setAscent(0.0);
    QCOMPARE(0.0, obj1.ascent());
    obj1.setAscent(1.2);
    QVERIFY(1.0 < obj1.ascent());

    // qreal QTextInlineObject::descent()
    // void QTextInlineObject::setDescent(qreal)
    obj1.setDescent(0.0);
    QCOMPARE(0.0, obj1.descent());
    obj1.setDescent(1.2);
    QVERIFY(1.0 < obj1.descent());

    QTextLayout obj2;
    // bool QTextLayout::cacheEnabled()
    // void QTextLayout::setCacheEnabled(bool)
    obj2.setCacheEnabled(false);
    QCOMPARE(false, obj2.cacheEnabled());
    obj2.setCacheEnabled(true);
    QCOMPARE(true, obj2.cacheEnabled());
}

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
// qfontdatabase.cpp
Q_AUTOTEST_EXPORT void qt_setQtEnableTestFont(bool value);
QT_END_NAMESPACE
#endif

tst_QTextLayout::tst_QTextLayout()
{
#ifdef QT_BUILD_INTERNAL
    qt_setQtEnableTestFont(true);
#endif
}

void tst_QTextLayout::init()
{
    testFont = QFont();
#ifdef QT_BUILD_INTERNAL
    testFont.setFamily("__Qt__Box__Engine__");
#endif
    testFont.setPixelSize(TESTFONT_SIZE);
    testFont.setWeight(QFont::Normal);
#ifdef QT_BUILD_INTERNAL
    QCOMPARE(QFontMetrics(testFont).horizontalAdvance('a'), testFont.pixelSize());
#endif
}

void tst_QTextLayout::cleanup()
{
    testFont = QFont();
}

void tst_QTextLayout::lineBreaking()
{
#if 0
    struct Breaks {
        const char *utf8;
        uchar breaks[32];
    };
    Breaks brks[] = {
        { "11", { false, 0xff } },
        { "aa", { false, 0xff } },
        { "++", { false, 0xff } },
        { "--", { false, 0xff } },
        { "((", { false, 0xff } },
        { "))", { false, 0xff } },
        { "..", { false, 0xff } },
        { "\"\"", { false, 0xff } },
        { "$$", { false, 0xff } },
        { "!!", { false, 0xff } },
        { "??", { false, 0xff } },
        { ",,", { false, 0xff } },

        { ")()", { true, false, 0xff } },
        { "?!?", { false, false, 0xff } },
        { ".,.", { false, false, 0xff } },
        { "+-+", { false, false, 0xff } },
        { "+=+", { false, false, 0xff } },
        { "+(+", { false, false, 0xff } },
        { "+)+", { false, false, 0xff } },

        { "a b", { false, true, 0xff } },
        { "a(b", { false, false, 0xff } },
        { "a)b", { false, false, 0xff } },
        { "a-b", { false, true, 0xff } },
        { "a.b", { false, false, 0xff } },
        { "a+b", { false, false, 0xff } },
        { "a?b", { false, false, 0xff } },
        { "a!b", { false, false, 0xff } },
        { "a$b", { false, false, 0xff } },
        { "a,b", { false, false, 0xff } },
        { "a/b", { false, false, 0xff } },
        { "1/2", { false, false, 0xff } },
        { "./.", { false, false, 0xff } },
        { ",/,", { false, false, 0xff } },
        { "!/!", { false, false, 0xff } },
        { "\\/\\", { false, false, 0xff } },
        { "1 2", { false, true, 0xff } },
        { "1(2", { false, false, 0xff } },
        { "1)2", { false, false, 0xff } },
        { "1-2", { false, false, 0xff } },
        { "1.2", { false, false, 0xff } },
        { "1+2", { false, false, 0xff } },
        { "1?2", { false, true, 0xff } },
        { "1!2", { false, true, 0xff } },
        { "1$2", { false, false, 0xff } },
        { "1,2", { false, false, 0xff } },
        { "1/2", { false, false, 0xff } },
        { "\330\260\331\216\331\204\331\220\331\203\331\216", { false, false, false, false, false, 0xff } },
        { "\330\247\331\204\331\205 \330\247\331\204\331\205", { false, false, false, true, false, false, 0xff } },
        { "1#2", { false, false, 0xff } },
        { "!#!", { false, false, 0xff } },
        { 0, {} }
    };
    Breaks *b = brks;
    while (b->utf8) {
        QString str = QString::fromUtf8(b->utf8);
        QTextEngine engine(str, QFont());
        const QCharAttributes *attrs = engine.attributes();
        QVERIFY(!attrs[0].lineBreak);
        int i;
        for (i = 0; i < (int)str.length() - 1; ++i) {
            QVERIFY(b->breaks[i] != 0xff);
            if ( attrs[i + 1].lineBreak != (bool)b->breaks[i] ) {
                qDebug("test case \"%s\" failed at char %d; break type: %d", b->utf8, i, attrs[i + 1].lineBreak);
                QCOMPARE( attrs[i + 1].lineBreak, (bool)b->breaks[i] );
            }
        }
        QCOMPARE(b->breaks[i], (uchar)0xff);
        ++b;
    }
#endif
}

#ifdef QT_BUILD_INTERNAL
void tst_QTextLayout::simpleBoundingRect()
{
    /* just check if boundingRect() gives sane values. The text is not broken. */

    QString hello("hello world");

    const int width = hello.size() * testFont.pixelSize();

    QTextLayout layout(hello, testFont);
    layout.beginLayout();

    QTextLine line = layout.createLine();
    line.setLineWidth(width);
    QCOMPARE(line.textLength(), hello.size());
    QCOMPARE(layout.boundingRect(), QRectF(0, 0, width, QFontMetrics(testFont).height()));
}

void tst_QTextLayout::threeLineBoundingRect_data()
{
    QTest::addColumn<QChar>("wordBoundary1");
    QTest::addColumn<QChar>("wordBoundary2");
    QTest::newRow("2x' '") << QChar(' ') << QChar(' ');
    QTest::newRow("2x'\\n'") << QChar('\n') << QChar('\n');
    QTest::newRow("' ' + '\\n'") << QChar(' ') << QChar('\n');
    QTest::newRow("'\\n' + ' '") << QChar('\n') << QChar(' ');
    QTest::newRow("2x'\\t'") << QChar('\t') << QChar('\t');
    QTest::newRow("2xsoft hyphen") << QChar(0xad) << QChar(0xad);
    QTest::newRow("2x'-'") << QChar('-') << QChar('-');
    QTest::newRow("2x'/'") << QChar('/') << QChar('/');
    QTest::newRow("soft hyphen + ' '") << QChar(0xad) << QChar(' ');
    QTest::newRow("soft hyphen + '\\n'") << QChar(0xad) << QChar('\n');
    QTest::newRow("soft hyphen + '-'") << QChar(0xad) << QChar('-');
    QTest::newRow("' ' + soft hyphen") << QChar(' ') << QChar(0xad);
    QTest::newRow("'\\n' + soft hyphen") << QChar('\n') << QChar(0xad);
    QTest::newRow("'-' + soft hyphen") << QChar('-') << QChar(0xad);
}

void tst_QTextLayout::threeLineBoundingRect()
{
    /* stricter check. break text into three lines */
    QFETCH(QChar, wordBoundary1);
    QFETCH(QChar, wordBoundary2);

    QString firstWord("hello");
    QString secondWord("test");
    QString thirdWord("world");
    QString text(firstWord + wordBoundary1 + secondWord + wordBoundary2 + thirdWord);

    int firstLineWidth = firstWord.size() * testFont.pixelSize();
    int secondLineWidth = secondWord.size() * testFont.pixelSize();
    int thirdLineWidth = thirdWord.size() * testFont.pixelSize();
    // Trailing spaces do not count to line width:
    if (!wordBoundary1.isSpace())
        firstLineWidth += testFont.pixelSize();
    if (!wordBoundary2.isSpace())
        secondLineWidth += testFont.pixelSize();
    // But trailing spaces do count to line length:
    const int firstLineLength = firstWord.size() + 1;
    const int secondLineLength = secondWord.size() + 1;
    const int thirdLineLength = thirdWord.size();

    const int longestLine = qMax(firstLineWidth, qMax(secondLineWidth, thirdLineWidth));

    QTextLayout layout(text, testFont);
    layout.beginLayout();

    int pos = 0;
    int y = 0;
    QTextLine line = layout.createLine();
    line.setLineWidth(firstLineWidth);
    line.setPosition(QPoint(0, y));
    QCOMPARE(line.textStart(), pos);
    QCOMPARE(line.textLength(), firstLineLength);
    QCOMPARE(qRound(line.naturalTextWidth()), firstLineWidth);

    pos += line.textLength();
    y += qRound(line.ascent() + line.descent());

    line = layout.createLine();
    line.setLineWidth(secondLineWidth);
    line.setPosition(QPoint(0, y));
    QCOMPARE(line.textStart(), pos);
    QCOMPARE(line.textLength(), secondLineLength);
    QCOMPARE(qRound(line.naturalTextWidth()), secondLineWidth);

    pos += line.textLength();
    y += qRound(line.ascent() + line.descent());

    line = layout.createLine();
    line.setLineWidth(secondLineWidth);
    line.setPosition(QPoint(0, y));
    QCOMPARE(line.textStart(), pos);
    QCOMPARE(line.textLength(), thirdLineLength);
    QCOMPARE(qRound(line.naturalTextWidth()), thirdLineWidth);
    y += qRound(line.ascent() + line.descent());

    QCOMPARE(layout.boundingRect(), QRectF(0, 0, longestLine, y));
}

void tst_QTextLayout::boundingRectWithLongLineAndNoWrap()
{
    QString longString("thisisaverylongstringthatcannotbewrappedatallitjustgoesonandonlikeonebigword");

    const int width = longString.size() * testFont.pixelSize() / 20; // very small widthx

    QTextLayout layout(longString, testFont);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(width);

    QVERIFY(layout.boundingRect().width() >= line.width());
    QCOMPARE(layout.boundingRect().width(), line.naturalTextWidth());
}

void tst_QTextLayout::forcedBreaks()
{
    QString text = "A\n\nB\nC";
    text.replace('\n', QChar::LineSeparator);

    QTextLayout layout(text, testFont);

    layout.beginLayout();

    int pos = 0;

    QTextLine line = layout.createLine();
    line.setLineWidth(0x10000);
    QCOMPARE(line.textStart(), pos);
    QCOMPARE(line.textLength(),2);
    QCOMPARE(qRound(line.naturalTextWidth()),testFont.pixelSize());
    QCOMPARE((int) line.height(), testFont.pixelSize());
    QCOMPARE(line.xToCursor(0), line.textStart());
    pos += line.textLength();

    line = layout.createLine();
    line.setLineWidth(0x10000);
    QCOMPARE(line.textStart(),pos);
    QCOMPARE(line.textLength(),1);
    QCOMPARE(qRound(line.naturalTextWidth()), 0);
    QCOMPARE((int) line.height(), testFont.pixelSize());
    QCOMPARE(line.xToCursor(0), line.textStart());
    pos += line.textLength();

    line = layout.createLine();
    line.setLineWidth(0x10000);
    QCOMPARE(line.textStart(),pos);
    QCOMPARE(line.textLength(),2);
    QCOMPARE(qRound(line.naturalTextWidth()),testFont.pixelSize());
    QCOMPARE(qRound(line.height()), testFont.pixelSize());
    QCOMPARE(line.xToCursor(0), line.textStart());
    pos += line.textLength();

    line = layout.createLine();
    line.setLineWidth(0x10000);
    QCOMPARE(line.textStart(),pos);
    QCOMPARE(line.textLength(),1);
    QCOMPARE(qRound(line.naturalTextWidth()), testFont.pixelSize());
    QCOMPARE((int) line.height(), testFont.pixelSize());
    QCOMPARE(line.xToCursor(0), line.textStart());

    layout.endLayout();
}

void tst_QTextLayout::breakAny()
{
    QString text = "ABCD";

    QTextLayout layout(text, testFont);
    layout.setCacheEnabled(true);
    QTextLine line;

    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAnywhere);
    layout.setTextOption(opt);
    layout.beginLayout();

    line = layout.createLine();
    line.setLineWidth(testFont.pixelSize() * 2);
    QCOMPARE(line.textStart(), 0);
    QCOMPARE(line.textLength(), 2);

    line = layout.createLine();
    line.setLineWidth(testFont.pixelSize() * 2);
    QCOMPARE(line.textStart(), 2);
    QCOMPARE(line.textLength(), 2);

    line = layout.createLine();
    QVERIFY(!line.isValid());

    layout.endLayout();

    text = "ABCD EFGH";
    layout.setText(text);
    layout.beginLayout();

    line = layout.createLine();
    line.setLineWidth(testFont.pixelSize() * 7);
    QCOMPARE(line.textStart(), 0);
    QCOMPARE(line.textLength(), 7);

    layout.endLayout();
}

void tst_QTextLayout::noWrap()
{
    QString text = "AB CD";

    QTextLayout layout(text, testFont);
    QTextLine line;

    QTextOption opt;
    opt.setWrapMode(QTextOption::NoWrap);
    layout.setTextOption(opt);
    layout.beginLayout();

    line = layout.createLine();
    line.setLineWidth(testFont.pixelSize() * 2);
    QCOMPARE(line.textStart(), 0);
    QCOMPARE(line.textLength(), 5);

    line = layout.createLine();
    QVERIFY(!line.isValid());

    layout.endLayout();
}

void tst_QTextLayout::cursorToXForInlineObjects()
{
    QString text = QStringLiteral("<html><body><img src=\"\" width=\"32\" height=\"32\" /></body></html>");

    QTextDocument document;
    document.setHtml(text);
    QCOMPARE(document.blockCount(), 1);

    // Trigger layout
    {
        QImage img(1, 1, QImage::Format_ARGB32_Premultiplied);
        QPainter p(&img);
        document.drawContents(&p);
    }

    QTextLayout *layout = document.firstBlock().layout();
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->lineCount(), 1);

    QTextLine line = layout->lineAt(0);
    QCOMPARE(line.cursorToX(0), qreal(0));
    QCOMPARE(line.cursorToX(1), qreal(32));
}

void tst_QTextLayout::cursorToXForSetColumns()
{
    QTextLayout lay("abc", testFont);
    lay.setCacheEnabled(true);
    QTextOption o = lay.textOption();
    o.setWrapMode(QTextOption::WrapAnywhere);

    // enable/disable this line for full effect ;)
    o.setAlignment(Qt::AlignHCenter);

    lay.setTextOption(o);
    lay.beginLayout();
    QTextLine line = lay.createLine();
    line.setNumColumns(1);
    lay.endLayout();
    QCOMPARE(line.cursorToX(0), 0.);
    QCOMPARE(line.cursorToX(1), (qreal) TESTFONT_SIZE);
}

void tst_QTextLayout::cursorToXForTrailingSpaces_data()
{
    qreal width = TESTFONT_SIZE * 4;

    QTest::addColumn<QTextOption::WrapMode>("wrapMode");
    QTest::addColumn<Qt::LayoutDirection>("textDirection");
    QTest::addColumn<Qt::AlignmentFlag>("alignment");
    QTest::addColumn<qreal>("cursorAt0");
    QTest::addColumn<qreal>("cursorAt4");
    QTest::addColumn<qreal>("cursorAt6");

    // Aligned left from start of visible characters.
    QTest::newRow("ltr nowrap lalign")
            << QTextOption::NoWrap
            << Qt::LeftToRight
            << Qt::AlignLeft
            << qreal(0)
            << width
            << qreal(TESTFONT_SIZE * 6);

    // Aligned left from start of visible characters.
    QTest::newRow("ltr wrap lalign")
            << QTextOption::WrapAnywhere
            << Qt::LeftToRight
            << Qt::AlignLeft
            << qreal(0)
            << width
            << width;

    // Aligned right from end of whitespace characters.
    QTest::newRow("ltr nowrap ralign")
            << QTextOption::NoWrap
            << Qt::LeftToRight
            << Qt::AlignRight
            << qreal(TESTFONT_SIZE * -2)
            << qreal(TESTFONT_SIZE *  2)
            << width;

    // Aligned right from end of visible characters.
    QTest::newRow("ltr wrap ralign")
            << QTextOption::WrapAnywhere
            << Qt::LeftToRight
            << Qt::AlignRight
            << qreal(TESTFONT_SIZE)
            << width
            << width;

    // Aligned center of all characters
    QTest::newRow("ltr nowrap calign")
            << QTextOption::NoWrap
            << Qt::LeftToRight
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE * -1)
            << qreal(TESTFONT_SIZE *  3)
            << qreal(TESTFONT_SIZE *  5);

    // Aligned center of visible characters
    QTest::newRow("ltr wrap calign")
            << QTextOption::WrapAnywhere
            << Qt::LeftToRight
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE * 0.5)
            << qreal(width)
            << qreal(width);

    // Aligned right from start of visible characters
    QTest::newRow("rtl nowrap ralign")
            << QTextOption::NoWrap
            << Qt::RightToLeft
            << Qt::AlignRight
            << width
            << qreal(0)
            << qreal(TESTFONT_SIZE * -2);

    // Aligned right from start of visible characters
    QTest::newRow("rtl wrap ralign")
            << QTextOption::WrapAnywhere
            << Qt::RightToLeft
            << Qt::AlignRight
            << width
            << qreal(0)
            << qreal(0);

    // Aligned left from end of whitespace characters
    QTest::newRow("rtl nowrap lalign")
            << QTextOption::NoWrap
            << Qt::RightToLeft
            << Qt::AlignLeft
            << qreal(TESTFONT_SIZE * 6)
            << qreal(TESTFONT_SIZE * 2)
            << qreal(0);

    // Aligned left from end of visible characters
    QTest::newRow("rtl wrap lalign")
            << QTextOption::WrapAnywhere
            << Qt::RightToLeft
            << Qt::AlignLeft
            << qreal(TESTFONT_SIZE * 3)
            << qreal(0)
            << qreal(0);

    // Aligned center of all characters
    QTest::newRow("rtl nowrap calign")
            << QTextOption::NoWrap
            << Qt::RightToLeft
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE *  5)
            << qreal(TESTFONT_SIZE *  1)
            << qreal(TESTFONT_SIZE * -1);

    // Aligned center of visible characters
    QTest::newRow("rtl wrap calign")
            << QTextOption::WrapAnywhere
            << Qt::RightToLeft
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE * 3.5)
            << qreal(0)
            << qreal(0);
}

void tst_QTextLayout::cursorToXForTrailingSpaces()
{
    QFETCH(QTextOption::WrapMode, wrapMode);
    QFETCH(Qt::LayoutDirection, textDirection);
    QFETCH(Qt::AlignmentFlag, alignment);
    QFETCH(qreal, cursorAt0);
    QFETCH(qreal, cursorAt4);
    QFETCH(qreal, cursorAt6);

    QTextLayout layout("%^&   ", testFont);

    QTextOption o = layout.textOption();
    o.setTextDirection(textDirection);
    o.setAlignment(alignment);
    o.setWrapMode(wrapMode);
    layout.setTextOption(o);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(TESTFONT_SIZE * 4);
    layout.endLayout();

    QCOMPARE(line.cursorToX(0), cursorAt0);
    QCOMPARE(line.cursorToX(4), cursorAt4);
    QCOMPARE(line.cursorToX(6), cursorAt6);
}

void tst_QTextLayout::cursorToXInvalidInput()
{
    QTextLayout layout("aaa", testFont);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(5);
    layout.endLayout();

    int cursorPos;

    cursorPos = 0;
    layout.lineAt(0).cursorToX(&cursorPos);
    QCOMPARE(cursorPos, 0);
    cursorPos = -300;
    layout.lineAt(0).cursorToX(&cursorPos);
    QCOMPARE(cursorPos, 0);
    cursorPos = 300;
    layout.lineAt(0).cursorToX(&cursorPos);
    QCOMPARE(cursorPos, 3);
}

void tst_QTextLayout::cursorToXForBidiBoundaries_data()
{
    QTest::addColumn<Qt::LayoutDirection>("textDirection");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<int>("runsToInclude");

    QTest::addRow("LTR, abcشزذabc, 0") << Qt::LeftToRight << "abcشزذabc"
        << 0 << 0;
    QTest::addRow("RTL, abcشزذabc, 9") << Qt::RightToLeft << "abcشزذabc"
        << 9 << 1;
    QTest::addRow("LTR, abcشزذabc, 3") << Qt::LeftToRight << "abcشزذabc"
        << 0 << 0;
    QTest::addRow("RTL, abcشزذabc, 6") << Qt::RightToLeft << "abcشزذabc"
        << 9 << 1;

    QTest::addRow("LTR, شزذabcشزذ, 0") << Qt::LeftToRight << "شزذabcشزذ"
        << 0 << 1;
    QTest::addRow("RTL, شزذabcشزذ, 9") << Qt::RightToLeft << "شزذabcشزذ"
        << 9 << 0;
    QTest::addRow("LTR, شزذabcشزذ, 3") << Qt::LeftToRight << "شزذabcشزذ"
        << 3 << 1;
    QTest::addRow("RTL, شزذabcشزذ, 3") << Qt::RightToLeft << "شزذabcشزذ"
        << 3 << 2;
    QTest::addRow("LTR, شزذabcشزذ, 6") << Qt::LeftToRight << "شزذabcشزذ"
        << 6 << 2;
    QTest::addRow("RTL, شزذabcشزذ, 6") << Qt::RightToLeft << "شزذabcشزذ"
        << 6 << 1;
}

void tst_QTextLayout::cursorToXForBidiBoundaries()
{
    QFETCH(Qt::LayoutDirection, textDirection);
    QFETCH(QString, text);
    QFETCH(int, cursorPosition);
    QFETCH(int, runsToInclude);

    QTextOption option;
    option.setTextDirection(textDirection);

    QTextLayout layout(text, testFont);
    layout.setTextOption(option);
    layout.beginLayout();

    {
        QTextLine line = layout.createLine();
        line.setLineWidth(0x10000);
    }
    layout.endLayout();

    QTextLine line = layout.lineAt(0);
    QList<QGlyphRun> glyphRuns = line.glyphRuns(-1,
                                                -1,
                                                QTextLayout::RetrieveStringIndexes
                                                | QTextLayout::RetrieveGlyphIndexes);
    QVERIFY(runsToInclude <= glyphRuns.size());

    std::sort(glyphRuns.begin(), glyphRuns.end(),
              [](const QGlyphRun &first, const QGlyphRun &second) {
                    return first.stringIndexes().first() < second.stringIndexes().first();
                 });

    qreal expectedX = 0.0;
    for (int i = 0; i < runsToInclude; ++i) {
        expectedX += glyphRuns.at(i).boundingRect().width();
    }

    QCOMPARE(line.cursorToX(cursorPosition), expectedX);
}

void tst_QTextLayout::horizontalAlignment_data()
{
    qreal width = TESTFONT_SIZE * 4;

    QTest::addColumn<QTextOption::WrapMode>("wrapMode");
    QTest::addColumn<Qt::LayoutDirection>("textDirection");
    QTest::addColumn<Qt::AlignmentFlag>("alignment");
    QTest::addColumn<qreal>("naturalLeft");
    QTest::addColumn<qreal>("naturalRight");

    // Aligned left from start of visible characters.
    QTest::newRow("ltr nowrap lalign")
            << QTextOption::NoWrap
            << Qt::LeftToRight
            << Qt::AlignLeft
            << qreal(0)
            << qreal(TESTFONT_SIZE * 6);

    // Aligned left from start of visible characters.
    QTest::newRow("ltr wrap lalign")
            << QTextOption::WrapAnywhere
            << Qt::LeftToRight
            << Qt::AlignLeft
            << qreal(0)
            << qreal(TESTFONT_SIZE * 3);

    // Aligned right from end of whitespace characters.
    QTest::newRow("ltr nowrap ralign")
            << QTextOption::NoWrap
            << Qt::LeftToRight
            << Qt::AlignRight
            << qreal(TESTFONT_SIZE *  - 2)
            << width;

    // Aligned right from end of visible characters.
    QTest::newRow("ltr wrap ralign")
            << QTextOption::WrapAnywhere
            << Qt::LeftToRight
            << Qt::AlignRight
            << qreal(TESTFONT_SIZE)
            << width;

    // Aligned center of all characters
    QTest::newRow("ltr nowrap calign")
            << QTextOption::NoWrap
            << Qt::LeftToRight
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE * -1)
            << qreal(TESTFONT_SIZE *  5);

    // Aligned center of visible characters
    QTest::newRow("ltr wrap calign")
            << QTextOption::WrapAnywhere
            << Qt::LeftToRight
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE * 0.5)
            << qreal(TESTFONT_SIZE * 3.5);

    // Aligned right from start of visible characters
    QTest::newRow("rtl nowrap ralign")
            << QTextOption::NoWrap
            << Qt::RightToLeft
            << Qt::AlignRight
            << qreal(TESTFONT_SIZE * -2)
            << width;

    // Aligned right from start of visible characters
    QTest::newRow("rtl wrap ralign")
            << QTextOption::WrapAnywhere
            << Qt::RightToLeft
            << Qt::AlignRight
            << qreal(TESTFONT_SIZE * 1)
            << width;

    // Aligned left from end of whitespace characters
    QTest::newRow("rtl nowrap lalign")
            << QTextOption::NoWrap
            << Qt::RightToLeft
            << Qt::AlignLeft
            << qreal(0)
            << qreal(TESTFONT_SIZE * 6);

    // Aligned left from end of visible characters
    QTest::newRow("rtl wrap lalign")
            << QTextOption::WrapAnywhere
            << Qt::RightToLeft
            << Qt::AlignLeft
            << qreal(0)
            << qreal(TESTFONT_SIZE * 3);

    // Aligned center of all characters
    QTest::newRow("rtl nowrap calign")
            << QTextOption::NoWrap
            << Qt::RightToLeft
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE * -1)
            << qreal(TESTFONT_SIZE *  5);

    // Aligned center of visible characters
    QTest::newRow("rtl wrap calign")
            << QTextOption::WrapAnywhere
            << Qt::RightToLeft
            << Qt::AlignHCenter
            << qreal(TESTFONT_SIZE * 0.5)
            << qreal(TESTFONT_SIZE * 3.5);
}

void tst_QTextLayout::horizontalAlignment()
{
    QFETCH(QTextOption::WrapMode, wrapMode);
    QFETCH(Qt::LayoutDirection, textDirection);
    QFETCH(Qt::AlignmentFlag, alignment);
    QFETCH(qreal, naturalLeft);
    QFETCH(qreal, naturalRight);

    QTextLayout layout("%^&   ", testFont);

    QTextOption o = layout.textOption();
    o.setTextDirection(textDirection);
    o.setAlignment(alignment);
    o.setWrapMode(wrapMode);
    layout.setTextOption(o);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(TESTFONT_SIZE * 4);
    layout.endLayout();

    QRectF naturalRect = line.naturalTextRect();
    QCOMPARE(naturalRect.left(), naturalLeft);
    QCOMPARE(naturalRect.right(), naturalRight);
}


void tst_QTextLayout::horizontalAlignmentMultiline_data()
{
    qreal width = TESTFONT_SIZE * 8;

    const QString linebreakText = QStringLiteral("^%$&") + QChar(0x2028) + QStringLiteral("^%&*^$");
    QString wrappingText("^%$&^%&*^$");
    QString wrappingWhitespaceText("^%$&        ^%&*^$");

    QTest::addColumn<QString>("text");
    QTest::addColumn<Qt::LayoutDirection>("textDirection");
    QTest::addColumn<Qt::AlignmentFlag>("alignment");
    QTest::addColumn<qreal>("firstLeft");
    QTest::addColumn<qreal>("firstRight");
    QTest::addColumn<qreal>("lastLeft");
    QTest::addColumn<qreal>("lastRight");

    Qt::LayoutDirection textDirection[] = { Qt::LeftToRight, Qt::RightToLeft };
    QByteArray textDirectionText [] = { "ltr ", "rtl " };
    for (int i = 0; i < 2; ++i) {
        // Aligned left from start of visible characters.
        QTest::newRow(textDirectionText[i] + "linebreak lalign")
                << linebreakText
                << textDirection[i]
                << Qt::AlignLeft
                << qreal(0)
                << qreal(TESTFONT_SIZE * 4)
                << qreal(0)
                << qreal(TESTFONT_SIZE * 6);

        // Aligned left from start of visible characters.
        QTest::newRow(textDirectionText[i] + "wrap-text lalign")
                << wrappingText
                << textDirection[i]
                << Qt::AlignLeft
                << qreal(0)
                << width
                << qreal(0)
                << qreal(TESTFONT_SIZE * 2);

        // Aligned left from start of visible characters.
        QTest::newRow(textDirectionText[i] + "wrap-ws lalign")
                << wrappingWhitespaceText
                << textDirection[i]
                << Qt::AlignLeft
                << qreal(0)
                << qreal(TESTFONT_SIZE * 4)
                << qreal(0)
                << qreal(TESTFONT_SIZE * 6);

        // Aligned right from start of visible characters.
        QTest::newRow(textDirectionText[i] + "linebreak ralign")
                << linebreakText
                << textDirection[i]
                << Qt::AlignRight
                << qreal(TESTFONT_SIZE * 4)
                << width
                << qreal(TESTFONT_SIZE * 2)
                << width;

        // Aligned right from start of visible characters.
        QTest::newRow(textDirectionText[i] + "wrap-text ralign")
                << wrappingText
                << textDirection[i]
                << Qt::AlignRight
                << qreal(0)
                << width
                << qreal(TESTFONT_SIZE * 6)
                << width;

        // Aligned left from start of visible characters.
        QTest::newRow(textDirectionText[i] + "wrap-ws ralign")
                << wrappingWhitespaceText
                << textDirection[i]
                << Qt::AlignRight
                << qreal(TESTFONT_SIZE * 4)
                << width
                << qreal(TESTFONT_SIZE * 2)
                << width;

        // Aligned center from start of visible characters.
        QTest::newRow(textDirectionText[i] + "linebreak calign")
                << linebreakText
                << textDirection[i]
                << Qt::AlignCenter
                << qreal(TESTFONT_SIZE * 2)
                << qreal(TESTFONT_SIZE * 6)
                << qreal(TESTFONT_SIZE * 1)
                << qreal(TESTFONT_SIZE * 7);

        // Aligned center from start of visible characters.
        QTest::newRow(textDirectionText[i] + "wrap-text calign")
                << wrappingText
                << textDirection[i]
                << Qt::AlignCenter
                << qreal(0)
                << width
                << qreal(TESTFONT_SIZE * 3)
                << qreal(TESTFONT_SIZE * 5);

        // Aligned center from start of visible characters.
        QTest::newRow(textDirectionText[i] + "wrap-ws calign")
                << wrappingWhitespaceText
                << textDirection[i]
                << Qt::AlignCenter
                << qreal(TESTFONT_SIZE * 2)
                << qreal(TESTFONT_SIZE * 6)
                << qreal(TESTFONT_SIZE * 1)
                << qreal(TESTFONT_SIZE * 7);
    }
}

void tst_QTextLayout::horizontalAlignmentMultiline()
{
    QFETCH(QString, text);
    QFETCH(Qt::LayoutDirection, textDirection);
    QFETCH(Qt::AlignmentFlag, alignment);
    QFETCH(qreal, firstLeft);
    QFETCH(qreal, firstRight);
    QFETCH(qreal, lastLeft);
    QFETCH(qreal, lastRight);

    QTextLayout layout(text, testFont);

    QTextOption o = layout.textOption();
    o.setTextDirection(textDirection);
    o.setAlignment(alignment);
    o.setWrapMode(QTextOption::WrapAnywhere);
    layout.setTextOption(o);

    layout.beginLayout();
    QTextLine firstLine = layout.createLine();
    QTextLine lastLine;
    for (QTextLine line = firstLine; line.isValid(); line = layout.createLine()) {
        line.setLineWidth(TESTFONT_SIZE * 8);
        lastLine = line;
    }
    layout.endLayout();

    qDebug() << firstLine.textLength() << firstLine.naturalTextRect() << lastLine.naturalTextRect();

    QRectF rect = firstLine.naturalTextRect();
    QCOMPARE(rect.left(), firstLeft);
    QCOMPARE(rect.right(), firstRight);

    rect = lastLine.naturalTextRect();
    QCOMPARE(rect.left(), lastLeft);
    QCOMPARE(rect.right(), lastRight);
}
#endif

void tst_QTextLayout::defaultWordSeparators_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("startPos");
    QTest::addColumn<int>("endPos");

    QString separators(".,:;-<>[](){}=/+%&^*");
    separators += QLatin1String("!?");
    for (int i = 0; i < separators.size(); ++i) {
        QTest::newRow(QString::number(i).toLatin1().data())
            << QString::fromLatin1("abcd") + separators.at(i) + QString::fromLatin1("efgh")
            <<  0 << 4;
    }

    QTest::newRow("nbsp")
            << QString::fromLatin1("abcd") + QString(QChar::Nbsp) + QString::fromLatin1("efgh")
            << 0 << 5;

    QTest::newRow("tab")
        << QString::fromLatin1("abcd") + QString::fromLatin1("\t") + QString::fromLatin1("efgh")
            << 0 << 5;

    QTest::newRow("lineseparator")
            << QString::fromLatin1("abcd") + QString(QChar::LineSeparator) + QString::fromLatin1("efgh")
            << 0 << 5;

    QTest::newRow("empty")
            << QString()
            << 0 << 0;
}

void tst_QTextLayout::defaultWordSeparators()
{
    QFETCH(QString, text);
    QFETCH(int, startPos);
    QFETCH(int, endPos);
    QTextLayout layout(text);

    QCOMPARE(layout.nextCursorPosition(startPos, QTextLayout::SkipWords), endPos);
    QCOMPARE(layout.previousCursorPosition(endPos, QTextLayout::SkipWords), startPos);
}

void tst_QTextLayout::cursorMovementFromInvalidPositions()
{
    int badpos = 10000;

    QTextLayout layout("ABC");

    QCOMPARE(layout.previousCursorPosition(-badpos, QTextLayout::SkipCharacters), -badpos);
    QCOMPARE(layout.nextCursorPosition(-badpos, QTextLayout::SkipCharacters), -badpos);

    QCOMPARE(layout.previousCursorPosition(badpos, QTextLayout::SkipCharacters), badpos);
    QCOMPARE(layout.nextCursorPosition(badpos, QTextLayout::SkipCharacters), badpos);
}

void tst_QTextLayout::cursorMovementInsideSpaces()
{
    QTextLayout layout("ABC            DEF");

    QCOMPARE(layout.previousCursorPosition(6, QTextLayout::SkipWords), 0);
    QCOMPARE(layout.nextCursorPosition(6, QTextLayout::SkipWords), 15);


    QTextLayout layout2("ABC\t\t\t\t\t\t\t\t\t\t\t\tDEF");

    QCOMPARE(layout2.previousCursorPosition(6, QTextLayout::SkipWords), 0);
    QCOMPARE(layout2.nextCursorPosition(6, QTextLayout::SkipWords), 15);
}

void tst_QTextLayout::charWordStopOnLineSeparator()
{
    const QChar lineSeparator(QChar::LineSeparator);
    QString txt;
    txt.append(lineSeparator);
    txt.append(lineSeparator);
    QTextLayout layout(txt);
    QTextEngine *engine = layout.engine();
    const QCharAttributes *attrs = engine->attributes();
    QVERIFY(attrs);
    QVERIFY(attrs[1].graphemeBoundary);
}

#ifdef QT_BUILD_INTERNAL
void tst_QTextLayout::xToCursorAtEndOfLine()
{
    QString text = "FirstLine SecondLine";
    text.replace('\n', QChar::LineSeparator);

    const qreal firstLineWidth = QString("FirstLine").size() * testFont.pixelSize();

    QTextLayout layout(text, testFont);
    layout.setCacheEnabled(true);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    QVERIFY(line.isValid());
    line.setLineWidth(firstLineWidth);
    QVERIFY(layout.createLine().isValid());
    QVERIFY(!layout.createLine().isValid());
    layout.endLayout();

    line = layout.lineAt(0);
    QCOMPARE(line.xToCursor(100000), 9);
    line = layout.lineAt(1);
    QCOMPARE(line.xToCursor(100000), 20);
}
#endif


void tst_QTextLayout::xToCursorForBidiEnds_data()
{
    QTest::addColumn<Qt::LayoutDirection>("textDirection");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("leftPosition");
    QTest::addColumn<int>("rightPosition");

    QTest::addRow("LTR, abcشزذ") << Qt::LeftToRight << "abcشزذ"
        << 0 << 6;
    QTest::addRow("RTL, abcشزذ") << Qt::RightToLeft << "abcشزذ"
        << 6 << 0;
    QTest::addRow("LTR, شزذabc") << Qt::LeftToRight << "شزذabc"
        << 0 << 6;
    QTest::addRow("RTL, شزذabc") << Qt::RightToLeft << "شزذabc"
        << 6 << 0;
    QTest::addRow("LTR, شزذ123") << Qt::LeftToRight << "شزذ123"
        << 0 << 6;
    QTest::addRow("RTL, شزذ123") << Qt::RightToLeft << "شزذ123"
        << 6 << 0;

    QTest::addRow("LTR, abcشزذabc") << Qt::LeftToRight << "abcشزذabc"
        << 0 << 9;
    QTest::addRow("RTL, abcشزذabc") << Qt::RightToLeft << "abcشزذabc"
        << 9 << 0;
    QTest::addRow("LTR, شزذabcشزذ") << Qt::LeftToRight << "شزذabcشزذ"
        << 0 << 9;
    QTest::addRow("RTL, شزذabcشزذ") << Qt::RightToLeft << "شزذabcشزذ"
        << 9 << 0;
}

void tst_QTextLayout::xToCursorForBidiEnds()
{
    QFETCH(Qt::LayoutDirection, textDirection);
    QFETCH(QString, text);
    QFETCH(int, leftPosition);
    QFETCH(int, rightPosition);

    QTextOption option;
    option.setTextDirection(textDirection);

    QTextLayout layout(text, testFont);
    layout.setTextOption(option);
    layout.beginLayout();

    QTextLine line = layout.createLine();
    line.setLineWidth(0x10000);

    QCOMPARE(line.xToCursor(0), leftPosition);
    QCOMPARE(line.xToCursor(line.width()), rightPosition);

    layout.endLayout();
}

void tst_QTextLayout::boundingRectTopLeft()
{
    QString text = "FirstLine\nSecondLine";
    text.replace('\n', QChar::LineSeparator);

    QTextLayout layout(text);
    layout.setCacheEnabled(true);

    layout.beginLayout();
    QTextLine firstLine = layout.createLine();
    QVERIFY(firstLine.isValid());
    firstLine.setPosition(QPointF(10, 10));
    QTextLine secondLine = layout.createLine();
    QVERIFY(secondLine.isValid());
    secondLine.setPosition(QPointF(20, 20));
    layout.endLayout();

    QCOMPARE(layout.boundingRect().topLeft(), firstLine.position());
}

void tst_QTextLayout::graphemeBoundaryForSurrogatePairs()
{
    QString txt;
    txt.append(QLatin1Char('a'));
    txt.append(QChar(0xd87e));
    txt.append(QChar(0xdc25));
    txt.append(QLatin1Char('b'));
    QTextLayout layout(txt);
    QTextEngine *engine = layout.engine();
    const QCharAttributes *attrs = engine->attributes();
    QVERIFY(attrs);
    QVERIFY(attrs[0].graphemeBoundary);
    QVERIFY(attrs[1].graphemeBoundary);
    QVERIFY(!attrs[2].graphemeBoundary);
    QVERIFY(attrs[3].graphemeBoundary);
}

void tst_QTextLayout::tabStops()
{
    QString txt("Hello there\tworld");
    QTextLayout layout(txt);
    layout.beginLayout();
    QTextLine line = layout.createLine();

    QVERIFY(line.isValid());
    line.setNumColumns(11);
    QCOMPARE(line.textLength(), 12);

    line = layout.createLine();
    QVERIFY(line.isValid());
    line.setNumColumns(5);
    QCOMPARE(line.textLength(), 5);

    layout.endLayout();
}

void tst_QTextLayout::integerOverflow()
{
    QString txt("Hello world... ");

    for (int i = 0; i < 8; ++i)
        txt += txt;

    QTextLayout layout(txt);
    layout.beginLayout();
    QTextLine line = layout.createLine();

    QVERIFY(line.isValid());
    line.setLineWidth(INT_MAX);
    QCOMPARE(line.textLength(), txt.size());

    QVERIFY(!layout.createLine().isValid());

    layout.endLayout();
}

#ifdef QT_BUILD_INTERNAL
void tst_QTextLayout::setNumColumnsWrapAtWordBoundaryOrAnywhere()
{
    QString txt("This is a small test text");
    QTextLayout layout(txt, testFont);
    layout.setCacheEnabled(true);
    QTextOption option = layout.textOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    QVERIFY(line1.isValid());
    line1.setNumColumns(1);

    // qDebug() << line1.naturalTextWidth();
    QCOMPARE(line1.textLength(), 1);
    QVERIFY(line1.naturalTextWidth() == testFont.pixelSize()); // contains only one character

    QTextLine line2 = layout.createLine();
    QVERIFY(line2.isValid());

    layout.endLayout();
}

void tst_QTextLayout::setNumColumnsWordWrap()
{
    QString txt("This is a small test text");
    QTextLayout layout(txt, testFont);
    layout.setCacheEnabled(true);
    QTextOption option = layout.textOption();
    option.setWrapMode(QTextOption::WordWrap);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    QVERIFY(line1.isValid());
    line1.setNumColumns(1);

    // qDebug() << line1.naturalTextWidth();
    QCOMPARE(line1.textLength(), 5);
    QVERIFY(line1.naturalTextWidth() > 20.0); // contains the whole first word.

    QTextLine line2 = layout.createLine();
    QVERIFY(line2.isValid());

    layout.endLayout();
}

void tst_QTextLayout::smallTextLengthNoWrap()
{
    QString txt("This is a small test text");
    QTextLayout layout(txt, testFont);
    layout.setCacheEnabled(true);
    QTextOption option = layout.textOption();
    option.setWrapMode(QTextOption::NoWrap);
    layout.setTextOption(option);

    /// NoWrap
    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    QVERIFY(line1.isValid());
    line1.setLineWidth(5); // most certainly too short for the word 'This' to fit.

    QCOMPARE(line1.width(), 5.0);
    QVERIFY(line1.naturalTextWidth() > 70); // contains all the text.

    QTextLine line2 = layout.createLine();
    QVERIFY(! line2.isValid());

    layout.endLayout();
}

void tst_QTextLayout::smallTextLengthWordWrap()
{
    QString txt("This is a small test text");
    QTextLayout layout(txt, testFont);
    layout.setCacheEnabled(true);
    QTextOption option = layout.textOption();
    option.setWrapMode(QTextOption::WordWrap);
    layout.setTextOption(option);

    /// WordWrap
    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    QVERIFY(line1.isValid());
    line1.setLineWidth(5); // most certainly too short for the word 'This' to fit.

    QCOMPARE(line1.width(), 5.0);
    QVERIFY(line1.naturalTextWidth() > 20.0); // contains the whole first word.
    QCOMPARE(line1.textLength(), 5);

    QTextLine line2 = layout.createLine();
    QVERIFY(line2.isValid());

    layout.endLayout();
}

void tst_QTextLayout::smallTextLengthWrapAtWordBoundaryOrAnywhere()
{
    QString txt("This is a small test text");
    QTextLayout layout(txt, testFont);
    layout.setCacheEnabled(true);
    QTextOption option = layout.textOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    QVERIFY(line1.isValid());
    line1.setLineWidth(5); // most certainly too short for the word 'This' to fit.

    QCOMPARE(line1.width(), 5.0);
    // qDebug() << line1.naturalTextWidth();
    QCOMPARE(line1.textLength(), 1);
    QVERIFY(line1.naturalTextWidth() == testFont.pixelSize()); // contains just the characters that fit.

    QTextLine line2 = layout.createLine();
    QVERIFY(line2.isValid());

    layout.endLayout();
}

void tst_QTextLayout::testDefaultTabs()
{
    QTextLayout layout("Foo\tBar\ta slightly longer text\tend.", testFont);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(1000);
    layout.endLayout();

    //qDebug() << "After the tab: " << line.cursorToX(4);
    QCOMPARE(line.cursorToX(4), 80.); // default tab is 80
    QCOMPARE(line.cursorToX(8), 160.);
    QCOMPARE(line.cursorToX(31), 480.);

    QTextOption option = layout.textOption();
    option.setTabStopDistance(90);
    layout.setTextOption(option);
    layout.beginLayout();
    line = layout.createLine();
    line.setLineWidth(1000);
    layout.endLayout();

    QCOMPARE(line.cursorToX(4), 90.);
    QCOMPARE(line.cursorToX(8), 180.);
    QCOMPARE(line.cursorToX(31), 450.);

    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab;
    tab.position = 110; // set one tab to 110, but since the rest is unset they will be at the normal interval again.
    tabs.append(tab);
    option.setTabs(tabs);
    layout.setTextOption(option);
    layout.beginLayout();
    line = layout.createLine();
    line.setLineWidth(1000);
    layout.endLayout();

    QCOMPARE(line.cursorToX(4), 110.);
    QCOMPARE(line.cursorToX(8), 180.);
    QCOMPARE(line.cursorToX(31), 450.);
}

void tst_QTextLayout::testTabs()
{
    QTextLayout layout("Foo\tBar.", testFont);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    layout.setCacheEnabled(true);
    QTextOption option = layout.textOption();
    option.setTabStopDistance(150);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(200.);
    layout.endLayout();

    QVERIFY(line.naturalTextWidth() > 150);
    QCOMPARE(line.cursorToX(4), 150.);
}

void tst_QTextLayout::testMultilineTab()
{
    QTextLayout layout("Lorem ipsum dolor sit\tBar.", testFont);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    layout.setCacheEnabled(true);
    // test if this works on the second line.
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(220.); // moves 'sit' to next line.
    line = layout.createLine();
    line.setLineWidth(220.);
    layout.endLayout();


    QCOMPARE(line.cursorToX(22), 80.);
}

void tst_QTextLayout::testMultiTab()
{
    QTextLayout layout("Foo\t\t\tBar.", testFont);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(1000.);
    layout.endLayout();

    QCOMPARE(line.cursorToX(6), 80. * 3);
}

void tst_QTextLayout::testTabsInAlignedParag()
{
    QTextLayout layout("Foo\tsome more words", testFont);
    layout.setCacheEnabled(true);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    QTextOption option = layout.textOption();
    // right
    option.setAlignment(Qt::AlignRight);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(300.);
    layout.endLayout();

    const qreal textWidth = 80 + 15 * TESTFONT_SIZE; // 15 chars right of the tab
    QCOMPARE(line.naturalTextWidth(), textWidth);
    QCOMPARE(line.cursorToX(0), 300. - textWidth);

    // centered
    option.setAlignment(Qt::AlignCenter);
    layout.setTextOption(option);

    layout.beginLayout();
    line = layout.createLine();
    line.setLineWidth(300.);
    layout.endLayout();

    QCOMPARE(line.naturalTextWidth(), textWidth);
    QCOMPARE(line.cursorToX(0), (300. - textWidth) / 2.);

    // justified
    option.setAlignment(Qt::AlignJustify);
    layout.setTextOption(option);

    layout.beginLayout();
    line = layout.createLine();
    line.setLineWidth(textWidth - 10); // make the last word slip to the next line so justification actually happens.
    layout.endLayout();

    QCOMPARE(line.cursorToX(0), 0.);
    QCOMPARE(line.cursorToX(4), 80.);

    //QTextLayout layout2("Foo\tUt wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis", testFont); // means it will be more then one line long.
}

void tst_QTextLayout::testRightTab()
{
    QTextLayout layout("Foo\tLorem ipsum te sit\tBar baz\tText\tEnd", testFont);
    /*                     ^ a                 ^ b      ^ c   ^ d
     a = 4, b = 22, c = 30, d = 35 (position)

     I expect the output to be:
        Foo Lorem ipsum te
        sit      Bar Baz
        Text         End

     a) tab replaced with a single space due to the text not fitting before the tab.
     b) tab takes space so the text until the 3th tab fits to the tab pos.
     c) tab is after last tab (both auto and defined) and thus moves text to start of next line.
     d) tab takes space so text until enter fits to tab pos.
    */
    layout.setCacheEnabled(true);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    QTextOption option = layout.textOption();
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab;
    tab.type = QTextOption::RightTab;
    tab.position = 190; // which means only 15(.8) chars of our test font fit left of it.
    tabs.append(tab);
    option.setTabs(tabs);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    line1.setLineWidth(220.);
    // qDebug() << "=====line 2";
    QTextLine line2 = layout.createLine();
    QVERIFY(line2.isValid());
    line2.setLineWidth(220.);
    // qDebug() << "=====line 3";
    QTextLine line3 = layout.createLine();
    QVERIFY(line3.isValid());
    line3.setLineWidth(220.);
    // qDebug() << "=====line 4";
    QTextLine line4 = layout.createLine();
    QVERIFY(! line4.isValid());
    layout.endLayout();
    // qDebug() << "--------";

    QCOMPARE(line1.cursorToX(4), 3. * TESTFONT_SIZE ); // a
    QCOMPARE(line1.textLength(), 19);
    QCOMPARE(line2.cursorToX(23), 190. - 7. * TESTFONT_SIZE); // b
    QCOMPARE(line2.textLength(), 12);
    QCOMPARE(line3.cursorToX(31), 0.); // c
    QCOMPARE(line3.cursorToX(36), 190 - 3. * TESTFONT_SIZE); // d
}

void tst_QTextLayout::testCenteredTab()
{
    QTextLayout layout("Foo\tBar", testFont);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    layout.setCacheEnabled(true);
    // test if centering the tab works.  We expect the center of 'Bar.' to be at the tab point.
    QTextOption option = layout.textOption();
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab(150, QTextOption::CenterTab);
    tabs.append(tab);
    option.setTabs(tabs);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(200.);
    layout.endLayout();

    const qreal wordLength = 3 * TESTFONT_SIZE; // the length of 'Bar'
    QCOMPARE(line.cursorToX(4), 150 - wordLength / 2.);
}

void tst_QTextLayout::testDelimiterTab()
{
    QTextLayout layout("Foo\tBar. Barrabas", testFont);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    layout.setCacheEnabled(true);
    // try the different delimiter characters to see if the alignment works there.
    QTextOption option = layout.textOption();
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab(100, QTextOption::DelimiterTab, QChar('.'));
    tabs.append(tab);
    option.setTabs(tabs);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(200.);
    layout.endLayout();

    const qreal distanceBeforeTab = 3.5 * TESTFONT_SIZE; // the length of 'bar' and half the width of the dot.
    QCOMPARE(line.cursorToX(4), 100 - distanceBeforeTab);
}

void tst_QTextLayout::testLineBreakingAllSpaces()
{
    QTextLayout layout("                    123", testFont); // thats 20 spaces
    layout.setCacheEnabled(true);
    const qreal firstLineWidth = 17 * TESTFONT_SIZE;
    layout.beginLayout();
    QTextLine line1 = layout.createLine();
    line1.setLineWidth(firstLineWidth);
    QTextLine line2 = layout.createLine();
    line2.setLineWidth(1000); // the rest
    layout.endLayout();
    QCOMPARE(line1.width(), firstLineWidth);
    QCOMPARE(line1.naturalTextWidth(), 0.); // spaces don't take space
    QCOMPARE(line1.textLength(), 20);
    QCOMPARE(line2.textLength(), 3);
    QCOMPARE(line2.naturalTextWidth(), 3. * TESTFONT_SIZE);
}

void tst_QTextLayout::tabsForRtl()
{
    QString word(QChar(0x5e9)); // a hebrew character
    word = word + word + word;  // 3 hebrew characters ;)

    QTextLayout layout(word +'\t'+ word +'\t'+ word +'\t'+ word, testFont);
//QTextLayout layout(word +' '+ word +' '+ word +' '+ word, testFont);// tester ;)
    /*                             ^ a         ^ b         ^ c
     a = 4, b = 8, c = 12, d = 16 (position)

     a) Left tab in RTL is a righ tab; so a is at width - 80
     b) Like a
     c) right tab on RTL is a left tab; so its at width - 240
     d) center tab is still a centered tab.
    */
    layout.setCacheEnabled(true);

    QFont font = layout.font();
    QFontPrivate *fd = QFontPrivate::get(font);
    qreal dpiScale = qreal(fd->dpi) / qreal(qt_defaultDpiY());
    if (!qFuzzyCompare(dpiScale, 1.0))
        QSKIP("Test logic does not work when tabs are scaled by dpi");

    QTextOption option = layout.textOption();
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab;
    tab.position = 80;
    tabs.append(tab);
    tab.position = 160;
    tabs.append(tab);
    tab.position = 240;
    tab.type = QTextOption::RightTab;
    tabs.append(tab);
    option.setTabs(tabs);
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    const qreal WIDTH = 400.;
    line.setLineWidth(WIDTH);
    layout.endLayout();

//qDebug() << "layout ended --------------";

    QCOMPARE(line.cursorToX(0), WIDTH);
    QCOMPARE(line.cursorToX(1), WIDTH - TESTFONT_SIZE); // check its right-aligned
    QCOMPARE(line.cursorToX(4), WIDTH - 80 + 3 * TESTFONT_SIZE);
    QCOMPARE(line.cursorToX(8), WIDTH - 160 + 3 * TESTFONT_SIZE);
    QCOMPARE(line.cursorToX(12), WIDTH - 240);
}

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT int qt_defaultDpiY();
QT_END_NAMESPACE

void tst_QTextLayout::testTabDPIScale()
{
    class MyPaintDevice : public QPaintDevice {
        QPaintEngine *paintEngine () const override { return 0; }
        int metric (QPaintDevice::PaintDeviceMetric metric) const override {
            switch(metric) {
            case QPaintDevice::PdmWidth:
            case QPaintDevice::PdmHeight:
            case QPaintDevice::PdmWidthMM:
            case QPaintDevice::PdmHeightMM:
            case QPaintDevice::PdmNumColors:
                return INT_MAX;
            case QPaintDevice::PdmDepth:
                return 32;
            case QPaintDevice::PdmDpiX:
            case QPaintDevice::PdmDpiY:
            case QPaintDevice::PdmPhysicalDpiX:
            case QPaintDevice::PdmPhysicalDpiY:
                return 72;
            case QPaintDevice::PdmDevicePixelRatio:
            case QPaintDevice::PdmDevicePixelRatioScaled:
                ; // fall through
            }
            return 0;
        }
    };

    MyPaintDevice pd;

    QTextLayout layout("text1\ttext2\ttext3\tend", testFont, &pd);
    layout.setCacheEnabled(true);

    QTextOption option = layout.textOption();
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab;
    tab.position = 300;
    tabs.append(tab);

    tab.position = 600;
    tab.type = QTextOption::RightTab;
    tabs.append(tab);

    tab.position = 800;
    tab.type = QTextOption::CenterTab;
    tabs.append(tab);
    option.setTabs(tabs);
    layout.setTextOption(option);

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(1500.);
    layout.endLayout();
    QCOMPARE(line.cursorToX(0), 0.);
    QCOMPARE(line.cursorToX(1), (double) TESTFONT_SIZE); // check that the font does not resize
    qreal scale = 72 / (qreal) qt_defaultDpiY();
    // lets do the transformation of deminishing resolution that QFixed has as effect.
    int fixedScale = (int)( scale * qreal(64)); // into a QFixed
    scale = ((qreal)fixedScale)/(qreal)64;      // and out of a QFixed

    QCOMPARE(line.cursorToX(6),  tabs.at(0).position * scale);
    QCOMPARE(line.cursorToX(12), tabs.at(1).position * scale - TESTFONT_SIZE * 5);
    QCOMPARE(line.cursorToX(18), tabs.at(2).position * scale - TESTFONT_SIZE * 3 / 2.0);
}
#endif

void tst_QTextLayout::tabHeight()
{
    QTextLayout layout("\t");
    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    QCOMPARE(qRound(line.ascent()), QFontMetrics(layout.font()).ascent());
    QCOMPARE(qRound(line.descent()), QFontMetrics(layout.font()).descent());
}

void tst_QTextLayout::capitalization_allUpperCase()
{
    QFont font;
    font.setCapitalization(QFont::AllUppercase);
    QTextLayout layout("Test", font);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QTextEngine *engine = layout.engine();
    engine->itemize();
    QCOMPARE(engine->layoutData->items.size(), 1);
    QCOMPARE(engine->layoutData->items.at(0).analysis.flags, ushort(QScriptAnalysis::Uppercase));
}

void tst_QTextLayout::capitalization_allUpperCase_newline()
{
    QFont font;
    font.setCapitalization(QFont::AllUppercase);

    QString tmp = "hello\nworld!";
    tmp.replace(QLatin1Char('\n'), QChar::LineSeparator);

    QTextLayout layout(tmp, font);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QTextEngine *engine = layout.engine();
    engine->itemize();
    QCOMPARE(engine->layoutData->items.size(), 3);
    QCOMPARE(engine->layoutData->items.at(0).analysis.flags, ushort(QScriptAnalysis::Uppercase));
    QCOMPARE(engine->layoutData->items.at(1).analysis.flags, ushort(QScriptAnalysis::LineOrParagraphSeparator));
    QCOMPARE(engine->layoutData->items.at(2).analysis.flags, ushort(QScriptAnalysis::Uppercase));
}

void tst_QTextLayout::capitalization_allLowerCase()
{
    QFont font;
    font.setCapitalization(QFont::AllLowercase);
    QTextLayout layout("Test", font);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QTextEngine *engine = layout.engine();
    engine->itemize();
    QCOMPARE(engine->layoutData->items.size(), 1);
    QCOMPARE(engine->layoutData->items.at(0).analysis.flags, ushort(QScriptAnalysis::Lowercase));
}

void tst_QTextLayout::capitalization_smallCaps()
{
    QFont font;
    font.setCapitalization(QFont::SmallCaps);
    QTextLayout layout("Test", font);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QTextEngine *engine = layout.engine();
    engine->itemize();
    QCOMPARE(engine->layoutData->items.size(), 2);
    QCOMPARE(engine->layoutData->items.at(0).analysis.flags, ushort(QScriptAnalysis::None));
    QCOMPARE(engine->layoutData->items.at(1).analysis.flags, ushort(QScriptAnalysis::SmallCaps));
}

void tst_QTextLayout::capitalization_capitalize()
{
    QFont font;
    font.setCapitalization(QFont::Capitalize);
    QTextLayout layout("hello\tworld", font);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    QTextEngine *engine = layout.engine();
    engine->itemize();
    QCOMPARE(engine->layoutData->items.size(), 5);
    QCOMPARE(engine->layoutData->items.at(0).analysis.flags, ushort(QScriptAnalysis::Uppercase));
    QCOMPARE(engine->layoutData->items.at(1).analysis.flags, ushort(QScriptAnalysis::None));
    QCOMPARE(engine->layoutData->items.at(2).analysis.flags, ushort(QScriptAnalysis::Tab));
    QCOMPARE(engine->layoutData->items.at(3).analysis.flags, ushort(QScriptAnalysis::Uppercase));
    QCOMPARE(engine->layoutData->items.at(4).analysis.flags, ushort(QScriptAnalysis::None));
}

void tst_QTextLayout::longText()
{
    QString longText(128000, 'a');

    {
        QTextLayout layout(longText);
        layout.setCacheEnabled(true);
        layout.beginLayout();
        QTextLine line = layout.createLine();
        layout.endLayout();
        QVERIFY(line.isValid());
        QVERIFY(line.cursorToX(line.textLength() - 1) > 0);
    }

    for (int cap = QFont::MixedCase; cap < QFont::Capitalize + 1; ++cap) {
        QFont f;
        f.setCapitalization(QFont::Capitalization(cap));
        QTextLayout layout(longText, f);
        layout.setCacheEnabled(true);
        layout.beginLayout();
        QTextLine line = layout.createLine();
        layout.endLayout();
        QVERIFY(line.isValid());
        QVERIFY(line.cursorToX(line.textLength() - 1) > 0);
    }

    {
        QTextLayout layout(longText);
        layout.setCacheEnabled(true);
        layout.setFlags(Qt::TextForceLeftToRight);
        layout.beginLayout();
        QTextLine line = layout.createLine();
        layout.endLayout();
        QVERIFY(line.isValid());
        QVERIFY(line.cursorToX(line.textLength() - 1) > 0);
    }

    {
        QTextLayout layout(longText);
        layout.setCacheEnabled(true);
        layout.setFlags(Qt::TextForceRightToLeft);
        layout.beginLayout();
        QTextLine line = layout.createLine();
        layout.endLayout();
        QVERIFY(line.isValid());
        QVERIFY(line.cursorToX(line.textLength() - 1) > 0);
    }

    {
        QTextLayout layout(QString("Qt rocks! ").repeated(200000));
        layout.setCacheEnabled(true);
        layout.beginLayout();
        forever {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
        }
        layout.endLayout();
        QFontMetricsF fm(layout.font());
        QVERIFY(layout.maximumWidth() - fm.horizontalAdvance(' ') <= QFIXED_MAX);
    }

    {
        QTextLayout layout(QString("AAAAAAAA").repeated(200000));
        layout.setCacheEnabled(true);
        layout.beginLayout();
        forever {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;
        }
        layout.endLayout();
        QFontMetricsF fm(layout.font());
        QVERIFY(layout.maximumWidth() - fm.horizontalAdvance('A') <= QFIXED_MAX);
    }
}

void tst_QTextLayout::widthOfTabs()
{
    QTextEngine engine;
    engine.text = "ddd\t\t";
    engine.ignoreBidi = true;
    engine.itemize();
    QCOMPARE(qRound(engine.width(0, 5)), qRound(engine.boundingBox(0, 5).width));
}

void tst_QTextLayout::columnWrapWithTabs()
{
    QTextLayout textLayout;
    {
        QTextOption textOption;
        textOption.setWrapMode(QTextOption::WordWrap);
        textLayout.setTextOption(textOption);
    }

    // Make sure string with spaces does not break
    {
        QString text = "Foo bar foo bar foo bar";
        textLayout.setText(text);

        textLayout.beginLayout();
        QTextLine line = textLayout.createLine();
        line.setNumColumns(30);
        QCOMPARE(line.textLength(), text.size());
        textLayout.endLayout();
    }

    // Make sure string with tabs breaks
    {
        QString text = "Foo\tbar\tfoo\tbar\tfoo\tbar";
        textLayout.setText(text);
        textLayout.beginLayout();
        QTextLine line = textLayout.createLine();
        line.setNumColumns(30);
        QVERIFY(line.textLength() < text.size());
        textLayout.endLayout();
    }

}

void tst_QTextLayout::boundingRectForUnsetLineWidth()
{
    QTextLayout layout("FOOBAR");
    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    QCOMPARE(layout.boundingRect().width(), line.naturalTextWidth());
}

void tst_QTextLayout::boundingRectForSetLineWidth()
{
    QTextLayout layout("FOOBAR");
    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(QFIXED_MAX - 1);
    layout.endLayout();

    QCOMPARE(layout.boundingRect().width(), qreal(QFIXED_MAX - 1));
}

void tst_QTextLayout::lineWidthFromBOM()
{
    const QString string(QChar(0xfeff)); // BYTE ORDER MARK
    QTextLayout layout(string);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(INT_MAX / 256);
    layout.endLayout();

    // Don't spin into an infinite loop
 }

void tst_QTextLayout::glyphLessItems()
{
    {
        QTextLayout layout;
        layout.setText("\t\t");
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();
    }

    {
        QTextLayout layout;
        layout.setText(QString::fromLatin1("AA") + QChar(QChar::LineSeparator));
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();
    }
}

void tst_QTextLayout::textWidthVsWIdth()
{
    QTextLayout layout;
    layout.setCacheEnabled(true);
    QTextOption opt;
    opt.setWrapMode(QTextOption::WrapAnywhere);
#if defined(Q_OS_WIN)
    layout.setFont(QFont(QString::fromLatin1("Arial")));
#endif
    layout.setTextOption(opt);
    layout.setText(QString::fromLatin1(
                       "g++ -c -m64 -pipe -g -fvisibility=hidden -fvisibility-inlines-hidden -Wall -W -D_REENTRANT -fPIC -DCORE_LIBRARY -DIDE_LIBRARY_BASENAME=\"lib\" -DWITH_TESTS "
                       "-DQT_NO_CAST_TO_ASCII -DQT_USE_FAST_OPERATOR_PLUS -DQT_USE_FAST_CONCATENATION -DQT_PLUGIN -DQT_TESTLIB_LIB -DQT_SCRIPT_LIB -DQT_SVG_LIB -DQT_SQL_LIB -DQT_XM"
                       "L_LIB -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_CORE_LIB -DQT_SHARED -I../../../../qt-qml/mkspecs/linux-g++-64 -I. -I../../../../qt-qml/include/QtCore -I../../../."
                       "./qt-qml/include/QtNetwork -I../../../../qt-qml/include/QtGui -I../../../../qt-qml/include/QtXml -I../../../../qt-qml/include/QtSql -I../../../../qt-qml/inc"
                       "lude/QtSvg -I../../../../qt-qml/include/QtScript -I../../../../qt-qml/include/QtTest -I../../../../qt-qml/include -I../../../../qt-qml/include/QtHelp -I../."
                       "./libs -I/home/ettrich/dev/creator/tools -I../../plugins -I../../shared/scriptwrapper -I../../libs/3rdparty/botan/build -Idialogs -Iactionmanager -Ieditorma"
                       "nager -Iprogressmanager -Iscriptmanager -I.moc/debug-shared -I.uic -o .obj/debug-shared/sidebar.o sidebar.cpp"));

    // The naturalTextWidth includes right bearing, but should never be LARGER than line width if
    // there is space for at least one character. Unfortunately that assumption may not hold if the
    // font engine fails to report an accurate minimum right bearing for the font, eg. when the
    // minimum right bearing reported by the font engine doesn't cover all the glyphs in the font.
    // The result is that this test may fail in some cases. We should fix this by running the test
    // with a font that we know have no suprising right bearings. See qtextlayout.cpp for details.
    QFontMetricsF fontMetrics(layout.font());
    QSet<char16_t> checked;
    qreal minimumRightBearing = 0.0;
    for (int i = 0; i < layout.text().size(); ++i) {
        QChar c = layout.text().at(i);
        if (!checked.contains(c.unicode())) {
            qreal rightBearing = fontMetrics.rightBearing(c);
            if (rightBearing < minimumRightBearing)
                minimumRightBearing = rightBearing;
            checked.insert(c.unicode());
        }
    }
    if (minimumRightBearing < fontMetrics.minRightBearing())
        QSKIP("Font reports invalid minimum right bearing, and can't be used for this test.");

    for (int width = 100; width < 1000; ++width) {
        layout.beginLayout();
        QTextLine line = layout.createLine();
        line.setLineWidth(width);
        layout.endLayout();

        qreal textWidthIsLargerBy = qMax(qreal(0), line.naturalTextWidth() - line.width());
        qreal thisMustBeZero = 0;
        QCOMPARE(textWidthIsLargerBy, thisMustBeZero);
    }
}

void tst_QTextLayout::textWithSurrogates_qtbug15679()
{
    QString str = QString::fromUtf8("🀀a🀀");
    QTextLayout layout(str);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    qreal x[6];
    for (int i = 0; i < 6; i++)
        x[i] = line.cursorToX(i);

    // If the first and third character are using the same
    // font, they must have the same advance (since they
    // are surrogate pairs, we need to add two for each
    // character)
    QCOMPARE(x[2] - x[0], x[5] - x[3]);
}

void tst_QTextLayout::textWidthWithStackedTextEngine()
{
    QString text = QString::fromUtf8("คลิก ถัดไป เพื่อดำเนินการต่อ");

    QTextLayout layout(text);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    QStackTextEngine layout2(text, layout.font());

    QVERIFY(layout2.width(0, text.size()).toReal() >= line.naturalTextWidth());
}

void tst_QTextLayout::textWidthWithLineSeparator()
{
    QString s1("Save Project"), s2("Save Project\ntest");
    s2.replace('\n', QChar::LineSeparator);

    QTextLayout layout1(s1), layout2(s2);
    layout1.beginLayout();
    layout2.beginLayout();

    QTextLine line1 = layout1.createLine();
    QTextLine line2 = layout2.createLine();
    line1.setLineWidth(0x1000);
    line2.setLineWidth(0x1000);
    QCOMPARE(line1.naturalTextWidth(), line2.naturalTextWidth());
}

void tst_QTextLayout::cursorInLigatureWithMultipleLines()
{
    QTextLayout layout("first line finish", QFont("Times", 20));
    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setNumColumns(10);
    QTextLine line2 = layout.createLine();
    layout.endLayout();

    // The second line will be "finish"
    QCOMPARE(layout.text().mid(line2.textStart(), line2.textLength()), QString::fromLatin1("finish"));

    QVERIFY(line.cursorToX(1) != line.cursorToX(0));
    QCOMPARE(line2.cursorToX(line2.textStart()), line.cursorToX(0));
    QCOMPARE(line2.cursorToX(line2.textStart() + 1), line.cursorToX(1));
}

void tst_QTextLayout::xToCursorForLigatures()
{
#if defined(Q_OS_WIN32)
    QTextLayout layout("fi", QFont("Calibri", 20));
#else
    QTextLayout layout("fi", QFont("Times", 20));
#endif

    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    QVERIFY(line.xToCursor(0) != line.xToCursor(line.naturalTextWidth() / 2));

    // U+0061 U+0308
    QTextLayout layout2(QString::fromUtf8("\x61\xCC\x88"), QFont("Times", 20));
    layout2.setCacheEnabled(true);
    layout2.beginLayout();
    line = layout2.createLine();
    layout2.endLayout();

    qreal width = line.naturalTextWidth();
    QVERIFY(line.xToCursor(0) == line.xToCursor(width / 2) ||
            line.xToCursor(width) == line.xToCursor(width / 2));
}

void tst_QTextLayout::cursorInNonStopChars()
{
    QTextLayout layout(QString::fromUtf8("\xE0\xA4\xA4\xE0\xA5\x8D\xE0\xA4\xA8"));
    layout.setCacheEnabled(true);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    QCOMPARE(line.cursorToX(1), line.cursorToX(3));
    QCOMPARE(line.cursorToX(2), line.cursorToX(3));
}

void tst_QTextLayout::justifyTrailingSpaces()
{
    QTextLayout layout(QStringLiteral("     t"));
    layout.setTextOption(QTextOption(Qt::AlignJustify));
    layout.beginLayout();

    QTextLine line = layout.createLine();
    line.setLineWidth(5);

    layout.endLayout();

    QVERIFY(qFuzzyIsNull(layout.lineAt(0).cursorToX(0)));
}

void tst_QTextLayout::nbsp()
{
    QString s = QString() + QChar(' ') + QChar('a') + QString(10, QChar::Nbsp) + QChar('a') + QChar(' ') + QChar('A');
    QString text = s + s + s + s + s + s + s + s + s + s + s + s + s + s;
    QTextLayout layout(text);
    layout.setCacheEnabled(true);
    layout.beginLayout();
    layout.createLine();
    layout.endLayout();

    int naturalWidth = qCeil(layout.lineAt(0).naturalTextWidth());
    int lineWidth = naturalWidth;

    layout.beginLayout();
    QTextLine line = layout.createLine();
    while (lineWidth-- > naturalWidth / 2) {
        line.setLineWidth(lineWidth);
        QVERIFY(text.at(line.textLength()-1).unicode() != QChar::Nbsp);
    }

    layout.endLayout();
}

void tst_QTextLayout::layoutWithCustomTabStops()
{
    QScopedPointer<QTextLayout> textLayout(new QTextLayout);
    QList<QTextOption::Tab> tabStops;

    const int tabWidth = 18;
    const int maxTabPos = 2500;
    for (int tabPos = tabWidth; tabPos < maxTabPos; tabPos += tabWidth)
        tabStops << QTextOption::Tab(tabPos, QTextOption::LeftTab);

    QTextOption textOption;
    textOption.setTabs(tabStops);
    textLayout->setTextOption(textOption);

    textLayout->setText(QStringLiteral("\ta aa aa aa aa aa aa"));

    textLayout->beginLayout();
    textLayout->createLine();
    textLayout->endLayout();

    qreal shortWidth = textLayout->maximumWidth();

    textLayout->setText(QStringLiteral("\ta aa aa aa aa aa aa aa aa aa aa aa aa a"));

    textLayout->beginLayout();
    textLayout->createLine();
    textLayout->endLayout();

    qreal longWidth = textLayout->maximumWidth();

    QVERIFY(longWidth > shortWidth);
}

void tst_QTextLayout::noModificationOfInputString()
{
    QString s = QString(QChar(QChar::LineSeparator));
    {
        QTextLayout layout;
        layout.setText(s);

        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QCOMPARE(s.size(), 1);
        QCOMPARE(s.at(0), QChar(QChar::LineSeparator));
    }

    {
        QTextLayout layout;
        layout.setText(s);

        QTextOption option;
        option.setFlags(QTextOption::ShowLineAndParagraphSeparators);
        layout.setTextOption(option);

        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QCOMPARE(s.size(), 1);
        QCOMPARE(s.at(0), QChar(QChar::LineSeparator));
    }
}

void tst_QTextLayout::showLineAndParagraphSeparatorsCrash()
{
    QString s = QString(100000, QChar('a')) + QChar(QChar::LineSeparator);
    {
        QTextLayout layout;
        layout.setText(s);

        QTextOption option;
        option.setFlags(QTextOption::ShowLineAndParagraphSeparators);
        layout.setTextOption(option);

        layout.beginLayout();
        layout.createLine();
        layout.endLayout();
    }
}

void tst_QTextLayout::superscriptCrash_qtbug53911()
{
    static int fontSizes = 64;
    static QString layoutText = "THIS IS SOME EXAMPLE TEXT THIS IS SOME EXAMPLE TEXT";

    QList<QTextLayout*> textLayouts;
    for (int i = 0; i < fontSizes; ++i) {
        for (int j = 0; j < 4; ++j) {
            QTextLayout* newTextLayout = new QTextLayout();
            newTextLayout->setText(layoutText);
            QTextLayout::FormatRange formatRange;

            formatRange.format.setFont(QFont());
            formatRange.format.setFontPointSize(i + 5);

            switch (j) {
            case 0:
                formatRange.format.setFontWeight(QFont::Normal);
                formatRange.format.setFontItalic(false);
                break;
            case 1:
                formatRange.format.setFontWeight(QFont::Bold);
                formatRange.format.setFontItalic(false);
                break;
            case 2:
                formatRange.format.setFontWeight(QFont::Bold);
                formatRange.format.setFontItalic(true);
                break;
            case 3:
                formatRange.format.setFontWeight(QFont::Normal);
                formatRange.format.setFontItalic(true);
                break;
            }

            formatRange.format.setVerticalAlignment( QTextCharFormat::AlignSuperScript);

            formatRange.start = 0;
            formatRange.length = layoutText.size();
            newTextLayout->setFormats({formatRange});

            textLayouts.push_front(newTextLayout);
        }
    }

    // This loop would crash before fix for QTBUG-53911
    foreach (QTextLayout *textLayout, textLayouts) {
        textLayout->beginLayout();
        while (textLayout->createLine().isValid());
        textLayout->endLayout();
    }

    qDeleteAll(textLayouts);
}

void tst_QTextLayout::nbspWithFormat()
{
    QString s1 = QLatin1String("ABCDEF ");
    QString s2 = QLatin1String("GHI");
    QChar nbsp(QChar::Nbsp);
    QString s3 = QLatin1String("JKLMNOPQRSTUVWXYZ");

    QTextLayout layout;
    layout.setText(s1 + s2 + nbsp + s3);

    QTextLayout::FormatRange formatRange;
    formatRange.start = s1.size() + s2.size();
    formatRange.length = 1;
    formatRange.format.setFontUnderline(true);

    layout.setFormats({formatRange});

    layout.beginLayout();
    forever {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(1);
    }
    layout.endLayout();

    QCOMPARE(layout.lineCount(), 2);
    QCOMPARE(layout.lineAt(0).textStart(), 0);
    QCOMPARE(layout.lineAt(0).textLength(), s1.size());
    QCOMPARE(layout.lineAt(1).textStart(), s1.size());
    QCOMPARE(layout.lineAt(1).textLength(), s2.size() + 1 + s3.size());
}

void tst_QTextLayout::koreanWordWrap()
{
    QString s = QString::fromUtf8("안녕하세요 여러분!");
    QTextLayout layout;
    QTextOption option = layout.textOption();
    option.setWrapMode(QTextOption::WordWrap);
    option.setFlags(QTextOption::Flag(QTextOption::IncludeTrailingSpaces));
    layout.setTextOption(option);
    layout.setText(s);

    QFontMetrics metrics(layout.font());

    layout.beginLayout();
    forever {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(metrics.horizontalAdvance(s) * 0.8);
    }
    layout.endLayout();
    QCOMPARE(layout.lineCount(), 2);
    QCOMPARE(layout.lineAt(0).textLength(), 6);
    QCOMPARE(layout.lineAt(1).textLength(), 4);
}

void tst_QTextLayout::tooManyDirectionalCharctersCrash_qtbug77819()
{
    QString data;
    data += QString::fromUtf8("\xe2\x81\xa8"); // U+2068 FSI character
    data += QString::fromUtf8("\xe2\x81\xa7"); // U+2067 RLI character

    // duplicating the text
    for (int i = 0; i < 10; i++)
        data += data;

    // Nothing to test. It must not crash in beginLayout().
    QTextLayout tl(data);
    tl.beginLayout();
    tl.endLayout();
}

void tst_QTextLayout::softHyphens_data()
{
    QTest::addColumn<int>("fontSize");

    QTest::newRow("12") << 12;
    QTest::newRow("14") << 14;
    QTest::newRow("16") << 16;
}

void tst_QTextLayout::softHyphens()
{
    QFETCH(int, fontSize);
    QString text = QStringLiteral("xxxx\u00ad") + QStringLiteral("xxxx\u00ad");

    QFont font;
    font.setPixelSize(fontSize);
    font.setHintingPreference(QFont::PreferNoHinting);
    const float xAdvance = QFontMetricsF(font).horizontalAdvance(QChar::fromLatin1('x'));
    float shyWidth = 0.0f;
    QTextLayout layout(text, font);
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(option);
    {
        // Calculate the effective width of a line-ending hyphen
        // This calculation is currently done to work-around odditities on
        // macOS 11 (see QTBUG-90698).
        QTextLayout test(QStringLiteral("x\u00ad"), font);
        // Note: This only works because Qt show the soft-hyphen when ending a text.
        // This _could_ be considered a bug and the test would need to be changed
        // if we stop doing that.
        test.beginLayout();
        QTextLine line = test.createLine();
        line.setLineWidth(10 * xAdvance);
        line.setPosition(QPoint(0, 0));
        shyWidth = line.naturalTextWidth() - xAdvance;
        test.endLayout();
    }
    qreal linefit;
    // Loose fit
    // xxxx- |
    // xxxx- |
    {
        int pos = 0;
        int y = 0;
        layout.beginLayout();
        QTextLine line = layout.createLine();
        line.setLineWidth(qCeil(5 * xAdvance + shyWidth) + 1);
        line.setPosition(QPoint(0, y));
        QCOMPARE(line.textStart(), pos);
        QCOMPARE(line.textLength(), 5);
        linefit = line.naturalTextWidth();
        QVERIFY(qAbs(linefit - qCeil(4 * xAdvance + shyWidth)) <= 1.0);

        pos += line.textLength();
        y += qRound(line.ascent() + line.descent());

        line = layout.createLine();
        line.setLineWidth(qCeil(5 * xAdvance + shyWidth) + 1);
        line.setPosition(QPoint(0, y));
        QCOMPARE(line.textStart(), pos);
        QCOMPARE(line.textLength(), 5);
        QVERIFY(qAbs(line.naturalTextWidth() - linefit) <= 1.0);
        layout.endLayout();
    }

    // Tight fit
    // xxxx-|
    // xxxx-|
    {
        int pos = 0;
        int y = 0;
        layout.beginLayout();
        QTextLine line = layout.createLine();
        line.setLineWidth(qCeil(linefit) + 1);
        line.setPosition(QPoint(0, y));
        QCOMPARE(line.textStart(), pos);
        QCOMPARE(line.textLength(), 5);
        QVERIFY(qAbs(line.naturalTextWidth() - linefit) <= 1.0);

        pos += line.textLength();
        y += qRound(line.ascent() + line.descent());

        line = layout.createLine();
        line.setLineWidth(qCeil(linefit) + 1);
        line.setPosition(QPoint(0, y));
        QCOMPARE(line.textStart(), pos);
        QCOMPARE(line.textLength(), 5);
        QVERIFY(qAbs(line.naturalTextWidth() - linefit) <= 1.0);
        layout.endLayout();
    }

    // Very tight fit
    // xxxx|
    // xxxx|
    // -   |
    {
        int pos = 0;
        int y = 0;
        layout.beginLayout();
        QTextLine line = layout.createLine();
        line.setLineWidth(qCeil(4 * xAdvance) + 2);
        line.setPosition(QPoint(0, y));
        QCOMPARE(line.textStart(), pos);
        QCOMPARE(line.textLength(), 4);
        QVERIFY(qAbs(line.naturalTextWidth() - qCeil(4 * xAdvance)) <= 1.0);

        pos += line.textLength();
        y += qRound(line.ascent() + line.descent());

        line = layout.createLine();
        line.setLineWidth(qCeil(4 * xAdvance) + 2);
        line.setPosition(QPoint(0, y));
        QCOMPARE(line.textStart(), pos);
        QCOMPARE(line.textLength(), 5);
        QVERIFY(qAbs(line.naturalTextWidth() - qCeil(4 * xAdvance)) <= 1.0);

        pos += line.textLength();
        y += qRound(line.ascent() + line.descent());

        line = layout.createLine();
        line.setLineWidth(qCeil(4 * xAdvance) + 2);
        line.setPosition(QPoint(0, y));
        QCOMPARE(line.textStart(), pos);
        QCOMPARE(line.textLength(), 1);
        QVERIFY(qAbs(line.naturalTextWidth() - shyWidth) <= 1.0);
        layout.endLayout();
    }
}

void tst_QTextLayout::min_maximumWidth_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("long string") << QStringLiteral("lmong_long_crazy_87235982735_23857239682376923876923876-fuwhfhfw-names-AAAA-deeaois2019-03-03.and.more");
    QTest::newRow("QTBUG-106947") << QStringLiteral("text                                text");
    QTest::newRow("spaces") << QStringLiteral("                text                text                ");
    QTest::newRow("QTBUG-104986") << QStringLiteral("text\ntext\ntext");
    QTest::newRow("spaces + line breaks") << QStringLiteral("       \n         text\n                \ntext       \n         ");
}

void tst_QTextLayout::min_maximumWidth()
{
    QFETCH(QString, text);
    text.replace('\n', QChar::LineSeparator);

    QTextLayout layout(text, testFont);
    layout.setCacheEnabled(true);

    QTextOption opt;
    opt.setWrapMode(QTextOption::NoWrap);
    layout.setTextOption(opt);
    layout.beginLayout();
    while (layout.createLine().isValid()) { }
    layout.endLayout();

    const qreal nonWrappedMaxWidth = layout.maximumWidth();

    for (int wrapMode = QTextOption::NoWrap; wrapMode <= QTextOption::WrapAtWordBoundaryOrAnywhere; ++wrapMode) {
        opt.setWrapMode((QTextOption::WrapMode)wrapMode);
        layout.setTextOption(opt);
        layout.beginLayout();
        while (layout.createLine().isValid()) { }
        layout.endLayout();
        const qreal minWidth = layout.minimumWidth();
        const qreal maxWidth = layout.maximumWidth();

        QCOMPARE_LE(minWidth, maxWidth);
        QCOMPARE_LE(maxWidth, nonWrappedMaxWidth); // maxWidth for wrapped text shouldn't exceed maxWidth for the text without wrapping.

        // Try the layout from slightly wider than the widest (maxWidth)
        // and narrow it down to slighly narrower than minWidth
        // layout.maximumWidth() should return the same regardless
        qreal width = qCeil(maxWidth/10)*10 + 10;    // begin a bit wider
        const qreal stepSize = 20;
        while (width >= minWidth - stepSize) {
            layout.beginLayout();
            for (;;) {
                QTextLine line = layout.createLine();
                if (!line.isValid())
                    break;
                line.setLineWidth(width);
            }
            layout.endLayout();
            QCOMPARE(layout.minimumWidth(), minWidth);
            QCOMPARE(layout.maximumWidth(), maxWidth);
            width -= stepSize;
        }
    }
}

void tst_QTextLayout::negativeLineWidth()
{
    QTextLayout layout;
    layout.setText("Foo bar");
    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(-1);
    QVERIFY(line.textLength() > 0);
    layout.endLayout();
}

QTEST_MAIN(tst_QTextLayout)
#include "tst_qtextlayout.moc"
