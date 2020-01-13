/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qtextmarkdownimporter_p.h"
#include "qtextdocumentfragment_p.h"
#include <QLoggingCategory>
#if QT_CONFIG(regularexpression)
#include <QRegularExpression>
#endif
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextList>
#include <QTextTable>
#include "../../3rdparty/md4c/md4c.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcMD, "qt.text.markdown")

static const QChar Newline = QLatin1Char('\n');
static const QChar Space = QLatin1Char(' ');

// TODO maybe eliminate the margins after all views recognize BlockQuoteLevel, CSS can format it, etc.
static const int BlockQuoteIndent = 40; // pixels, same as in QTextHtmlParserNode::initializeProperties

Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureCollapseWhitespace) == MD_FLAG_COLLAPSEWHITESPACE);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeaturePermissiveATXHeaders) == MD_FLAG_PERMISSIVEATXHEADERS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeaturePermissiveURLAutoLinks) == MD_FLAG_PERMISSIVEURLAUTOLINKS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeaturePermissiveMailAutoLinks) == MD_FLAG_PERMISSIVEEMAILAUTOLINKS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureNoIndentedCodeBlocks) == MD_FLAG_NOINDENTEDCODEBLOCKS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureNoHTMLBlocks) == MD_FLAG_NOHTMLBLOCKS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureNoHTMLSpans) == MD_FLAG_NOHTMLSPANS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureTables) == MD_FLAG_TABLES);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureStrikeThrough) == MD_FLAG_STRIKETHROUGH);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeaturePermissiveWWWAutoLinks) == MD_FLAG_PERMISSIVEWWWAUTOLINKS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeaturePermissiveAutoLinks) == MD_FLAG_PERMISSIVEAUTOLINKS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureTasklists) == MD_FLAG_TASKLISTS);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::FeatureNoHTML) == MD_FLAG_NOHTML);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::DialectCommonMark) == MD_DIALECT_COMMONMARK);
Q_STATIC_ASSERT(int(QTextMarkdownImporter::DialectGitHub) == MD_DIALECT_GITHUB);

// --------------------------------------------------------
// MD4C callback function wrappers

static int CbEnterBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbEnterBlock(int(type), detail);
}

static int CbLeaveBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbLeaveBlock(int(type), detail);
}

static int CbEnterSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbEnterSpan(int(type), detail);
}

static int CbLeaveSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbLeaveSpan(int(type), detail);
}

static int CbText(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbText(int(type), text, size);
}

static void CbDebugLog(const char *msg, void *userdata)
{
    Q_UNUSED(userdata)
    qCDebug(lcMD) << msg;
}

// MD4C callback function wrappers
// --------------------------------------------------------

static Qt::Alignment MdAlignment(MD_ALIGN a, Qt::Alignment defaultAlignment = Qt::AlignLeft | Qt::AlignVCenter)
{
    switch (a) {
    case MD_ALIGN_LEFT:
        return Qt::AlignLeft | Qt::AlignVCenter;
    case MD_ALIGN_CENTER:
        return Qt::AlignHCenter | Qt::AlignVCenter;
    case MD_ALIGN_RIGHT:
        return Qt::AlignRight | Qt::AlignVCenter;
    default: // including MD_ALIGN_DEFAULT
        return defaultAlignment;
    }
}

QTextMarkdownImporter::QTextMarkdownImporter(QTextMarkdownImporter::Features features)
  : m_monoFont(QFontDatabase::systemFont(QFontDatabase::FixedFont))
  , m_features(features)
{
}

QTextMarkdownImporter::QTextMarkdownImporter(QTextDocument::MarkdownFeatures features)
  : QTextMarkdownImporter(static_cast<QTextMarkdownImporter::Features>(int(features)))
{
}

