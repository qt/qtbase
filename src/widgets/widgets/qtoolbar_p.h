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

#ifndef QDYNAMICTOOLBAR_P_H
#define QDYNAMICTOOLBAR_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qtoolbar.h"
#include "QtWidgets/qaction.h"
#include "private/qwidget_p.h"
#include <QtCore/qbasictimer.h>

QT_REQUIRE_CONFIG(toolbar);

QT_BEGIN_NAMESPACE

class QToolBarLayout;
class QTimer;

class QToolBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QToolBar)

public:
    inline QToolBarPrivate()
        : explicitIconSize(false), explicitToolButtonStyle(false), movable(true), floatable(true),
          allowedAreas(Qt::AllToolBarAreas), orientation(Qt::Horizontal),
          toolButtonStyle(Qt::ToolButtonIconOnly),
          layout(nullptr), state(nullptr)
#ifdef Q_OS_MACOS
        , macWindowDragging(false)
#endif
    { }

    void init();
    void actionTriggered();
    void _q_toggleView(bool b);
    void _q_updateIconSize(const QSize &sz);
    void _q_updateToolButtonStyle(Qt::ToolButtonStyle style);

    bool explicitIconSize;
    bool explicitToolButtonStyle;
    bool movable;
    bool floatable;
    Qt::ToolBarAreas allowedAreas;
    Qt::Orientation orientation;
    Qt::ToolButtonStyle toolButtonStyle;
    QSize iconSize;

    QAction *toggleViewAction;

    QToolBarLayout *layout;

    struct DragState {
        QPoint pressPos;
        bool dragging;
        bool moving;
        QLayoutItem *widgetItem;
    };
    DragState *state;

#ifdef Q_OS_MACOS
    bool macWindowDragging;
    QPoint macWindowDragPressPosition;
#endif

    bool mousePressEvent(QMouseEvent *e);
    bool mouseReleaseEvent(QMouseEvent *e);
    bool mouseMoveEvent(QMouseEvent *e);

    void updateWindowFlags(bool floating, bool unplug = false);
    void setWindowState(bool floating, bool unplug = false, const QRect &rect = QRect());
    void initDrag(const QPoint &pos);
    void startDrag(bool moving = false);
    void endDrag();

    void unplug(const QRect &r);
    void plug(const QRect &r);

    QBasicTimer waitForPopupTimer;
};

QT_END_NAMESPACE

#endif // QDYNAMICTOOLBAR_P_H
