// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSUDMFCONVERSIONS_H
#define QOHOSUDMFCONVERSIONS_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qmimedata.h>
#include <database/udmf/udmf.h>
#include <functional>
#include <memory>
#include <qohosplugincore.h>
#include <qohosudmf.h>
#include <string>
#include <vector>

QT_BEGIN_NAMESPACE

std::function<QOhosUdmfData()> makeUdmfDataFactoryFromQMimeData(
    const QMimeData &mimeData, const QOhosOptional<bool> &shareInAppOnly);

std::function<QOhosUdmfData()> makeLazyProcessingUdmfDataFactoryFromQMimeData(
    std::shared_ptr<QMimeData> mimeData, const QOhosOptional<bool> &shareInAppOnly);

QOhosSupplier<std::unique_ptr<QMimeData>> createQMimeDataFactoryFromUdmfData(QOhosUdmfData udmfData);

QOhosSupplier<std::unique_ptr<QMimeData>> makeLazyFetchingQMimeDataFactoryFromUdmfData(QOhosUdmfData udmfData);

QOhosSupplier<std::unique_ptr<QMimeData>> makeDummyQMimeDataFactoryFromUdmfDataTypes(
    std::vector<std::string> udmfDataTypes);

bool isQOhosUdmfDataConvertedFromThisProcessMimeData(QOhosUdmfData &udmfData);

QOhosOptional<std::string> tryMapUtdTypeIdToMimeType(const std::string &utdTypeId);

QOhosOptional<std::string> tryMapMimeTypeToUtdTypeId(const std::string &mimeType);

QT_END_NAMESPACE

#endif
