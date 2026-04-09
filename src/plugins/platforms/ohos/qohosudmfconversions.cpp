// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qspan.h>
#include <QtCore/qurl.h>
#include <QtGui/qimage.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <database/udmf/udmf_err_code.h>
#include <database/udmf/udmf_meta.h>
#include <database/udmf/utd.h>
#include <qarkui/qarkuiutils.h>
#include <qohosmimedata.h>
#include <qohospixelmapconversions.h>
#include <qohosplatformservices.h>
#include <qohosplugincore.h>
#include <qohosudmfconversions.h>
#include <qohosudmf.h>
#include <qohosudsobject.h>
#include <qohosutils.h>
#include <random>
#include <unistd.h>

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace {

constexpr const char *mimeTextPlain = "text/plain";
constexpr const char *mimeTextHtml = "text/html";
constexpr const char *mimeTextUriList = "text/uri-list";
constexpr const char *mimeAppXQtImage = "application/x-qt-image";

const auto qtAppInfoDataPseudoMimeType = "io.qt.ohos.appInfoData";

constexpr auto processQMimeDataInQtThreadTimeout = ch::seconds(2);

class UdmfRecordEntryFactory
{
public:
    template<typename RawUdsObject>
    static UdmfRecordEntryFactory makeForUdsObjectFactory(
        QOhosSupplier<QOhosUdsObject<RawUdsObject>> udsObjectFactory);

    static UdmfRecordEntryFactory makeForArrayBufferWithMimeType(
        std::string mimeType, QOhosSupplier<QSpan<const std::uint8_t>> dataSupplier);

    static UdmfRecordEntryFactory makeDummy();

    void addEntryToRecord(QOhosUdmfRecord &udmfRecord) const;

    void *makeRecordProviderDataForEntry() const;

private:
    UdmfRecordEntryFactory(
        QOhosSupplier<void *> recordProviderDataFactory,
        std::function<void(QOhosUdmfRecord &)> recordAddEntryFunc);

    QOhosSupplier<void *> m_recordProviderDataFactory;
    std::function<void(QOhosUdmfRecord &)> m_recordAddEntryFunc;
};

struct UdmfRecordEntryMetaFactory
{
    QOhosSupplier<QOhosOptional<std::string>> optUdmfMetaIdFactory;
    std::function<UdmfRecordEntryFactory(const QMimeData &)> metaFactoryFunc;
};

std::vector<std::uint8_t> getAppInfoDataForThisProcess()
{
    static std::vector<std::uint8_t> appInfoData = []() {
        std::vector<std::uint8_t> appInfoData;

        auto pidString = std::to_string(::getpid()) + ":";
        std::copy(pidString.begin(), pidString.end(), std::back_inserter(appInfoData));

        using RandomBytesEngine = std::independent_bits_engine<std::default_random_engine, 8, std::uint8_t>;
        std::vector<std::uint8_t> key(64);
        std::generate(std::begin(key), std::end(key), RandomBytesEngine(std::random_device()()));
        std::copy(key.begin(), key.end(), std::back_inserter(appInfoData));

        return appInfoData;
    }();

    return appInfoData;
}

template<typename T>
QOhosOptional<T> tryEvalInQtThreadWithConsumer(
    std::function<void(QOhosConsumer<T>)> qtEvalFunc, ch::nanoseconds timeout)
{
    auto sharedResultBox = std::make_shared<QOhosOptional<T>>();

    auto qtTaskFinished = QtOhos::tryInvokeInQtThreadAndTryWaitForContinue(
        [sharedResultBox, qtEvalFunc = std::move(qtEvalFunc)](std::function<void()> continueFunc) {
            qtEvalFunc(
                [sharedResultBox, continueFunc = std::move(continueFunc)](T result) {
                    *sharedResultBox = std::move(result);
                    continueFunc();
                });
        },
        timeout);

    return qtTaskFinished
        ? std::move(*sharedResultBox)
        : makeEmptyQOhosOptional();
}

template<typename T, typename K>
std::vector<T> getMapKeys(const std::map<T, K> &inputMap)
{
    std::vector<T> mapKeys;
    mapKeys.reserve(inputMap.size());
    for (const auto &mapEntry : inputMap)
        mapKeys.push_back(mapEntry.first);
    return mapKeys;
}

template<typename T>
QOhosSupplier<QOhosOptional<T>> makeOptionalValueSupplier(T value)
{
    return [value = std::move(value)]() {
        return makeQOhosOptional(value);
    };
}

bool hasQMimeDataPeerType(const char *type)
{
    static constexpr const char *udmfTypesWithQMimeDataPeerTypes[] = {
        QOhosUdsMeta<::OH_UdsFileUri>::udmfMetaId,
        QOhosUdsMeta<::OH_UdsHyperlink>::udmfMetaId,
        QOhosUdsMeta<::OH_UdsHtml>::udmfMetaId,
        QOhosUdsMeta<::OH_UdsPixelMap>::udmfMetaId,
        QOhosUdsMeta<::OH_UdsPlainText>::udmfMetaId,
    };

    return std::any_of(
        std::begin(udmfTypesWithQMimeDataPeerTypes),
        std::end(udmfTypesWithQMimeDataPeerTypes),
        [&](const char *specificMetaType) {
            return std::strcmp(type, specificMetaType) == 0;
        });
}

bool isUdmfMetaFileType(const std::string &type)
{
    static constexpr const char *udmfMetaFileTypes[] = {
        UDMF_META_GENERAL_FILE,
        UDMF_META_AUDIO,
        UDMF_META_FOLDER,
        UDMF_META_IMAGE,
        UDMF_META_VIDEO,
    };

    return std::any_of(
        std::begin(udmfMetaFileTypes),
        std::end(udmfMetaFileTypes),
        [&](const char *specificMetaType) {
            return type == specificMetaType;
        });
}

std::vector<std::string> utdGetTypesByMimeType(const std::string &mimeType)
{
    unsigned int typesCount = 0;
    auto func = Q_OHOS_NAMED_FUNC(::OH_Utd_GetTypesByMimeType);
    const char **utdTypeIds = QArkUi::callArkUi(func, QArkUi::CZString(mimeType.c_str()), &typesCount);
    if (utdTypeIds == nullptr && typesCount != 0) {
        qOhosReportFatalErrorAndAbort(
            "%s: got inconsistent result from %s() call: array is null, size is %u",
            Q_FUNC_INFO, func.name(), typesCount);
    }

    auto result = utdTypeIds != nullptr
        ? std::vector<std::string>(utdTypeIds, utdTypeIds + typesCount)
        : std::vector<std::string>();

    QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_Utd_DestroyStringList), utdTypeIds, typesCount);

    return result;
}

