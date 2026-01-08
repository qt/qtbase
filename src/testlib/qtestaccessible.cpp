// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qtestaccessible.h"

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

auto QTestAccessibility::eventList() -> EventList&
{
    Q_CONSTINIT static EventList list;
    return list;
}

QTestAccessibility *&QTestAccessibility::instance()
{
    Q_CONSTINIT static QTestAccessibility *ta = nullptr;
    return ta;
}


QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
