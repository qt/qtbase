// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosclipboardobject.h>

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qurl.h>
#include <database/pasteboard/oh_pasteboard.h>
#include <database/pasteboard/oh_pasteboard_err_code.h>
#include <database/udmf/udmf.h>
#include <database/udmf/uds.h>
#include <memory>
#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <qarkui/qarkuiutils.h>
#include <qohosapppermissions_p.h>
#include <qohosplugincore.h>
#include <qohosudmfconversions.h>
#include <qohosutils.h>

QT_BEGIN_NAMESPACE

namespace {

const auto mimeTextPlain = QString::fromUtf8("text/plain");
const auto mimeTextHtml = QString::fromUtf8("text/html");
const auto mimeTextUriList = QString::fromUtf8("text/uri-list");
const auto mimeAppXQtImage = QString::fromUtf8("application/x-qt-image");

struct PasteboardObserverContext
{
    std::weak_ptr<QOhosClipboardObject> weakThis;
};

const char *getPasteboardNotifyTypeAsString(::Pasteboard_NotifyType notifyType)
{
    switch (notifyType) {
    case ::Pasteboard_NotifyType::NOTIFY_LOCAL_DATA_CHANGE:
        return "NOTIFY_LOCAL_DATA_CHANGE";
    case ::Pasteboard_NotifyType::NOTIFY_REMOTE_DATA_CHANGE:
        return "NOTIFY_REMOTE_DATA_CHANGE";
    }
    return "<illegal-Pasteboard_NotifyType>";
}

const char *getPasteboardDataSourceAsString(QOhosClipboardObject::PasteboardDataSource dataSource)
{
    switch (dataSource) {
    case QOhosClipboardObject::PasteboardDataSource::OurProcess:
        return "OurProcess";
    case QOhosClipboardObject::PasteboardDataSource::OtherProcess:
        return "OtherProcess";
    }
    return "<illegal-PasteboardDataSource>";
}

std::shared_ptr<::OH_PasteboardObserver> createPasteboardDataObserver()
{
    return std::shared_ptr<::OH_PasteboardObserver>(
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_PasteboardObserver_Create)),
        [](auto *pasteboardDataObserver) {
            int observerDestroyRes = QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_PasteboardObserver_Destroy),
                pasteboardDataObserver);
            if (observerDestroyRes != ::PASTEBOARD_ErrCode::ERR_OK) {
                qOhosPrintfError(
                    "%s: failed at destroying pasteboard observer.", Q_FUNC_INFO);
            }
        });
}

std::shared_ptr<void> subscribePasteboardObserver(
    std::shared_ptr<::OH_Pasteboard> pasteboard,
    std::shared_ptr<::OH_PasteboardObserver> pasteboardObserver,
    ::Pasteboard_NotifyType notifyType,
    std::function<void()> dataChangedFunc)
{
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_Pasteboard_Subscribe),
        pasteboard.get(), notifyType, pasteboardObserver.get());

    auto sharedDataChangedFunc = QtOhos::moveToSharedPtr(std::move(dataChangedFunc));

    auto subscriptionHandle = QtOhos::makeDestroyNotifier(
        [pasteboard, notifyType, pasteboardObserver, sharedDataChangedFunc]() {
            int unsubscribeRes = QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_Pasteboard_Unsubscribe),
                pasteboard.get(), notifyType, pasteboardObserver.get());
            if (unsubscribeRes != ::PASTEBOARD_ErrCode::ERR_OK) {
                qOhosPrintfError(
                    "%s: failed at unsubscribing from pasteboard %s.", Q_FUNC_INFO,
                    getPasteboardNotifyTypeAsString(notifyType));
            }
        });

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_PasteboardObserver_SetData),
        pasteboardObserver.get(), sharedDataChangedFunc.get(),
        [](void *ctx, ::Pasteboard_NotifyType) {
            auto *dataChangedFunc = reinterpret_cast<std::function<void()> *>(ctx);
            (*dataChangedFunc)();
        },
        [](void *) {});

    return subscriptionHandle;
}

