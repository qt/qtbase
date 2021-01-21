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

#ifndef QTEXTOBJECT_P_H
#define QTEXTOBJECT_P_H

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
#include "QtGui/qtextobject.h"
#include "private/qobject_p.h"
#include "QtGui/qtextdocument.h"

QT_BEGIN_NAMESPACE

class QTextDocumentPrivate;

class QTextObjectPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTextObject)
public:
    QTextObjectPrivate(QTextDocument *doc)
        : pieceTable(doc->d_func()), objectIndex(-1)
    {
    }
    QTextDocumentPrivate *pieceTable;
    int objectIndex;
};

class QTextBlockGroupPrivate : public QTextObjectPrivate
{
    Q_DECLARE_PUBLIC(QTextBlockGroup)
public:
    QTextBlockGroupPrivate(QTextDocument *doc)
        : QTextObjectPrivate(doc)
    {
    }
    typedef QList<QTextBlock> BlockList;
    BlockList blocks;
    void markBlocksDirty();
};

class QTextFrameLayoutData;

class QTextFramePrivate : public QTextObjectPrivate
{
    friend class QTextDocumentPrivate;
    Q_DECLARE_PUBLIC(QTextFrame)
public:
    QTextFramePrivate(QTextDocument *doc)
        : QTextObjectPrivate(doc), fragment_start(0), fragment_end(0), parentFrame(nullptr), layoutData(nullptr)
    {
    }
    virtual void fragmentAdded(QChar type, uint fragment);
    virtual void fragmentRemoved(QChar type, uint fragment);
    void remove_me();

    uint fragment_start;
    uint fragment_end;

    QTextFrame *parentFrame;
    QList<QTextFrame *> childFrames;
    QTextFrameLayoutData *layoutData;
};

QT_END_NAMESPACE

#endif // QTEXTOBJECT_P_H
