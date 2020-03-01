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

#include <QDebug>
#include <QTextDocument>
#include <QTextDocumentWriter>
#include <QTextLayout>
#include <QTextCursor>
#include <qmath.h>
#include <QFile>
#include <QPainter>
#include <QBuffer>
#include <qtest.h>

Q_DECLARE_METATYPE(QVector<QTextLayout::FormatRange>)

class tst_QText: public QObject
{
    Q_OBJECT
public:
    tst_QText() {
        m_lorem = QString::fromLatin1("Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi.");
        m_shortLorem = QString::fromLatin1("Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
    }

private slots:
    void loadHtml_data();
    void loadHtml();

    void shaping_data();
    void shaping();

    void odfWriting_empty();
    void odfWriting_text();
    void odfWriting_images();

    void constructDocument();

    void newLineReplacement();
    void formatManipulation();
    void fontResolution();

    void layout_data();
    void layout();
    void formattedLayout_data();
    void formattedLayout();
    void paintLayoutToPixmap();
    void paintLayoutToPixmap_painterFill();

    void document();
    void paintDocToPixmap();
    void paintDocToPixmap_painterFill();

private:
    QSize setupTextLayout(QTextLayout *layout, bool wrap = true, int wrapWidth = 100);

    QString m_lorem;
    QString m_shortLorem;
};

void tst_QText::loadHtml_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("empty") << QString();
    QTest::newRow("simple") << QString::fromLatin1("<html><b>Foo</b></html>");
    QTest::newRow("simple2") << QString::fromLatin1("<b>Foo</b>");

    QString parag = QString::fromLatin1("<p>%1</p>").arg(m_lorem);
    QString header = QString::fromLatin1("<html><head><title>test</title></head><body>");
    QTest::newRow("long") << QString::fromLatin1("<html><head><title>test</title></head><body>") + parag + parag + parag
        + parag + parag + parag + parag + parag + parag + parag + parag + parag + parag + parag + parag + parag + parag
        + QString::fromLatin1("</html>");
    QTest::newRow("table") <<  header + QLatin1String("<table border=\"1\"1><tr><td>xx</td></tr><tr><td colspan=\"2\">")
        + parag + QLatin1String("</td></tr></table></html");
    QTest::newRow("crappy") <<  header + QLatin1String("<table border=\"1\"1><tr><td>xx</td></tr><tr><td colspan=\"2\">")
        + parag;
}

void tst_QText::loadHtml()
{
    QFETCH(QString, source);
    QTextDocument doc;
    QBENCHMARK {
        doc.setHtml(source);
    }
}

