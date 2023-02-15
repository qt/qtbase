// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextmarkdownwriter_p.h"
#include "qtextdocumentlayout_p.h"
#include "qfontinfo.h"
#include "qfontmetrics.h"
#include "qtextdocument_p.h"
#include "qtextlist.h"
#include "qtexttable.h"
#include "qtextcursor.h"
#include "qtextimagehandler_p.h"
#include "qloggingcategory.h"
#if QT_CONFIG(itemmodel)
#include "qabstractitemmodel.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcMDW, "qt.text.markdown.writer")

static const QChar qtmw_Space = u' ';
static const QChar qtmw_Tab = u'\t';
static const QChar qtmw_Newline = u'\n';
static const QChar qtmw_CarriageReturn = u'\r';
static const QChar qtmw_LineBreak = u'\x2028';
static const QChar qtmw_DoubleQuote = u'"';
static const QChar qtmw_Backtick = u'`';
static const QChar qtmw_Backslash = u'\\';
static const QChar qtmw_Period = u'.';

QTextMarkdownWriter::QTextMarkdownWriter(QTextStream &stream, QTextDocument::MarkdownFeatures features)
  : m_stream(stream), m_features(features)
{
}

bool QTextMarkdownWriter::writeAll(const QTextDocument *document)
{
    writeFrame(document->rootFrame());
    return true;
}

#if QT_CONFIG(itemmodel)
void QTextMarkdownWriter::writeTable(const QAbstractItemModel *table)
{
    QList<int> tableColumnWidths(table->columnCount());
    for (int col = 0; col < table->columnCount(); ++col) {
        tableColumnWidths[col] = table->headerData(col, Qt::Horizontal).toString().size();
        for (int row = 0; row < table->rowCount(); ++row) {
            tableColumnWidths[col] = qMax(tableColumnWidths[col],
                table->data(table->index(row, col)).toString().size());
        }
    }

    // write the header and separator
    for (int col = 0; col < table->columnCount(); ++col) {
        QString s = table->headerData(col, Qt::Horizontal).toString();
        m_stream << '|' << s << QString(tableColumnWidths[col] - s.size(), qtmw_Space);
    }
    m_stream << "|" << Qt::endl;
    for (int col = 0; col < tableColumnWidths.size(); ++col)
        m_stream << '|' << QString(tableColumnWidths[col], u'-');
    m_stream << '|'<< Qt::endl;

    // write the body
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QString s = table->data(table->index(row, col)).toString();
            m_stream << '|' << s << QString(tableColumnWidths[col] - s.size(), qtmw_Space);
        }
        m_stream << '|'<< Qt::endl;
    }
    m_listInfo.clear();
}
#endif

