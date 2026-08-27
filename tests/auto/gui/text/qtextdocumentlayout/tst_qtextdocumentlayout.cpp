// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>

#include <qtextdocument.h>
#include <qabstracttextdocumentlayout.h>
#include <qdebug.h>
#include <qpainter.h>
#include <qtexttable.h>
#ifndef QT_NO_WIDGETS
#include <qtextedit.h>
#include <qscrollbar.h>
#endif

using namespace Qt::StringLiterals;

class tst_QTextDocumentLayout : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void cleanupTestCase();
    void defaultPageSizeHandling();
    void idealWidth();
#ifndef QT_NO_TEXTHTMLPARSER
    void lineSeparatorFollowingTable();
#endif
#ifndef QT_NO_WIDGETS
    void wrapAtWordBoundaryOrAnywhere();
#endif
    void inlineImage();
#ifndef QT_NO_TEXTHTMLPARSER
    void clippedTableCell();
#endif
    void floatingTablePageBreak();
    void imageAtRightAlignedTab();
    void blockVisibility();
#ifndef QT_NO_TEXTHTMLPARSER
    void testHitTest();

    void largeImage();
#endif
    void wrapTextWithSpacesAndTabs();
    void listMarkerFollowsBaseline();
    void listMarkerHitTest();

private:
    QTextDocument *doc;
    void buildListItem(QTextListFormat::Style style, int imageHeight, bool bigFont,
                       QTextBlockFormat::MarkerType marker = QTextBlockFormat::MarkerType::NoMarker);
    QRect renderAndFindMarker();
};

void tst_QTextDocumentLayout::init()
{
    doc = new QTextDocument;
}

void tst_QTextDocumentLayout::cleanup()
{
    delete doc;
    doc = 0;
}

void tst_QTextDocumentLayout::cleanupTestCase()
{
    if (qgetenv("QTEST_KEEP_IMAGEDATA").toInt() == 0) {
        QFile::remove(QLatin1String("expected.png"));
        QFile::remove(QLatin1String("img.png"));
    }
}

void tst_QTextDocumentLayout::defaultPageSizeHandling()
{
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QVERIFY(layout);

    QVERIFY(!doc->pageSize().isValid());
    QSizeF docSize = layout->documentSize();
    QVERIFY(docSize.width() > 0 && docSize.width() < 1000);
    QVERIFY(docSize.height() > 0 && docSize.height() < 1000);

    doc->setPlainText("Some text\nwith a few lines\nand not real information\nor anything otherwise useful");

    docSize = layout->documentSize();
    QVERIFY(docSize.isValid());
    QVERIFY(docSize.width() != INT_MAX);
    QVERIFY(docSize.height() != INT_MAX);
}

void tst_QTextDocumentLayout::idealWidth()
{
    doc->setPlainText("Some text\nwith a few lines\nand not real information\nor anything otherwise useful");
    doc->setTextWidth(1000);
    QCOMPARE(doc->textWidth(), qreal(1000));
    QCOMPARE(doc->size().width(), doc->textWidth());
    QVERIFY(doc->idealWidth() < doc->textWidth());
    QVERIFY(doc->idealWidth() > 0);

    QTextBlockFormat fmt;
    fmt.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    QTextCursor cursor(doc);
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(fmt);

    QCOMPARE(doc->textWidth(), qreal(1000));
    QCOMPARE(doc->size().width(), doc->textWidth());
    QVERIFY(doc->idealWidth() < doc->textWidth());
    QVERIFY(doc->idealWidth() > 0);
}

#ifndef QT_NO_TEXTHTMLPARSER
// none of the QTextLine items in the document should intersect with the margin rect
void tst_QTextDocumentLayout::lineSeparatorFollowingTable()
{
    QString html_begin("<html><table border=1><tr><th>Column 1</th></tr><tr><td>Data</td></tr></table><br>");
    QString html_text("bla bla bla bla bla bla bla bla<br>");
    QString html_end("<table border=1><tr><th>Column 1</th></tr><tr><td>Data</td></tr></table></html>");

    QString html = html_begin;

    for (int i = 0; i < 80; ++i)
        html += html_text;

    html += html_end;

    doc->setHtml(html);

    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Start);

    const int margin = 87;
    const int pageWidth = 873;
    const int pageHeight = 1358;

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setMargin(margin);
    doc->rootFrame()->setFrameFormat(fmt);

    QFont font(doc->defaultFont());
    font.setPointSize(10);
    doc->setDefaultFont(font);
    doc->setPageSize(QSizeF(pageWidth, pageHeight));

    QRectF marginRect(QPointF(0, pageHeight - margin), QSizeF(pageWidth, 2 * margin));

    // force layouting
    doc->pageCount();

    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        QTextLayout *layout = block.layout();
        for (int i = 0; i < layout->lineCount(); ++i) {
            QTextLine line = layout->lineAt(i);
            QRectF rect = line.rect().translated(layout->position());
            QVERIFY(!rect.intersects(marginRect));
        }
    }
}
#endif

