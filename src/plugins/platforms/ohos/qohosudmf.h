// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSUDMF_H
#define QOHOSUDMF_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qspan.h>
#include <database/udmf/udmf_err_code.h>
#include <database/udmf/udmf.h>
#include <memory>
#include <qarkui/qarkuiutils.h>
#include <qohosplugincore.h>
#include <qohosudsobject.h>
#include <qohosutils.h>
#include <string>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

class QOhosUdmfRecord
{
public:
    QOhosUdmfRecord();

    QOhosUdmfRecord(const QOhosUdmfRecord &) = delete;
    QOhosUdmfRecord &operator=(const QOhosUdmfRecord &) = delete;
    QOhosUdmfRecord(QOhosUdmfRecord &&) = default;
    QOhosUdmfRecord &operator=(QOhosUdmfRecord &&) = default;

    std::vector<std::string> getTypes() const;

    void setProviderForDataFetchFunc(
        std::vector<std::string> typeIds, std::function<void *(const char *)> dataFetchFunc);

    template<typename T>
    void addEntry(QOhosUdsObject<T> udsObject);

    void addGeneralEntry(const std::string &typeId, std::uint8_t *buff, std::uint32_t buffSize);

    template<typename T>
    QOhosUdsObject<T> getEntry();

    QOhosOptional<QSpan<std::uint8_t>> tryGetGeneralEntry(const std::string &typeId);

    bool isEmpty() const;

private:
    static QOhosUdmfRecord makeAsView(::OH_UdmfRecord *nativePtr);

    QOhosUdmfRecord(std::unique_ptr<::OH_UdmfRecord, void(*)(::OH_UdmfRecord *)> &&nativePtr);

    ::OH_UdmfRecord *release() &&;

    std::unique_ptr<::OH_UdmfRecord, void(*)(::OH_UdmfRecord *)> m_nativePtr;

    bool m_invalidated {false};

    friend class QOhosUdmfData;
};

class QOhosUdmfData
{
public:
    static QOhosUdmfData takeOwnership(::OH_UdmfData *nativePtr);

    QOhosUdmfData();

    QOhosUdmfData(const QOhosUdmfData &) = delete;
    QOhosUdmfData &operator=(const QOhosUdmfData &) = delete;
    QOhosUdmfData(QOhosUdmfData &&) = default;
    QOhosUdmfData &operator=(QOhosUdmfData &&) = default;

    std::vector<std::string> getTypes() const;

    std::vector<QOhosUdmfRecord> getRecords();
    void addRecord(QOhosUdmfRecord &&record);

    ::OH_UdmfData *release() &&;
    ::OH_UdmfData *nativePtr();

private:
    QOhosUdmfData(::OH_UdmfData *nativePtr);

    std::unique_ptr<::OH_UdmfData, void(*)(::OH_UdmfData *)> m_nativePtr;
};

namespace details_qohosudmfrecord_h
{

template<typename T>
struct UdmfRecordMeta
{
};

template<>
struct UdmfRecordMeta<::OH_UdsPlainText>
{
    static auto addContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_AddPlainText); }
    static auto getContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_GetPlainText); }
};

template<>
struct UdmfRecordMeta<::OH_UdsHtml>
{
    static auto addContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_AddHtml); }
    static auto getContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_GetHtml); }
};

template<>
struct UdmfRecordMeta<::OH_UdsFileUri>
{
    static auto addContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_AddFileUri); }
    static auto getContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_GetFileUri); }
};

template<>
struct UdmfRecordMeta<::OH_UdsHyperlink>
{
    static auto addContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_AddHyperlink); }
    static auto getContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_GetHyperlink); }
};

template<>
struct UdmfRecordMeta<::OH_UdsPixelMap>
{
    static auto addContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_AddPixelMap); }
    static auto getContentNamedFunc() { return Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_GetPixelMap); }
};

}

template<typename T>
void QOhosUdmfRecord::addEntry(QOhosUdsObject<T> udsObject)
{
    QArkUi::callArkUiOrFailOnErrorResult(
        ::details_qohosudmfrecord_h::UdmfRecordMeta<T>::addContentNamedFunc(),
        m_nativePtr.get(), udsObject.data());
}

template<typename T>
QOhosUdsObject<T> QOhosUdmfRecord::getEntry()
{
    if (m_invalidated) {
        qOhosPrintfError(
            "%s: This record is invalidated. Returning an entry with default content.", Q_FUNC_INFO);
        return QOhosUdsObject<T>();
    }

    QOhosUdsObject<T> udsObject;
    auto getContentFuncResult = details_qohosudmfrecord_h::UdmfRecordMeta<T>::getContentNamedFunc()(
        m_nativePtr.get(), udsObject.data());
    if (getContentFuncResult != ::Udmf_ErrCode::UDMF_E_OK) {
        qOhosPrintfError(
            "%s: '%s' function failed with error: %d."
            " Any further operations done to this record will be ignored or will result in dummy values.",
            Q_FUNC_INFO, details_qohosudmfrecord_h::UdmfRecordMeta<T>::getContentNamedFunc().name(),
            getContentFuncResult);
        m_invalidated = true;

        return QOhosUdsObject<T>();
    }

    return udsObject;
}

QT_END_NAMESPACE

#endif
