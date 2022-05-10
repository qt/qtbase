// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QDragMoveEvent>
#include <QFrame>
#include <QMimeData>
#include <QWidget>

namespace droprectangle {

struct Window : public QWidget
{
    void dragMoveEvent(QDragMoveEvent *event);

    QFrame *dropFrame = nullptr;
};


//! [0]
void Window::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain")
        && event->answerRect().intersects(dropFrame->geometry()))

        event->acceptProposedAction();
}
//! [0]


} // droprectangle
