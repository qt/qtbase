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
#include <QAbstractTextDocumentLayout>
#include <private/qtextdocument_p.h>

#ifndef COMMON_H
#define COMMON_H

class QTestDocumentLayout : public QAbstractTextDocumentLayout
{
    Q_OBJECT
public:
    QTestDocumentLayout(QTextDocument *doc) : QAbstractTextDocumentLayout(doc), f(-1), called(false) {}
    virtual void draw(QPainter *, const PaintContext &)  {}
    virtual int hitTest(const QPointF &, Qt::HitTestAccuracy ) const { return 0; }

    virtual void documentChanged(int from, int oldLength, int length)
    {
        called = true;
        lastDocumentLengths.append(document()->docHandle()->length());

        if (f < 0)
            return;

        if(from != f ||
                o != oldLength ||
                l != length) {
            qDebug("checkDocumentChanged: got %d %d %d, expected %d %d %d", from, oldLength, length, f, o, l);
            error = true;
        }
    }

    virtual int pageCount() const { return 1; }
    virtual QSizeF documentSize() const { return QSizeF(); }

    virtual QRectF frameBoundingRect(QTextFrame *) const { return QRectF(); }
    virtual QRectF blockBoundingRect(const QTextBlock &) const { return QRectF(); }

    int f;
    int o;
    int l;

    void expect(int from, int oldLength, int length) {
        f = from;
        o = oldLength;
        l = length;
        error = false;
        called = false;
    }
    bool error;
    bool called;
    QList<int> lastDocumentLengths;
};

#endif
