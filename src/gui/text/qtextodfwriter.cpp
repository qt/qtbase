/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qglobal.h>

#ifndef QT_NO_TEXTODFWRITER

#include "qtextodfwriter_p.h"

#include <QImageReader>
#include <QImageWriter>
#include <QTextListFormat>
#include <QTextList>
#include <QBuffer>
#include <QUrl>

#include "qtextdocument_p.h"
#include "qtexttable.h"
#include "qtextcursor.h"
#include "qtextimagehandler_p.h"
#include "qzipwriter_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

/// Convert pixels to postscript point units
static QString pixelToPoint(qreal pixels)
{
    // we hardcode 96 DPI, we do the same in the ODF importer to have a perfect roundtrip.
    return QString::number(pixels * 72 / 96) + QLatin1String("pt");
}

// strategies
class QOutputStrategy {
public:
    QOutputStrategy() : contentStream(nullptr), counter(1) { }
    virtual ~QOutputStrategy() {}
    virtual void addFile(const QString &fileName, const QString &mimeType, const QByteArray &bytes) = 0;

    QString createUniqueImageName()
    {
        return QString::fromLatin1("Pictures/Picture%1").arg(counter++);
    }

    QIODevice *contentStream;
    int counter;
};

class QXmlStreamStrategy : public QOutputStrategy {
public:
    QXmlStreamStrategy(QIODevice *device)
    {
        contentStream = device;
    }

    ~QXmlStreamStrategy()
    {
        if (contentStream)
            contentStream->close();
    }
    virtual void addFile(const QString &, const QString &, const QByteArray &) override
    {
        // we ignore this...
    }
};