std::shared_ptr<::OH_Utd> utdCreateOrNull(const std::string &typeId)
{
    return {
        QArkUi::callArkUi(
            Q_OHOS_NAMED_FUNC(::OH_Utd_Create),
            QArkUi::CZString(typeId.c_str())),
        [](::OH_Utd *utd) {
            if (utd != nullptr)
                QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_Utd_Destroy), utd);
        }
    };
}

std::vector<std::string> utdGetMimeTypes(::OH_Utd *utd)
{
    unsigned int typesCount = 0;
    auto func = Q_OHOS_NAMED_FUNC(::OH_Utd_GetMimeTypes);
    const char **mimeTypes = QArkUi::callArkUi(func, utd, &typesCount);
    if (mimeTypes == nullptr && typesCount != 0) {
        qOhosReportFatalErrorAndAbort(
            "%s: got inconsistent result from %s() call: array is null, size is %u",
            Q_FUNC_INFO, func.name(), typesCount);
    }

    auto result = mimeTypes != nullptr
        ? std::vector<std::string>(mimeTypes, mimeTypes + typesCount)
        : std::vector<std::string>();

    return result;
}

template<typename T>
bool hasMatchingTypeEntryInRecords(QSpan<const QOhosUdmfRecord> records)
{
    static const auto searchTypeId = QOhosUdsMeta<T>::udmfMetaId;
    return std::any_of(
        records.begin(), records.end(),
        [&](auto &record) {
            auto recordTypes = record.getTypes();
            return std::find(recordTypes.begin(), recordTypes.end(), searchTypeId) != recordTypes.end();
        });
}

template<typename T>
void tryProcessEntriesOfTypeInRecords(
    QSpan<const QOhosUdmfRecord> records,
    const QOhosConsumer<QOhosUdsObject<T>> &processEntryFunc)
{
    for (auto &record : records) {
        for (auto &typeId : record.getTypes()) {
            if (QOhosUdsMeta<T>::udmfMetaId == typeId)
                processEntryFunc(record.getEntry<T>());
        }
    }
}

