// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosappcontext_p.h"
#include <QtCore/qglobalstatic.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QOhosAppContext
{
    using AppContextType = QMap<Type, QString>;
    Q_GLOBAL_STATIC(AppContextType, s_appContext)

    void init(QMap<Type, QString> appcontext)
    {
        *s_appContext = std::move(appcontext);
    }

    QString getProperty(Type prop)
    {
        return s_appContext->value(prop);
    }

    QMap<Type, QString> getAllProperties()
    {
        return *s_appContext;
    }
}

QT_END_NAMESPACE
