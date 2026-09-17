// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTMARKDOWNWRITER_P_H
#define QTEXTMARKDOWNWRITER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QTextStream>

#include "qtextdocument_p.h"
#include "qtextdocumentwriter.h"

QT_BEGIN_NAMESPACE

class QAbstractItemModel;
class QTextTableCell;

class Q_GUI_EXPORT QTextMarkdownWriter
{
public:
    QTextMarkdownWriter(QTextStream &stream, QTextDocument::MarkdownFeatures features);
    bool writeAll(const QTextDocument *document);
#if QT_CONFIG(itemmodel)
    void writeTable(const QAbstractItemModel *table);
#endif

    int writeBlock(const QTextBlock &block, bool table, bool ignoreFormat, bool ignoreEmpty);
    void writeFrame(const QTextFrame *frame);
    void writeFrontMatter(const QString &fm);

private:
    struct ListInfo {
        bool loose;
    };

    ListInfo listInfo(QTextList *list);
    void setLinePrefixForBlockQuote(int level);
    int measureCellWidth(const QTextTableCell &cell, bool ignoreFormat);

private:
    // The part of the writer's state that writeBlock() mutates as it goes.
    // Grouped together so that measureCellWidth() can save, reset and restore
    // it in one go.
    struct BlockState {
        QString linePrefix;
        QString codeBlockFence;
        int wrappedLineIndent = 0;
        int lastListIndent = 1;
        bool doubleNewlineWritten = false;
        bool linePrefixWritten = false;
        bool indentedCodeBlock = false;
        bool fencedCodeBlock = false;
    };

    // A pointer rather than a reference, because measureCellWidth() temporarily
    // redirects output to a scratch stream; but it is never null.
    QTextStream *m_stream;

    QTextDocument::MarkdownFeatures m_features;
    QMap<QTextList *, ListInfo> m_listInfo;
    BlockState m_blockState;
};

QT_END_NAMESPACE

#endif // QTEXTMARKDOWNWRITER_P_H
