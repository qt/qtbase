// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSMIMEDATA_H
#define QOHOSMIMEDATA_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>
#include <map>
#include <memory>

QT_BEGIN_NAMESPACE

QOhosSupplier<std::unique_ptr<QMimeData>> makeQOhosLazyFetchMimeDataFactory(
    std::map<QString, QOhosSupplier<QVariant>> mimeValuesSuppliers);
QOhosSupplier<std::unique_ptr<QMimeData>> makeQOhosMimeDataFactory(std::map<QString, QVariant> mimeValues);

QT_END_NAMESPACE

#endif
