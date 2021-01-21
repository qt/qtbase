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

#ifndef QTEXTTABLE_P_H
#define QTEXTTABLE_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "private/qtextobject_p.h"
#include "private/qtextdocument_p.h"

#include <vector>

QT_BEGIN_NAMESPACE

class QTextTablePrivate : public QTextFramePrivate
{
    Q_DECLARE_PUBLIC(QTextTable)
public:
    QTextTablePrivate(QTextDocument *document) : QTextFramePrivate(document), nRows(0), nCols(0), dirty(true), blockFragmentUpdates(false) {}

    static QTextTable *createTable(QTextDocumentPrivate *, int pos, int rows, int cols, const QTextTableFormat &tableFormat);
    void fragmentAdded(QChar type, uint fragment) override;
    void fragmentRemoved(QChar type, uint fragment) override;

    void update() const;

    int findCellIndex(int fragment) const;

    QVector<int> cells;
    // symmetric to cells array and maps to indecs in grid,
    // used for fast-lookup for row/column by fragment
    mutable QVector<int> cellIndices;
    mutable std::vector<int> grid;
    mutable int nRows;
    mutable int nCols;
    mutable bool dirty;
    bool blockFragmentUpdates;
};

QT_END_NAMESPACE

#endif // QTEXTTABLE_P_H