class QZipStreamStrategy : public QOutputStrategy {
public:
    QZipStreamStrategy(QIODevice *device)
        : zip(device),
        manifestWriter(&manifest)
    {
        QByteArray mime("application/vnd.oasis.opendocument.text");
        zip.setCompressionPolicy(QZipWriter::NeverCompress);
        zip.addFile(QString::fromLatin1("mimetype"), mime); // for mime-magick
        zip.setCompressionPolicy(QZipWriter::AutoCompress);
        contentStream = &content;
        content.open(QIODevice::WriteOnly);
        manifest.open(QIODevice::WriteOnly);

        manifestNS = QString::fromLatin1("urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
        // prettyfy
        manifestWriter.setAutoFormatting(true);
        manifestWriter.setAutoFormattingIndent(1);

        manifestWriter.writeNamespace(manifestNS, QString::fromLatin1("manifest"));
        manifestWriter.writeStartDocument();
        manifestWriter.writeStartElement(manifestNS, QString::fromLatin1("manifest"));
        manifestWriter.writeAttribute(manifestNS, QString::fromLatin1("version"), QString::fromLatin1("1.2"));
        addFile(QString::fromLatin1("/"), QString::fromLatin1("application/vnd.oasis.opendocument.text"));
        addFile(QString::fromLatin1("content.xml"), QString::fromLatin1("text/xml"));
    }

    ~QZipStreamStrategy()
    {
        manifestWriter.writeEndDocument();
        manifest.close();
        zip.addFile(QString::fromLatin1("META-INF/manifest.xml"), &manifest);
        content.close();
        zip.addFile(QString::fromLatin1("content.xml"), &content);
        zip.close();
    }

    virtual void addFile(const QString &fileName, const QString &mimeType, const QByteArray &bytes) override
    {
        zip.addFile(fileName, bytes);
        addFile(fileName, mimeType);
    }

private:
    void addFile(const QString &fileName, const QString &mimeType)
    {
        manifestWriter.writeEmptyElement(manifestNS, QString::fromLatin1("file-entry"));
        manifestWriter.writeAttribute(manifestNS, QString::fromLatin1("media-type"), mimeType);
        manifestWriter.writeAttribute(manifestNS, QString::fromLatin1("full-path"), fileName);
    }

    QBuffer content;
    QBuffer manifest;
    QZipWriter zip;
    QXmlStreamWriter manifestWriter;
    QString manifestNS;
};

static QString bulletChar(QTextListFormat::Style style)
{
    switch(style) {
    case QTextListFormat::ListDisc:
        return QChar(0x25cf); // bullet character
    case QTextListFormat::ListCircle:
        return QChar(0x25cb); // white circle
    case QTextListFormat::ListSquare:
        return QChar(0x25a1); // white square
    case QTextListFormat::ListDecimal:
        return QString::fromLatin1("1");
    case QTextListFormat::ListLowerAlpha:
        return QString::fromLatin1("a");
    case QTextListFormat::ListUpperAlpha:
        return QString::fromLatin1("A");
    case QTextListFormat::ListLowerRoman:
        return QString::fromLatin1("i");
    case QTextListFormat::ListUpperRoman:
        return QString::fromLatin1("I");
    default:
    case QTextListFormat::ListStyleUndefined:
        return QString();
    }
}

static QString borderStyleName(QTextFrameFormat::BorderStyle style)
{
    switch (style) {
    case QTextFrameFormat::BorderStyle_None:
        return QString::fromLatin1("none");
    case QTextFrameFormat::BorderStyle_Dotted:
        return QString::fromLatin1("dotted");
    case QTextFrameFormat::BorderStyle_Dashed:
        return QString::fromLatin1("dashed");
    case QTextFrameFormat::BorderStyle_Solid:
        return QString::fromLatin1("solid");
    case QTextFrameFormat::BorderStyle_Double:
        return QString::fromLatin1("double");
    case QTextFrameFormat::BorderStyle_DotDash:
        return QString::fromLatin1("dashed");
    case QTextFrameFormat::BorderStyle_DotDotDash:
        return QString::fromLatin1("dotted");
    case QTextFrameFormat::BorderStyle_Groove:
        return QString::fromLatin1("groove");
    case QTextFrameFormat::BorderStyle_Ridge:
        return QString::fromLatin1("ridge");
    case QTextFrameFormat::BorderStyle_Inset:
        return QString::fromLatin1("inset");
    case QTextFrameFormat::BorderStyle_Outset:
        return QString::fromLatin1("outset");
    }
    return QString::fromLatin1("");
}

void QTextOdfWriter::writeFrame(QXmlStreamWriter &writer, const QTextFrame *frame)
{
    Q_ASSERT(frame);
    const QTextTable *table = qobject_cast<const QTextTable*> (frame);

    if (table) { // Start a table.
        writer.writeStartElement(tableNS, QString::fromLatin1("table"));
        writer.writeAttribute(tableNS, QString::fromLatin1("style-name"),
                              QString::fromLatin1("Table%1").arg(table->formatIndex()));
        // check if column widths are set, if so add TableNS line above for all columns and link to style
        if (m_tableFormatsWithColWidthConstraints.contains(table->formatIndex())) {
            for (int colit = 0; colit < table->columns(); ++colit) {
                writer.writeStartElement(tableNS, QString::fromLatin1("table-column"));
                writer.writeAttribute(tableNS, QString::fromLatin1("style-name"),
                                      QString::fromLatin1("Table%1.%2").arg(table->formatIndex()).arg(colit));
                writer.writeEndElement();
            }
        } else {
            writer.writeEmptyElement(tableNS, QString::fromLatin1("table-column"));
            writer.writeAttribute(tableNS, QString::fromLatin1("number-columns-repeated"),
                                  QString::number(table->columns()));
        }
    } else if (frame->document() && frame->document()->rootFrame() != frame) { // start a section
        writer.writeStartElement(textNS, QString::fromLatin1("section"));
    }

    QTextFrame::iterator iterator = frame->begin();
    QTextFrame *child = nullptr;

    int tableRow = -1;
    while (! iterator.atEnd()) {
        if (iterator.currentFrame() && child != iterator.currentFrame())
            writeFrame(writer, iterator.currentFrame());
        else { // no frame, its a block
            QTextBlock block = iterator.currentBlock();
            if (table) {
                QTextTableCell cell = table->cellAt(block.position());
                if (tableRow < cell.row()) {
                    if (tableRow >= 0)
                        writer.writeEndElement(); // close table row
                    tableRow = cell.row();
                    writer.writeStartElement(tableNS, QString::fromLatin1("table-row"));
                }
                writer.writeStartElement(tableNS, QString::fromLatin1("table-cell"));
                if (cell.columnSpan() > 1)
                    writer.writeAttribute(tableNS, QString::fromLatin1("number-columns-spanned"), QString::number(cell.columnSpan()));
                if (cell.rowSpan() > 1)
                    writer.writeAttribute(tableNS, QString::fromLatin1("number-rows-spanned"), QString::number(cell.rowSpan()));
                if (cell.format().isTableCellFormat()) {
                    if (m_cellFormatsInTablesWithBorders.contains(cell.tableCellFormatIndex()) ) {
                        // writing table:style-name tag in <table:table-cell> element
                        writer.writeAttribute(tableNS, QString::fromLatin1("style-name"),
                                              QString::fromLatin1("TB%1.%2").arg(table->formatIndex())
                                              .arg(cell.tableCellFormatIndex()));
                    } else {
                        writer.writeAttribute(tableNS, QString::fromLatin1("style-name"),
                                              QString::fromLatin1("T%1").arg(cell.tableCellFormatIndex()));
                    }
                }
            }
            writeBlock(writer, block);
            if (table)
                writer.writeEndElement(); // table-cell
        }
        child = iterator.currentFrame();
        ++iterator;
    }
    if (tableRow >= 0)
        writer.writeEndElement(); // close table-row

    if (table || (frame->document() && frame->document()->rootFrame() != frame))
        writer.writeEndElement();  // close table or section element
}

void QTextOdfWriter::writeBlock(QXmlStreamWriter &writer, const QTextBlock &block)
{
    if (block.textList()) { // its a list-item
        const int listLevel = block.textList()->format().indent();
        if (m_listStack.isEmpty() || m_listStack.top() != block.textList()) {
            // not the same list we were in.
            while (m_listStack.count() >= listLevel && !m_listStack.isEmpty() && m_listStack.top() != block.textList() ) { // we need to close tags
                m_listStack.pop();
                writer.writeEndElement(); // list
                if (m_listStack.count())
                    writer.writeEndElement(); // list-item
            }
            while (m_listStack.count() < listLevel) {
                if (m_listStack.count())
                    writer.writeStartElement(textNS, QString::fromLatin1("list-item"));
                writer.writeStartElement(textNS, QString::fromLatin1("list"));
                if (m_listStack.count() == listLevel - 1) {
                    m_listStack.push(block.textList());
                    writer.writeAttribute(textNS, QString::fromLatin1("style-name"), QString::fromLatin1("L%1")
                            .arg(block.textList()->formatIndex()));
                }
                else {
                    m_listStack.push(0);
                }
            }
        }
        writer.writeStartElement(textNS, QString::fromLatin1("list-item"));
    }
    else {
        while (! m_listStack.isEmpty()) {
            m_listStack.pop();
            writer.writeEndElement(); // list
            if (m_listStack.count())
                writer.writeEndElement(); // list-item
        }
    }

    if (block.length() == 1) { // only a linefeed
        writer.writeEmptyElement(textNS, QString::fromLatin1("p"));
        writer.writeAttribute(textNS, QString::fromLatin1("style-name"), QString::fromLatin1("p%1")
            .arg(block.blockFormatIndex()));
        if (block.textList())
            writer.writeEndElement(); // numbered-paragraph
        return;
    }
    writer.writeStartElement(textNS, QString::fromLatin1("p"));
    writer.writeAttribute(textNS, QString::fromLatin1("style-name"), QString::fromLatin1("p%1")
        .arg(block.blockFormatIndex()));
    for (QTextBlock::Iterator frag = block.begin(); !frag.atEnd(); ++frag) {
        bool isHyperlink = frag.fragment().charFormat().hasProperty(QTextFormat::AnchorHref);
        if (isHyperlink) {
            QString value = frag.fragment().charFormat().property(QTextFormat::AnchorHref).toString();
            writer.writeStartElement(textNS, QString::fromLatin1("a"));
            writer.writeAttribute(xlinkNS, QString::fromLatin1("href"), value);
        }
        writer.writeCharacters(QString()); // Trick to make sure that the span gets no linefeed in front of it.
        writer.writeStartElement(textNS, QString::fromLatin1("span"));

        QString fragmentText = frag.fragment().text();
        if (fragmentText.length() == 1 && fragmentText[0] == QChar(0xFFFC)) { // its an inline character.
            writeInlineCharacter(writer, frag.fragment());
            writer.writeEndElement(); // span
            continue;
        }

        writer.writeAttribute(textNS, QString::fromLatin1("style-name"), QString::fromLatin1("c%1")
            .arg(frag.fragment().charFormatIndex()));
        bool escapeNextSpace = true;
        int precedingSpaces = 0;
        int exportedIndex = 0;
        for (int i=0; i <= fragmentText.count(); ++i) {
            QChar character = (i == fragmentText.count() ? QChar() : fragmentText.at(i));
            bool isSpace = character.unicode() == ' ';

            // find more than one space. -> <text:s text:c="2" />
            if (!isSpace && escapeNextSpace && precedingSpaces > 1) {
                const bool startParag = exportedIndex == 0 && i == precedingSpaces;
                if (!startParag)
                    writer.writeCharacters(fragmentText.mid(exportedIndex, i - precedingSpaces + 1 - exportedIndex));
                writer.writeEmptyElement(textNS, QString::fromLatin1("s"));
                const int count = precedingSpaces - (startParag?0:1);
                if (count > 1)
                    writer.writeAttribute(textNS, QString::fromLatin1("c"), QString::number(count));
                precedingSpaces = 0;
                exportedIndex = i;
            }

            if (i < fragmentText.count()) {
                if (character.unicode() == 0x2028) { // soft-return
                    //if (exportedIndex < i)
                    writer.writeCharacters(fragmentText.mid(exportedIndex, i - exportedIndex));
                    // adding tab before line-break, so last line in justified paragraph
                    // will not stretch to the end
                    writer.writeEmptyElement(textNS, QString::fromLatin1("tab"));
                    writer.writeEmptyElement(textNS, QString::fromLatin1("line-break"));
                    exportedIndex = i+1;
                    continue;
                } else if (character.unicode() == '\t') { // Tab
                    //if (exportedIndex < i)
                        writer.writeCharacters(fragmentText.mid(exportedIndex, i - exportedIndex));
                    writer.writeEmptyElement(textNS, QString::fromLatin1("tab"));
                    exportedIndex = i+1;
                    precedingSpaces = 0;
                } else if (isSpace) {
                    ++precedingSpaces;
                    escapeNextSpace = true;
                } else if (!isSpace) {
                    precedingSpaces = 0;
                }
            }
        }

        writer.writeCharacters(fragmentText.mid(exportedIndex));
        writer.writeEndElement(); // span
        writer.writeCharacters(QString()); // Trick to make sure that the span gets no linefeed behind it.
        if (isHyperlink)
            writer.writeEndElement(); // a
    }
    writer.writeCharacters(QString()); // Trick to make sure that the span gets no linefeed behind it.
    writer.writeEndElement(); // p
    if (block.textList())
        writer.writeEndElement(); // list-item
}

static bool probeImageData(QIODevice *device, QImage *image, QString *mimeType, qreal *width, qreal *height)
{
    QImageReader reader(device);
    const QByteArray format = reader.format().toLower();
    if (format == "png") {
        *mimeType = QStringLiteral("image/png");
    } else if (format == "jpg") {
        *mimeType = QStringLiteral("image/jpg");
    } else if (format == "svg") {
        *mimeType = QStringLiteral("image/svg+xml");
    } else {
        *image = reader.read();
        return false;
    }

    const QSize size = reader.size();

    *width = size.width();
    *height = size.height();

    return true;
}

void QTextOdfWriter::writeInlineCharacter(QXmlStreamWriter &writer, const QTextFragment &fragment) const
{
    writer.writeStartElement(drawNS, QString::fromLatin1("frame"));
    if (m_strategy == nullptr) {
        // don't do anything.
    }
    else if (fragment.charFormat().isImageFormat()) {
        QTextImageFormat imageFormat = fragment.charFormat().toImageFormat();
        writer.writeAttribute(drawNS, QString::fromLatin1("name"), imageFormat.name());

        QByteArray data;
        QString mimeType;
        qreal width = 0;
        qreal height = 0;

        QImage image;
        QString name = imageFormat.name();
        if (name.startsWith(QLatin1String(":/"))) // auto-detect resources
            name.prepend(QLatin1String("qrc"));
        QUrl url = QUrl(name);
        const QVariant variant = m_document->resource(QTextDocument::ImageResource, url);
        if (variant.userType() == QMetaType::QImage) {
            image = qvariant_cast<QImage>(variant);
        } else if (variant.userType() == QMetaType::QByteArray) {
            data = variant.toByteArray();

            QBuffer buffer(&data);
            buffer.open(QIODevice::ReadOnly);
            probeImageData(&buffer, &image, &mimeType, &width, &height);
        } else {
            // try direct loading
            QFile file(imageFormat.name());
            if (file.open(QIODevice::ReadOnly) && !probeImageData(&file, &image, &mimeType, &width, &height)) {
                file.seek(0);
                data = file.readAll();
            }
        }

        if (! image.isNull()) {
            QBuffer imageBytes;

            int imgQuality = imageFormat.quality();
            if (imgQuality >= 100 || imgQuality < 0 || image.hasAlphaChannel()) {
                QImageWriter imageWriter(&imageBytes, "png");
                imageWriter.write(image);

                data = imageBytes.data();
                mimeType = QStringLiteral("image/png");
            } else {
                // Write images without alpha channel as jpg with quality set by QTextImageFormat
                QImageWriter imageWriter(&imageBytes, "jpg");
                imageWriter.setQuality(imgQuality);
                imageWriter.write(image);

                data = imageBytes.data();
                mimeType = QStringLiteral("image/jpg");
            }

            width = image.width();
            height = image.height();
        }

        if (!data.isEmpty()) {
            if (imageFormat.hasProperty(QTextFormat::ImageWidth)) {
                width = imageFormat.width();
            }
            if (imageFormat.hasProperty(QTextFormat::ImageHeight)) {
                height = imageFormat.height();
            }

            QString filename = m_strategy->createUniqueImageName();

            m_strategy->addFile(filename, mimeType, data);

            writer.writeAttribute(svgNS, QString::fromLatin1("width"), pixelToPoint(width));
            writer.writeAttribute(svgNS, QString::fromLatin1("height"), pixelToPoint(height));
            writer.writeAttribute(textNS, QStringLiteral("anchor-type"), QStringLiteral("as-char"));
            writer.writeStartElement(drawNS, QString::fromLatin1("image"));
            writer.writeAttribute(xlinkNS, QString::fromLatin1("href"), filename);
            writer.writeEndElement(); // image
        }
    }
    writer.writeEndElement(); // frame
}

void QTextOdfWriter::writeFormats(QXmlStreamWriter &writer, const QSet<int> &formats) const
{
    writer.writeStartElement(officeNS, QString::fromLatin1("automatic-styles"));
    QVector<QTextFormat> allStyles = m_document->allFormats();
    for (int formatIndex : formats) {
        QTextFormat textFormat = allStyles.at(formatIndex);
        switch (textFormat.type()) {
        case QTextFormat::CharFormat:
            if (textFormat.isTableCellFormat())
                writeTableCellFormat(writer, textFormat.toTableCellFormat(), formatIndex, allStyles);
            else
                writeCharacterFormat(writer, textFormat.toCharFormat(), formatIndex);
            break;
        case QTextFormat::BlockFormat:
            writeBlockFormat(writer, textFormat.toBlockFormat(), formatIndex);
            break;
        case QTextFormat::ListFormat:
            writeListFormat(writer, textFormat.toListFormat(), formatIndex);
            break;
        case QTextFormat::FrameFormat:
            if (textFormat.isTableFormat())
                writeTableFormat(writer, textFormat.toTableFormat(), formatIndex);
            else
                writeFrameFormat(writer, textFormat.toFrameFormat(), formatIndex);
            break;
#if QT_DEPRECATED_SINCE(5, 3)
        case QTextFormat::TableFormat:
            // this case never happens, because TableFormat is a FrameFormat
            Q_UNREACHABLE();
            break;
#endif
        }
    }

    writer.writeEndElement(); // automatic-styles
}

void QTextOdfWriter::writeBlockFormat(QXmlStreamWriter &writer, QTextBlockFormat format, int formatIndex) const
{
    writer.writeStartElement(styleNS, QString::fromLatin1("style"));
    writer.writeAttribute(styleNS, QString::fromLatin1("name"), QString::fromLatin1("p%1").arg(formatIndex));
    writer.writeAttribute(styleNS, QString::fromLatin1("family"), QString::fromLatin1("paragraph"));
    writer.writeStartElement(styleNS, QString::fromLatin1("paragraph-properties"));

    if (format.hasProperty(QTextBlockFormat::LineHeightType)) {
        const int blockLineHeightType = format.lineHeightType();
        const qreal blockLineHeight = format.lineHeight();
        QString type, value;
        switch (blockLineHeightType) {
        case QTextBlockFormat::SingleHeight:
            type = QString::fromLatin1("line-height");
            value = QString::fromLatin1("100%");
            break;
        case QTextBlockFormat::ProportionalHeight:
            type = QString::fromLatin1("line-height");
            value = QString::number(blockLineHeight) + QString::fromLatin1("%");
            break;
        case QTextBlockFormat::FixedHeight:
            type = QString::fromLatin1("line-height");
            value = pixelToPoint(qMax(qreal(0.), blockLineHeight));
            break;
        case QTextBlockFormat::MinimumHeight:
            type = QString::fromLatin1("line-height-at-least");
            value = pixelToPoint(qMax(qreal(0.), blockLineHeight));
            break;
        case QTextBlockFormat::LineDistanceHeight:
            type = QString::fromLatin1("line-spacing");
            value = pixelToPoint(qMax(qreal(0.), blockLineHeight));
        }

        if (!type.isNull())
            writer.writeAttribute(styleNS, type, value);
    }

    if (format.hasProperty(QTextFormat::BlockAlignment)) {
        const Qt::Alignment alignment = format.alignment() & Qt::AlignHorizontal_Mask;
        QString value;
        if (alignment == Qt::AlignLeading)
            value = QString::fromLatin1("start");
        else if (alignment == Qt::AlignTrailing)
            value = QString::fromLatin1("end");
        else if (alignment == (Qt::AlignLeft | Qt::AlignAbsolute))
            value = QString::fromLatin1("left");
        else if (alignment == (Qt::AlignRight | Qt::AlignAbsolute))
            value = QString::fromLatin1("right");
        else if (alignment == Qt::AlignHCenter)
            value = QString::fromLatin1("center");
        else if (alignment == Qt::AlignJustify)
            value = QString::fromLatin1("justify");
        else
            qWarning() << "QTextOdfWriter: unsupported paragraph alignment; " << format.alignment();
        if (! value.isNull())
            writer.writeAttribute(foNS, QString::fromLatin1("text-align"), value);
    }

    if (format.hasProperty(QTextFormat::BlockTopMargin))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-top"), pixelToPoint(qMax(qreal(0.), format.topMargin())) );
    if (format.hasProperty(QTextFormat::BlockBottomMargin))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-bottom"), pixelToPoint(qMax(qreal(0.), format.bottomMargin())) );
    if (format.hasProperty(QTextFormat::BlockLeftMargin) || format.hasProperty(QTextFormat::BlockIndent))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-left"), pixelToPoint(qMax(qreal(0.),
            format.leftMargin() + format.indent())));
    if (format.hasProperty(QTextFormat::BlockRightMargin))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-right"), pixelToPoint(qMax(qreal(0.), format.rightMargin())) );
    if (format.hasProperty(QTextFormat::TextIndent))
        writer.writeAttribute(foNS, QString::fromLatin1("text-indent"), pixelToPoint(format.textIndent()));
    if (format.hasProperty(QTextFormat::PageBreakPolicy)) {
        if (format.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysBefore)
            writer.writeAttribute(foNS, QString::fromLatin1("break-before"), QString::fromLatin1("page"));
        if (format.pageBreakPolicy() & QTextFormat::PageBreak_AlwaysAfter)
            writer.writeAttribute(foNS, QString::fromLatin1("break-after"), QString::fromLatin1("page"));
    }
    if (format.hasProperty(QTextFormat::BackgroundBrush)) {
        QBrush brush = format.background();
        writer.writeAttribute(foNS, QString::fromLatin1("background-color"), brush.color().name());
    }
    if (format.hasProperty(QTextFormat::BlockNonBreakableLines))
        writer.writeAttribute(foNS, QString::fromLatin1("keep-together"),
                format.nonBreakableLines() ? QString::fromLatin1("true") : QString::fromLatin1("false"));
    if (format.hasProperty(QTextFormat::TabPositions)) {
        QList<QTextOption::Tab> tabs = format.tabPositions();
        writer.writeStartElement(styleNS, QString::fromLatin1("tab-stops"));
        QList<QTextOption::Tab>::Iterator iterator = tabs.begin();
        while(iterator != tabs.end()) {
            writer.writeEmptyElement(styleNS, QString::fromLatin1("tab-stop"));
            writer.writeAttribute(styleNS, QString::fromLatin1("position"), pixelToPoint(iterator->position) );
            QString type;
            switch(iterator->type) {
            case QTextOption::DelimiterTab: type = QString::fromLatin1("char"); break;
            case QTextOption::LeftTab: type = QString::fromLatin1("left"); break;
            case QTextOption::RightTab: type = QString::fromLatin1("right"); break;
            case QTextOption::CenterTab: type = QString::fromLatin1("center"); break;
            }
            writer.writeAttribute(styleNS, QString::fromLatin1("type"), type);
            if (!iterator->delimiter.isNull())
                writer.writeAttribute(styleNS, QString::fromLatin1("char"), iterator->delimiter);
            ++iterator;
        }

        writer.writeEndElement(); // tab-stops
    }

    writer.writeEndElement(); // paragraph-properties
    writer.writeEndElement(); // style
}

