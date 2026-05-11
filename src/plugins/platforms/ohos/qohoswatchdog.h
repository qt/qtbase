// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWATCHDOG_H
#define QOHOSWATCHDOG_H

#include <QtCore/qglobal.h>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtOhosWatchdog {

std::shared_ptr<void> makeWatchdog();

}

QT_END_NAMESPACE

#endif /* QOHOSWATCHDOG_H */