void addMimeDataSuppliersForUrlLikeEntriesFromRecords(
    std::shared_ptr<void> context, QSpan<const QOhosUdmfRecord> inputRecords,
    std::map<QString, QOhosSupplier<QVariant>> &outMimeDataSuppliers)
{
    if (!hasMatchingTypeEntryInRecords<::OH_UdsFileUri>(inputRecords)
        && !hasMatchingTypeEntryInRecords<::OH_UdsHyperlink>(inputRecords)) {
        return;
    }

    outMimeDataSuppliers.emplace(
        QString::fromUtf8(mimeTextUriList),
        [context, inputRecords]() {
            QList<QUrl> urls;
            tryProcessEntriesOfTypeInRecords<::OH_UdsFileUri>(
                inputRecords,
                [&](auto udsEntry) {
                    urls.append(
                        {
                            QUrl::fromLocalFile(
                                QString::fromStdString(
                                    QOhosPlatformServices::mapOhosFileUriToPathInJsThread(
                                        udsEntry.getContent()))),
                        });
                });
            tryProcessEntriesOfTypeInRecords<::OH_UdsHyperlink>(
                inputRecords,
                [&](auto udsEntry) {
                    urls.append(QUrl(QString::fromStdString(udsEntry.getContent())));
                });

            QList<QVariant> urlsVariants;
            for (const auto &url : urls)
                urlsVariants.append(url);

            return QVariant(urlsVariants);
        });
}

void addMimeDataSuppliersForGeneralEntriesFromRecords(
    std::shared_ptr<void> context, QSpan<const QOhosUdmfRecord> inputRecords,
    std::map<QString, QOhosSupplier<QVariant>> &outMimeDataSuppliers)
{
    for (auto &record : inputRecords) {
        for (auto &typeId : record.getTypes()) {
            if (!hasQMimeDataPeerType(typeId.c_str()) && !isUdmfMetaFileType(typeId)) {
                // FIXME: the `utdGetMimeTypes()` result interpretation is not quite
                // clear. Documentaion does not mention why there is a list of possible mime types.
                // Are they ordered in a specific manear? Is it possible to get en ampty list?
                // Improve and fix the code when these are known.
                auto optUtdWithTypeId = utdCreateOrNull(typeId);
                auto mimeTypes = optUtdWithTypeId
                    ? utdGetMimeTypes(optUtdWithTypeId.get())
                    : std::vector<std::string>();
                if (!mimeTypes.empty()) {
                    outMimeDataSuppliers.emplace(
                        QString::fromStdString(mimeTypes.front()),
                        [context, recordPtr = &record, typeId]() {
                            auto generalEntry = recordPtr->tryGetGeneralEntry(typeId);
                            if (!generalEntry.hasValue())
                                return QVariant();

                            return QVariant(
                                QByteArray(
                                    reinterpret_cast<const char *>(generalEntry.value().data()),
                                    generalEntry.value().size()));
                        });
                } else {
                    qOhosPrintfWarning(
                        "%s: cannot convert UTD type id '%s' to mime type. Skip this entry.",
                        Q_FUNC_INFO, typeId.c_str());
                }
            }
        }
    }
}

std::map<QString, QOhosSupplier<QVariant>> makeMimeDataSuppliersMapFromUdmfData(QOhosUdmfData udmfData)
{
    struct Context
    {
        QOhosUdmfData udmfData;
        std::vector<QOhosUdmfRecord> records;
    };

    auto context = QtOhos::makeProxyWithJsThreadDeleter(std::make_shared<Context>());
    context->udmfData = std::move(udmfData);
    context->records = context->udmfData.getRecords();

    if (context->records.size() == 0)
        return {};

    std::map<QString, QOhosSupplier<QVariant>> mimeDataSuppliers;

    auto firstRecordSpan = QSpan(context->records.data(), 1);

    if (hasMatchingTypeEntryInRecords<::OH_UdsPlainText>(firstRecordSpan)) {
        mimeDataSuppliers.emplace(
            QString::fromUtf8(mimeTextPlain),
            [context]() {
                QVariant value;
                tryProcessEntriesOfTypeInRecords<::OH_UdsPlainText>(
                    QSpan(context->records.data(), 1),
                    [&](auto udsEntry) {
                        value = QString::fromUtf8(udsEntry.getContent(), -1);
                    });
                return value;
            });
    }

    if (hasMatchingTypeEntryInRecords<::OH_UdsHtml>(firstRecordSpan)) {
        mimeDataSuppliers.emplace(
            QString::fromUtf8(mimeTextHtml),
            [context]() {
                QVariant value;
                tryProcessEntriesOfTypeInRecords<::OH_UdsHtml>(
                    QSpan(context->records.data(), 1),
                    [&](auto udsEntry) {
                        value = QString::fromUtf8(udsEntry.getContent(), -1);
                    });
                return value;
            });
    }

    if (hasMatchingTypeEntryInRecords<::OH_UdsPixelMap>(firstRecordSpan)) {
        mimeDataSuppliers.emplace(
            QString::fromUtf8(mimeAppXQtImage),
            [context]() {
                QVariant value;
                tryProcessEntriesOfTypeInRecords<::OH_UdsPixelMap>(
                    QSpan(context->records.data(), 1),
                    [&](auto udsEntry) {
                        value = createQImageFromNativePixelMap(udsEntry.getContent().get());
                    });
                return value;
            });
    }

    addMimeDataSuppliersForUrlLikeEntriesFromRecords(
        context, QSpan(context->records.data(), context->records.size()), mimeDataSuppliers);

    addMimeDataSuppliersForGeneralEntriesFromRecords(context, firstRecordSpan, mimeDataSuppliers);

    return mimeDataSuppliers;
}

