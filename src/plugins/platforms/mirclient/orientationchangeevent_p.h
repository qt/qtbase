/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ORIENTATIONCHANGEEVENT_P_H
#define ORIENTATIONCHANGEEVENT_P_H

#include <QEvent>
#include <QOrientationReading>
#include "logging.h"

class OrientationChangeEvent : public QEvent {
public:
    OrientationChangeEvent(QEvent::Type type, QOrientationReading::Orientation orientation)
        : QEvent(type)
        , mOrientation(orientation)
    {
    }

    static const QEvent::Type mType;
    QOrientationReading::Orientation mOrientation;
};

#endif // ORIENTATIONCHANGEEVENT_P_H
