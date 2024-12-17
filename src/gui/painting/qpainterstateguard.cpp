// Copyright (C) 2024 Christian Ehrlicher <ch.ehrlicher@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpainterstateguard.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPainterStateGuard
    \brief The QPainterStateGuard is a RAII convenience class for balanced
    QPainter::save() and QPainter::restore() calls.
    \since 6.9

    \inmodule QtGui
    \ingroup painting

    \reentrant

    \sa QPainter

    QPainterStateGuard should be used everywhere as a replacement for QPainter::save()
    to make sure that the corresponding QPainter::restore() is called upon finishing
    of the painting routine to avoid unbalanced calls between those two functions.

    Example with QPainter::save()/QPainter::restore():
    \snippet code/src_gui_painting_qpainterstateguard.cpp 0

    Example with QPainterStateGuard:
    \snippet code/src_gui_painting_qpainterstateguard.cpp 1
*/

/*!
    \fn QPainterStateGuard::QPainterStateGuard(QPainter *painter, InitialState state = InitialState::Save)
    Constructs a QPainterStateGuard and calls save() on \a painter if \a state
    is \c InitialState::Save (which is the default). When QPainterStateGuard is
    destroyed, restore() is called as often as save() was called to restore the
    QPainter's state.
*/

/*!
    \fn QPainterStateGuard::QPainterStateGuard(QPainterStateGuard &&other)

    Move-constructs a painter state guard from \a other.
*/

/*!
    \fn QPainterStateGuard &QPainterStateGuard::operator=(QPainterStateGuard &&other)

    Move-assigns \a other to this painter state guard.
*/

/*!
    \fn void QPainterStateGuard::swap(QPainterStateGuard &other)

    Swaps the \a other with this painter state guard. This operation is very
    fast and never fails.
*/

/*!
    \fn QPainterStateGuard::~QPainterStateGuard()
    Destroys the QPainterStateGuard instance and calls restore() as often as save()
    was called to restore the QPainter's state.
*/

/*!
    \fn void QPainterStateGuard::save()
    Calls QPainter::save() and increases the internal save/restore counter by one.
*/

/*!
    \fn void QPainterStateGuard::restore()
    Calls QPainter::restore() if the internal save/restore counter is greater than zero.

    \note This function asserts in debug builds if the counter has already reached zero.
*/

QT_END_NAMESPACE
