// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSBUNDLEMANAGER_H
#define QOHOSBUNDLEMANAGER_H

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

struct ElementName
{
    QString deviceId;
    QString bundleName;
    QString abilityName;
    QString uri;
    QString shortName;
    QString moduleName;
};

}

QT_END_NAMESPACE

#endif // QOHOSBUNDLEMANAGER_H