void QTextOdfWriter::writeCharacterFormat(QXmlStreamWriter &writer, QTextCharFormat format, int formatIndex) const
{
    writer.writeStartElement(styleNS, QString::fromLatin1("style"));
    writer.writeAttribute(styleNS, QString::fromLatin1("name"), QString::fromLatin1("c%1").arg(formatIndex));
    writer.writeAttribute(styleNS, QString::fromLatin1("family"), QString::fromLatin1("text"));
    writer.writeEmptyElement(styleNS, QString::fromLatin1("text-properties"));
    if (format.fontItalic())
        writer.writeAttribute(foNS, QString::fromLatin1("font-style"), QString::fromLatin1("italic"));
    if (format.hasProperty(QTextFormat::FontWeight) && format.fontWeight() != QFont::Normal) {
        QString value;
        if (format.fontWeight() == QFont::Bold)
            value = QString::fromLatin1("bold");
        else
            value = QString::number(format.fontWeight() * 10);
        writer.writeAttribute(foNS, QString::fromLatin1("font-weight"), value);
    }
    if (format.hasProperty(QTextFormat::FontFamily))
        writer.writeAttribute(foNS, QString::fromLatin1("font-family"), format.fontFamily());
    else
        writer.writeAttribute(foNS, QString::fromLatin1("font-family"), QString::fromLatin1("Sans")); // Qt default
    if (format.hasProperty(QTextFormat::FontPointSize))
        writer.writeAttribute(foNS, QString::fromLatin1("font-size"), QString::fromLatin1("%1pt").arg(format.fontPointSize()));
    if (format.hasProperty(QTextFormat::FontCapitalization)) {
        switch(format.fontCapitalization()) {
        case QFont::MixedCase:
            writer.writeAttribute(foNS, QString::fromLatin1("text-transform"), QString::fromLatin1("none")); break;
        case QFont::AllUppercase:
            writer.writeAttribute(foNS, QString::fromLatin1("text-transform"), QString::fromLatin1("uppercase")); break;
        case QFont::AllLowercase:
            writer.writeAttribute(foNS, QString::fromLatin1("text-transform"), QString::fromLatin1("lowercase")); break;
        case QFont::Capitalize:
            writer.writeAttribute(foNS, QString::fromLatin1("text-transform"), QString::fromLatin1("capitalize")); break;
        case QFont::SmallCaps:
            writer.writeAttribute(foNS, QString::fromLatin1("font-variant"), QString::fromLatin1("small-caps")); break;
        }
    }
    if (format.hasProperty(QTextFormat::FontLetterSpacing))
        writer.writeAttribute(foNS, QString::fromLatin1("letter-spacing"), pixelToPoint(format.fontLetterSpacing()));
    if (format.hasProperty(QTextFormat::FontWordSpacing) && format.fontWordSpacing() != 0)
            writer.writeAttribute(foNS, QString::fromLatin1("word-spacing"), pixelToPoint(format.fontWordSpacing()));
    if (format.hasProperty(QTextFormat::FontUnderline))
        writer.writeAttribute(styleNS, QString::fromLatin1("text-underline-type"),
                format.fontUnderline() ? QString::fromLatin1("single") : QString::fromLatin1("none"));
    if (format.hasProperty(QTextFormat::FontOverline)) {
        //   bool   fontOverline () const  TODO
    }
    if (format.hasProperty(QTextFormat::FontStrikeOut))
        writer.writeAttribute(styleNS,QString::fromLatin1( "text-line-through-type"),
                format.fontStrikeOut() ? QString::fromLatin1("single") : QString::fromLatin1("none"));
    if (format.hasProperty(QTextFormat::TextUnderlineColor))
        writer.writeAttribute(styleNS, QString::fromLatin1("text-underline-color"), format.underlineColor().name());
    if (format.hasProperty(QTextFormat::FontFixedPitch)) {
        //   bool   fontFixedPitch () const  TODO
    }
    if (format.hasProperty(QTextFormat::TextUnderlineStyle)) {
        QString value;
        switch (format.underlineStyle()) {
        case QTextCharFormat::NoUnderline: value = QString::fromLatin1("none"); break;
        case QTextCharFormat::SingleUnderline: value = QString::fromLatin1("solid"); break;
        case QTextCharFormat::DashUnderline: value = QString::fromLatin1("dash"); break;
        case QTextCharFormat::DotLine: value = QString::fromLatin1("dotted"); break;
        case QTextCharFormat::DashDotLine: value = QString::fromLatin1("dash-dot"); break;
        case QTextCharFormat::DashDotDotLine: value = QString::fromLatin1("dot-dot-dash"); break;
        case QTextCharFormat::WaveUnderline: value = QString::fromLatin1("wave"); break;
        case QTextCharFormat::SpellCheckUnderline: value = QString::fromLatin1("none"); break;
        }
        writer.writeAttribute(styleNS, QString::fromLatin1("text-underline-style"), value);
    }
    if (format.hasProperty(QTextFormat::TextVerticalAlignment)) {
        QString value;
        switch (format.verticalAlignment()) {
        case QTextCharFormat::AlignMiddle:
        case QTextCharFormat::AlignNormal: value = QString::fromLatin1("0%"); break;
        case QTextCharFormat::AlignSuperScript: value = QString::fromLatin1("super"); break;
        case QTextCharFormat::AlignSubScript: value = QString::fromLatin1("sub"); break;
        case QTextCharFormat::AlignTop: value = QString::fromLatin1("100%"); break;
        case QTextCharFormat::AlignBottom : value = QString::fromLatin1("-100%"); break;
        case QTextCharFormat::AlignBaseline: break;
        }
        writer.writeAttribute(styleNS, QString::fromLatin1("text-position"), value);
    }
    if (format.hasProperty(QTextFormat::TextOutline))
        writer.writeAttribute(styleNS, QString::fromLatin1("text-outline"), QString::fromLatin1("true"));
    if (format.hasProperty(QTextFormat::TextToolTip)) {
        //   QString   toolTip () const  TODO
    }
    if (format.hasProperty(QTextFormat::IsAnchor)) {
        //   bool   isAnchor () const  TODO
    }
    if (format.hasProperty(QTextFormat::AnchorHref)) {
        //   QString   anchorHref () const  TODO
    }
    if (format.hasProperty(QTextFormat::AnchorName)) {
        //   QString   anchorName () const  TODO
    }
    if (format.hasProperty(QTextFormat::ForegroundBrush)) {
        QBrush brush = format.foreground();
        writer.writeAttribute(foNS, QString::fromLatin1("color"), brush.color().name());
    }
    if (format.hasProperty(QTextFormat::BackgroundBrush)) {
        QBrush brush = format.background();
        writer.writeAttribute(foNS, QString::fromLatin1("background-color"), brush.color().name());
    }

    writer.writeEndElement(); // style
}

