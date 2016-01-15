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
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextList>
#include <QTextTable>
#include <QBuffer>
#include <QDebug>

#include <private/qtextodfwriter_p.h>

class tst_QTextOdfWriter : public QObject
{
    Q_OBJECT
public slots:
    void init();
    void cleanup();

private slots:
    void testWriteParagraph_data();
    void testWriteParagraph();
    void testWriteStyle1_data();
    void testWriteStyle1();
    void testWriteStyle2();
    void testWriteList();
    void testWriteList2();
    void createArchive();
    void testWriteAll();
    void testWriteSection();
    void testWriteTable();
    void testWriteFrameFormat();

private:
    /// closes the document and returns the part of the XML stream that the test wrote
    QString getContentFromXml();

private:
    QTextDocument *document;
    QXmlStreamWriter *xmlWriter;
    QTextOdfWriter *odfWriter;
    QBuffer *buffer;
};

void tst_QTextOdfWriter::init()
{
    document = new QTextDocument();
    odfWriter = new QTextOdfWriter(*document, 0);

    buffer = new QBuffer();
    buffer->open(QIODevice::WriteOnly);
    xmlWriter = new QXmlStreamWriter(buffer);
    xmlWriter->writeNamespace(odfWriter->officeNS, "office");
    xmlWriter->writeNamespace(odfWriter->textNS, "text");
    xmlWriter->writeNamespace(odfWriter->styleNS, "style");
    xmlWriter->writeNamespace(odfWriter->foNS, "fo");
    xmlWriter->writeNamespace(odfWriter->tableNS, "table");
    xmlWriter->writeStartDocument();
    xmlWriter->writeStartElement("dummy");
}

void tst_QTextOdfWriter::cleanup()
{
    delete document;
    delete odfWriter;
    delete xmlWriter;
    delete buffer;
}

QString tst_QTextOdfWriter::getContentFromXml()
{
    xmlWriter->writeEndDocument();
    buffer->close();
    QString stringContent = QString::fromUtf8(buffer->data());
    QString ret;
    int index = stringContent.indexOf("<dummy");
    if (index > 0) {
        index = stringContent.indexOf('>', index);
        if (index > 0)
            ret = stringContent.mid(index+1, stringContent.length() - index - 10);
    }
    return ret;
}

void tst_QTextOdfWriter::testWriteParagraph_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("xml");

    QTest::newRow("empty") << "" <<
        "<text:p text:style-name=\"p1\"/>";
    QTest::newRow("spaces") << "foobar   word" <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">foobar <text:s text:c=\"2\"/>word</text:span></text:p>";
    QTest::newRow("starting spaces") << "  starting spaces" <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\"><text:s text:c=\"2\"/>starting spaces</text:span></text:p>";
    QTest::newRow("trailing spaces") << "trailing spaces  " <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">trailing spaces <text:s/></text:span></text:p>";
    QTest::newRow("tab") << "word\ttab x" <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">word<text:tab/>tab x</text:span></text:p>";
    QTest::newRow("tab2") << "word\t\ttab\tx" <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">word<text:tab/><text:tab/>tab<text:tab/>x</text:span></text:p>";
    QTest::newRow("misc") << "foobar   word\ttab x" <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">foobar <text:s text:c=\"2\"/>word<text:tab/>tab x</text:span></text:p>";
    QTest::newRow("misc2") << "\t     \tFoo" <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\"><text:tab/> <text:s text:c=\"4\"/><text:tab/>Foo</text:span></text:p>";
    QTest::newRow("linefeed") << (QStringLiteral("line1") + QChar(0x2028) + QStringLiteral("line2")) <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">line1<text:line-break/>line2</text:span></text:p>";
    QTest::newRow("spaces") << "The quick brown fox jumped over the lazy dog" <<
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">The quick brown fox jumped over the lazy dog</text:span></text:p>";
}

void tst_QTextOdfWriter::testWriteParagraph()
{
    QFETCH(QString, input);
    QFETCH(QString, xml);

    QTextCursor cursor(document);
    cursor.insertText(input);

    odfWriter->writeBlock(*xmlWriter, document->begin());
    QCOMPARE( getContentFromXml(), xml);
}

