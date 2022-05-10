// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>

#include "filterobject.h"

FilterObject::FilterObject(QObject *parent)
    : QObject(parent), target(0)
{
}

//! [0]
bool FilterObject::eventFilter(QObject *object, QEvent *event)
{
    if (object == target && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            // Special tab handling
            return true;
        } else
            return false;
    }
    return false;
}
//! [0]

void FilterObject::setFilteredObject(QObject *object)
{
    if (target)
        target->removeEventFilter(this);

    target = object;

    if (target)
        target->installEventFilter(this);
}
