// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSETTINGS_H
#define QOHOSSETTINGS_H

#include <QtCore/QtGlobal>
#include <QtCore/private/qohoscommon_p.h>

QT_BEGIN_NAMESPACE

class QOhosSettings
{
public:
    QOhosSettings();

    double fontSizeScale() const;
    bool isWindowPcModeEnabled() const;

private:
    QOhosSupplier<bool> m_windowPcModeEnabled;
};

QT_END_NAMESPACE

#endif