void tst_QTextOdfWriter::testWriteStyle1_data()
{
    QTest::addColumn<QString>("htmlInput");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<QString>("xml");

    QString text1 = "Normal<b>bold</b><i>italic</i><b><i>Bold/Italic</i></b>";
    QTest::newRow("normal") << text1 << 2 <<
        "<style:style style:name=\"c4\" style:family=\"text\"><style:text-properties fo:font-family=\"Sans\"/></style:style>";
    QTest::newRow("bold") << text1 << 10 <<
        "<style:style style:name=\"c4\" style:family=\"text\"><style:text-properties fo:font-weight=\"bold\" fo:font-family=\"Sans\"/></style:style>";
    QTest::newRow("italic") << text1 << 14 <<
        "<style:style style:name=\"c4\" style:family=\"text\"><style:text-properties fo:font-style=\"italic\" fo:font-family=\"Sans\"/></style:style>";
    QTest::newRow("bold+italic") << text1 << 25 <<
        "<style:style style:name=\"c4\" style:family=\"text\"><style:text-properties fo:font-style=\"italic\" fo:font-weight=\"bold\" fo:font-family=\"Sans\"/></style:style>";
    QString colorText = "<span style=\"color: #00FF00; background-color: #FF0000;\"> Color Text </span>";
    QTest::newRow("green/red") << colorText  << 3 <<
        "<style:style style:name=\"c4\" style:family=\"text\"><style:text-properties fo:font-family=\"Sans\" fo:color=\"#00ff00\" fo:background-color=\"#ff0000\"/></style:style>";

}

void tst_QTextOdfWriter::testWriteStyle1()
{
    QFETCH(QString, htmlInput);
    QFETCH(int, cursorPosition);
    QFETCH(QString, xml);
    document->setHtml(htmlInput);

    QTextCursor cursor(document);
    cursor.setPosition(cursorPosition);
    odfWriter->writeCharacterFormat(*xmlWriter, cursor.charFormat(), 4);
    QCOMPARE( getContentFromXml(), xml);
}

void tst_QTextOdfWriter::testWriteStyle2()
{
    QTextBlockFormat bf; // = cursor.blockFormat();
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab1(40, QTextOption::RightTab);
    tabs << tab1;
    QTextOption::Tab tab2(80, QTextOption::DelimiterTab, 'o');
    tabs << tab2;
    bf.setTabPositions(tabs);

    odfWriter->writeBlockFormat(*xmlWriter, bf, 1);
    QString xml = QString::fromLatin1(
        "<style:style style:name=\"p1\" style:family=\"paragraph\">"
            "<style:paragraph-properties>"
                "<style:tab-stops>"
                    "<style:tab-stop style:position=\"30pt\" style:type=\"right\"/>"
                    "<style:tab-stop style:position=\"60pt\" style:type=\"char\" style:char=\"o\"/>"
                "</style:tab-stops>"
            "</style:paragraph-properties>"
        "</style:style>");
    QCOMPARE(getContentFromXml(), xml);
}

void tst_QTextOdfWriter::testWriteList()
{
    QTextCursor cursor(document);
    QTextList *list = cursor.createList(QTextListFormat::ListDisc);
    cursor.insertText("ListItem 1");
    list->add(cursor.block());
    cursor.insertBlock();
    cursor.insertText("ListItem 2");
    list->add(cursor.block());

    odfWriter->writeBlock(*xmlWriter, cursor.block());
    QString xml = QString::fromLatin1(
        "<text:list text:style-name=\"L2\">"
          "<text:list-item>"
        //"<text:numbered-paragraph text:style-name=\"L2\" text:level=\"1\">"
            //"<text:number>")+ QChar(0x25cf) + QString::fromLatin1("</text:number>" // 0x25cf is a bullet
            "<text:p text:style-name=\"p3\"><text:span text:style-name=\"c0\">ListItem 2</text:span></text:p>"
          "</text:list-item>"
        "</text:list>");

    QCOMPARE(getContentFromXml(), xml);
}

