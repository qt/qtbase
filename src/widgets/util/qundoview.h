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

#ifndef QUNDOVIEW_H
#define QUNDOVIEW_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qlistview.h>
#include <QtCore/qstring.h>

QT_REQUIRE_CONFIG(undoview);

QT_BEGIN_NAMESPACE

class QUndoViewPrivate;
class QUndoStack;
class QUndoGroup;
class QIcon;


class Q_WIDGETS_EXPORT QUndoView : public QListView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QUndoView)
    Q_PROPERTY(QString emptyLabel READ emptyLabel WRITE setEmptyLabel)
    Q_PROPERTY(QIcon cleanIcon READ cleanIcon WRITE setCleanIcon)

public:
    explicit QUndoView(QWidget *parent = nullptr);
    explicit QUndoView(QUndoStack *stack, QWidget *parent = nullptr);
#if QT_CONFIG(undogroup)
    explicit QUndoView(QUndoGroup *group, QWidget *parent = nullptr);
#endif
    ~QUndoView();

    QUndoStack *stack() const;
#if QT_CONFIG(undogroup)
    QUndoGroup *group() const;
#endif

    void setEmptyLabel(const QString &label);
    QString emptyLabel() const;

    void setCleanIcon(const QIcon &icon);
    QIcon cleanIcon() const;

public Q_SLOTS:
    void setStack(QUndoStack *stack);
#if QT_CONFIG(undogroup)
    void setGroup(QUndoGroup *group);
#endif

private:
    Q_DISABLE_COPY(QUndoView)
};

QT_END_NAMESPACE

#endif // QUNDOVIEW_H