void QTextMarkdownImporter::import(QTextDocument *doc, const QString &markdown)
{
    MD_PARSER callbacks = {
        0, // abi_version
        unsigned(m_features),
        &CbEnterBlock,
        &CbLeaveBlock,
        &CbEnterSpan,
        &CbLeaveSpan,
        &CbText,
        &CbDebugLog,
        nullptr // syntax
    };
    m_doc = doc;
    m_paragraphMargin = m_doc->defaultFont().pointSize() * 2 / 3;
    m_cursor = new QTextCursor(doc);
    doc->clear();
    if (doc->defaultFont().pointSize() != -1)
        m_monoFont.setPointSize(doc->defaultFont().pointSize());
    else
        m_monoFont.setPixelSize(doc->defaultFont().pixelSize());
    qCDebug(lcMD) << "default font" << doc->defaultFont() << "mono font" << m_monoFont;
    QByteArray md = markdown.toUtf8();
    md_parse(md.constData(), MD_SIZE(md.size()), &callbacks, this);
    delete m_cursor;
    m_cursor = nullptr;
}

int QTextMarkdownImporter::cbEnterBlock(int blockType, void *det)
{
    m_blockType = blockType;
    switch (blockType) {
    case MD_BLOCK_P:
        if (!m_listStack.isEmpty())
            qCDebug(lcMD, m_listItem ? "P of LI at level %d"  : "P continuation inside LI at level %d", m_listStack.count());
        else
            qCDebug(lcMD, "P");
        m_needsInsertBlock = true;
        break;
    case MD_BLOCK_QUOTE:
        ++m_blockQuoteDepth;
        qCDebug(lcMD, "QUOTE level %d", m_blockQuoteDepth);
        break;
    case MD_BLOCK_CODE: {
        MD_BLOCK_CODE_DETAIL *detail = static_cast<MD_BLOCK_CODE_DETAIL *>(det);
        m_codeBlock = true;
        m_blockCodeLanguage = QLatin1String(detail->lang.text, int(detail->lang.size));
        m_blockCodeFence = detail->fence_char;
        QString info = QLatin1String(detail->info.text, int(detail->info.size));
        m_needsInsertBlock = true;
        if (m_blockQuoteDepth)
            qCDebug(lcMD, "CODE lang '%s' info '%s' fenced with '%c' inside QUOTE %d", qPrintable(m_blockCodeLanguage), qPrintable(info), m_blockCodeFence, m_blockQuoteDepth);
        else
            qCDebug(lcMD, "CODE lang '%s' info '%s' fenced with '%c'", qPrintable(m_blockCodeLanguage), qPrintable(info), m_blockCodeFence);
    } break;
    case MD_BLOCK_H: {
        MD_BLOCK_H_DETAIL *detail = static_cast<MD_BLOCK_H_DETAIL *>(det);
        QTextBlockFormat blockFmt;
        QTextCharFormat charFmt;
        int sizeAdjustment = 4 - int(detail->level); // H1 to H6: +3 to -2
        charFmt.setProperty(QTextFormat::FontSizeAdjustment, sizeAdjustment);
        charFmt.setFontWeight(QFont::Bold);
        blockFmt.setHeadingLevel(int(detail->level));
        m_needsInsertBlock = false;
        if (m_doc->isEmpty()) {
            m_cursor->setBlockFormat(blockFmt);
            m_cursor->setCharFormat(charFmt);
        } else {
            m_cursor->insertBlock(blockFmt, charFmt);
        }
        qCDebug(lcMD, "H%d", detail->level);
    } break;
    case MD_BLOCK_LI: {
        m_needsInsertBlock = true;
        m_listItem = true;
        MD_BLOCK_LI_DETAIL *detail = static_cast<MD_BLOCK_LI_DETAIL *>(det);
        m_markerType = detail->is_task ?
                    (detail->task_mark == ' ' ? QTextBlockFormat::MarkerType::Unchecked : QTextBlockFormat::MarkerType::Checked) :
                    QTextBlockFormat::MarkerType::NoMarker;
        qCDebug(lcMD) << "LI";
    } break;
    case MD_BLOCK_UL: {
        if (m_needsInsertList) // list nested in an empty list
            m_listStack.push(m_cursor->insertList(m_listFormat));
        else
            m_needsInsertList = true;
        MD_BLOCK_UL_DETAIL *detail = static_cast<MD_BLOCK_UL_DETAIL *>(det);
        m_listFormat = QTextListFormat();
        m_listFormat.setIndent(m_listStack.count() + 1);
        switch (detail->mark) {
        case '*':
            m_listFormat.setStyle(QTextListFormat::ListCircle);
            break;
        case '+':
            m_listFormat.setStyle(QTextListFormat::ListSquare);
            break;
        default: // including '-'
            m_listFormat.setStyle(QTextListFormat::ListDisc);
            break;
        }
        qCDebug(lcMD, "UL %c level %d", detail->mark, m_listStack.count() + 1);
    } break;
    case MD_BLOCK_OL: {
        if (m_needsInsertList) // list nested in an empty list
            m_listStack.push(m_cursor->insertList(m_listFormat));
        else
            m_needsInsertList = true;
        MD_BLOCK_OL_DETAIL *detail = static_cast<MD_BLOCK_OL_DETAIL *>(det);
        m_listFormat = QTextListFormat();
        m_listFormat.setIndent(m_listStack.count() + 1);
        m_listFormat.setNumberSuffix(QChar::fromLatin1(detail->mark_delimiter));
        m_listFormat.setStyle(QTextListFormat::ListDecimal);
        qCDebug(lcMD, "OL xx%d level %d", detail->mark_delimiter, m_listStack.count() + 1);
    } break;
    case MD_BLOCK_TD: {
        MD_BLOCK_TD_DETAIL *detail = static_cast<MD_BLOCK_TD_DETAIL *>(det);
        ++m_tableCol;
        // absolute movement (and storage of m_tableCol) shouldn't be necessary, but
        // movePosition(QTextCursor::NextCell) doesn't work
        QTextTableCell cell = m_currentTable->cellAt(m_tableRowCount - 1, m_tableCol);
        if (!cell.isValid()) {
            qWarning("malformed table in Markdown input");
            return 1;
        }
        *m_cursor = cell.firstCursorPosition();
        QTextBlockFormat blockFmt = m_cursor->blockFormat();
        blockFmt.setAlignment(MdAlignment(detail->align));
        m_cursor->setBlockFormat(blockFmt);
        qCDebug(lcMD) << "TD; align" << detail->align << MdAlignment(detail->align) << "col" << m_tableCol;
    } break;
    case MD_BLOCK_TH: {
        ++m_tableColumnCount;
        ++m_tableCol;
        if (m_currentTable->columns() < m_tableColumnCount)
            m_currentTable->appendColumns(1);
        auto cell = m_currentTable->cellAt(m_tableRowCount - 1, m_tableCol);
        if (!cell.isValid()) {
            qWarning("malformed table in Markdown input");
            return 1;
        }
        auto fmt = cell.format();
        fmt.setFontWeight(QFont::Bold);
        cell.setFormat(fmt);
    } break;
    case MD_BLOCK_TR: {
        ++m_tableRowCount;
        m_nonEmptyTableCells.clear();
        if (m_currentTable->rows() < m_tableRowCount)
            m_currentTable->appendRows(1);
        m_tableCol = -1;
        qCDebug(lcMD) << "TR" << m_currentTable->rows();
    } break;
    case MD_BLOCK_TABLE:
        m_tableColumnCount = 0;
        m_tableRowCount = 0;
        m_currentTable = m_cursor->insertTable(1, 1); // we don't know the dimensions yet
        break;
    case MD_BLOCK_HR: {
        qCDebug(lcMD, "HR");
        QTextBlockFormat blockFmt;
        blockFmt.setProperty(QTextFormat::BlockTrailingHorizontalRulerWidth, 1);
        m_cursor->insertBlock(blockFmt, QTextCharFormat());
    } break;
    default:
        break; // nothing to do for now
    }
    return 0; // no error
}