void QTextMarkdownWriter::writeFrame(const QTextFrame *frame)
{
    Q_ASSERT(frame);
    const QTextTable *table = qobject_cast<const QTextTable*> (frame);
    QTextFrame::iterator iterator = frame->begin();
    QTextFrame *child = nullptr;
    int tableRow = -1;
    bool lastWasList = false;
    QList<int> tableColumnWidths;
    if (table) {
        tableColumnWidths.resize(table->columns());
        for (int col = 0; col < table->columns(); ++col) {
            for (int row = 0; row < table->rows(); ++ row) {
                QTextTableCell cell = table->cellAt(row, col);
                int cellTextLen = 0;
                auto it = cell.begin();
                while (it != cell.end()) {
                    QTextBlock block = it.currentBlock();
                    if (block.isValid())
                        cellTextLen += block.text().size();
                    ++it;
                }
                if (cell.columnSpan() == 1 && tableColumnWidths[col] < cellTextLen)
                    tableColumnWidths[col] = cellTextLen;
            }
        }
    }
    while (!iterator.atEnd()) {
        if (iterator.currentFrame() && child != iterator.currentFrame())
            writeFrame(iterator.currentFrame());
        else { // no frame, it's a block
            QTextBlock block = iterator.currentBlock();
            // Look ahead and detect some cases when we should
            // suppress needless blank lines, when there will be a big change in block format
            bool nextIsDifferent = false;
            bool ending = false;
            {
                QTextFrame::iterator next = iterator;
                ++next;
                if (next.atEnd()) {
                    nextIsDifferent = true;
                    ending = true;
                } else {
                    QTextBlockFormat format = iterator.currentBlock().blockFormat();
                    QTextBlockFormat nextFormat = next.currentBlock().blockFormat();
                    if (nextFormat.indent() != format.indent() ||
                            nextFormat.property(QTextFormat::BlockCodeLanguage) != format.property(QTextFormat::BlockCodeLanguage))
                        nextIsDifferent = true;
                }
            }
            if (table) {
                QTextTableCell cell = table->cellAt(block.position());
                if (tableRow < cell.row()) {
                    if (tableRow == 0) {
                        m_stream << qtmw_Newline;
                        for (int col = 0; col < tableColumnWidths.size(); ++col)
                            m_stream << '|' << QString(tableColumnWidths[col], u'-');
                        m_stream << '|';
                    }
                    m_stream << qtmw_Newline << '|';
                    tableRow = cell.row();
                }
            } else if (!block.textList()) {
                if (lastWasList)
                    m_stream << qtmw_Newline;
            }
            int endingCol = writeBlock(block, !table, table && tableRow == 0,
                                       nextIsDifferent && !block.textList());
            m_doubleNewlineWritten = false;
            if (table) {
                QTextTableCell cell = table->cellAt(block.position());
                int paddingLen = -endingCol;
                int spanEndCol = cell.column() + cell.columnSpan();
                for (int col = cell.column(); col < spanEndCol; ++col)
                    paddingLen += tableColumnWidths[col];
                if (paddingLen > 0)
                    m_stream << QString(paddingLen, qtmw_Space);
                for (int col = cell.column(); col < spanEndCol; ++col)
                    m_stream << "|";
            } else if (m_fencedCodeBlock && ending) {
                m_stream << qtmw_Newline << m_linePrefix << QString(m_wrappedLineIndent, qtmw_Space)
                         << m_codeBlockFence << qtmw_Newline << qtmw_Newline;
                m_codeBlockFence.clear();
            } else if (m_indentedCodeBlock && nextIsDifferent) {
                m_stream << qtmw_Newline << qtmw_Newline;
            } else if (endingCol > 0) {
                if (block.textList() || block.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage)) {
                    m_stream << qtmw_Newline;
                } else {
                    m_stream << qtmw_Newline << qtmw_Newline;
                    m_doubleNewlineWritten = true;
                }
            }
            lastWasList = block.textList();
        }
        child = iterator.currentFrame();
        ++iterator;
    }
    if (table) {
        m_stream << qtmw_Newline << qtmw_Newline;
        m_doubleNewlineWritten = true;
    }
    m_listInfo.clear();
}

QTextMarkdownWriter::ListInfo QTextMarkdownWriter::listInfo(QTextList *list)
{
    if (!m_listInfo.contains(list)) {
        // decide whether this list is loose or tight
        ListInfo info;
        info.loose = false;
        if (list->count() > 1) {
            QTextBlock first = list->item(0);
            QTextBlock last = list->item(list->count() - 1);
            QTextBlock next = first.next();
            while (next.isValid()) {
                if (next == last)
                    break;
                qCDebug(lcMDW) << "next block in list" << list << next.text() << "part of list?" << next.textList();
                if (!next.textList()) {
                    // If we find a continuation paragraph, this list is "loose"
                    // because it will need a blank line to separate that paragraph.
                    qCDebug(lcMDW) << "decided list beginning with" << first.text() << "is loose after" << next.text();
                    info.loose = true;
                    break;
                }
                next = next.next();
            }
        }
        m_listInfo.insert(list, info);
        return info;
    }
    return m_listInfo.value(list);
}

static int nearestWordWrapIndex(const QString &s, int before)
{
    before = qMin(before, s.size());
    int fragBegin = qMax(before - 15, 0);
    if (lcMDW().isDebugEnabled()) {
        QString frag = s.mid(fragBegin, 30);
        qCDebug(lcMDW) << frag << before;
        qCDebug(lcMDW) << QString(before - fragBegin, qtmw_Period) + u'<';
    }
    for (int i = before - 1; i >= 0; --i) {
        if (s.at(i).isSpace()) {
            qCDebug(lcMDW) << QString(i - fragBegin, qtmw_Period) + u'^' << i;
            return i;
        }
    }
    qCDebug(lcMDW, "not possible");
    return -1;
}

static int adjacentBackticksCount(const QString &s)
{
    int start = -1, len = s.size();
    int ret = 0;
    for (int i = 0; i < len; ++i) {
        if (s.at(i) == qtmw_Backtick) {
            if (start < 0)
                start = i;
        } else if (start >= 0) {
            ret = qMax(ret, i - start);
            start = -1;
        }
    }
    if (s.at(len - 1) == qtmw_Backtick)
        ret = qMax(ret, len - start);
    return ret;
}

