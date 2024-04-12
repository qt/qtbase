// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
exec(QCursor::pos());
//! [0]


//! [1]
exec(somewidget.mapToGlobal(QPoint(0,0)));
//! [1]


//! [2]
exec(e->globalPosition().toPoint());
//! [2]


//! [3]
exec(QCursor::pos());
//! [3]


//! [4]
exec(somewidget.mapToGlobal(QPoint(0, 0)));
//! [4]


//! [5]
exec(e->globalPosition().toPoint());
//! [5]


//! [6]
QMenu menu;
QAction *at = actions[0]; // Assumes actions is not empty
for (QAction *a : std::as_const(actions))
    menu.addAction(a);
menu.exec(pos, at);
//! [6]
