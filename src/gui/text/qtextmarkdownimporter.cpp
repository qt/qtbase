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
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextList>
#include <QTextTable>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcMD, "qt.text.markdown")

// --------------------------------------------------------
// MD4C callback function wrappers

static int CbEnterBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbEnterBlock(type, detail);
}

static int CbLeaveBlock(MD_BLOCKTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbLeaveBlock(type, detail);
}

static int CbEnterSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbEnterSpan(type, detail);
}

static int CbLeaveSpan(MD_SPANTYPE type, void *detail, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbLeaveSpan(type, detail);
}

static int CbText(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata)
{
    QTextMarkdownImporter *mdi = static_cast<QTextMarkdownImporter *>(userdata);
    return mdi->cbText(type, text, size);
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
    m_cursor = new QTextCursor(doc);
    doc->clear();
    qCDebug(lcMD) << "default font" << doc->defaultFont() << "mono font" << m_monoFont;
    QByteArray md = markdown.toUtf8();
    md_parse(md.constData(), md.size(), &callbacks, this);
    delete m_cursor;
    m_cursor = nullptr;
}

int QTextMarkdownImporter::cbEnterBlock(MD_BLOCKTYPE type, void *det)
{
    m_blockType = type;
    switch (type) {
    case MD_BLOCK_P: {
        QTextBlockFormat blockFmt;
        int margin = m_doc->defaultFont().pointSize() / 2;
        blockFmt.setTopMargin(margin);
        blockFmt.setBottomMargin(margin);
        m_cursor->insertBlock(blockFmt, QTextCharFormat());
    } break;
    case MD_BLOCK_CODE: {
        QTextBlockFormat blockFmt;
        QTextCharFormat charFmt;
        charFmt.setFont(m_monoFont);
        m_cursor->insertBlock(blockFmt, charFmt);
    } break;
    case MD_BLOCK_H: {
        MD_BLOCK_H_DETAIL *detail = static_cast<MD_BLOCK_H_DETAIL *>(det);
        QTextBlockFormat blockFmt;
        QTextCharFormat charFmt;
        int sizeAdjustment = 4 - detail->level; // H1 to H6: +3 to -2
        charFmt.setProperty(QTextFormat::FontSizeAdjustment, sizeAdjustment);
        charFmt.setFontWeight(QFont::Bold);
        blockFmt.setHeadingLevel(detail->level);
        m_cursor->insertBlock(blockFmt, charFmt);
    } break;
    case MD_BLOCK_LI: {
        MD_BLOCK_LI_DETAIL *detail = static_cast<MD_BLOCK_LI_DETAIL *>(det);
        QTextList *list = m_listStack.top();
        QTextBlockFormat bfmt = list->item(list->count() - 1).blockFormat();
        bfmt.setMarker(detail->is_task ?
                           (detail->task_mark == ' ' ? QTextBlockFormat::Unchecked : QTextBlockFormat::Checked) :
                           QTextBlockFormat::NoMarker);
        if (!m_emptyList) {
            m_cursor->insertBlock(bfmt, QTextCharFormat());
            list->add(m_cursor->block());
        }
        m_cursor->setBlockFormat(bfmt);
        m_emptyList = false; // Avoid insertBlock for the first item (because insertList already did that)
    } break;
    case MD_BLOCK_UL: {
        MD_BLOCK_UL_DETAIL *detail = static_cast<MD_BLOCK_UL_DETAIL *>(det);
        QTextListFormat fmt;
        fmt.setIndent(m_listStack.count() + 1);
        switch (detail->mark) {
        case '*':
            fmt.setStyle(QTextListFormat::ListCircle);
            break;
        case '+':
            fmt.setStyle(QTextListFormat::ListSquare);
            break;
        default: // including '-'
            fmt.setStyle(QTextListFormat::ListDisc);
            break;
        }
        m_listStack.push(m_cursor->insertList(fmt));
        m_emptyList = true;
    } break;
    case MD_BLOCK_OL: {
        MD_BLOCK_OL_DETAIL *detail = static_cast<MD_BLOCK_OL_DETAIL *>(det);
        QTextListFormat fmt;
        fmt.setIndent(m_listStack.count() + 1);
        fmt.setNumberSuffix(QChar::fromLatin1(detail->mark_delimiter));
        fmt.setStyle(QTextListFormat::ListDecimal);
        m_listStack.push(m_cursor->insertList(fmt));
        m_emptyList = true;
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
        QTextBlockFormat blockFmt = m_cursor->blockFormat();
        blockFmt.setProperty(QTextFormat::BlockTrailingHorizontalRulerWidth, 1);
        m_cursor->insertBlock(blockFmt, QTextCharFormat());
    } break;
    default:
        break; // nothing to do for now
    }
    return 0; // no error
}