void QTextOdfWriter::writeListFormat(QXmlStreamWriter &writer, QTextListFormat format, int formatIndex) const
{
    writer.writeStartElement(textNS, QString::fromLatin1("list-style"));
    writer.writeAttribute(styleNS, QString::fromLatin1("name"), QString::fromLatin1("L%1").arg(formatIndex));

    QTextListFormat::Style style = format.style();
    if (style == QTextListFormat::ListDecimal || style == QTextListFormat::ListLowerAlpha
            || style == QTextListFormat::ListUpperAlpha
            || style == QTextListFormat::ListLowerRoman
            || style == QTextListFormat::ListUpperRoman) {
        writer.writeStartElement(textNS, QString::fromLatin1("list-level-style-number"));
        writer.writeAttribute(styleNS, QString::fromLatin1("num-format"), bulletChar(style));

        if (format.hasProperty(QTextFormat::ListNumberSuffix))
            writer.writeAttribute(styleNS, QString::fromLatin1("num-suffix"), format.numberSuffix());
        else
            writer.writeAttribute(styleNS, QString::fromLatin1("num-suffix"), QString::fromLatin1("."));

        if (format.hasProperty(QTextFormat::ListNumberPrefix))
            writer.writeAttribute(styleNS, QString::fromLatin1("num-prefix"), format.numberPrefix());

    } else {
        writer.writeStartElement(textNS, QString::fromLatin1("list-level-style-bullet"));
        writer.writeAttribute(textNS, QString::fromLatin1("bullet-char"), bulletChar(style));
    }

    writer.writeAttribute(textNS, QString::fromLatin1("level"), QString::number(format.indent()));
    writer.writeEmptyElement(styleNS, QString::fromLatin1("list-level-properties"));
    writer.writeAttribute(foNS, QString::fromLatin1("text-align"), QString::fromLatin1("start"));
    QString spacing = QString::fromLatin1("%1mm").arg(format.indent() * 8);
    writer.writeAttribute(textNS, QString::fromLatin1("space-before"), spacing);
    //writer.writeAttribute(textNS, QString::fromLatin1("min-label-width"), spacing);

    writer.writeEndElement(); // list-level-style-*
    writer.writeEndElement(); // list-style
}

