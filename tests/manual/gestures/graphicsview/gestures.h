/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GESTURE_H
#define GESTURE_H

#include <QGestureRecognizer>
#include <QGesture>

class ThreeFingerSlideGesture : public QGesture
{
    Q_OBJECT
public:
    static Qt::GestureType Type;

    ThreeFingerSlideGesture(QObject *parent = 0) : QGesture(parent) { }

    bool gestureFired;
};

class ThreeFingerSlideGestureRecognizer : public QGestureRecognizer
{
private:
    QGesture *create(QObject *target);
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event);
    void reset(QGesture *state);
};

class RotateGestureRecognizer : public QGestureRecognizer
{
public:
    RotateGestureRecognizer();

private:
    QGesture *create(QObject *target);
    QGestureRecognizer::Result recognize(QGesture *state, QObject *watched, QEvent *event);
    void reset(QGesture *state);
};

#endif // GESTURE_H