int QTextMarkdownImporter::cbLeaveBlock(int blockType, void *detail)
{
    Q_UNUSED(detail)
    switch (blockType) {
    case MD_BLOCK_P:
        m_listItem = false;
        break;
    case MD_BLOCK_UL:
    case MD_BLOCK_OL:
        if (Q_UNLIKELY(m_needsInsertList))
            m_listStack.push(m_cursor->createList(m_listFormat));
        if (Q_UNLIKELY(m_listStack.isEmpty())) {
            qCWarning(lcMD, "list ended unexpectedly");
        } else {
            qCDebug(lcMD, "list at level %d ended", m_listStack.count());
            m_listStack.pop();
        }
        break;
    case MD_BLOCK_TR: {
        // https://github.com/mity/md4c/issues/29
        // MD4C doesn't tell us explicitly which cells are merged, so merge empty cells
        // with previous non-empty ones
        int mergeEnd = -1;
        int mergeBegin = -1;
        for (int col = m_tableCol; col >= 0; --col) {
            if (m_nonEmptyTableCells.contains(col)) {
                if (mergeEnd >= 0 && mergeBegin >= 0) {
                    qCDebug(lcMD) << "merging cells" << mergeBegin << "to" << mergeEnd << "inclusive, on row" << m_currentTable->rows() - 1;
                    m_currentTable->mergeCells(m_currentTable->rows() - 1, mergeBegin - 1, 1, mergeEnd - mergeBegin + 2);
                }
                mergeEnd = -1;
                mergeBegin = -1;
            } else {
                if (mergeEnd < 0)
                    mergeEnd = col;
                else
                    mergeBegin = col;
            }
        }
    } break;
    case MD_BLOCK_QUOTE: {
        qCDebug(lcMD, "QUOTE level %d ended", m_blockQuoteDepth);
        --m_blockQuoteDepth;
        m_needsInsertBlock = true;
    } break;
    case MD_BLOCK_TABLE:
        qCDebug(lcMD) << "table ended with" << m_currentTable->columns() << "cols and" << m_currentTable->rows() << "rows";
        m_currentTable = nullptr;
        m_cursor->movePosition(QTextCursor::End);
        break;
    case MD_BLOCK_LI:
        qCDebug(lcMD, "LI at level %d ended", m_listStack.count());
        m_listItem = false;
        break;
    case MD_BLOCK_CODE: {
        m_codeBlock = false;
        m_blockCodeLanguage.clear();
        m_blockCodeFence = 0;
        if (m_blockQuoteDepth)
            qCDebug(lcMD, "CODE ended inside QUOTE %d", m_blockQuoteDepth);
        else
            qCDebug(lcMD, "CODE ended");
        m_needsInsertBlock = true;
    } break;
    case MD_BLOCK_H:
        m_cursor->setCharFormat(QTextCharFormat());
        break;
    default:
        break;
    }
    return 0; // no error
}