int QTextMarkdownImporter::cbLeaveBlock(MD_BLOCKTYPE type, void *detail)
{
    Q_UNUSED(detail)
    switch (type) {
    case MD_BLOCK_UL:
    case MD_BLOCK_OL:
        m_listStack.pop();
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
        QTextBlockFormat blockFmt = m_cursor->blockFormat();
        blockFmt.setIndent(1);
        m_cursor->setBlockFormat(blockFmt);
    } break;
    case MD_BLOCK_TABLE:
        qCDebug(lcMD) << "table ended with" << m_currentTable->columns() << "cols and" << m_currentTable->rows() << "rows";
        m_currentTable = nullptr;
        m_cursor->movePosition(QTextCursor::End);
        break;
    default:
        break;
    }
    return 0; // no error
}

int QTextMarkdownImporter::cbEnterSpan(MD_SPANTYPE type, void *det)
{
    QTextCharFormat charFmt;
    switch (type) {
    case MD_SPAN_EM:
        charFmt.setFontItalic(true);
        break;
    case MD_SPAN_STRONG:
        charFmt.setFontWeight(QFont::Bold);
        break;
    case MD_SPAN_A: {
        MD_SPAN_A_DETAIL *detail = static_cast<MD_SPAN_A_DETAIL *>(det);
        QString url = QString::fromLatin1(detail->href.text, detail->href.size);
        QString title = QString::fromLatin1(detail->title.text, detail->title.size);
        charFmt.setAnchorHref(url);
        charFmt.setAnchorNames(QStringList(title));
        charFmt.setForeground(m_palette.link());
        qCDebug(lcMD) << "anchor" << url << title;
        } break;
    case MD_SPAN_IMG: {
        m_imageSpan = true;
        MD_SPAN_IMG_DETAIL *detail = static_cast<MD_SPAN_IMG_DETAIL *>(det);
        QString src = QString::fromUtf8(detail->src.text, detail->src.size);
        QString title = QString::fromUtf8(detail->title.text, detail->title.size);
        QTextImageFormat img;
        img.setName(src);
        qCDebug(lcMD) << "image" << src << "title" << title << "relative to" << m_doc->baseUrl();
        m_cursor->insertImage(img);
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
    m_cursor->setCharFormat(charFmt);
    return 0; // no error
}

int QTextMarkdownImporter::cbLeaveSpan(MD_SPANTYPE type, void *detail)
{
    Q_UNUSED(detail)
    QTextCharFormat charFmt;
    if (!m_spanFormatStack.isEmpty()) {
        m_spanFormatStack.pop();
        if (!m_spanFormatStack.isEmpty())
            charFmt = m_spanFormatStack.top();
    }
    m_cursor->setCharFormat(charFmt);
    if (type == MD_SPAN_IMG)
        m_imageSpan = false;
    return 0; // no error
}

int QTextMarkdownImporter::cbText(MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size)
{
    if (m_imageSpan)
        return 0; // it's the alt-text
    static const QRegularExpression openingBracket(QStringLiteral("<[a-zA-Z]"));
    static const QRegularExpression closingBracket(QStringLiteral("(/>|</)"));
    QString s = QString::fromUtf8(text, size);

    switch (type) {
    case MD_TEXT_NORMAL:
        if (m_htmlTagDepth) {
            m_htmlAccumulator += s;
            s = QString();
        }
        break;
    case MD_TEXT_NULLCHAR:
        s = QString(QChar(0xFFFD)); // CommonMark-required replacement for null
        break;
    case MD_TEXT_BR:
        s = QLatin1String("\n");
        break;
    case MD_TEXT_SOFTBR:
        s = QLatin1String(" ");
        break;
    case MD_TEXT_CODE:
        // We'll see MD_SPAN_CODE too, which will set the char format, and that's enough.
        break;
    case MD_TEXT_ENTITY:
        m_cursor->insertHtml(s);
        s = QString();
        break;
    case MD_TEXT_HTML:
        // count how many tags are opened and how many are closed
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
        s = QString();
        if (!m_htmlTagDepth) { // all open tags are now closed
            qCDebug(lcMD) << "HTML" << m_htmlAccumulator;
            m_cursor->insertHtml(m_htmlAccumulator);
            if (m_spanFormatStack.isEmpty())
                m_cursor->setCharFormat(QTextCharFormat());
            else
                m_cursor->setCharFormat(m_spanFormatStack.top());
            m_htmlAccumulator = QString();
        }
        break;
    }

    switch (m_blockType) {
    case MD_BLOCK_TD:
        m_nonEmptyTableCells.append(m_tableCol);
        break;
    default:
        break;
    }

    if (!s.isEmpty())
        m_cursor->insertText(s);
    return 0; // no error
}

QT_END_NAMESPACE
