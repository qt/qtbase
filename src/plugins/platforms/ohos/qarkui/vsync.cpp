// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/vsync.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <native_vsync/native_vsync.h>
#include <native_window/external_window.h>
#include <qarkui/qarkuiutils.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <render/qohosbatchingrequestshandler.h>
#include <unordered_set>

using VsyncId = QtOhos::TypedId<std::uintptr_t, struct VsyncIdTag>;

template<>
struct std::hash<VsyncId>
{
    std::size_t operator()(const VsyncId &vsyncId) const noexcept;
};

QT_BEGIN_NAMESPACE

namespace QArkUi {

namespace {

VsyncId generateVsyncId()
{
    static std::uint64_t lastIdValue = 0;
    static_assert(sizeof(std::uint64_t) == sizeof(std::uintptr_t), "uintptr_t size mismatch");
    auto vsyncIdValue = lastIdValue;
    lastIdValue++;
    return VsyncId(vsyncIdValue);
}

std::string generateVSyncName(::OHNativeWindow *nativeWindow, VsyncId id)
{
    return QtOhos::printfToString("__qt_vsync_%p_%lu", nativeWindow, id.value());
}

class QOhosVSyncRegistry
{
public:
    static QOhosVSyncRegistry &instance();

    std::function<void()> create(
        ::OHNativeWindow *nativeWindow, std::function<void()> vsyncFrameReadyFunc);

private:
    static void frameCallback(long long timestamp, void *userData);

    struct VSyncContext
    {
        std::function<void()> vsyncFrameReadyFunc;
        std::shared_ptr<::OH_NativeVSync> vsync;
    };

    QOhosVSyncRegistry();
    void notifyFrameReadyFromAnyThread(VsyncId id);

    std::map<VsyncId, std::shared_ptr<VSyncContext>> m_registry;
    QOhosConsumer<VsyncId> m_notifyFrameReadyFromAnyThread;
};

QOhosVSyncRegistry &QOhosVSyncRegistry::instance()
{
    static QOhosVSyncRegistry result;
    return result;
}

void QOhosVSyncRegistry::frameCallback(long long, void *userData)
{
    auto vsyncId = VsyncId(reinterpret_cast<std::uintptr_t>(userData));
    instance().m_notifyFrameReadyFromAnyThread(vsyncId);
}

std::function<void()> QOhosVSyncRegistry::create(
    ::OHNativeWindow *nativeWindow, std::function<void()> vsyncFrameReadyFunc)
{
    auto vsyncId = generateVsyncId();
    auto vsyncName = generateVSyncName(nativeWindow, vsyncId);

    std::uint64_t surfaceId = 0;
    callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_NativeWindow_GetSurfaceId),
        nativeWindow, &surfaceId);

    auto vsync = std::shared_ptr<::OH_NativeVSync>(
        callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_NativeVSync_Create_ForAssociatedWindow),
            surfaceId, vsyncName.c_str(), vsyncName.length()),
        [](::OH_NativeVSync *vsync) {
            callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_NativeVSync_Destroy),
                vsync);
        });

    auto context = QtOhos::moveToSharedPtr(
        VSyncContext{
            .vsyncFrameReadyFunc = std::move(vsyncFrameReadyFunc),
            .vsync = vsync,
        });

    m_registry.emplace(vsyncId, context);

    auto contextWithDeleter =
        QtOhos::makeSharedPtrWithAttachedExtraData(
            context,
            QtOhos::makeDestroyNotifier(
                [this, vsyncId]() {
                    std::ignore = m_registry.erase(vsyncId);
                }));

    return [vsyncId, contextWithDeleter]() {
        callArkUiOrFailOnErrorResult(
            Q_OHOS_NAMED_FUNC(::OH_NativeVSync_RequestFrame),
            contextWithDeleter->vsync.get(), &QOhosVSyncRegistry::frameCallback,
            reinterpret_cast<void *>(vsyncId.value()));
    };
}

QOhosVSyncRegistry::QOhosVSyncRegistry()
{
    auto vsyncFrameReadyHandler = makeQtOhosBatchingMTRequestsHandler<std::unordered_set<VsyncId>>(
        [](std::function<void()> task) {
            QtOhos::invokeInJsThread(
                [task = std::move(task)](QtOhos::JsState &){
                    task();
                });
        },
        [this](std::unordered_set<VsyncId> &&vsyncIds) {
            for (auto vsyncId: vsyncIds) {
                auto vsyncContextIt = m_registry.find(vsyncId);
                if (vsyncContextIt != m_registry.end()) {
                    auto vsyncContext = vsyncContextIt->second;
                    vsyncContext->vsyncFrameReadyFunc();
                }
            }
        });

    m_notifyFrameReadyFromAnyThread = [vsyncFrameReadyHandler = std::move(vsyncFrameReadyHandler)](VsyncId vsyncId) {
        vsyncFrameReadyHandler(
            [&](std::unordered_set<VsyncId> &vsyncIds) {
                vsyncIds.insert(vsyncId);
            });
    };
}

}

std::function<void()> makeVSyncFrameRequester(
    ::OHNativeWindow *nativeWindow, std::function<void()> vsyncFrameReadyFunc)
{
    return QOhosVSyncRegistry::instance().create(nativeWindow, std::move(vsyncFrameReadyFunc));
}

}

std::size_t std::hash<VsyncId>::operator()(const VsyncId &vsyncId) const noexcept
{
    return vsyncId.value();
}

QT_END_NAMESPACE