void QTextOdfWriter::writeFrameFormat(QXmlStreamWriter &writer, QTextFrameFormat format, int formatIndex) const
{
    writer.writeStartElement(styleNS, QString::fromLatin1("style"));
    writer.writeAttribute(styleNS, QString::fromLatin1("name"), QString::fromLatin1("s%1").arg(formatIndex));
    writer.writeAttribute(styleNS, QString::fromLatin1("family"), QString::fromLatin1("section"));
    writer.writeEmptyElement(styleNS, QString::fromLatin1("section-properties"));
    if (format.hasProperty(QTextFormat::FrameTopMargin))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-top"), pixelToPoint(qMax(qreal(0.), format.topMargin())) );
    if (format.hasProperty(QTextFormat::FrameBottomMargin))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-bottom"), pixelToPoint(qMax(qreal(0.), format.bottomMargin())) );
    if (format.hasProperty(QTextFormat::FrameLeftMargin))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-left"), pixelToPoint(qMax(qreal(0.), format.leftMargin())) );
    if (format.hasProperty(QTextFormat::FrameRightMargin))
        writer.writeAttribute(foNS, QString::fromLatin1("margin-right"), pixelToPoint(qMax(qreal(0.), format.rightMargin())) );

    writer.writeEndElement(); // style

// TODO consider putting the following properties in a qt-namespace.
// Position   position () const
// qreal   border () const
// QBrush   borderBrush () const
// BorderStyle   borderStyle () const
// qreal   padding () const
// QTextLength   width () const
// QTextLength   height () const
// PageBreakFlags   pageBreakPolicy () const
}