int QTextMarkdownImporter::cbEnterSpan(int spanType, void *det)
{
    QTextCharFormat charFmt;
    if (!m_spanFormatStack.isEmpty())
        charFmt = m_spanFormatStack.top();
    switch (spanType) {
    case MD_SPAN_EM:
        charFmt.setFontItalic(true);
        break;
    case MD_SPAN_STRONG:
        charFmt.setFontWeight(QFont::Bold);
        break;
    case MD_SPAN_A: {
        MD_SPAN_A_DETAIL *detail = static_cast<MD_SPAN_A_DETAIL *>(det);
        QString url = QString::fromUtf8(detail->href.text, int(detail->href.size));
        QString title = QString::fromUtf8(detail->title.text, int(detail->title.size));
        charFmt.setAnchor(true);
        charFmt.setAnchorHref(url);
        if (!title.isEmpty())
            charFmt.setToolTip(title);
        charFmt.setForeground(m_palette.link());
        qCDebug(lcMD) << "anchor" << url << title;
        } break;
    case MD_SPAN_IMG: {
        m_imageSpan = true;
        m_imageFormat = QTextImageFormat();
        MD_SPAN_IMG_DETAIL *detail = static_cast<MD_SPAN_IMG_DETAIL *>(det);
        m_imageFormat.setName(QString::fromUtf8(detail->src.text, int(detail->src.size)));
        m_imageFormat.setProperty(QTextFormat::ImageTitle, QString::fromUtf8(detail->title.text, int(detail->title.size)));
        break;
    }
    case MD_SPAN_CODE:
        charFmt.setFont(m_monoFont);
        break;
    case MD_SPAN_DEL:
        charFmt.setFontStrikeOut(true);
        break;
    }
    m_spanFormatStack.push(charFmt);
    qCDebug(lcMD) << spanType << "setCharFormat" << charFmt.font().family() << charFmt.fontWeight()
                  << (charFmt.fontItalic() ? "italic" : "") << charFmt.foreground().color().name();
    m_cursor->setCharFormat(charFmt);
    return 0; // no error
}

