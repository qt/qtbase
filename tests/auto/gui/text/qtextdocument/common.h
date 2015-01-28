/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
