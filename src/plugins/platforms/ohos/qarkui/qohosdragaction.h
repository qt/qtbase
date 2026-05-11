// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSDRAGACTION_H
#define QOHOSDRAGACTION_H

#include <QtCore/qglobal.h>
#include <arkui/drag_and_drop.h>
#include <arkui/native_type.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <qohosudmf.h>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QArkUi {

class DragAction
{
public:
    explicit DragAction(::ArkUI_NodeHandle node);

    void setPointerId(std::int32_t pointerId);
    void setTouchPoint(float x, float y);
    void setPixelMaps(std::vector<std::shared_ptr<::OH_PixelmapNative>> pixelMaps);
    void setData(QOhosUdmfData udmfData);
    void setStatusListener(std::function<void(::ArkUI_DragAndDropInfo *)> statusListener);
    void resetStatusListener();
    void setDragPreviewOption(ArkUI_DragPreviewOption &option);

    ::ArkUI_DragAction *nativePtr();

private:
    std::shared_ptr<::ArkUI_DragAction> m_dragAction;
    std::vector<std::shared_ptr<::OH_PixelmapNative>> m_pixelMaps;
    std::vector<::OH_PixelmapNative *> m_pixelMapsPointers;
    std::shared_ptr<QOhosUdmfData> m_udmfData;
    std::shared_ptr<void> m_statusListenerRegistrationHandle;
};

}

QT_END_NAMESPACE

#endif