int QTextMarkdownImporter::cbLeaveSpan(int spanType, void *detail)
{
    Q_UNUSED(detail)
    QTextCharFormat charFmt;
    if (!m_spanFormatStack.isEmpty()) {
        m_spanFormatStack.pop();
        if (!m_spanFormatStack.isEmpty())
            charFmt = m_spanFormatStack.top();
    }
    m_cursor->setCharFormat(charFmt);
    qCDebug(lcMD) << spanType << "setCharFormat" << charFmt.font().family() << charFmt.fontWeight()
                  << (charFmt.fontItalic() ? "italic" : "") << charFmt.foreground().color().name();
    if (spanType == int(MD_SPAN_IMG))
        m_imageSpan = false;
    return 0; // no error
}

int QTextMarkdownImporter::cbText(int textType, const char *text, unsigned size)
{
    if (m_needsInsertBlock)
        insertBlock();
#if QT_CONFIG(regularexpression)
    static const QRegularExpression openingBracket(QStringLiteral("<[a-zA-Z]"));
    static const QRegularExpression closingBracket(QStringLiteral("(/>|</)"));
#endif
    QString s = QString::fromUtf8(text, int(size));

    switch (textType) {
    case MD_TEXT_NORMAL:
#if QT_CONFIG(regularexpression)
        if (m_htmlTagDepth) {
            m_htmlAccumulator += s;
            s = QString();
        }
#endif
        break;
    case MD_TEXT_NULLCHAR:
        s = QString(QChar(0xFFFD)); // CommonMark-required replacement for null
        break;
    case MD_TEXT_BR:
        s = QString(Newline);
        break;
    case MD_TEXT_SOFTBR:
        s = QString(Space);
        break;
    case MD_TEXT_CODE:
        // We'll see MD_SPAN_CODE too, which will set the char format, and that's enough.
        break;
#if QT_CONFIG(texthtmlparser)
    case MD_TEXT_ENTITY:
        m_cursor->insertHtml(s);
        s = QString();
        break;
#endif
    case MD_TEXT_HTML:
        // count how many tags are opened and how many are closed
#if QT_CONFIG(regularexpression) && QT_CONFIG(texthtmlparser)
        {
            int startIdx = 0;
            while ((startIdx = s.indexOf(openingBracket, startIdx)) >= 0) {
                ++m_htmlTagDepth;
                startIdx += 2;
            }
            startIdx = 0;
            while ((startIdx = s.indexOf(closingBracket, startIdx)) >= 0) {
                --m_htmlTagDepth;
                startIdx += 2;
            }
        }
        m_htmlAccumulator += s;
        if (!m_htmlTagDepth) { // all open tags are now closed
            qCDebug(lcMD) << "HTML" << m_htmlAccumulator;
            m_cursor->insertHtml(m_htmlAccumulator);
            if (m_spanFormatStack.isEmpty())
                m_cursor->setCharFormat(QTextCharFormat());
            else
                m_cursor->setCharFormat(m_spanFormatStack.top());
            m_htmlAccumulator = QString();
        }
#endif
        s = QString();
        break;
    }

    switch (m_blockType) {
    case MD_BLOCK_TD:
        m_nonEmptyTableCells.append(m_tableCol);
        break;
    default:
        break;
    }

    if (m_imageSpan) {
        // TODO we don't yet support alt text with formatting, because of the cases where m_cursor
        // already inserted the text above.  Rather need to accumulate it in case we need it here.
        m_imageFormat.setProperty(QTextFormat::ImageAltText, s);
        qCDebug(lcMD) << "image" << m_imageFormat.name()
                      << "title" << m_imageFormat.stringProperty(QTextFormat::ImageTitle)
                      << "alt" << s << "relative to" << m_doc->baseUrl();
        m_cursor->insertImage(m_imageFormat);
        return 0; // no error
    }

    if (!s.isEmpty())
        m_cursor->insertText(s);
    if (m_cursor->currentList()) {
        // The list item will indent the list item's text, so we don't need indentation on the block.
        QTextBlockFormat bfmt = m_cursor->blockFormat();
        bfmt.setIndent(0);
        m_cursor->setBlockFormat(bfmt);
    }
    if (lcMD().isEnabled(QtDebugMsg)) {
        QTextBlockFormat bfmt = m_cursor->blockFormat();
        QString debugInfo;
        if (m_cursor->currentList())
            debugInfo = QLatin1String("in list at depth ") + QString::number(m_cursor->currentList()->format().indent());
        if (bfmt.hasProperty(QTextFormat::BlockQuoteLevel))
            debugInfo += QLatin1String("in blockquote at depth ") +
                    QString::number(bfmt.intProperty(QTextFormat::BlockQuoteLevel));
        if (bfmt.hasProperty(QTextFormat::BlockCodeLanguage))
            debugInfo += QLatin1String("in a code block");
        qCDebug(lcMD) << textType << "in block" << m_blockType << s << qPrintable(debugInfo)
                      << "bindent" << bfmt.indent() << "tindent" << bfmt.textIndent()
                      << "margins" << bfmt.leftMargin() << bfmt.topMargin() << bfmt.bottomMargin() << bfmt.rightMargin();
    }
    qCDebug(lcMD) << textType << "in block" << m_blockType << s << "in list?" << m_cursor->currentList()
                  << "indent" << m_cursor->blockFormat().indent();
    return 0; // no error
}