#ifndef QT_NO_WIDGETS
void tst_QTextDocumentLayout::wrapAtWordBoundaryOrAnywhere()
{
    //task 150562
    QTextEdit edit;
    edit.setText("<table><tr><td>hello hello hello"
            "thisisabigwordthisisabigwordthisisabigwordthisisabigwordthisisabigword"
            "hello hello hello</td></tr></table>");
    edit.setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    edit.resize(100, 100);
    edit.show();
    QVERIFY(!edit.horizontalScrollBar()->isVisible());
}
#endif

void tst_QTextDocumentLayout::inlineImage()
{
    doc->setPageSize(QSizeF(800, 500));

    QImage img(400, 400, QImage::Format_RGB32);
    QLatin1String name("bigImage");

    doc->addResource(QTextDocument::ImageResource, QUrl(name), img);

    QTextImageFormat imgFormat;
    imgFormat.setName(name);
    imgFormat.setWidth(img.width());

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    qreal height = doc->pageSize().height() - fmt.topMargin() - fmt.bottomMargin();
    imgFormat.setHeight(height);

    QTextCursor cursor(doc);
    cursor.insertImage(imgFormat);

    QCOMPARE(doc->pageCount(), 1);
}

#ifndef QT_NO_TEXTHTMLPARSER
void tst_QTextDocumentLayout::clippedTableCell()
{
    const char *html =
        "<table style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\""
        "border=\"0\" margin=\"0\" cellpadding=\"0\" cellspacing=\"0\"><tr><td></td></tr></table>";

    doc->setHtml(html);
    doc->pageSize();

    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Right);

    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextCursor cellCursor = table->cellAt(0, 0).firstCursorPosition();
    QImage src(16, 16, QImage::Format_ARGB32_Premultiplied);
    src.fill(0xffff0000);
    cellCursor.insertImage(src);

    QTextBlock block = cellCursor.block();
    QRectF r = doc->documentLayout()->blockBoundingRect(block);

    QRectF rect(0, 0, r.left() + 1, 64);

    QImage img(64, 64, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x0);
    QImage expected = img;
    QPainter p(&img);
    doc->drawContents(&p, rect);
    p.end();
    p.begin(&expected);
    r.setWidth(1);
    p.fillRect(r, Qt::red);
    p.end();

    img.save("img.png");
    expected.save("expected.png");
    QCOMPARE(img, expected);
}
#endif

void tst_QTextDocumentLayout::floatingTablePageBreak()
{
    doc->clear();

    QTextCursor cursor(doc);

    QTextTableFormat tableFormat;
    tableFormat.setPosition(QTextFrameFormat::FloatLeft);
    QTextTable *table = cursor.insertTable(50, 1, tableFormat);
    Q_UNUSED(table);

    // Make height of document 2/3 of the table, fitting the table into two pages
    QSizeF documentSize = doc->size();
    documentSize.rheight() *= 2.0 / 3.0;

    doc->setPageSize(documentSize);

    QCOMPARE(doc->pageCount(), 2);
}

void tst_QTextDocumentLayout::imageAtRightAlignedTab()
{
    doc->clear();

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setMargin(0);
    doc->rootFrame()->setFrameFormat(fmt);

    QTextCursor cursor(doc);
    QTextBlockFormat blockFormat;
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab;
    tab.position = 300;
    tab.type = QTextOption::RightTab;
    tabs.append(tab);
    blockFormat.setTabPositions(tabs);

    // First block: text, some of it right-aligned
    cursor.insertBlock(blockFormat);
    cursor.insertText("first line\t");
    cursor.insertText("right-aligned text");

    // Second block: text, then right-aligned image
    cursor.insertBlock(blockFormat);
    cursor.insertText("second line\t");
    QImage img(48, 48, QImage::Format_RGB32);
    const QString name = QString::fromLatin1("image");
    doc->addResource(QTextDocument::ImageResource, QUrl(name), img);
    QTextImageFormat imgFormat;
    imgFormat.setName(name);
    cursor.insertImage(imgFormat);

    // Everything should fit into the 300 pixels
    qreal bearing = QFontMetricsF(doc->defaultFont()).rightBearing(QLatin1Char('t'));
    QCOMPARE(doc->idealWidth(), std::max(300.0, 300.0 - bearing));
}