UdmfRecordEntryFactory makeUdmfRecordEntryFactoryForUrl(const QUrl &url)
{
    return url.isLocalFile()
        ? UdmfRecordEntryFactory::makeForUdsObjectFactory<::OH_UdsFileUri>(
            [url]() {
                return QOhosUdsObject<::OH_UdsFileUri>(
                    QOhosPlatformServices::mapPathToOhosUriInJsThread(
                        url.toLocalFile().toStdString()).c_str());
            })
        : UdmfRecordEntryFactory::makeForUdsObjectFactory<::OH_UdsHyperlink>(
            [url]() {
                return QOhosUdsObject<::OH_UdsHyperlink>(
                    url.toString().toStdString().c_str());
            });
}

void addUrlEntryToUdmfRecord(const QUrl &url, QOhosUdmfRecord &outRecord)
{
    makeUdmfRecordEntryFactoryForUrl(url).addEntryToRecord(outRecord);
}

std::shared_ptr<::OH_UdmfProperty> createUdmfPropertyForUdmfData(::OH_UdmfData *udmfData)
{
    return {
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_UdmfProperty_Create),
            udmfData),
        [](::OH_UdmfProperty *udmfProperty) {
            QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_UdmfProperty_Destroy),
                udmfProperty);
        }
    };
}

std::unique_ptr<::OH_UdsArrayBuffer, void (*)(::OH_UdsArrayBuffer *)> createUdsArrayBuffer()
{
    return {
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_UdsArrayBuffer_Create)),
        [](::OH_UdsArrayBuffer *udsArrayBuffer) {
            QArkUi::callArkUiOrFailOnErrorResult(
                Q_OHOS_NAMED_FUNC(::OH_UdsArrayBuffer_Destroy),
                udsArrayBuffer);
        }
    };
}

std::unique_ptr<::OH_UdsArrayBuffer, void (*)(::OH_UdsArrayBuffer *)>
    createUdsArrayBufferWithData(QSpan<const std::uint8_t> data)
{
    auto udsArrayBuffer = createUdsArrayBuffer();
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_UdsArrayBuffer_SetData),
        udsArrayBuffer.get(), const_cast<std::uint8_t *>(data.data()), data.size());
    return udsArrayBuffer;
}

void addGeneralEntryToRecord(std::string mimeType, QSpan<const std::uint8_t> dataBytes, QOhosUdmfRecord &record)
{
    if (dataBytes.size() == 0) {
        qOhosPrintfWarning(
            "%s: got empty data for type '%s', which is not supported by UDMF record, skipping this entry",
            Q_FUNC_INFO, mimeType.c_str());
        return;
    }

    auto optUtdTypeId = tryMapMimeTypeToUtdTypeId(mimeType);
    if (optUtdTypeId.hasValue()) {
        record.addGeneralEntry(optUtdTypeId.value(), dataBytes);
    } else {
        qOhosPrintfWarning(
            "%s: cannot convert mime type '%s' to type id. Skip this entry.",
            Q_FUNC_INFO, mimeType.c_str());
    }
}