std::shared_ptr<void> addPasteboardDataChangedListener(
    std::shared_ptr<::OH_Pasteboard> pasteboard, std::function<void()> dataChangedListener,
    std::vector<::Pasteboard_NotifyType> monitoredNotifyTypes)
{
    std::vector<std::shared_ptr<void>> subscriptionHandles;
    auto sharedDataChangedFunc = QtOhos::moveToSharedPtr(std::move(dataChangedListener));

    for (auto notifyType : monitoredNotifyTypes) {
        auto observer = createPasteboardDataObserver();
        subscriptionHandles.push_back(
            subscribePasteboardObserver(
                pasteboard, observer, notifyType,
                [sharedDataChangedFunc]() {
                    (*sharedDataChangedFunc)();
                }));
    }

    return QtOhos::moveToSharedPtr(std::move(subscriptionHandles));
}

std::unique_ptr<QOhosUdmfData> tryGetUdmfDataFromPasteboard(::OH_Pasteboard *pasteboard)
{
    if (!QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_Pasteboard_HasData), pasteboard))
        return nullptr;

    int res;
    auto *udmfDataPtr = QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_Pasteboard_GetData),
        pasteboard, &res);

    if (res != ::PASTEBOARD_ErrCode::ERR_OK || udmfDataPtr == nullptr) {
        qOhosPrintfError("%s: error reading pasteboard data: %d / %p", Q_FUNC_INFO, res, udmfDataPtr);
        return nullptr;
    }

    return std::make_unique<QOhosUdmfData>(QOhosUdmfData::takeOwnership(udmfDataPtr));
}

}

QOhosClipboardObject::QOhosClipboardObject(
    std::function<void(QOhosOptional<PasteboardDataSource>)> &&pasteboardUpdatesNotifier)
{
    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            m_pasteboard = std::shared_ptr<::OH_Pasteboard>(
                QArkUi::callArkUiOrFailOnNullResult(
                    Q_OHOS_NAMED_FUNC(::OH_Pasteboard_Create)),
                [](auto *pasteboard) {
                    QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_Pasteboard_Destroy), pasteboard);
                });
            auto sharedPasteboardUpdatesNotifier = QtOhos::moveToSharedPtr(
                std::move(pasteboardUpdatesNotifier));

            auto pasteboardDataChangedJsThreadListener = std::make_shared<std::function<void()>>(
                [this, weakPasteboardUpdatesNotifier = QtOhos::makeWeakPtr(sharedPasteboardUpdatesNotifier)]() {
                    auto optPasteboardUdmfData = tryGetUdmfDataFromPasteboard(m_pasteboard.get());
                    auto pasteboardDataSource =
                        optPasteboardUdmfData
                            ? makeQOhosOptional(
                                isQOhosUdmfDataConvertedFromThisProcessMimeData(*optPasteboardUdmfData)
                                    ? PasteboardDataSource::OurProcess
                                    : PasteboardDataSource::OtherProcess)
                            : makeEmptyQOhosOptional();

                    qOhosPrintfDebug(
                        "%s: Pasteboard data source: %s",
                        Q_FUNC_INFO, pasteboardDataSource.transform(&getPasteboardDataSourceAsString).valueOr("<empty>"));

                    QtOhos::invokeInQtThread(
                        [weakPasteboardUpdatesNotifier, pasteboardDataSource]() {
                            auto sharedPasteboardUpdatesNotifier = weakPasteboardUpdatesNotifier.lock();
                            if (sharedPasteboardUpdatesNotifier)
                                (*sharedPasteboardUpdatesNotifier)(pasteboardDataSource);
                        });
                });

            m_pasteboardDataChangedListenerHandle = QtOhos::makeSharedPtrWithAttachedExtraData(
                addPasteboardDataChangedListener(
                    m_pasteboard,
                    [pasteboardDataChangedJsThreadListener]() {
                        auto __dbg = make_QCScopedDebugJS(Q_FUNC_INFO);
                        QtOhos::invokeInJsThread(
                            [pasteboardDataChangedJsThreadListener](auto &) {
                                (*pasteboardDataChangedJsThreadListener)();
                            });
                    },
                    {
                        ::Pasteboard_NotifyType::NOTIFY_LOCAL_DATA_CHANGE,
                        ::Pasteboard_NotifyType::NOTIFY_REMOTE_DATA_CHANGE,
                    }),
                sharedPasteboardUpdatesNotifier);
        },
        Q_FUNC_INFO);
}

