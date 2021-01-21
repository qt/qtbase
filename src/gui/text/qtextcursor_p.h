/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTEXTCURSOR_P_H
#define QTEXTCURSOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qtextcursor.h"
#include "qtextdocument.h"
#include "qtextdocument_p.h"
#include <private/qtextformat_p.h>
#include "qtextobject.h"

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QTextCursorPrivate : public QSharedData
{
public:
    QTextCursorPrivate(QTextDocumentPrivate *p);
    QTextCursorPrivate(const QTextCursorPrivate &rhs);
    ~QTextCursorPrivate();

    static inline QTextCursorPrivate *getPrivate(QTextCursor *c) { return c->d; }

    enum AdjustResult { CursorMoved, CursorUnchanged };
    AdjustResult adjustPosition(int positionOfChange, int charsAddedOrRemoved, QTextUndoCommand::Operation op);

    void adjustCursor(QTextCursor::MoveOperation m);

    void remove();
    void clearCells(QTextTable *table, int startRow, int startCol, int numRows, int numCols, QTextUndoCommand::Operation op);
    inline bool setPosition(int newPosition) {
        Q_ASSERT(newPosition >= 0 && newPosition < priv->length());
        bool moved = position != newPosition;
        if (moved) {
            position = newPosition;
            currentCharFormat = -1;
        }
        return moved;
    }
    void setX();
    bool canDelete(int pos) const;

    void insertBlock(const QTextBlockFormat &format, const QTextCharFormat &charFormat);
    bool movePosition(QTextCursor::MoveOperation op, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);

    inline QTextBlock block() const
        { return QTextBlock(priv, priv->blockMap().findNode(position)); }
    inline QTextBlockFormat blockFormat() const
        { return block().blockFormat(); }

    QTextLayout *blockLayout(QTextBlock &block) const;

    QTextTable *complexSelectionTable() const;
    void selectedTableCells(int *firstRow, int *numRows, int *firstColumn, int *numColumns) const;

    void setBlockCharFormat(const QTextCharFormat &format, QTextDocumentPrivate::FormatChangeMode changeMode);
    void setBlockFormat(const QTextBlockFormat &format, QTextDocumentPrivate::FormatChangeMode changeMode);
    void setCharFormat(const QTextCharFormat &format, QTextDocumentPrivate::FormatChangeMode changeMode);

    void aboutToRemoveCell(int from, int to);

    static QTextCursor fromPosition(QTextDocumentPrivate *d, int pos)
    { return QTextCursor(d, pos); }

    QTextDocumentPrivate *priv;
    qreal x;
    int position;
    int anchor;
    int adjusted_anchor;
    int currentCharFormat;
    uint visualNavigation : 1;
    uint keepPositionOnInsert : 1;
    uint changed : 1;
};

QT_END_NAMESPACE

#endif // QTEXTCURSOR_P_H