template<typename RawUdsObject>
UdmfRecordEntryFactory UdmfRecordEntryFactory::makeForUdsObjectFactory(
    QOhosSupplier<QOhosUdsObject<RawUdsObject>> udsObjectFactory)
{
    auto sharedUdsObjectFactory = QtOhos::moveToSharedPtr(std::move(udsObjectFactory));
    return UdmfRecordEntryFactory(
        [sharedUdsObjectFactory]() {
            void *data = (*sharedUdsObjectFactory)().release();
            qOhosPrintfWarning(
                "%s: generated '%s' data for OH_UdmfRecordProvider: %p",
                Q_FUNC_INFO, QOhosUdsMeta<RawUdsObject>::udmfMetaId, data);
            return data;
        },
        [sharedUdsObjectFactory](QOhosUdmfRecord &record) {
            record.addEntry((*sharedUdsObjectFactory)());
        });
}

UdmfRecordEntryFactory UdmfRecordEntryFactory::makeForArrayBufferWithMimeType(
    std::string mimeType, QOhosSupplier<QSpan<const std::uint8_t>> dataSupplier)
{
    auto sharedDataSupplier = QtOhos::moveToSharedPtr(std::move(dataSupplier));
    return UdmfRecordEntryFactory(
        [mimeType, sharedDataSupplier]() {
            auto dataSpan = (*sharedDataSupplier)();
            auto udsArrayBuffer = createUdsArrayBufferWithData(dataSpan);
            qOhosPrintfDebug(
                "%s: generated array buffer for OH_UdmfRecordProvider, mimeType='%s', size=%zu",
                Q_FUNC_INFO, mimeType.c_str(), static_cast<std::size_t>(dataSpan.size()));
            return udsArrayBuffer.release();
        },
        [mimeType, sharedDataSupplier](QOhosUdmfRecord &record) {
            addGeneralEntryToRecord(mimeType, (*sharedDataSupplier)(), record);
        });
}

UdmfRecordEntryFactory UdmfRecordEntryFactory::makeDummy()
{
    return UdmfRecordEntryFactory(
        []() {
            return nullptr;
        },
        [](QOhosUdmfRecord &) {
        });
}

UdmfRecordEntryFactory::UdmfRecordEntryFactory(
    QOhosSupplier<void *> recordProviderDataFactory,
    std::function<void(QOhosUdmfRecord &)> recordAddEntryFunc)
    : m_recordProviderDataFactory(std::move(recordProviderDataFactory))
    , m_recordAddEntryFunc(std::move(recordAddEntryFunc))
{
}

void UdmfRecordEntryFactory::addEntryToRecord(QOhosUdmfRecord &udmfRecord) const
{
    m_recordAddEntryFunc(udmfRecord);
}

void *UdmfRecordEntryFactory::makeRecordProviderDataForEntry() const
{
    return m_recordProviderDataFactory();
}

std::vector<UdmfRecordEntryMetaFactory> makeRecordEntryMetaFactoriesForMimeDataFormats(QStringList mimeDataFormats)
{
    std::vector<UdmfRecordEntryMetaFactory> recordEntryMetaFactories;
    for (auto qStrFormat : mimeDataFormats) {
        auto format = qStrFormat.toStdString();
        if (format == mimeTextPlain) {
            recordEntryMetaFactories.emplace_back(
                UdmfRecordEntryMetaFactory{
                    makeOptionalValueSupplier<std::string>(QOhosUdsMeta<::OH_UdsPlainText>::udmfMetaId),
                    [](const QMimeData &mimeData) {
                        auto dataText = mimeData.text();
                        return UdmfRecordEntryFactory::makeForUdsObjectFactory<::OH_UdsPlainText>(
                            [dataText]() {
                                return QOhosUdsObject<::OH_UdsPlainText>(dataText.toStdString().c_str());
                            });
                    }
                });
        } else if (format == mimeTextHtml) {
            recordEntryMetaFactories.emplace_back(
                UdmfRecordEntryMetaFactory{
                    makeOptionalValueSupplier<std::string>(QOhosUdsMeta<::OH_UdsHtml>::udmfMetaId),
                    [](const QMimeData &mimeData) {
                        auto dataHtml = mimeData.html();
                        return UdmfRecordEntryFactory::makeForUdsObjectFactory<::OH_UdsHtml>(
                            [dataHtml]() {
                                return QOhosUdsObject<::OH_UdsHtml>(dataHtml.toStdString().c_str());
                            });
                    }
                });
        } else if (format == mimeTextUriList) {
            recordEntryMetaFactories.emplace_back(
                UdmfRecordEntryMetaFactory{
                    {},
                    [](const QMimeData &mimeData) {
                        auto dataUrls = mimeData.urls();
                        return !dataUrls.isEmpty()
                            ? makeUdmfRecordEntryFactoryForUrl(dataUrls.first())
                            : UdmfRecordEntryFactory::makeDummy();
                    }
                });
        } else if (format == mimeAppXQtImage) {
            recordEntryMetaFactories.emplace_back(
                UdmfRecordEntryMetaFactory{
                    makeOptionalValueSupplier<std::string>(QOhosUdsMeta<::OH_UdsPixelMap>::udmfMetaId),
                    [](const QMimeData &mimeData) {
                        auto dataImage = qvariant_cast<QImage>(mimeData.imageData());
                        return UdmfRecordEntryFactory::makeForUdsObjectFactory<::OH_UdsPixelMap>(
                            [dataImage]() {
                                return QOhosUdsObject<::OH_UdsPixelMap>(createNativePixelMapFromQImage(dataImage));
                            });
                    }
                });
        } else {
            recordEntryMetaFactories.emplace_back(
                UdmfRecordEntryMetaFactory{
                    [qStrFormat]() {
                        return tryMapMimeTypeToUtdTypeId(qStrFormat.toStdString());
                    },
                    [qStrFormat](const QMimeData &mimeData) {
                        auto dataBytes = mimeData.data(qStrFormat);
                        return UdmfRecordEntryFactory::makeForArrayBufferWithMimeType(
                            qStrFormat.toStdString(),
                            [dataBytes]() {
                                return QSpan<const std::uint8_t>(
                                    reinterpret_cast<const std::uint8_t *>(dataBytes.constData()),
                                    dataBytes.length());
                            });
                    }
                });
        }
    }

    return recordEntryMetaFactories;
}

