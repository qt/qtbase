// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtWidgets/QWidget>

#pragma once // Yeah, it's deprecated in general, but it's standard practice for Mac OS X.

QString nativeWindowTitle(QWidget *widget, Qt::WindowState state);
bool nativeWindowModified(QWidget *widget);