static void maybeEscapeFirstChar(QString &s)
{
    QString sTrimmed = s.trimmed();
    if (sTrimmed.isEmpty())
        return;
    char firstChar = sTrimmed.at(0).toLatin1();
    if (firstChar == '*' || firstChar == '+' || firstChar == '-') {
        int i = s.indexOf(QLatin1Char(firstChar));
        s.insert(i, u'\\');
    }
}

struct LineEndPositions {
    const QChar *lineEnd;
    const QChar *nextLineBegin;
};

static LineEndPositions findLineEnd(const QChar *begin, const QChar *end)
{
    LineEndPositions result{ end, end };

    while (begin < end) {
        if (*begin == qtmw_Newline) {
            result.lineEnd = begin;
            result.nextLineBegin = begin + 1;
            break;
        } else if (*begin == qtmw_CarriageReturn) {
            result.lineEnd = begin;
            result.nextLineBegin = begin + 1;
            if (((begin + 1) < end) && begin[1] == qtmw_Newline)
                ++result.nextLineBegin;
            break;
        }

        ++begin;
    }

    return result;
}

static bool isBlankLine(const QChar *begin, const QChar *end)
{
    while (begin < end) {
        if (*begin != qtmw_Space && *begin != qtmw_Tab)
            return false;
        ++begin;
    }
    return true;
}

static QString createLinkTitle(const QString &title)
{
    QString result;
    result.reserve(title.size() + 2);
    result += qtmw_DoubleQuote;

    const QChar *data = title.data();
    const QChar *end = data + title.size();

    while (data < end) {
        const auto lineEndPositions = findLineEnd(data, end);

        if (!isBlankLine(data, lineEndPositions.lineEnd)) {
            while (data < lineEndPositions.nextLineBegin) {
                if (*data == qtmw_DoubleQuote)
                    result += qtmw_Backslash;
                result += *data;
                ++data;
            }
        }

        data = lineEndPositions.nextLineBegin;
    }

    result += qtmw_DoubleQuote;
    return result;
}

