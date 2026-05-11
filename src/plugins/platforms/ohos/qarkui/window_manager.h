// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUI__WINDOW_MANAGER_H
#define QARKUI__WINDOW_MANAGER_H

#include <qohosdisplayinfo.h>
#include <qarkui/window.h>
#include <qarkui/input.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qpoint.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

std::shared_ptr<void> registerMouseEventsConsumer(
    JsWindowId jsWindowId,
    QOhosConsumer<const MouseEvent &> eventsConsumer);

std::shared_ptr<void> registerKeyEventsConsumer(
    JsWindowId jsWindowId,
    QOhosConsumer<const KeyEvent &> eventsConsumer);

std::shared_ptr<void> registerTouchEventsConsumer(
    JsWindowId jsWindowId,
    QOhosConsumer<const TouchEvent &> eventsConsumer);

}

QT_END_NAMESPACE

#endif