void QTextOdfWriter::writeTableFormat(QXmlStreamWriter &writer, QTextTableFormat format, int formatIndex) const
{
    // start writing table style element
    writer.writeStartElement(styleNS, QString::fromLatin1("style"));
    writer.writeAttribute(styleNS, QString::fromLatin1("name"),
                          QString::fromLatin1("Table%1").arg(formatIndex));
    writer.writeAttribute(styleNS, QString::fromLatin1("family"), QString::fromLatin1("table"));
    writer.writeEmptyElement(styleNS, QString::fromLatin1("table-properties"));

    if (m_tableFormatsWithBorders.contains(formatIndex)) {
        // write border format collapsing to table style
        writer.writeAttribute(tableNS, QString::fromLatin1("border-model"),
                              QString::fromLatin1("collapsing"));
    }
    const char* align = nullptr;
    switch (format.alignment()) {
    case Qt::AlignLeft:
        align = "left";
        break;
    case Qt::AlignRight:
        align = "right";
        break;
    case Qt::AlignHCenter:
        align = "center";
        break;
    case Qt::AlignJustify:
        align = "margins";
        break;
    }
    if (align)
       writer.writeAttribute(tableNS, QString::fromLatin1("align"), QString::fromLatin1(align));
    if (format.width().rawValue()) {
        writer.writeAttribute(styleNS, QString::fromLatin1("width"),
                              QString::number(format.width().rawValue()) + QLatin1String("pt"));
    }
    writer.writeEndElement();
    // start writing table-column style element
    if (format.columnWidthConstraints().size()) {
        // write table-column-properties for columns with constraints
        m_tableFormatsWithColWidthConstraints.insert(formatIndex); // needed for linking of columns to styles
        for (int colit = 0; colit < format.columnWidthConstraints().size(); ++colit) {
            writer.writeStartElement(styleNS, QString::fromLatin1("style"));
            writer.writeAttribute(styleNS, QString::fromLatin1("name"),
                                  QString::fromLatin1("Table%1.%2").arg(formatIndex).arg(colit));
            writer.writeAttribute(styleNS, QString::fromLatin1("family"), QString::fromLatin1("table-column"));
            writer.writeEmptyElement(styleNS, QString::fromLatin1("table-column-properties"));
            QString columnWidth;
            if (format.columnWidthConstraints().at(colit).type() == QTextLength::PercentageLength) {
                columnWidth = QString::number(format.columnWidthConstraints().at(colit).rawValue())
                        + QLatin1String("%");
            } else if (format.columnWidthConstraints().at(colit).type() == QTextLength::FixedLength) {
                columnWidth = QString::number(format.columnWidthConstraints().at(colit).rawValue())
                        + QLatin1String("pt");
            } else {
                //!! HARD-CODING variableWidth Constraints to 100% / nr constraints
                columnWidth = QString::number(100 / format.columnWidthConstraints().size())
                                      + QLatin1String("%");
            }
            writer.writeAttribute(styleNS, QString::fromLatin1("column-width"), columnWidth);
            writer.writeEndElement();
        }
    }
}