void tst_QTextOdfWriter::testWriteList2()
{
    QTextCursor cursor(document);
    QTextList *list = cursor.createList(QTextListFormat::ListDisc);
    cursor.insertText("Cars");
    list->add(cursor.block());
    cursor.insertBlock();
    QTextListFormat level2;
    level2.setStyle(QTextListFormat::ListSquare);
    level2.setIndent(2);
    QTextList *list2 = cursor.createList(level2);
    cursor.insertText("Model T");
    list2->add(cursor.block());
    cursor.insertBlock();
    cursor.insertText("Kitt");
    list2->add(cursor.block());
    cursor.insertBlock();
    cursor.insertText("Animals");
    list->add(cursor.block());

    cursor.insertBlock(QTextBlockFormat(), QTextCharFormat()); // start a new completely unrelated list.
    QTextList *list3 = cursor.createList(QTextListFormat::ListDecimal);
    cursor.insertText("Foo");
    list3->add(cursor.block());

    // and another block thats NOT in a list.
    cursor.insertBlock(QTextBlockFormat(), QTextCharFormat());
    cursor.insertText("Bar");

    odfWriter->writeFrame(*xmlWriter, document->rootFrame());
    QString xml = QString::fromLatin1(
        "<text:list text:style-name=\"L2\">"
          "<text:list-item>"
        //"<text:numbered-paragraph text:style-name=\"L2\" text:level=\"1\">"
            //"<text:number>")+ QChar(0x25cf) + QString::fromLatin1("</text:number>" // 0x25cf is a bullet
            "<text:p text:style-name=\"p3\"><text:span text:style-name=\"c0\">Cars</text:span></text:p>"
          "</text:list-item>"
          "<text:list-item>"
            "<text:list text:style-name=\"L4\">"
              "<text:list-item>"
                "<text:p text:style-name=\"p5\"><text:span text:style-name=\"c0\">Model T</text:span></text:p>"
              "</text:list-item>"
              "<text:list-item>"
                "<text:p text:style-name=\"p5\"><text:span text:style-name=\"c0\">Kitt</text:span></text:p>"
              "</text:list-item>"
            "</text:list>"
          "</text:list-item>"
          "<text:list-item>"
            "<text:p text:style-name=\"p3\"><text:span text:style-name=\"c0\">Animals</text:span></text:p>"
          "</text:list-item>"
        "</text:list>"
        "<text:list text:style-name=\"L6\">"
          "<text:list-item>"
            "<text:p text:style-name=\"p7\"><text:span text:style-name=\"c0\">Foo</text:span></text:p>"
          "</text:list-item>"
        "</text:list>"
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">Bar</text:span></text:p>");

    // QString x = getContentFromXml();
    // for (int i=0; i < x.length(); i+=150) qDebug() << x.mid(i, 150);
    QCOMPARE(getContentFromXml(), xml);
}


void tst_QTextOdfWriter::createArchive()
{
    document->setPlainText("a"); // simple doc is enough ;)
    QTextOdfWriter writer(*document, buffer);
    QCOMPARE(writer.createArchive(), true); // default
    writer.writeAll();
/*
QFile file("createArchive-odt");
file.open(QIODevice::WriteOnly);
file.write(buffer->data());
file.close();
*/
    QVERIFY(buffer->data().length() > 80);
    QCOMPARE(buffer->data()[0], 'P'); // its a zip :)
    QCOMPARE(buffer->data()[1], 'K');
    QString mimetype(buffer->data().mid(38, 39));
    QCOMPARE(mimetype, QString::fromLatin1("application/vnd.oasis.opendocument.text"));
}

void tst_QTextOdfWriter::testWriteAll()
{
    document->setPlainText("a"); // simple doc is enough ;)
    QTextOdfWriter writer(*document, buffer);
    QCOMPARE(writer.createArchive(), true);
    writer.setCreateArchive(false);
    writer.writeAll();
    QString result = QString(buffer->data());
    // details we check elsewhere, all we have to do is check availability.
    QVERIFY(result.indexOf("office:automatic-styles") >= 0);
    QVERIFY(result.indexOf("<style:style style:name=\"p1\"") >= 0);
    QVERIFY(result.indexOf("<style:style style:name=\"c0\"") >= 0);
    QVERIFY(result.indexOf("office:body") >= 0);
    QVERIFY(result.indexOf("office:text") >= 0);
    QVERIFY(result.indexOf("style:style") >= 0);
}

