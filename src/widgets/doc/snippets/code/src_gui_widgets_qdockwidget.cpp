// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QDockWidget *dockWidget = qobject_cast<QDockWidget*>(parentWidget());
if (dockWidget->features() & QDockWidget::DockWidgetVerticalTitleBar) {
    // I need to be vertical
} else {
    // I need to be horizontal
}
//! [0]
