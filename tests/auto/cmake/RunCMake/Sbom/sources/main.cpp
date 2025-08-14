// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

int gui_helper_func();
int utils_helper_func();

int main(int argc, char **argv) {
    return utils_helper_func() + gui_helper_func();
}
