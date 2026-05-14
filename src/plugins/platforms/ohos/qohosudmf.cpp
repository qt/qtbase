// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosudmf.h>

#include <QtCore/private/qohoslogger_p.h>
#include <qarkui/qarkuiutils.h>
#include <qohosutils.h>
#include <typeinfo>

QT_BEGIN_NAMESPACE

namespace
{

template<typename T, typename Deleter>
std::unique_ptr<T, Deleter> makeNonNullUniquePtrOrFail(T *ptr, Deleter &&deleter)
{
    if (ptr == nullptr) {
        qOhosReportFatalErrorAndAbort(
            "%s: got null pointer with deleter of type '%s'",
            Q_FUNC_INFO, typeid(deleter).name());
    }

    return std::unique_ptr<T, Deleter>(ptr, deleter);
}

std::unique_ptr<::OH_UdmfRecordProvider, void (*)(::OH_UdmfRecordProvider *)> createUdmfRecordProvider()
{
    return {
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_UdmfRecordProvider_Create)),
        [](::OH_UdmfRecordProvider *udmfRecordProvider) {
            QArkUi::callArkUiOrFailOnErrorResult(
                Q_OHOS_NAMED_FUNC(::OH_UdmfRecordProvider_Destroy),
                udmfRecordProvider);
        }
    };
}

std::unique_ptr<::OH_UdmfRecordProvider, void (*)(::OH_UdmfRecordProvider *)> createUdmfRecordProviderForDataFetchFunction(
    std::function<void *(const char *)> dataFetchFunc)
{
    using DataFetchFunc = std::function<void *(const char *)>;

    auto udmfRecordProvider = createUdmfRecordProvider();

    auto *dataFetchFuncPtr = new DataFetchFunc(std::move(dataFetchFunc));
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_UdmfRecordProvider_SetData),
        udmfRecordProvider.get(), static_cast<void *>(dataFetchFuncPtr),
        [](void *context, const char *type) {
            auto *dataFetchFuncPtr = static_cast<DataFetchFunc *>(context);
            return (*dataFetchFuncPtr)(type);
        },
        [](void *context) {
            auto *dataFetchFuncPtr = static_cast<DataFetchFunc *>(context);
            delete dataFetchFuncPtr;
        });

    return udmfRecordProvider;
}

}

QOhosUdmfRecord::QOhosUdmfRecord()
    : QOhosUdmfRecord(
        std::unique_ptr<::OH_UdmfRecord, void (*)(::OH_UdmfRecord *)>(
            QArkUi::callArkUiOrFailOnNullResult(Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_Create)),
            ::OH_UdmfRecord_Destroy))
{
}

QOhosUdmfRecord::QOhosUdmfRecord(
    std::unique_ptr<::OH_UdmfRecord, void (*)(::OH_UdmfRecord *)> &&nativePtr)
    : m_nativePtr(std::move(nativePtr))
{
}

std::vector<std::string> QOhosUdmfRecord::getTypes() const
{
    if (m_invalidated) {
        qOhosPrintfError("%s: This record is invalidated. Returning empty value.", Q_FUNC_INFO);
        return {};
    }

    char **types = nullptr;
    unsigned int typesCount = 0;
    auto func = Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_GetTypes);
    types = QArkUi::callArkUi(func, m_nativePtr.get(), &typesCount);
    if (types == nullptr && typesCount != 0) {
        qOhosReportFatalErrorAndAbort(
            "%s: got inconsistent result from %s() call: array is null, size is %u",
            Q_FUNC_INFO, func.name(), typesCount);
    }

    return types != nullptr
        ? std::vector<std::string>(types, types + typesCount)
        : std::vector<std::string>();
}

QOhosUdmfRecord QOhosUdmfRecord::makeAsView(::OH_UdmfRecord *nativePtr)
{
    return QOhosUdmfRecord(
        std::unique_ptr<::OH_UdmfRecord, void (*)(::OH_UdmfRecord *)>(
            nativePtr,
            [](::OH_UdmfRecord *) {
            }));
}

::OH_UdmfRecord *QOhosUdmfRecord::release() &&
{
    return m_nativePtr.release();
}

void QOhosUdmfRecord::setProviderForDataFetchFunc(
    std::vector<std::string> typeIds, std::function<void *(const char *)> dataFetchFunc)
{
    std::vector<const char *> typeIdsCStrs;
    for (const auto &typeId : typeIds)
        typeIdsCStrs.push_back(typeId.c_str());

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_SetProvider),
        m_nativePtr.get(), typeIdsCStrs.data(), typeIdsCStrs.size(),
        createUdmfRecordProviderForDataFetchFunction(std::move(dataFetchFunc)).release());
}

