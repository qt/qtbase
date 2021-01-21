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

#ifndef QDRAG_H
#define QDRAG_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(draganddrop);

QT_BEGIN_NAMESPACE

class QMimeData;
class QDragPrivate;
class QPixmap;
class QPoint;
class QDragManager;


class Q_GUI_EXPORT QDrag : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QDrag)
public:
    explicit QDrag(QObject *dragSource);
    ~QDrag();

    void setMimeData(QMimeData *data);
    QMimeData *mimeData() const;

    void setPixmap(const QPixmap &);
    QPixmap pixmap() const;

    void setHotSpot(const QPoint &hotspot);
    QPoint hotSpot() const;

    QObject *source() const;
    QObject *target() const;

#if QT_DEPRECATED_SINCE(5, 13)
    QT_DEPRECATED_X("Use QDrag::exec() instead")
    Qt::DropAction start(Qt::DropActions supportedActions = Qt::CopyAction);
#endif
    Qt::DropAction exec(Qt::DropActions supportedActions = Qt::MoveAction);
    Qt::DropAction exec(Qt::DropActions supportedActions, Qt::DropAction defaultAction);

    void setDragCursor(const QPixmap &cursor, Qt::DropAction action);
    QPixmap dragCursor(Qt::DropAction action) const;

    Qt::DropActions supportedActions() const;
    Qt::DropAction defaultAction() const;

    static void cancel();

Q_SIGNALS:
    void actionChanged(Qt::DropAction action);
    void targetChanged(QObject *newTarget);

private:
    friend class QDragManager;
    Q_DISABLE_COPY(QDrag)
};

QT_END_NAMESPACE

#endif // QDRAG_H
