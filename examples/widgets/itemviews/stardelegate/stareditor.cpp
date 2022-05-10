// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "stareditor.h"
#include "starrating.h"

#include <QtWidgets>

//! [0]
StarEditor::StarEditor(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAutoFillBackground(true);
}
//! [0]

QSize StarEditor::sizeHint() const
{
    return myStarRating.sizeHint();
}

//! [1]
void StarEditor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    myStarRating.paint(&painter, rect(), palette(),
                       StarRating::EditMode::Editable);
}
//! [1]

//! [2]
void StarEditor::mouseMoveEvent(QMouseEvent *event)
{
    const int star = starAtPosition(event->position().toPoint().x());

    if (star != myStarRating.starCount() && star != -1) {
        myStarRating.setStarCount(star);
        update();
    }
    QWidget::mouseMoveEvent(event);
}
//! [2]

//! [3]
void StarEditor::mouseReleaseEvent(QMouseEvent *event)
{
    emit editingFinished();
    QWidget::mouseReleaseEvent(event);
}
//! [3]

//! [4]
int StarEditor::starAtPosition(int x) const
{
    const int star = (x / (myStarRating.sizeHint().width()
                           / myStarRating.maxStarCount())) + 1;
    if (star <= 0 || star > myStarRating.maxStarCount())
        return -1;

    return star;
}
//! [4]