void QOhosUdmfRecord::addGeneralEntry(
    const std::string &typeId, std::uint8_t *buff, std::uint32_t buffSize)
{
    if (m_invalidated) {
        qOhosPrintfError("%s: This record is invalidated. Ignoring.", Q_FUNC_INFO);
        return;
    }

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_UdmfRecord_AddGeneralEntry),
        m_nativePtr.get(), QArkUi::CZString(typeId.c_str()), buff, buffSize);
}

QOhosOptional<QSpan<const std::uint8_t>> QOhosUdmfRecord::tryGetGeneralEntry(
    const std::string &typeId) const
{
    if (m_invalidated) {
        qOhosPrintfError("%s: This record is invalidated. Returning empty value.", Q_FUNC_INFO);
        return makeEmptyQOhosOptional();
    }

    auto availableTypes = getTypes();
    auto foundTypeIt = std::find(std::begin(availableTypes), std::end(availableTypes), typeId);
    if (foundTypeIt == std::end(availableTypes))
        return makeEmptyQOhosOptional();

    std::uint8_t *buff;
    std::uint32_t buffSize;
    if (::OH_UdmfRecord_GetGeneralEntry(m_nativePtr.get(), typeId.c_str(), &buff, &buffSize)
        != ::Udmf_ErrCode::UDMF_E_OK) {
        qOhosPrintfError(
            "%s: OH_UdmfRecord_GetGeneralEntry() for typeId=%s resulted in an error even though "
            "the requested type should have been available. This is abnormal."
            "OH_UdmfRecord's inner state might have been compromised."
            "Any further operations done to this record will be ignored or will result in dummy values.",
            Q_FUNC_INFO, typeId.c_str());
        m_invalidated = true;

        return makeEmptyQOhosOptional();
    }

    return makeQOhosOptional(QSpan<const std::uint8_t>(buff, buffSize));
}

bool QOhosUdmfRecord::isEmpty() const
{
    return getTypes().size() == 0;
}

QOhosUdmfData QOhosUdmfData::takeOwnership(::OH_UdmfData *nativePtr)
{
    return QOhosUdmfData(nativePtr);
}

QOhosUdmfData::QOhosUdmfData()
    : QOhosUdmfData(
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_UdmfData_Create)))
{
}

QOhosUdmfData::QOhosUdmfData(::OH_UdmfData *nativePtr)
    : m_nativePtr(makeNonNullUniquePtrOrFail(nativePtr, ::OH_UdmfData_Destroy))
{
}

std::vector<std::string> QOhosUdmfData::getTypes() const
{
    char **types = nullptr;
    unsigned int typesCount = 0;
    auto func = Q_OHOS_NAMED_FUNC(::OH_UdmfData_GetTypes);
    types = QArkUi::callArkUi(func, m_nativePtr.get(), &typesCount);
    if (types == nullptr && typesCount != 0) {
        qOhosReportFatalErrorAndAbort(
            "%s: got inconsistent result from %s() call: array is null, size is %u",
            Q_FUNC_INFO, func.name(), typesCount);
    }

    return types != nullptr
        ? std::vector<std::string>(types, types + typesCount)
        : std::vector<std::string>();
}

std::vector<QOhosUdmfRecord> QOhosUdmfData::getRecords()
{
    ::OH_UdmfRecord **records = nullptr;
    std::uint32_t recordsNum;
    auto func = Q_OHOS_NAMED_FUNC(::OH_UdmfData_GetRecords);
    records = QArkUi::callArkUi(func, m_nativePtr.get(), &recordsNum);
    if (records == nullptr && recordsNum != 0) {
        qOhosReportFatalErrorAndAbort(
            "%s: got inconsistent result from %s() call: array is null, size is %u",
            Q_FUNC_INFO, func.name(), recordsNum);
    }

    std::vector<QOhosUdmfRecord> resultRecords;
    for (std::uint32_t i = 0; i < recordsNum; ++i)
        resultRecords.push_back(QOhosUdmfRecord::makeAsView(records[i]));

    return resultRecords;
}

void QOhosUdmfData::addRecord(QOhosUdmfRecord &&record)
{
    if (record.m_invalidated) {
        qOhosPrintfError("%s: Cannot add an invalidated record. Ignoring.", Q_FUNC_INFO);
        return;
    }

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_UdmfData_AddRecord),
        m_nativePtr.get(), std::move(record).release());
}

::OH_UdmfData *QOhosUdmfData::release() &&
{
    return m_nativePtr.release();
}

::OH_UdmfData *QOhosUdmfData::nativePtr()
{
    return m_nativePtr.get();
}

QT_END_NAMESPACE