std::function<QOhosUdmfRecord()> tryMakeDefaultUdmfRecordFactoryFromQMimeDataOrNull(
    const QMimeData &mimeData, std::shared_ptr<QMimeData> optLazyProcessingMimeData)
{
    auto recordEntryMetaFactories = makeRecordEntryMetaFactoriesForMimeDataFormats(mimeData.formats());
    if (recordEntryMetaFactories.empty())
        return nullptr;

    struct Context
    {
        std::vector<UdmfRecordEntryFactory> directEntryFactories;
        std::vector<UdmfRecordEntryMetaFactory> providerEntries;
    };

    auto context = std::make_shared<Context>();

    const bool useRecordProvider = optLazyProcessingMimeData != nullptr;

    for (auto &recordEntryFactoryEntry : recordEntryMetaFactories) {
        if (useRecordProvider && recordEntryFactoryEntry.optUdmfMetaIdFactory) {
            context->providerEntries.push_back(std::move(recordEntryFactoryEntry));
        } else {
            context->directEntryFactories.push_back(
                recordEntryFactoryEntry.metaFactoryFunc(mimeData));
        }
    }

    return [context, optLazyProcessingMimeData]() {
        QOhosUdmfRecord record;
        for (const auto &directEntryFactory : context->directEntryFactories)
            directEntryFactory.addEntryToRecord(record);

        auto entryMetaFactoryFuncsForProvider =
            std::make_shared<std::map<std::string, std::function<UdmfRecordEntryFactory(const QMimeData &)>>>();
        for (const auto &providerEntry : context->providerEntries) {
            auto optUdmfMetaId = providerEntry.optUdmfMetaIdFactory();
            if (optUdmfMetaId.hasValue()) {
                entryMetaFactoryFuncsForProvider->emplace(
                    optUdmfMetaId.value(), providerEntry.metaFactoryFunc);
            }
        }

        if (!entryMetaFactoryFuncsForProvider->empty()) {
            auto lazyProcessingMimeData = optLazyProcessingMimeData;
            record.setProviderForDataFetchFunc(
                getMapKeys(*entryMetaFactoryFuncsForProvider),
                [entryMetaFactoryFuncsForProvider, lazyProcessingMimeData](const char *type) {
                    qOhosPrintfDebug("%s: got request for data '%s'", Q_FUNC_INFO, type);

                    auto typeStr = std::string(type);
                    auto optUdmfRecordEntryFactory = tryEvalInQtThreadWithConsumer<UdmfRecordEntryFactory>(
                        [entryMetaFactoryFuncsForProvider, lazyProcessingMimeData, typeStr](QOhosConsumer<UdmfRecordEntryFactory> resultConsumer) {
                            qOhosPrintfDebug("%s: processing request for data '%s' in Qt thread", Q_FUNC_INFO, typeStr.c_str());
                            auto entryMetaFactoryFuncIter = entryMetaFactoryFuncsForProvider->find(typeStr);
                            resultConsumer(
                                entryMetaFactoryFuncIter != entryMetaFactoryFuncsForProvider->end()
                                    ? entryMetaFactoryFuncIter->second(*lazyProcessingMimeData)
                                    : UdmfRecordEntryFactory::makeDummy());
                            qOhosPrintfDebug("%s: finished processing in Qt thread", Q_FUNC_INFO);
                        },
                        processQMimeDataInQtThreadTimeout);
                    qOhosPrintfDebug(
                        "%s: got result from Qt thread: %s",
                        Q_FUNC_INFO, QtOhos::mapBoolToTrueFalseStr(optUdmfRecordEntryFactory.hasValue()));

                    void *providerResult = optUdmfRecordEntryFactory.hasValue()
                        ? optUdmfRecordEntryFactory.value().makeRecordProviderDataForEntry()
                        : nullptr;
                    qOhosPrintfDebug("%s: provider result: %p", Q_FUNC_INFO, providerResult);

                    return providerResult;
                });
        }
        return record;
    };
}

