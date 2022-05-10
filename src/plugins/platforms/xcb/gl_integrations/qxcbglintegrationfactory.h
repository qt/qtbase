// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBGLINTEGRATIONFACTORY_H
#define QXCBGLINTEGRATIONFACTORY_H

#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QXcbGlIntegration;

class QXcbGlIntegrationFactory
{
public:
    static QXcbGlIntegration *create(const QString &name);
};

QT_END_NAMESPACE

#endif //QXCBGLINTEGRATIONFACTORY_H

