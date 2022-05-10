// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
void MyView::resizeEvent(QResizeEvent *event) {
    horizontalScrollBar()->setRange(0, realWidth - width());
    ...
}
//! [0]

//! [1]
setIndexWidget(index, new QLineEdit);
...
setIndexWidget(index, new QTextEdit);
//! [1]

//! [2]
QItemSelectionModel *m = view->selectionModel();
view->setModel(new model);
delete m;
//! [2]
