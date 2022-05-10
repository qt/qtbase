// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtWidgets/QWidget>
#include <QtWidgets/QCheckBox>

#pragma once // Yeah, it's deprecated in general, but it's standard practice for Mac OS X.

QT_USE_NAMESPACE

bool testLineEdit();
bool testHierarchy(QWidget *w);
bool singleWidget();
bool notifications(QWidget *w);
bool testCheckBox(QCheckBox *ckBox);