std::unique_ptr<QOhosClipboardObject> QOhosClipboardObject::makeInstance(
    std::function<void(QOhosOptional<QOhosClipboardObject::PasteboardDataSource>)> &&pasteboardUpdatesNotifier)
{
    return std::unique_ptr<QOhosClipboardObject>(
        new QOhosClipboardObject(std::move(pasteboardUpdatesNotifier)));
}

QOhosClipboardObject::PasteboardData QOhosClipboardObject::getPasteboardDataWithLazyFetch()
{
    QOhosOptional<PasteboardDataSource> dataSource;
    QOhosSupplier<std::unique_ptr<QMimeData>> mimeDataFactory;
    std::tie(dataSource, mimeDataFactory) = QtOhos::evalInJsThreadWithPromise<std::pair<QOhosOptional<PasteboardDataSource>, QOhosSupplier<std::unique_ptr<QMimeData>>>>(
        [&](QtOhos::JsState &jsState, auto evalPromise) {
            static constexpr const char *ohosGetPasteboardDataPermission = "ohos.permission.READ_PASTEBOARD";
            auto sharedEvalPromise = QtOhos::moveToSharedPtr(std::move(evalPromise).makeChained(Q_FUNC_INFO));
            QOhosAppPermissions::checkAppPermissionGrantedWithConsumer(
                jsState, ohosGetPasteboardDataPermission,
                [this, sharedEvalPromise](auto &, bool permissionGranted) {
                    if (!permissionGranted) {
                        qOhosPrintfError(
                            "%s: %s hasn't been granted by user. Cannot read pasteboard data.", Q_FUNC_INFO,
                            ohosGetPasteboardDataPermission);
                        (*sharedEvalPromise)({{}, std::make_unique<QMimeData>});
                        return;
                    }

                    auto optPasteboardUdmfData = tryGetUdmfDataFromPasteboard(m_pasteboard.get());
                    if (!optPasteboardUdmfData) {
                        (*sharedEvalPromise)({{}, std::make_unique<QMimeData>});
                        return;
                    }

                    (*sharedEvalPromise)(
                        {
                            makeQOhosOptional(
                                isQOhosUdmfDataConvertedFromThisProcessMimeData(*optPasteboardUdmfData)
                                    ? PasteboardDataSource::OurProcess
                                    : PasteboardDataSource::OtherProcess),
                            makeLazyFetchingQMimeDataFactoryFromUdmfData(std::move(*optPasteboardUdmfData))
                        });
                });
        },
        Q_FUNC_INFO);

    return PasteboardData{
        .dataSource = dataSource,
        .lazyFetchingData = mimeDataFactory(),
    };
}

void QOhosClipboardObject::setMimeDataSync(
    std::shared_ptr<QMimeData> mimeData, const QOhosOptional<bool> &shareInAppOnly)
{
    if (!mimeData)
        return;

    auto udmfDataFactory = makeLazyProcessingUdmfDataFactoryFromQMimeData(mimeData, shareInAppOnly);

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &) {
            auto udmfData = udmfDataFactory();

            int res = QArkUi::callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_Pasteboard_SetData),
                m_pasteboard.get(), udmfData.nativePtr());
            if (res != ::PASTEBOARD_ErrCode::ERR_OK) {
                qOhosPrintfError("%s: cannot set data for pasteboard.", Q_FUNC_INFO);
                return;
            }
        },
        Q_FUNC_INFO);
}

QT_END_NAMESPACE