void QTextOdfWriter::writeTableCellFormat(QXmlStreamWriter &writer, QTextTableCellFormat format,
                                          int formatIndex, QVector<QTextFormat> &styles) const
{
    // check for all table cells here if they are in a table with border
    if (m_cellFormatsInTablesWithBorders.contains(formatIndex)) {
        const QVector<int> tableIdVector = m_cellFormatsInTablesWithBorders.value(formatIndex);
        for (const auto &tableId : tableIdVector) {
            const auto &tmpStyle = styles.at(tableId);
            if (tmpStyle.isTableFormat()) {
                QTextTableFormat tableFormatTmp = tmpStyle.toTableFormat();
                tableCellStyleElement(writer, formatIndex, format, true, tableId, tableFormatTmp);
            } else {
                qDebug("QTextOdfWriter::writeTableCellFormat: ERROR writing table border format");
            }
        }
    }
    tableCellStyleElement(writer, formatIndex, format, false);
}

void QTextOdfWriter::tableCellStyleElement(QXmlStreamWriter &writer, const int &formatIndex,
                                           const QTextTableCellFormat &format,
                                           bool hasBorder, int tableId,
                                           const QTextTableFormat tableFormatTmp) const {
    writer.writeStartElement(styleNS, QString::fromLatin1("style"));
    if (hasBorder) {
        writer.writeAttribute(styleNS, QString::fromLatin1("name"),
                              QString::fromLatin1("TB%1.%2").arg(tableId).arg(formatIndex));
    } else {
        writer.writeAttribute(styleNS, QString::fromLatin1("name"), QString::fromLatin1("T%1").arg(formatIndex));
    }
    writer.writeAttribute(styleNS, QString::fromLatin1("family"), QString::fromLatin1("table-cell"));
    writer.writeEmptyElement(styleNS, QString::fromLatin1("table-cell-properties"));
    if (hasBorder) {
        writer.writeAttribute(foNS, QString::fromLatin1("border"),
                              pixelToPoint(tableFormatTmp.border()) + QLatin1String(" ")
                              + borderStyleName(tableFormatTmp.borderStyle()) + QLatin1String(" ")
                              + tableFormatTmp.borderBrush().color().name(QColor::HexRgb));
    }
    qreal topPadding = format.topPadding();
    qreal padding = topPadding + tableFormatTmp.cellPadding();
    if (padding > 0 && topPadding == format.bottomPadding()
        && topPadding == format.leftPadding() && topPadding == format.rightPadding()) {
        writer.writeAttribute(foNS, QString::fromLatin1("padding"), pixelToPoint(padding));
    }
    else {
        if (padding > 0)
            writer.writeAttribute(foNS, QString::fromLatin1("padding-top"), pixelToPoint(padding));
        padding = format.bottomPadding() + tableFormatTmp.cellPadding();
        if (padding > 0)
            writer.writeAttribute(foNS, QString::fromLatin1("padding-bottom"),
                                  pixelToPoint(padding));
        padding = format.leftPadding() + tableFormatTmp.cellPadding();
        if (padding > 0)
            writer.writeAttribute(foNS, QString::fromLatin1("padding-left"),
                                  pixelToPoint(padding));
        padding = format.rightPadding() + tableFormatTmp.cellPadding();
        if (padding > 0)
            writer.writeAttribute(foNS, QString::fromLatin1("padding-right"),
                                  pixelToPoint(padding));
    }

    if (format.hasProperty(QTextFormat::TextVerticalAlignment)) {
        QString pos;
        switch (format.verticalAlignment()) {  // TODO - review: doesn't handle all cases
        case QTextCharFormat::AlignMiddle:
            pos = QString::fromLatin1("middle"); break;
        case QTextCharFormat::AlignTop:
            pos = QString::fromLatin1("top"); break;
        case QTextCharFormat::AlignBottom:
            pos = QString::fromLatin1("bottom"); break;
        default:
            pos = QString::fromLatin1("automatic"); break;
        }
        writer.writeAttribute(styleNS, QString::fromLatin1("vertical-align"), pos);
    }

    // TODO
    // ODF just search for style-table-cell-properties-attlist)
    // QTextFormat::BackgroundImageUrl
    // format.background
    writer.writeEndElement(); // style
}

