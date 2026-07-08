// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSICONENGINE_H
#define QOHOSICONENGINE_H

#include <QtGui/qiconengine.h>

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

QIconEngine *tryCreateQOhosIconEngine(const QString &iconName);

QT_END_NAMESPACE

#endif // QOHOSICONENGINE_H