void tst_QTextDocumentLayout::blockVisibility()
{
    QTextCursor cursor(doc);
    for (int i = 0; i < 10; ++i) {
        if (!doc->isEmpty())
            cursor.insertBlock();
        cursor.insertText("A");
    }

    qreal margin = doc->documentMargin();
    QSizeF emptySize(2 * margin, 2 * margin);
    QSizeF halfSize = doc->size();
    halfSize.rheight() -= 2 * margin;
    halfSize.rheight() /= 2;
    halfSize.rheight() += 2 * margin;

    for (int i = 0; i < 10; i += 2) {
        QTextBlock block = doc->findBlockByNumber(i);
        block.setVisible(false);
        doc->markContentsDirty(block.position(), block.length());
    }

    QCOMPARE(doc->size(), halfSize);

    for (int i = 1; i < 10; i += 2) {
        QTextBlock block = doc->findBlockByNumber(i);
        block.setVisible(false);
        doc->markContentsDirty(block.position(), block.length());
    }

    QCOMPARE(doc->size(), emptySize);

    for (int i = 0; i < 10; i += 2) {
        QTextBlock block = doc->findBlockByNumber(i);
        block.setVisible(true);
        doc->markContentsDirty(block.position(), block.length());
    }

    QCOMPARE(doc->size(), halfSize);
}

#ifndef QT_NO_TEXTHTMLPARSER
void tst_QTextDocumentLayout::largeImage()
{
     auto img = QImage(400, 500, QImage::Format_ARGB32_Premultiplied);
     img.fill(Qt::black);

     {
         QTextDocument document;

         document.addResource(QTextDocument::ImageResource,
                 QUrl("data://test.png"), QVariant(img));
         document.setPageSize({500, 504});

         auto html = "<img src=\"data://test.png\">";
         document.setHtml(html);

         QCOMPARE(document.pageCount(), 2);
     }

     {
         QTextDocument document;

         document.addResource(QTextDocument::ImageResource,
                 QUrl("data://test.png"), QVariant(img));
         document.setPageSize({500, 508});

         auto html = "<img src=\"data://test.png\">";
         document.setHtml(html);

         QCOMPARE(document.pageCount(), 1);
     }

     {
         QTextDocument document;

         document.addResource(QTextDocument::ImageResource,
                 QUrl("data://test.png"), QVariant(img));
         document.setPageSize({585, 250});

         auto html = "<img src=\"data://test.png\">";
         document.setHtml(html);

         QCOMPARE(document.pageCount(), 3);
     }

     {
         QTextDocument document;

         document.addResource(QTextDocument::ImageResource,
                 QUrl("data://test.png"), QVariant(img));
         document.setPageSize({585, 258});

         auto html = "<img src=\"data://test.png\">";
         document.setHtml(html);

         QCOMPARE(document.pageCount(), 2);
     }
}

void tst_QTextDocumentLayout::testHitTest()
{
    QTextDocument document;
    QTextCursor cur(&document);
    int topMargin = 20;

    //insert 500 blocks into textedit
    for (int i = 0; i < 500; i++) {
      cur.insertBlock();
      cur.insertHtml(QString("block %1").arg(i));
    }

    //randomly set half the blocks invisible
    QTextBlock blk=document.begin();
    for (int i = 0; i < 500; i++) {
      if (i % 7)
        blk.setVisible(0);
      blk = blk.next();
    }

    //set margin for all blocks (not strictly necessary, but makes easier to click in between blocks)
    QTextBlockFormat blkfmt;
    blkfmt.setTopMargin(topMargin);
    cur.movePosition(QTextCursor::Start);
    cur.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cur.mergeBlockFormat(blkfmt);

    for (int y = cur.selectionStart(); y < cur.selectionEnd(); y += 10) {
         QPoint mousePoint(1, y);
         int cursorPos = document.documentLayout()->hitTest(mousePoint, Qt::FuzzyHit);
         int positionY = document.findBlock(cursorPos).layout()->position().toPoint().y();
         //mousePoint is in the rect of the current Block
         QVERIFY(positionY - topMargin <= y);
    }
}
#endif

