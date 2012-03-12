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

#ifndef QPLATFORMDRAG_H
#define QPLATFORMDRAG_H

#include <QtCore/qglobal.h>
#include <QtGui/QPixmap>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QMimeData;
class QMouseEvent;
class QDrag;
class QObject;
class QEvent;
class QPlatformDragPrivate;

class Q_GUI_EXPORT QPlatformDropQtResponse
{
public:
    QPlatformDropQtResponse(bool accepted, Qt::DropAction acceptedAction);
    bool isAccepted() const;
    Qt::DropAction acceptedAction() const;

private:
    bool m_accepted;
    Qt::DropAction m_accepted_action;

};

class Q_GUI_EXPORT QPlatformDragQtResponse : public QPlatformDropQtResponse
{
public:
    QPlatformDragQtResponse(bool accepted, Qt::DropAction acceptedAction, QRect answerRect);

    QRect answerRect() const;

private:
    QRect m_answer_rect;
};

class Q_GUI_EXPORT QPlatformDrag
{
    Q_DECLARE_PRIVATE(QPlatformDrag)
public:
    QPlatformDrag();
    virtual ~QPlatformDrag();

    QDrag *currentDrag() const;
    virtual QMimeData *platformDropData() = 0;

    virtual Qt::DropAction drag(QDrag *m_drag) = 0;
    void updateAction(Qt::DropAction action);

    Qt::DropAction defaultAction(Qt::DropActions possibleActions, Qt::KeyboardModifiers modifiers) const;

    static QPixmap defaultPixmap();

private:
    QPlatformDragPrivate *d_ptr;

    Q_DISABLE_COPY(QPlatformDrag)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
