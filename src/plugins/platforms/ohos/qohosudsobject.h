// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSUDSOBJECT_H
#define QOHOSUDSOBJECT_H

#include <database/udmf/udmf_meta.h>
#include <database/udmf/uds.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <qarkui/qarkuiutils.h>
#include <qohospixelmapconversions.h>
#include <qohosutils.h>
#include <utility>

QT_BEGIN_NAMESPACE

template<typename T>
struct QOhosUdsMeta
{
};

template<>
struct QOhosUdsMeta<::OH_UdsPlainText>
{
    using Content = QArkUi::CZString;

    static constexpr const char *udmfMetaId = UDMF_META_PLAIN_TEXT;

    static QArkUi::CZString getDefaultContent()
    {
        return "";
    }
};

template<>
struct QOhosUdsMeta<::OH_UdsHtml>
{
    using Content = QArkUi::CZString;

    static constexpr const char *udmfMetaId = UDMF_META_HTML;

    static QArkUi::CZString getDefaultContent()
    {
        return "";
    }
};

template<>
struct QOhosUdsMeta<::OH_UdsFileUri>
{
    using Content = QArkUi::CZString;

    static constexpr const char *udmfMetaId = UDMF_META_GENERAL_FILE_URI;

    static QArkUi::CZString getDefaultContent()
    {
        return "";
    }
};

template<>
struct QOhosUdsMeta<::OH_UdsHyperlink>
{
    using Content = QArkUi::CZString;

    static constexpr const char *udmfMetaId = UDMF_META_HYPERLINK;

    static QArkUi::CZString getDefaultContent()
    {
        return "";
    }
};

template<>
struct QOhosUdsMeta<::OH_UdsPixelMap>
{
    using Content = std::shared_ptr<::OH_PixelmapNative>;

    static constexpr const char *udmfMetaId = UDMF_META_OPENHARMONY_PIXEL_MAP;

    static std::shared_ptr<::OH_PixelmapNative> getDefaultContent()
    {
        return makeEmptyNativePixelMap();
    }
};

template<typename T>
class QOhosUdsObject
{
public:
    QOhosUdsObject();
    QOhosUdsObject(typename QOhosUdsMeta<T>::Content content);

    QOhosUdsObject(const QOhosUdsObject<T> &) = delete;
    QOhosUdsObject<T> &operator=(const QOhosUdsObject<T> &) = delete;
    QOhosUdsObject(QOhosUdsObject<T> &&) = default;
    QOhosUdsObject<T> &operator=(QOhosUdsObject<T> &&) = default;

    T *release() &&;
    T *data();

    typename QOhosUdsMeta<T>::Content getContent();

private:
    std::unique_ptr<T, void (*)(T *)> m_data;
};

namespace details_qohosudsobject_h {

template<typename T>
struct UdsMeta
{
};

template<>
struct UdsMeta<::OH_UdsPlainText> : public QOhosUdsMeta<::OH_UdsPlainText>
{
    static auto createFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsPlainText_Create); }
    static auto destroyFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsPlainText_Destroy); }
    static auto contentSetterFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsPlainText_SetContent); }
    static auto getNullableContentFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsPlainText_GetContent); }
    static auto getEmptyContent() { return ""; }
};

template<>
struct UdsMeta<::OH_UdsHtml> : public QOhosUdsMeta<::OH_UdsHtml>
{
    static auto createFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHtml_Create); }
    static auto destroyFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHtml_Destroy); }
    static auto contentSetterFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHtml_SetContent); }
    static auto getNullableContentFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHtml_GetContent); }
    static auto getEmptyContent() { return ""; }
};

template<>
struct UdsMeta<::OH_UdsFileUri> : public QOhosUdsMeta<::OH_UdsFileUri>
{
    static auto createFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsFileUri_Create); }
    static auto destroyFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsFileUri_Destroy); }
    static auto contentSetterFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsFileUri_SetFileUri); }
    static auto getNullableContentFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsFileUri_GetFileUri); }
    static auto getEmptyContent() { return ""; }
};

template<>
struct UdsMeta<::OH_UdsHyperlink> : public QOhosUdsMeta<::OH_UdsHyperlink>
{
    static auto createFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHyperlink_Create); }
    static auto destroyFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHyperlink_Destroy); }
    static auto contentSetterFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHyperlink_SetUrl); }
    static auto getNullableContentFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsHyperlink_GetUrl); }
    static auto getEmptyContent() { return ""; }
};

template<>
struct UdsMeta<::OH_UdsPixelMap> : public QOhosUdsMeta<::OH_UdsPixelMap>
{
    static auto createFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsPixelMap_Create); }
    static auto destroyFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsPixelMap_Destroy); }
    static auto contentSetterFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdsPixelMap_SetPixelMap); }
};

template<typename T>
std::unique_ptr<T, void (*)(T *)> createUds()
{
    return {
        QArkUi::callArkUiOrFailOnNullResult(
            details_qohosudsobject_h::UdsMeta<T>::createFunc()),
        [](T *data) {
            QArkUi::callArkUi(
                details_qohosudsobject_h::UdsMeta<T>::destroyFunc(),
                data);
        }
    };
}

}

template<typename T>
QOhosUdsObject<T>::QOhosUdsObject()
    : QOhosUdsObject(QOhosUdsMeta<T>::getDefaultContent())
{
}

template<typename T>
QOhosUdsObject<T>::QOhosUdsObject(typename QOhosUdsMeta<T>::Content content)
    : m_data(details_qohosudsobject_h::createUds<T>())
{
    QArkUi::callArkUiOrFailOnErrorResult(
        details_qohosudsobject_h::UdsMeta<T>::contentSetterFunc(),
        m_data.get(), content);
}

template<>
QOhosUdsObject<::OH_UdsPixelMap>::QOhosUdsObject(
    typename QOhosUdsMeta<::OH_UdsPixelMap>::Content content);

template<>
QOhosUdsObject<::OH_UdsFileUri>::QOhosUdsObject(
    typename QOhosUdsMeta<::OH_UdsFileUri>::Content content);

template<typename T>
T *QOhosUdsObject<T>::release() &&
{
    return m_data.release();
}

template<typename T>
T *QOhosUdsObject<T>::data()
{
    return m_data.get();
}

template<typename T>
typename QOhosUdsMeta<T>::Content QOhosUdsObject<T>::getContent()
{
    using namespace details_qohosudsobject_h;

    auto *contentPtr = QArkUi::callArkUi(
        UdsMeta<T>::getNullableContentFunc(),
        m_data.get());
    return contentPtr != nullptr ? contentPtr : UdsMeta<T>::getEmptyContent();
}

template<>
typename QOhosUdsMeta<::OH_UdsPixelMap>::Content QOhosUdsObject<::OH_UdsPixelMap>::getContent();

QT_END_NAMESPACE

#endif
