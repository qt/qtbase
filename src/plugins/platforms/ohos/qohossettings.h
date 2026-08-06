// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSETTINGS_H
#define QOHOSSETTINGS_H

#include <QtCore/qglobal.h>
#include <QtCore/private/qohoscommon_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QOhosSettings
{
public:
    static QOhosSettings &instance();
    Q_REQUIRED_RESULT std::shared_ptr<void> installSettingsCache();

    QOhosSettings(const QOhosSettings &) = delete;
    QOhosSettings &operator=(const QOhosSettings &) = delete;

    QOhosSettings(QOhosSettings &&) = delete;
    QOhosSettings &operator=(QOhosSettings &&) = delete;

    double fontSizeScale() const;
    bool isWindowPcModeEnabled() const;

private:
    QOhosSettings();

    QOhosSupplier<bool> m_windowPcModeEnabled;
};

QT_END_NAMESPACE

#endif
