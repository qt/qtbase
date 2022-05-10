// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdrag.h"

#include <QtGui/private/qdnd_p.h>
#include <QtGui/QKeyEvent>
#include <QtGui/QGuiApplication>
#include <QtCore/QEventLoop>

QT_BEGIN_NAMESPACE

#ifdef QDND_DEBUG
#  include <QtCore/QDebug>
#endif

QPlatformDropQtResponse::QPlatformDropQtResponse(bool accepted, Qt::DropAction acceptedAction)
    : m_accepted(accepted)
    , m_accepted_action(acceptedAction)
{
}

bool QPlatformDropQtResponse::isAccepted() const
{
    return m_accepted;
}

Qt::DropAction QPlatformDropQtResponse::acceptedAction() const
{
    return m_accepted_action;
}

QPlatformDragQtResponse::QPlatformDragQtResponse(bool accepted, Qt::DropAction acceptedAction, QRect answerRect)
    : QPlatformDropQtResponse(accepted,acceptedAction)
    , m_answer_rect(answerRect)
{
}

QRect QPlatformDragQtResponse::answerRect() const
{
    return m_answer_rect;
}

class QPlatformDragPrivate {
public:
    QPlatformDragPrivate() : cursor_drop_action(Qt::IgnoreAction) {}

    Qt::DropAction cursor_drop_action;
};

/*!
    \class QPlatformDrag
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformDrag class provides an abstraction for drag.
 */
QPlatformDrag::QPlatformDrag() : d_ptr(new QPlatformDragPrivate)
{
}

QPlatformDrag::~QPlatformDrag()
{
    delete d_ptr;
}

QDrag *QPlatformDrag::currentDrag() const
{
    return QDragManager::self()->object();
}

Qt::DropAction QPlatformDrag::defaultAction(Qt::DropActions possibleActions,
                                           Qt::KeyboardModifiers modifiers) const
{
#ifdef QDND_DEBUG
    qDebug() << "QDragManager::defaultAction(Qt::DropActions possibleActions)\nkeyboard modifiers : " << modifiers;
#endif

    Qt::DropAction default_action = Qt::IgnoreAction;

    if (currentDrag()) {
        default_action = currentDrag()->defaultAction();
    }


    if (default_action == Qt::IgnoreAction) {
        //This means that the drag was initiated by QDrag::start and we need to
        //preserve the old behavior
        default_action = Qt::CopyAction;
    }

    if (modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier)
        default_action = Qt::LinkAction;
    else if (modifiers & Qt::ControlModifier)
        default_action = Qt::CopyAction;
    else if (modifiers & Qt::ShiftModifier)
        default_action = Qt::MoveAction;
    else if (modifiers & Qt::AltModifier)
        default_action = Qt::LinkAction;

#ifdef QDND_DEBUG
    qDebug() << "possible actions : " << possibleActions;
#endif

    // Check if the action determined is allowed
    if (!(possibleActions & default_action)) {
        if (possibleActions & Qt::CopyAction)
            default_action = Qt::CopyAction;
        else if (possibleActions & Qt::MoveAction)
            default_action = Qt::MoveAction;
        else if (possibleActions & Qt::LinkAction)
            default_action = Qt::LinkAction;
        else
            default_action = Qt::IgnoreAction;
    }

#ifdef QDND_DEBUG
    qDebug() << "default action : " << default_action;
#endif

    return default_action;
}

/*!
    \brief Cancels the currently active drag (only for drags of
    the current application initiated by QPlatformDrag::drag()).

    The default implementation does nothing.

    \since 5.7
 */

void QPlatformDrag::cancelDrag()
{
    Q_UNIMPLEMENTED();
}

/*!
    \brief Called to notify QDrag about changes of the current action.
 */

void QPlatformDrag::updateAction(Qt::DropAction action)
{
    Q_D(QPlatformDrag);
    if (d->cursor_drop_action != action) {
        d->cursor_drop_action = action;
        emit currentDrag()->actionChanged(action);
    }
}

static const char *const default_pm[] = {
"13 9 3 1",
".      c None",
"       c #000000",
"X      c #FFFFFF",
"X X X X X X X",
" X X X X X X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X X X X X X ",
"X X X X X X X",
};

Q_GLOBAL_STATIC_WITH_ARGS(QPixmap,qt_drag_default_pixmap,(default_pm))

QPixmap QPlatformDrag::defaultPixmap()
{
    return *qt_drag_default_pixmap();
}

/*!
    \since 5.4
    \brief Returns bool indicating whether QPlatformDrag takes ownership
    and therefore responsibility of deleting the QDrag object passed in
    from QPlatformDrag::drag. This can be useful on platforms where QDrag
    object has to be kept around.
 */
bool QPlatformDrag::ownsDragObject() const
{
    return false;
}

QT_END_NAMESPACE