int QTextMarkdownWriter::writeBlock(const QTextBlock &block, bool wrap, bool ignoreFormat, bool ignoreEmpty)
{
    if (block.text().isEmpty() && ignoreEmpty)
        return 0;
    const int ColumnLimit = 80;
    QTextBlockFormat blockFmt = block.blockFormat();
    bool missedBlankCodeBlockLine = false;
    const bool codeBlock = blockFmt.hasProperty(QTextFormat::BlockCodeFence) ||
            blockFmt.stringProperty(QTextFormat::BlockCodeLanguage).size() > 0 ||
            blockFmt.nonBreakableLines();
    if (m_fencedCodeBlock && !codeBlock) {
        m_stream << m_linePrefix << m_codeBlockFence << qtmw_Newline;
        m_fencedCodeBlock = false;
        m_codeBlockFence.clear();
    }
    if (block.textList()) { // it's a list-item
        auto fmt = block.textList()->format();
        const int listLevel = fmt.indent();
        // Negative numbers don't start a list in Markdown, so ignore them.
        const int start = fmt.start() >= 0 ? fmt.start() : 1;
        const int number = block.textList()->itemNumber(block) + start;
        QByteArray bullet = " ";
        bool numeric = false;
        switch (fmt.style()) {
        case QTextListFormat::ListDisc:
            bullet = "-";
            m_wrappedLineIndent = 2;
            break;
        case QTextListFormat::ListCircle:
            bullet = "*";
            m_wrappedLineIndent = 2;
            break;
        case QTextListFormat::ListSquare:
            bullet = "+";
            m_wrappedLineIndent = 2;
            break;
        case QTextListFormat::ListStyleUndefined: break;
        case QTextListFormat::ListDecimal:
        case QTextListFormat::ListLowerAlpha:
        case QTextListFormat::ListUpperAlpha:
        case QTextListFormat::ListLowerRoman:
        case QTextListFormat::ListUpperRoman:
            numeric = true;
            m_wrappedLineIndent = 4;
            break;
        }
        switch (blockFmt.marker()) {
        case QTextBlockFormat::MarkerType::Checked:
            bullet += " [x]";
            break;
        case QTextBlockFormat::MarkerType::Unchecked:
            bullet += " [ ]";
            break;
        default:
            break;
        }
        int indentFirstLine = (listLevel - 1) * (numeric ? 4 : 2);
        m_wrappedLineIndent += indentFirstLine;
        if (m_lastListIndent != listLevel && !m_doubleNewlineWritten && listInfo(block.textList()).loose)
            m_stream << qtmw_Newline;
        m_lastListIndent = listLevel;
        QString prefix(indentFirstLine, qtmw_Space);
        if (numeric) {
            QString suffix = fmt.numberSuffix();
            if (suffix.isEmpty())
                suffix = QString(qtmw_Period);
            QString numberStr = QString::number(number) + suffix + qtmw_Space;
            if (numberStr.size() == 3)
                numberStr += qtmw_Space;
            prefix += numberStr;
        } else {
            prefix += QLatin1StringView(bullet) + qtmw_Space;
        }
        m_stream << prefix;
    } else if (blockFmt.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        m_stream << "- - -\n"; // unambiguous horizontal rule, not an underline under a heading
        return 0;
    } else if (codeBlock) {
        // It's important to preserve blank lines in code blocks.  But blank lines in code blocks
        // inside block quotes are getting preserved anyway (along with the "> " prefix).
        if (!blockFmt.hasProperty(QTextFormat::BlockQuoteLevel))
            missedBlankCodeBlockLine = true; // only if we don't get any fragments below
        if (!m_fencedCodeBlock) {
            QString fenceChar = blockFmt.stringProperty(QTextFormat::BlockCodeFence);
            if (fenceChar.isEmpty())
                fenceChar = "`"_L1;
            m_codeBlockFence = QString(3, fenceChar.at(0));
            if (blockFmt.hasProperty(QTextFormat::BlockIndent))
                m_codeBlockFence = QString(m_wrappedLineIndent, qtmw_Space) + m_codeBlockFence;
            // A block quote can contain an indented code block, but not vice-versa.
            m_stream << m_linePrefix << m_codeBlockFence
                     << blockFmt.stringProperty(QTextFormat::BlockCodeLanguage) << qtmw_Newline;
            m_fencedCodeBlock = true;
        }
        wrap = false;
    } else if (!blockFmt.indent()) {
        m_wrappedLineIndent = 0;
        m_linePrefix.clear();
        if (blockFmt.hasProperty(QTextFormat::BlockQuoteLevel)) {
            int level = blockFmt.intProperty(QTextFormat::BlockQuoteLevel);
            QString quoteMarker = QStringLiteral("> ");
            m_linePrefix.reserve(level * 2);
            for (int i = 0; i < level; ++i)
                m_linePrefix += quoteMarker;
        }
        if (blockFmt.hasProperty(QTextFormat::BlockCodeLanguage)) {
            // A block quote can contain an indented code block, but not vice-versa.
            m_linePrefix += QString(4, qtmw_Space);
            m_indentedCodeBlock = true;
        }
    }
    if (blockFmt.headingLevel())
        m_stream << QByteArray(blockFmt.headingLevel(), '#') << ' ';
    else
        m_stream << m_linePrefix;

    QString wrapIndentString = m_linePrefix + QString(m_wrappedLineIndent, qtmw_Space);
    // It would be convenient if QTextStream had a lineCharPos() accessor,
    // to keep track of how many characters (not bytes) have been written on the current line,
    // but it doesn't.  So we have to keep track with this col variable.
    int col = wrapIndentString.size();
    bool mono = false;
    bool startsOrEndsWithBacktick = false;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikeOut = false;
    QString backticks(qtmw_Backtick);
    for (QTextBlock::Iterator frag = block.begin(); !frag.atEnd(); ++frag) {
        missedBlankCodeBlockLine = false;
        QString fragmentText = frag.fragment().text();
        while (fragmentText.endsWith(qtmw_Newline))
            fragmentText.chop(1);
        if (block.textList()) { // <li>first line</br>continuation</li>
            QString newlineIndent =
                    QString(qtmw_Newline) + QString(m_wrappedLineIndent, qtmw_Space);
            fragmentText.replace(QString(qtmw_LineBreak), newlineIndent);
        } else if (blockFmt.indent() > 0) { // <li>first line<p>continuation</p></li>
            m_stream << QString(m_wrappedLineIndent, qtmw_Space);
        } else {
            fragmentText.replace(qtmw_LineBreak, qtmw_Newline);
        }
        startsOrEndsWithBacktick |=
                fragmentText.startsWith(qtmw_Backtick) || fragmentText.endsWith(qtmw_Backtick);
        QTextCharFormat fmt = frag.fragment().charFormat();
        if (fmt.isImageFormat()) {
            QTextImageFormat ifmt = fmt.toImageFormat();
            QString desc = ifmt.stringProperty(QTextFormat::ImageAltText);
            if (desc.isEmpty())
                desc = "image"_L1;
            QString s = "!["_L1 + desc + "]("_L1 + ifmt.name();
            QString title = ifmt.stringProperty(QTextFormat::ImageTitle);
            if (!title.isEmpty())
                s += qtmw_Space + qtmw_DoubleQuote + title + qtmw_DoubleQuote;
            s += u')';
            if (wrap && col + s.size() > ColumnLimit) {
                m_stream << qtmw_Newline << wrapIndentString;
                col = m_wrappedLineIndent;
            }
            m_stream << s;
            col += s.size();
        } else if (fmt.hasProperty(QTextFormat::AnchorHref)) {
            const auto href = fmt.property(QTextFormat::AnchorHref).toString();
            const bool hasToolTip = fmt.hasProperty(QTextFormat::TextToolTip);
            QString s;
            if (!hasToolTip && href == fragmentText && !QUrl(href, QUrl::StrictMode).scheme().isEmpty()) {
                s = u'<' + href + u'>';
            } else {
                s = u'[' + fragmentText + "]("_L1 + href;
                if (hasToolTip) {
                    s += qtmw_Space;
                    s += createLinkTitle(fmt.property(QTextFormat::TextToolTip).toString());
                }
                s += u')';
            }
            if (wrap && col + s.size() > ColumnLimit) {
                m_stream << qtmw_Newline << wrapIndentString;
                col = m_wrappedLineIndent;
            }
            m_stream << s;
            col += s.size();
        } else {
            QFontInfo fontInfo(fmt.font());
            bool monoFrag = fontInfo.fixedPitch() || fmt.fontFixedPitch();
            QString markers;
            if (!ignoreFormat) {
                if (monoFrag != mono && !m_indentedCodeBlock && !m_fencedCodeBlock) {
                    if (monoFrag)
                        backticks =
                                QString(adjacentBackticksCount(fragmentText) + 1, qtmw_Backtick);
                    markers += backticks;
                    if (startsOrEndsWithBacktick)
                        markers += qtmw_Space;
                    mono = monoFrag;
                }
                if (!blockFmt.headingLevel() && !mono) {
                    if (fontInfo.bold() != bold) {
                        markers += "**"_L1;
                        bold = fontInfo.bold();
                    }
                    if (fontInfo.italic() != italic) {
                        markers += u'*';
                        italic = fontInfo.italic();
                    }
                    if (fontInfo.strikeOut() != strikeOut) {
                        markers += "~~"_L1;
                        strikeOut = fontInfo.strikeOut();
                    }
                    if (fontInfo.underline() != underline) {
                        // Markdown doesn't support underline, but the parser will treat a single underline
                        // the same as a single asterisk, and the marked fragment will be rendered in italics.
                        // That will have to do.
                        markers += u'_';
                        underline = fontInfo.underline();
                    }
                }
            }
            if (wrap && col + markers.size() * 2 + fragmentText.size() > ColumnLimit) {
                int i = 0;
                const int fragLen = fragmentText.size();
                bool breakingLine = false;
                while (i < fragLen) {
                    if (col >= ColumnLimit) {
                        m_stream << qtmw_Newline << wrapIndentString;
                        col = m_wrappedLineIndent;
                        while (i < fragLen && fragmentText[i].isSpace())
                            ++i;
                    }
                    int j = i + ColumnLimit - col;
                    if (j < fragLen) {
                        int wi = nearestWordWrapIndex(fragmentText, j);
                        if (wi < 0) {
                            j = fragLen;
                        } else if (wi >= i) {
                            j = wi;
                            breakingLine = true;
                        }
                    } else {
                        j = fragLen;
                        breakingLine = false;
                    }
                    QString subfrag = fragmentText.mid(i, j - i);
                    if (!i) {
                        m_stream << markers;
                        col += markers.size();
                    }
                    if (col == m_wrappedLineIndent)
                        maybeEscapeFirstChar(subfrag);
                    m_stream << subfrag;
                    if (breakingLine) {
                        m_stream << qtmw_Newline << wrapIndentString;
                        col = m_wrappedLineIndent;
                    } else {
                        col += subfrag.size();
                    }
                    i = j + 1;
                }
            } else {
                m_stream << markers << fragmentText;
                col += markers.size() + fragmentText.size();
            }
        }
    }
    if (mono) {
        if (startsOrEndsWithBacktick) {
            m_stream << qtmw_Space;
            col += 1;
        }
        m_stream << backticks;
        col += backticks.size();
    }
    if (bold) {
        m_stream << "**";
        col += 2;
    }
    if (italic) {
        m_stream << "*";
        col += 1;
    }
    if (underline) {
        m_stream << "_";
        col += 1;
    }
    if (strikeOut) {
        m_stream << "~~";
        col += 2;
    }
    if (missedBlankCodeBlockLine)
        m_stream << qtmw_Newline;
    return col;
}

QT_END_NAMESPACE