void tst_QTextDocumentLayout::wrapTextWithSpacesAndTabs()
{
    doc->clear();

    QTextOption option = doc->defaultTextOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    doc->setDefaultTextOption(option);
    doc->setTextWidth(200);

    QTextCursor cursor(doc);
    // Spaces followed by tab create specific width that triggers overflow handling
    cursor.insertText(QString(70, ' ') + QStringLiteral("\t") + QStringLiteral("text"));

    doc->documentLayout()->documentSize();

    QTextBlock block = doc->firstBlock();
    QVERIFY(block.isValid());

    // Verify wrap mode is correctly restored to WrapAtWordBoundaryOrAnywhere
    QCOMPARE(block.layout()->textOption().wrapMode(),
             QTextOption::WrapAtWordBoundaryOrAnywhere);
}

// Replaces the contents of doc with a single list item holding "Item", optionally
// followed by an inline image of the given height and/or a run of much larger text.
void tst_QTextDocumentLayout::buildListItem(QTextListFormat::Style style, int imageHeight,
                                            bool bigFont, QTextBlockFormat::MarkerType marker)
{
    QImage tallImage(16, 64, QImage::Format_RGB32);
    tallImage.fill(Qt::black);

    doc->clear();
    doc->addResource(QTextDocument::ImageResource, QUrl(u"tall"_s), tallImage);
    // Pin the font, so that the inline images below are reliably taller than a line of
    // text no matter how large the application font is.
    QFont font = doc->defaultFont();
    font.setPixelSize(12);
    doc->setDefaultFont(font);
    doc->setTextWidth(400);

    // createList() rather than insertList(), so that the list item is the first block
    // rather than being preceded by an empty one.
    QTextCursor cursor(doc);
    QTextListFormat listFormat;
    listFormat.setStyle(style);
    cursor.createList(listFormat);
    if (marker != QTextBlockFormat::MarkerType::NoMarker) {
        QTextBlockFormat blockFormat = cursor.blockFormat();
        blockFormat.setMarker(marker);
        cursor.setBlockFormat(blockFormat);
    }

    cursor.insertText(u"Item"_s);
    if (imageHeight > 0) {
        QTextImageFormat imageFormat;
        imageFormat.setName(u"tall"_s);
        imageFormat.setWidth(16);
        imageFormat.setHeight(imageHeight);
        cursor.insertImage(imageFormat);
    }
    if (bigFont) {
        QFont big = font;
        big.setPixelSize(font.pixelSize() * 4);
        QTextCharFormat bigFormat;
        bigFormat.setFont(big);
        cursor.insertText(u"Big"_s, bigFormat);
    }
}

// Renders doc and returns the bounding rect, in image coordinates, of everything
// painted to the left of the first block's text - that is, of the list item marker.
QRect tst_QTextDocumentLayout::renderAndFindMarker()
{
    const QTextBlock block = doc->firstBlock();
    if (!block.isValid() || !block.textList())
        return QRect();

    // Ask for the document size first, so that the block is fully laid out before its
    // line geometry is read.
    const QSize documentSize = doc->documentLayout()->documentSize().toSize();
    const QTextLayout *layout = block.layout();
    if (layout->lineCount() == 0)
        return QRect();
    const QTextLine line = layout->lineAt(0);

    QImage rendered(documentSize + QSize(4, 4), QImage::Format_RGB32);
    rendered.fill(Qt::white);
    {
        QPainter painter(&rendered);
        QAbstractTextDocumentLayout::PaintContext context;
        // Not drawContents(), which would use the application palette: under a dark
        // theme that paints the marker white on white.
        context.palette.setColor(QPalette::Text, Qt::black);
        doc->documentLayout()->draw(&painter, context);
    }

    // Only the marker is painted left of the text, and drawListItem() leaves the width
    // of a space between the two. Scan up to the marker's right edge, which keeps the
    // scan clear of the text even if its first glyph has a negative left side bearing.
    // Round the two terms separately, the way drawListItem() does, so that the boundary
    // cannot land a pixel away from the marker's right edge.
    const QFontMetrics fontMetrics(block.charFormat().font());
    const int space = fontMetrics.horizontalAdvance(u' ');
    const int textLeft = qRound(layout->position().x()) + qRound(line.naturalTextRect().left());
    const int markerRight = qMin(textLeft - space + 1, rendered.width());
    if (markerRight <= 0)
        return QRect();

    QRect found;
    for (int y = 0; y < rendered.height(); ++y) {
        for (int x = 0; x < markerRight; ++x) {
            if (rendered.pixel(x, y) != qRgb(255, 255, 255))
                found |= QRect(x, y, 1, 1);
        }
    }
    return found;
}

