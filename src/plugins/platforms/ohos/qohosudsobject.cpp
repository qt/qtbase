// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosudsobject.h>

#include <QtGui/qimage.h>
#include <qohospixelmapconversions.h>

QT_BEGIN_NAMESPACE

template<>
QOhosUdsObject<::OH_UdsPixelMap>::QOhosUdsObject(typename QOhosUdsMeta<::OH_UdsPixelMap>::Content content)
    : m_data(details_qohosudsobject_h::createUds<::OH_UdsPixelMap>())
{
    QArkUi::callArkUiOrFailOnErrorResult(
        details_qohosudsobject_h::UdsMeta<::OH_UdsPixelMap>::contentSetterFunc(),
        m_data.get(), content.get());
}

template<>
QOhosUdsObject<::OH_UdsFileUri>::QOhosUdsObject(
    typename QOhosUdsMeta<::OH_UdsFileUri>::Content content)
    : m_data(details_qohosudsobject_h::createUds<::OH_UdsFileUri>())
{
    QArkUi::callArkUiOrFailOnErrorResult(
        details_qohosudsobject_h::UdsMeta<::OH_UdsFileUri>::contentSetterFunc(),
        m_data.get(), content);

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_UdsFileUri_SetFileType),
        m_data.get(), QArkUi::CZString(UDMF_META_GENERAL_FILE));
}

template<>
typename QOhosUdsMeta<::OH_UdsPixelMap>::Content QOhosUdsObject<::OH_UdsPixelMap>::getContent()
{
    auto pixelMap = makeEmptyNativePixelMap();
    QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_UdsPixelMap_GetPixelMap),
        m_data.get(), pixelMap.get());

    return pixelMap;
}

QT_END_NAMESPACE
