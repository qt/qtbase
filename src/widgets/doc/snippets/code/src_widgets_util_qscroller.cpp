// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

//! [0]
    QWidget *w = ...;
    QScroller::grabGesture(w, QScroller::LeftMouseButtonGesture);
//! [0]

//! [1]
    QWidget *w = ...;
    QScroller *scroller = QScroller::scroller(w);
    scroller->scrollTo(QPointF(100, 100));
//! [1]
