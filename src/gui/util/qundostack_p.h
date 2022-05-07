/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QUNDOSTACK_P_H
#define QUNDOSTACK_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <private/qobject_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#if QT_CONFIG(action)
#  include <QtGui/qaction.h>
#endif

#include "qundostack.h"

QT_BEGIN_NAMESPACE
class QUndoCommand;
class QUndoGroup;

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

class QUndoCommandPrivate
{
public:
    QUndoCommandPrivate() : id(-1), obsolete(false) {}
    QList<QUndoCommand*> child_list;
    QString text;
    QString actionText;
    int id;
    bool obsolete;
};

#if QT_CONFIG(undostack)

class QUndoStackPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QUndoStack)
public:
    QUndoStackPrivate() : index(0), clean_index(0), group(nullptr), undo_limit(0) {}

    QList<QUndoCommand*> command_list;
    QList<QUndoCommand*> macro_stack;
    int index;
    int clean_index;
    QUndoGroup *group;
    int undo_limit;

    void setIndex(int idx, bool clean);
    bool checkUndoLimit();

#ifndef QT_NO_ACTION
    static void setPrefixedText(QAction *action, const QString &prefix, const QString &defaultText, const QString &text);
#endif
};

QT_END_NAMESPACE
#endif // QT_CONFIG(undostack)
#endif // QUNDOSTACK_P_H
