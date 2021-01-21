/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QUNDOSTACK_P_H
#define QUNDOSTACK_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <private/qobject_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtWidgets/qaction.h>

#include "qundostack.h"

QT_BEGIN_NAMESPACE
class QUndoCommand;
class QUndoGroup;

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
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
};

#ifndef QT_NO_ACTION
class QUndoAction : public QAction
{
    Q_OBJECT
public:
    explicit QUndoAction(const QString &prefix, QObject *parent = nullptr);
    void setTextFormat(const QString &textFormat, const QString &defaultText);
public Q_SLOTS:
    void setPrefixedText(const QString &text);
private:
    QString m_prefix;
    QString m_defaultText;
};
#endif // QT_NO_ACTION

QT_END_NAMESPACE
#endif // QT_CONFIG(undostack)
#endif // QUNDOSTACK_P_H