std::function<QOhosUdmfData()> makeUdmfDataFactoryFromQMimeDataImpl(
    const QMimeData &mimeData, const QOhosOptional<bool> &shareInAppOnly,
    std::shared_ptr<QMimeData> optLazyProcessingMimeData = nullptr)
{
    std::vector<std::function<QOhosUdmfRecord()>> recordFactories;

    auto optDefaultRecordFactory = tryMakeDefaultUdmfRecordFactoryFromQMimeDataOrNull(
        mimeData, optLazyProcessingMimeData);
    if (optDefaultRecordFactory)
        recordFactories.push_back(std::move(optDefaultRecordFactory));

    if (mimeData.hasUrls()) {
        auto urls = mimeData.urls();
        for (int i = 1; i < urls.size(); ++i) {
            auto dataUrl = urls.at(i);
            recordFactories.emplace_back(
                [dataUrl]() {
                    QOhosUdmfRecord record;
                    addUrlEntryToUdmfRecord(dataUrl, record);
                    return record;
                });
        }
    }

    recordFactories.emplace_back(
        []() {
            QOhosUdmfRecord record;
            auto appInfoDataBytes = getAppInfoDataForThisProcess();
            addGeneralEntryToRecord(
                qtAppInfoDataPseudoMimeType, QSpan<const std::uint8_t>(appInfoDataBytes.data(), appInfoDataBytes.size()), record);
            return record;
        });

    return [shareInAppOnly, recordFactories = std::move(recordFactories)]() {
        QOhosUdmfData udmfData;

        if (shareInAppOnly.hasValue()) {
            auto udmfShareProperty = createUdmfPropertyForUdmfData(udmfData.nativePtr());
            QArkUi::callArkUiOrFailOnErrorResult(
                Q_OHOS_NAMED_FUNC(::OH_UdmfProperty_SetShareOption),
                udmfShareProperty.get(),
                shareInAppOnly.value()
                    ? ::Udmf_ShareOption::SHARE_OPTIONS_IN_APP
                    : ::Udmf_ShareOption::SHARE_OPTIONS_CROSS_APP);
        }

        for (const auto &recordFactory : recordFactories)
            udmfData.addRecord(recordFactory());

        return udmfData;
    };
}

}

std::function<QOhosUdmfData()> makeUdmfDataFactoryFromQMimeData(
    const QMimeData &mimeData, const QOhosOptional<bool> &shareInAppOnly)
{
    return makeUdmfDataFactoryFromQMimeDataImpl(mimeData, shareInAppOnly, nullptr);
}

std::function<QOhosUdmfData()> makeLazyProcessingUdmfDataFactoryFromQMimeData(
    std::shared_ptr<QMimeData> mimeData, const QOhosOptional<bool> &shareInAppOnly)
{
    return makeUdmfDataFactoryFromQMimeDataImpl(*mimeData, shareInAppOnly, mimeData);
}

QOhosSupplier<std::unique_ptr<QMimeData>> createQMimeDataFactoryFromUdmfData(QOhosUdmfData udmfData)
{
    std::map<QString, QVariant> mimeDataMap;
    for (auto &supplierEntry : makeMimeDataSuppliersMapFromUdmfData(std::move(udmfData)))
        mimeDataMap.emplace(supplierEntry.first, supplierEntry.second());

    return makeQOhosMimeDataFactory(std::move(mimeDataMap));
}

