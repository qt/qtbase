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

#ifndef QTEXTMARKDOWNIMPORTER_H
#define QTEXTMARKDOWNIMPORTER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qfont.h>
#include <QtGui/qtguiglobal.h>
#include <QtGui/qpalette.h>
#include <QtGui/qtextlist.h>
#include <QtCore/qstack.h>

#include "../../3rdparty/md4c/md4c.h"

QT_BEGIN_NAMESPACE

class QTextCursor;
class QTextDocument;
class QTextTable;

class Q_GUI_EXPORT QTextMarkdownImporter
{
public:
    enum Feature {
        FeatureCollapseWhitespace = MD_FLAG_COLLAPSEWHITESPACE,
        FeaturePermissiveATXHeaders = MD_FLAG_PERMISSIVEATXHEADERS,
        FeaturePermissiveURLAutoLinks = MD_FLAG_PERMISSIVEURLAUTOLINKS,
        FeaturePermissiveMailAutoLinks = MD_FLAG_PERMISSIVEEMAILAUTOLINKS,
        FeatureNoIndentedCodeBlocks = MD_FLAG_NOINDENTEDCODEBLOCKS,
        FeatureNoHTMLBlocks = MD_FLAG_NOHTMLBLOCKS,
        FeatureNoHTMLSpans = MD_FLAG_NOHTMLSPANS,
        FeatureTables = MD_FLAG_TABLES,
        FeatureStrikeThrough = MD_FLAG_STRIKETHROUGH,
        FeaturePermissiveWWWAutoLinks = MD_FLAG_PERMISSIVEWWWAUTOLINKS,
        FeatureTasklists = MD_FLAG_TASKLISTS,
        // composite flags
        FeaturePermissiveAutoLinks = MD_FLAG_PERMISSIVEAUTOLINKS,
        FeatureNoHTML = MD_FLAG_NOHTML,
        DialectCommonMark = MD_DIALECT_COMMONMARK,
        DialectGitHub = MD_DIALECT_GITHUB
    };
    Q_DECLARE_FLAGS(Features, Feature)

    QTextMarkdownImporter(Features features);

    void import(QTextDocument *doc, const QString &markdown);

public:
    // MD4C callbacks
    int cbEnterBlock(MD_BLOCKTYPE type, void* detail);
    int cbLeaveBlock(MD_BLOCKTYPE type, void* detail);
    int cbEnterSpan(MD_SPANTYPE type, void* detail);
    int cbLeaveSpan(MD_SPANTYPE type, void* detail);
    int cbText(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size);

private:
    QTextDocument *m_doc = nullptr;
    QTextCursor *m_cursor = nullptr;
    QTextTable *m_currentTable = nullptr; // because m_cursor->currentTable() doesn't work
    QString m_htmlAccumulator;
    QVector<int> m_nonEmptyTableCells; // in the current row
    QStack<QTextList *> m_listStack;
    QStack<QTextCharFormat> m_spanFormatStack;
    QFont m_monoFont;
    QPalette m_palette;
    int m_htmlTagDepth = 0;
    int m_tableColumnCount = 0;
    int m_tableRowCount = 0;
    int m_tableCol = -1; // because relative cell movements (e.g. m_cursor->movePosition(QTextCursor::NextCell)) don't work
    Features m_features;
    MD_BLOCKTYPE m_blockType = MD_BLOCK_DOC;
    bool m_emptyList = false; // true when the last thing we did was insertList
    bool m_imageSpan = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTextMarkdownImporter::Features)

QT_END_NAMESPACE

#endif // QTEXTMARKDOWNIMPORTER_H
