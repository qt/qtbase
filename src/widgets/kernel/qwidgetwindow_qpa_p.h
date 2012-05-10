/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWIDGETWINDOW_QPA_P_H
#define QWIDGETWINDOW_QPA_P_H

#include <QtGui/qwindow.h>

#include <QtCore/private/qobject_p.h>
#include <QtGui/private/qevent_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QCloseEvent;
class QMoveEvent;

class QWidgetWindow : public QWindow
{
    Q_OBJECT
public:
    QWidgetWindow(QWidget *widget);

    QWidget *widget() const { return m_widget; }
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleInterface *accessibleRoot() const;
#endif

    QObject *focusObject() const;
protected:
    bool event(QEvent *);

    void handleCloseEvent(QCloseEvent *);
    void handleEnterLeaveEvent(QEvent *);
    void handleKeyEvent(QKeyEvent *);
    void handleMouseEvent(QMouseEvent *);
    void handleTouchEvent(QTouchEvent *);
    void handleMoveEvent(QMoveEvent *);
    void handleResizeEvent(QResizeEvent *);
    void handleWheelEvent(QWheelEvent *);
    void handleDragEvent(QEvent *);
    void handleExposeEvent(QExposeEvent *);
    void handleWindowStateChangedEvent(QWindowStateChangeEvent *event);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private slots:
    void updateObjectName();

private:
    void updateGeometry();

    QWidget *m_widget;
    QWeakPointer<QWidget> m_implicit_mouse_grabber;
    QWeakPointer<QWidget> m_dragTarget;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWIDGETWINDOW_QPA_P_H