QOhosSupplier<std::unique_ptr<QMimeData>> makeLazyFetchingQMimeDataFactoryFromUdmfData(QOhosUdmfData udmfData)
{
    std::map<QString, QOhosSupplier<QVariant>> threadSafeSuppliers;
    for (auto &supplierEntry : makeMimeDataSuppliersMapFromUdmfData(std::move(udmfData))) {
        threadSafeSuppliers.emplace(
            supplierEntry.first,
            [mimeType = supplierEntry.first.toStdString(), baseSupplier = std::move(supplierEntry.second)]() {
                qOhosPrintfDebug("%s: fetching data for MIME type '%s'", Q_FUNC_INFO, mimeType.c_str());
                return QtOhos::evalInJsThread(
                    [&](QtOhos::JsState &) {
                        return baseSupplier();
                    },
                    Q_FUNC_INFO);
            });
    }

    return makeQOhosLazyFetchMimeDataFactory(std::move(threadSafeSuppliers));
}

QOhosSupplier<std::unique_ptr<QMimeData>> makeDummyQMimeDataFactoryFromUdmfDataTypes(
    std::vector<std::string> udmfDataTypes)
{
    return [udmfDataTypes = std::move(udmfDataTypes)]() {
        const auto dummyText = QString::fromUtf8(
            "OHOS QPA: Drag data is not available before DROP. This is a system limitation.");

        auto mimeData = std::make_unique<QMimeData>();

        for (const auto &type : udmfDataTypes) {
            if (type == QOhosUdsMeta<::OH_UdsPlainText>::udmfMetaId) {
                mimeData->setText(dummyText);
            } else if (type == QOhosUdsMeta<::OH_UdsHtml>::udmfMetaId) {
                mimeData->setHtml(dummyText);
            } else if (type == QOhosUdsMeta<::OH_UdsPixelMap>::udmfMetaId) {
                mimeData->setImageData(QImage(1, 1, QImage::Format::Format_RGBA8888));
            } else if (type == QOhosUdsMeta<::OH_UdsFileUri>::udmfMetaId || isUdmfMetaFileType(type)) {
                mimeData->setUrls({});
            }
        }

        if (mimeData->formats().isEmpty())
            mimeData->setText(dummyText);

        return mimeData;
    };
}

bool isQOhosUdmfDataConvertedFromThisProcessMimeData(QOhosUdmfData &udmfData)
{
    auto qtAppInfoDataUtdTypeId = tryMapMimeTypeToUtdTypeId(qtAppInfoDataPseudoMimeType);
    if (!qtAppInfoDataUtdTypeId.hasValue()) {
        qOhosPrintfError("%s: Can't map %s to UTD type id", Q_FUNC_INFO, qtAppInfoDataPseudoMimeType);
        return false;
    }

    auto udmfRecords = udmfData.getRecords();
    auto thisProcessAppInfoData = getAppInfoDataForThisProcess();

    return std::any_of(
        udmfRecords.begin(), udmfRecords.end(),
        [&](auto &udmfRecord) {
            auto optAppInfoData = udmfRecord.tryGetGeneralEntry(qtAppInfoDataUtdTypeId.value());
            if (!optAppInfoData.hasValue())
                return false;

            auto appInfoData = optAppInfoData.value();

            return
                static_cast<std::size_t>(appInfoData.size()) == thisProcessAppInfoData.size()
                && std::memcmp(appInfoData.data(), thisProcessAppInfoData.data(), appInfoData.size()) == 0;
        });
}

QOhosOptional<std::string> tryMapUtdTypeIdToMimeType(const std::string &utdTypeId)
{
    auto optUtdWithTypeId = utdCreateOrNull(utdTypeId);
    auto mimeTypes = optUtdWithTypeId
        ? utdGetMimeTypes(optUtdWithTypeId.get())
        : std::vector<std::string>();

    return !mimeTypes.empty()
        ? makeQOhosOptional(mimeTypes.front())
        : makeEmptyQOhosOptional();
}

QOhosOptional<std::string> tryMapMimeTypeToUtdTypeId(const std::string &mimeType)
{
    // FIXME: the `utdGetTypesByMimeType()` result interpretation is not quite clear.
    // Documentaion does not mention why there is a list of possible type ids.
    // Are they ordered in a specific manear? Is it possible to get en ampty list?
    // Improve and fix the code when these are known.
    auto utdTypeIds = utdGetTypesByMimeType(mimeType);
    return
        !utdTypeIds.empty()
            ? makeQOhosOptional(utdTypeIds.front())
            : makeEmptyQOhosOptional();
}

QT_END_NAMESPACE