void tst_QText::shaping_data()
{
    QTest::addColumn<QString>("parag");
    QTest::newRow("empty") << QString();
    QTest::newRow("lorem") << m_lorem;
    QTest::newRow("short") << QString::fromLatin1("Lorem ipsum dolor sit amet");

    QString testFile = QFINDTESTDATA("bidi.txt");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file bidi.txt!");
    QFile file(testFile);
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(data.count() > 1000);
    QStringList list = QString::fromUtf8(data.data()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QVERIFY(list.count() %2 == 0); // even amount as we have title and then content.
    for (int i=0; i < list.count(); i+=2) {
        QTest::newRow(list.at(i).toLatin1()) << list.at(i+1);
    }
}

void tst_QText::shaping()
{
    QFETCH(QString, parag);

    QTextLayout lay(parag);
    lay.setCacheEnabled(false);

    // do one run to make sure any fonts are loaded.
    lay.beginLayout();
    lay.createLine();
    lay.endLayout();

    QBENCHMARK {
        lay.beginLayout();
        lay.createLine();
        lay.endLayout();
    }
}

void tst_QText::odfWriting_empty()
{
    QVERIFY(QTextDocumentWriter::supportedDocumentFormats().contains("ODF")); // odf compiled in
    QTextDocument *doc = new QTextDocument();
    // write it
    QBENCHMARK {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QTextDocumentWriter writer(&buffer, "ODF");
        writer.write(doc);
    }
    delete doc;
}

void tst_QText::odfWriting_text()
{
    QTextDocument *doc = new QTextDocument();
    QTextCursor cursor(doc);
    QTextBlockFormat bf;
    bf.setIndent(2);
    cursor.insertBlock(bf);
    cursor.insertText(m_lorem);
    bf.setTopMargin(10);
    cursor.insertBlock(bf);
    cursor.insertText(m_lorem);
    bf.setRightMargin(30);
    cursor.insertBlock(bf);
    cursor.insertText(m_lorem);

    // write it
    QBENCHMARK {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QTextDocumentWriter writer(&buffer, "ODF");
        writer.write(doc);
    }
    delete doc;
}

void tst_QText::odfWriting_images()
{
    QTextDocument *doc = new QTextDocument();
    QTextCursor cursor(doc);
    cursor.insertText(m_lorem);
    QImage image(400, 200, QImage::Format_ARGB32_Premultiplied);
    cursor.insertImage(image);
    cursor.insertText(m_lorem);

    // write it
    QBENCHMARK {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QTextDocumentWriter writer(&buffer, "ODF");
        writer.write(doc);
    }
    delete doc;
}

QSize tst_QText::setupTextLayout(QTextLayout *layout, bool wrap, int wrapWidth)
{
    layout->setCacheEnabled(true);

    int height = 0;
    qreal widthUsed = 0;
    qreal lineWidth = 0;

    //set manual width
    if (wrap)
        lineWidth = wrapWidth;

    layout->beginLayout();
    while (1) {
        QTextLine line = layout->createLine();
        if (!line.isValid())
            break;

        if (wrap)
            line.setLineWidth(lineWidth);
    }
    layout->endLayout();

    for (int i = 0; i < layout->lineCount(); ++i) {
        QTextLine line = layout->lineAt(i);
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
        line.setPosition(QPointF(0, height));
        height += int(line.height());
    }
    return QSize(qCeil(widthUsed), height);
}

void tst_QText::constructDocument()
{
    QTextDocument *doc = new QTextDocument;
    delete doc;

    QBENCHMARK {
        QTextDocument *doc = new QTextDocument;
        delete doc;
    }
}

//this step is needed before giving the string to a QTextLayout
void tst_QText::newLineReplacement()
{
    QString text = QString::fromLatin1("H\ne\nl\nl\no\n\nW\no\nr\nl\nd");

    QBENCHMARK {
        QString tmp = text;
        tmp.replace(QLatin1Char('\n'), QChar::LineSeparator);
    }
}

void tst_QText::formatManipulation()
{
    QFont font;

    QBENCHMARK {
        QTextCharFormat format;
        format.setFont(font);
    }
}

void tst_QText::fontResolution()
{
    QFont font;
    QFont font2;
    font.setFamily("DejaVu");
    font2.setBold(true);

    QBENCHMARK {
        QFont res = font.resolve(font2);
    }
}

void tst_QText::layout_data()
{
    QTest::addColumn<bool>("wrap");
    QTest::newRow("wrap") << true;
    QTest::newRow("nowrap") << false;
}

void tst_QText::layout()
{
    QFETCH(bool,wrap);
    QTextLayout layout(m_shortLorem);
    setupTextLayout(&layout, wrap);

    QBENCHMARK {
        QTextLayout layout(m_shortLorem);
        setupTextLayout(&layout, wrap);
    }
}

//### requires tst_QText to be a friend of QTextLayout
/*void tst_QText::stackTextLayout()
{
    QStackTextEngine engine(m_shortLorem, qApp->font());
    QTextLayout layout(&engine);
    setupTextLayout(&layout);

    QBENCHMARK {
        QStackTextEngine engine(m_shortLorem, qApp->font());
        QTextLayout layout(&engine);
        setupTextLayout(&layout);
    }
}*/

void tst_QText::formattedLayout_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QVector<QTextLayout::FormatRange> >("ranges");

    QTextCharFormat format;
    format.setForeground(QColor("steelblue"));

    {
        QVector<QTextLayout::FormatRange> ranges;

        QTextLayout::FormatRange formatRange;
        formatRange.format = format;
        formatRange.start = 0;
        formatRange.length = 50;
        ranges.append(formatRange);

        QTest::newRow("short-single") << m_shortLorem << ranges;
    }
    {
        QVector<QTextLayout::FormatRange> ranges;

        QString text = m_lorem.repeated(100);
        const int width = 1;
        for (int i = 0; i < text.size(); i += width) {
            QTextLayout::FormatRange formatRange;
            formatRange.format.setForeground(QBrush(QColor(i % 255, 255, 255)));
            formatRange.start = i;
            formatRange.length = width;
            ranges.append(formatRange);
        }

        QTest::newRow("long-many") << m_shortLorem << ranges;
    }
}

void tst_QText::formattedLayout()
{
    QFETCH(QString, text);
    QFETCH(QVector<QTextLayout::FormatRange>, ranges);

    QTextLayout layout(text);
    layout.setFormats(ranges);
    setupTextLayout(&layout);

    QBENCHMARK {
        QTextLayout layout(text);
        layout.setFormats(ranges);
        setupTextLayout(&layout);
    }
}

void tst_QText::paintLayoutToPixmap()
{
    QTextLayout layout(m_shortLorem);
    QSize size = setupTextLayout(&layout);

    QBENCHMARK {
        QPixmap img(size);
        img.fill(Qt::transparent);
        QPainter p(&img);
        layout.draw(&p, QPointF(0, 0));
    }
}

void tst_QText::paintLayoutToPixmap_painterFill()
{
    QTextLayout layout(m_shortLorem);
    QSize size = setupTextLayout(&layout);

    QBENCHMARK {
        QPixmap img(size);
        QPainter p(&img);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(0, 0, img.width(), img.height(), Qt::transparent);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        layout.draw(&p, QPointF(0, 0));
    }
}

void tst_QText::document()
{
    QTextDocument *doc = new QTextDocument;
    Q_UNUSED(doc)

    QBENCHMARK {
        QTextDocument *doc = new QTextDocument;
        doc->setHtml(m_shortLorem);
    }
}

void tst_QText::paintDocToPixmap()
{
    QTextDocument *doc = new QTextDocument;
    doc->setHtml(m_shortLorem);
    doc->setTextWidth(300);
    QSize size = doc->size().toSize();

    QBENCHMARK {
        QPixmap img(size);
        img.fill(Qt::transparent);
        QPainter p(&img);
        doc->drawContents(&p);
    }
}

void tst_QText::paintDocToPixmap_painterFill()
{
    QTextDocument *doc = new QTextDocument;
    doc->setHtml(m_shortLorem);
    doc->setTextWidth(300);
    QSize size = doc->size().toSize();

    QBENCHMARK {
        QPixmap img(size);
        QPainter p(&img);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(0, 0, img.width(), img.height(), Qt::transparent);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        doc->drawContents(&p);
    }
}

QTEST_MAIN(tst_QText)

#include "main.moc"
