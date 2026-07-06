// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSBUNDLEMANAGER_H
#define QOHOSBUNDLEMANAGER_H

#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>
#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

struct Q_OHOSAPPKIT_EXPORT QOhosElementName
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

Q_DECLARE_METATYPE(QtOhosAppKit::QOhosElementName)

#endif // QOHOSBUNDLEMANAGER_H