///////////////////////

QTextOdfWriter::QTextOdfWriter(const QTextDocument &document, QIODevice *device)
    : officeNS (QLatin1String("urn:oasis:names:tc:opendocument:xmlns:office:1.0")),
    textNS (QLatin1String("urn:oasis:names:tc:opendocument:xmlns:text:1.0")),
    styleNS (QLatin1String("urn:oasis:names:tc:opendocument:xmlns:style:1.0")),
    foNS (QLatin1String("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0")),
    tableNS (QLatin1String("urn:oasis:names:tc:opendocument:xmlns:table:1.0")),
    drawNS (QLatin1String("urn:oasis:names:tc:opendocument:xmlns:drawing:1.0")),
    xlinkNS (QLatin1String("http://www.w3.org/1999/xlink")),
    svgNS (QLatin1String("urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0")),
    m_document(&document),
    m_device(device),
    m_strategy(nullptr),
    m_codec(nullptr),
    m_createArchive(true)
{
}

bool QTextOdfWriter::writeAll()
{
    if (m_createArchive)
        m_strategy = new QZipStreamStrategy(m_device);
    else
        m_strategy = new QXmlStreamStrategy(m_device);

    if (!m_device->isWritable() && ! m_device->open(QIODevice::WriteOnly)) {
        qWarning("QTextOdfWriter::writeAll: the device cannot be opened for writing");
        return false;
    }
    QXmlStreamWriter writer(m_strategy->contentStream);
#if QT_CONFIG(textcodec)
    if (m_codec)
        writer.setCodec(m_codec);
#endif
    // prettyfy
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    writer.writeNamespace(officeNS, QString::fromLatin1("office"));
    writer.writeNamespace(textNS, QString::fromLatin1("text"));
    writer.writeNamespace(styleNS, QString::fromLatin1("style"));
    writer.writeNamespace(foNS, QString::fromLatin1("fo"));
    writer.writeNamespace(tableNS, QString::fromLatin1("table"));
    writer.writeNamespace(drawNS, QString::fromLatin1("draw"));
    writer.writeNamespace(xlinkNS, QString::fromLatin1("xlink"));
    writer.writeNamespace(svgNS, QString::fromLatin1("svg"));
    writer.writeStartDocument();
    writer.writeStartElement(officeNS, QString::fromLatin1("document-content"));
    writer.writeAttribute(officeNS, QString::fromLatin1("version"), QString::fromLatin1("1.2"));

    // add fragments. (for character formats)
    QTextDocumentPrivate::FragmentIterator fragIt = m_document->docHandle()->begin();
    QSet<int> formats;
    while (fragIt != m_document->docHandle()->end()) {
        const QTextFragmentData * const frag = fragIt.value();
        formats << frag->format;
        ++fragIt;
    }

    // add blocks (for blockFormats)
    QTextDocumentPrivate::BlockMap &blocks = m_document->docHandle()->blockMap();
    QTextDocumentPrivate::BlockMap::Iterator blockIt = blocks.begin();
    while (blockIt != blocks.end()) {
        const QTextBlockData * const block = blockIt.value();
        formats << block->format;
        ++blockIt;
    }

    // add objects for lists, frames and tables
    const QVector<QTextFormat> allFormats = m_document->allFormats();
    const QList<int> copy = formats.values();
    for (auto index : copy) {
        QTextObject *object = m_document->objectForFormat(allFormats[index]);
        if (object) {
            formats << object->formatIndex();
            if (auto *tableobject = qobject_cast<QTextTable *>(object)) {
                if (tableobject->format().borderStyle()) {
                    int tableID = tableobject->formatIndex();
                    m_tableFormatsWithBorders.insert(tableID);
                    // loop through all rows and cols of table and store cell IDs,
                    // create Hash with cell ID as Key and table IDs as Vector
                    for (int rowindex = 0; rowindex < tableobject->rows(); ++rowindex) {
                        for (int colindex = 0; colindex < tableobject->columns(); ++colindex) {
                            const int cellFormatID = tableobject->cellAt(rowindex, colindex).tableCellFormatIndex();
                            QVector<int> tableIdsTmp;
                            if (m_cellFormatsInTablesWithBorders.contains(cellFormatID))
                                tableIdsTmp = m_cellFormatsInTablesWithBorders.value(cellFormatID);
                            if (!tableIdsTmp.contains(tableID))
                                tableIdsTmp.append(tableID);
                            m_cellFormatsInTablesWithBorders.insert(cellFormatID, tableIdsTmp);
                        }
                    }
                }
            }
        }
    }

    writeFormats(writer, formats);

    writer.writeStartElement(officeNS, QString::fromLatin1("body"));
    writer.writeStartElement(officeNS, QString::fromLatin1("text"));
    QTextFrame *rootFrame = m_document->rootFrame();
    writeFrame(writer, rootFrame);
    writer.writeEndElement(); // text
    writer.writeEndElement(); // body
    writer.writeEndElement(); // document-content
    writer.writeEndDocument();
    delete m_strategy;
    m_strategy = nullptr;

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_TEXTODFWRITER
