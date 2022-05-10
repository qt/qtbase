// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//! [0]
    QWidget *w = ...;
    QScroller::grabGesture(w, QScroller::LeftMouseButtonGesture);
//! [0]

//! [1]
    QWidget *w = ...;
    QScroller *scroller = QScroller::scroller(w);
    scroller->scrollTo(QPointF(100, 100));
//! [1]