void tst_QTextDocumentLayout::listMarkerFollowsBaseline()
{
    // The list item marker sits on the first line's baseline. Content that makes that
    // line taller - an inline image, or a larger font - pushes the baseline down, and
    // the marker has to follow it rather than float above the content. So the marker's
    // offset from the baseline must not depend on how tall the line is. QTBUG-141568.
    //
    // A square marker is painted as a plain filled rectangle rather than as a glyph,
    // so its extent is easy to measure from the rendered image.

    struct Content {
        const char *name;
        int imageHeight;
        bool bigFont;
    };
    static constexpr Content contents[] = {
        { "plain",      0,  false },
        { "24px image", 24, false },
        { "64px image", 64, false },
        { "big font",   0,  true  },
    };

    qreal reference = 0;
    qreal referenceAscent = 0;
    bool first = true;

    for (const Content &content : contents) {
        buildListItem(QTextListFormat::ListSquare, content.imageHeight, content.bigFont);

        const QRect marker = renderAndFindMarker();
        QVERIFY2(!marker.isNull(), content.name);

        const QTextBlock block = doc->firstBlock();
        const QTextLayout *layout = block.layout();
        const QTextLine line = layout->lineAt(0);
        const QFontMetrics fontMetrics(block.charFormat().font());

        const qreal lineTop = qRound(layout->position().y()) + qRound(line.naturalTextRect().top());
        const qreal markerCentre = (marker.top() + marker.bottom()) / 2.0;
        const qreal offset = markerCentre - (lineTop + line.ascent());

        if (first) {
            // The plain case must be exactly what it was before markers were moved onto
            // the baseline: centred on the marker font's height, measured from the top
            // of the line. Without this the relative comparisons below would still hold
            // if every marker were displaced by the same amount. The half pixel is the
            // difference between a continuous centre and the centre of a pixel range.
            const qreal expected = lineTop + fontMetrics.height() / 2 - 0.5;
            QVERIFY2(qAbs(markerCentre - expected) <= 1.0,
                     qPrintable(u"plain: marker centre at %1, expected %2"_s
                                .arg(markerCentre).arg(expected)));
            reference = offset;
            referenceAscent = line.ascent();
            first = false;
            continue;
        }

        // Guard against the content silently failing to make the line taller, which
        // would make the comparison below pass for the wrong reason.
        QCOMPARE_GT(line.ascent(), referenceAscent);

        // The marker font is the same throughout, so its offset from the baseline must
        // be too - give or take the rounding of that offset to whole pixels.
        QVERIFY2(qAbs(offset - reference) <= 1.5,
                 qPrintable(u"%1: marker centre is %2 below the baseline, expected %3"_s
                            .arg(QLatin1StringView(content.name))
                            .arg(offset).arg(reference)));
    }
}

void tst_QTextDocumentLayout::listMarkerHitTest()
{
    // blockWithMarkerAt() has to look for the marker where drawListItem() paints it.
    // Both follow the first line's baseline, so a checkbox in an item with a tall first
    // line stays clickable. QTBUG-141568.

    for (int imageHeight : { 0, 64 }) {
        buildListItem(QTextListFormat::ListDisc, imageHeight, false,
                      QTextBlockFormat::MarkerType::Unchecked);

        // With a marker set, the checkbox outline is painted instead of the disc.
        const QRect marker = renderAndFindMarker();
        QVERIFY2(!marker.isNull(),
                 qPrintable(u"image height %1: no marker was painted"_s.arg(imageHeight)));

        const QTextBlock hit = doc->documentLayout()->blockWithMarkerAt(marker.center());
        QVERIFY2(hit.isValid(),
                 qPrintable(u"image height %1: nothing hit at %2, marker was painted at %3"_s
                            .arg(imageHeight)
                            .arg(QDebug::toString(marker.center()), QDebug::toString(marker))));
    }
}

QTEST_MAIN(tst_QTextDocumentLayout)
#include "tst_qtextdocumentlayout.moc"