/*!
    Insert a new block based on stored state.

    m_cursor cannot store the state for the _next_ block ahead of time, because
    m_cursor->setBlockFormat() controls the format of the block that the cursor
    is already in; so cbLeaveBlock() cannot call setBlockFormat() without
    altering the block that was just added. Therefore cbLeaveBlock() and the
    following cbEnterBlock() set variables to remember what formatting should
    come next, and insertBlock() is called just before the actual text
    insertion, to create a new block with the right formatting.
*/
void QTextMarkdownImporter::insertBlock()
{
    QTextCharFormat charFormat;
    if (!m_spanFormatStack.isEmpty())
        charFormat = m_spanFormatStack.top();
    QTextBlockFormat blockFormat;
    if (!m_listStack.isEmpty() && !m_needsInsertList && m_listItem) {
        QTextList *list = m_listStack.top();
        if (list)
            blockFormat = list->item(list->count() - 1).blockFormat();
        else
            qWarning() << "attempted to insert into a list that no longer exists";
    }
    if (m_blockQuoteDepth) {
        blockFormat.setProperty(QTextFormat::BlockQuoteLevel, m_blockQuoteDepth);
        blockFormat.setLeftMargin(BlockQuoteIndent * m_blockQuoteDepth);
        blockFormat.setRightMargin(BlockQuoteIndent);
    }
    if (m_codeBlock) {
        blockFormat.setProperty(QTextFormat::BlockCodeLanguage, m_blockCodeLanguage);
        if (m_blockCodeFence)
            blockFormat.setProperty(QTextFormat::BlockCodeFence, QString(QLatin1Char(m_blockCodeFence)));
        charFormat.setFont(m_monoFont);
    } else {
        blockFormat.setTopMargin(m_paragraphMargin);
        blockFormat.setBottomMargin(m_paragraphMargin);
    }
    if (m_markerType == QTextBlockFormat::MarkerType::NoMarker)
        blockFormat.clearProperty(QTextFormat::BlockMarker);
    else
        blockFormat.setMarker(m_markerType);
    if (!m_listStack.isEmpty())
        blockFormat.setIndent(m_listStack.count());
    if (m_doc->isEmpty()) {
        m_cursor->setBlockFormat(blockFormat);
        m_cursor->setCharFormat(charFormat);
    } else {
        m_cursor->insertBlock(blockFormat, charFormat);
    }
    if (m_needsInsertList) {
        m_listStack.push(m_cursor->createList(m_listFormat));
    } else if (!m_listStack.isEmpty() && m_listItem && m_listStack.top()) {
        m_listStack.top()->add(m_cursor->block());
    }
    m_needsInsertList = false;
    m_needsInsertBlock = false;
}

QT_END_NAMESPACE
