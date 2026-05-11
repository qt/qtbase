// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <qarkui/qarkuiutils.h>
#include <qarkui/qohosdragaction.h>
#include <qohospixelmapconversions.h>
#include <qohosutils.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QArkUi {

DragAction::DragAction(::ArkUI_NodeHandle node)
    : m_dragAction(
        QArkUi::callArkUiOrFailOnNullResult(
            Q_OHOS_NAMED_FUNC(::OH_ArkUI_CreateDragActionWithNode),
            node),
        &::OH_ArkUI_DragAction_Dispose)
{
}

void DragAction::setPointerId(std::int32_t pointerId)
{
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_SetPointerId),
        m_dragAction.get(), pointerId);
}

void DragAction::setTouchPoint(float touchX, float touchY)
{
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_SetTouchPointX),
        m_dragAction.get(), touchX);
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_SetTouchPointY),
        m_dragAction.get(), touchY);
}

void DragAction::setPixelMaps(std::vector<std::shared_ptr<::OH_PixelmapNative>> pixelMaps)
{
    m_pixelMaps = std::move(pixelMaps);
    m_pixelMapsPointers.resize(m_pixelMaps.size() + 1);

    for (std::size_t i = 0; i < m_pixelMaps.size(); ++i)
        m_pixelMapsPointers[i] = m_pixelMaps[i].get();
    m_pixelMapsPointers[m_pixelMaps.size()] = nullptr;

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_SetPixelMaps),
        m_dragAction.get(), m_pixelMapsPointers.data(), m_pixelMaps.size());
}

void DragAction::setData(QOhosUdmfData udmfData)
{
    m_udmfData = QtOhos::moveToSharedPtr(std::move(udmfData));

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_SetData),
        m_dragAction.get(), m_udmfData->nativePtr());
}

void DragAction::setStatusListener(
    std::function<void(::ArkUI_DragAndDropInfo *)> statusListener)
{
    resetStatusListener();

    auto sharedStatusListener = QtOhos::moveToSharedPtr(std::move(statusListener));

    m_statusListenerRegistrationHandle = QtOhos::makeDestroyNotifier(
        [sharedStatusListener, weakDragAction = QtOhos::makeWeakPtr(m_dragAction)]() {
            auto dragAction = weakDragAction.lock();
            if (dragAction) {
                QArkUi::callArkUi(
                    Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_UnregisterStatusListener),
                    dragAction.get());
            }
        });

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_RegisterStatusListener),
        m_dragAction.get(), sharedStatusListener.get(),
        [](::ArkUI_DragAndDropInfo *dragAndDropInfo, void *statusListenerVoidPtr) {
            auto *statusListener = static_cast<std::function<void(::ArkUI_DragAndDropInfo *)> *>(statusListenerVoidPtr);
            (*statusListener)(dragAndDropInfo);
        });
}

void DragAction::resetStatusListener()
{
    m_statusListenerRegistrationHandle.reset();
}

void DragAction::setDragPreviewOption(ArkUI_DragPreviewOption &option)
{
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragAction_SetDragPreviewOption),
        m_dragAction.get(), &option);
}

::ArkUI_DragAction *DragAction::nativePtr()
{
    return m_dragAction.get();
}

}

QT_END_NAMESPACE