void tst_QTextOdfWriter::testWriteSection()
{
    QTextCursor cursor(document);
    cursor.insertText("foo\nBar");
    QTextFrameFormat ff;
    cursor.insertFrame(ff);
    cursor.insertText("baz");

    odfWriter->writeFrame(*xmlWriter, document->rootFrame());
    QString xml = QString::fromLatin1(
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">foo</text:span></text:p>"
        "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">Bar</text:span></text:p>"
        "<text:section>"
            "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">baz</text:span></text:p>"
        "</text:section>"
        "<text:p text:style-name=\"p1\"/>");

    QCOMPARE(getContentFromXml(), xml);
}

void tst_QTextOdfWriter::testWriteTable()
{
    // create table with merged cells
    QTextCursor cursor(document);
    QTextTable * table = cursor.insertTable(3, 3);
    table->mergeCells(1, 0, 2, 2);
    table->mergeCells(0, 1, 1, 2);
    cursor = table->cellAt(0, 0).firstCursorPosition();
    cursor.insertText("a");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("b");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("c");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("d");
    cursor.movePosition(QTextCursor::NextCell);
    cursor.insertText("e");
    /*
      +-+---+
      |a|b  |
      +-+-+-+
      |c  |d|
      +   +-+
      |   |e|
      +-+-+-+
    */

    odfWriter->writeFrame(*xmlWriter, document->rootFrame());
    QString xml = QString::fromLatin1(
        "<text:p text:style-name=\"p1\"/>"
        "<table:table>"
            "<table:table-column table:number-columns-repeated=\"3\"/>"
            "<table:table-row>"
                "<table:table-cell table:style-name=\"T3\">"
                    "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">a</text:span></text:p>"
                "</table:table-cell>"
                "<table:table-cell table:number-columns-spanned=\"2\" table:style-name=\"T6\">"
                    "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c7\">b</text:span></text:p>"
                "</table:table-cell>"
            "</table:table-row>"
            "<table:table-row>"
                "<table:table-cell table:number-columns-spanned=\"2\" table:number-rows-spanned=\"2\" table:style-name=\"T5\">"
                    "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c8\">c</text:span></text:p>"
                "</table:table-cell>"
                "<table:table-cell table:style-name=\"T3\">"
                    "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">d</text:span></text:p>"
                "</table:table-cell>"
            "</table:table-row>"
            "<table:table-row>"
                "<table:table-cell table:style-name=\"T3\">"
                    "<text:p text:style-name=\"p1\"><text:span text:style-name=\"c0\">e</text:span></text:p>"
                "</table:table-cell>"
            "</table:table-row>"
        "</table:table>"
        "<text:p text:style-name=\"p1\"/>");

    QCOMPARE(getContentFromXml(), xml);
}

void tst_QTextOdfWriter::testWriteFrameFormat()
{
    QTextFrameFormat tff;
    tff.setTopMargin(20);
    tff.setBottomMargin(20);
    tff.setLeftMargin(20);
    tff.setRightMargin(20);
    QTextCursor tc(document);
    odfWriter->writeFrameFormat(*xmlWriter, tff, 0);
    // Value of 15pt is based on the pixelToPoint() calculation done in qtextodfwriter.cpp
    QString xml = QString::fromLatin1(
            "<style:style style:name=\"s0\" style:family=\"section\">"
            "<style:section-properties fo:margin-top=\"15pt\" fo:margin-bottom=\"15pt\""
            " fo:margin-left=\"15pt\" fo:margin-right=\"15pt\"/>"
            "</style:style>");
    QCOMPARE(getContentFromXml(), xml);
}

QTEST_MAIN(tst_QTextOdfWriter)
#include "tst_qtextodfwriter.moc"
