// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfdcontainer_p.h"
#include <private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

void QFdContainer::reset() noexcept
{
    if (m_fd >= 0)
        qt_safe_close(m_fd);
    m_fd = -1;
}

QT_END_NAMESPACE
