// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>

int core_helper_func();
int gui_helper_func();

int main(int argc, char **argv) {
    QWindow w;
    w.show();
    return core_helper_func() + gui_helper_func();
}
