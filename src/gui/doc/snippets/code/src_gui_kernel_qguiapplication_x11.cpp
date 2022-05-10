// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QCursor>
#include <QGuiApplication>

namespace src_gui_kernel_qguiapplication_x11 {
void calculateHugeMandelbrot();

void wrapper() {


//! [0]
QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
calculateHugeMandelbrot();              // lunch time...
QGuiApplication::restoreOverrideCursor();
//! [0]


} // wrapper
} // src_gui_kernel_qguiapplication_x11
