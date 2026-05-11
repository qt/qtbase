// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/qarkuiutils.h>
#include <render/qohosdrageventutils.h>

QT_BEGIN_NAMESPACE

Qt::DropAction mapQOhosArkUiDropOperationToQt(::ArkUI_DropOperation dropOperation)
{
    switch (dropOperation) {
    case ::ARKUI_DROP_OPERATION_COPY:
        return Qt::CopyAction;
    case ::ARKUI_DROP_OPERATION_MOVE:
        return Qt::MoveAction;
    }
    return Qt::CopyAction;
}

QOhosOptional<::ArkUI_DropOperation> tryMapQOhosArkUiDropOperationFromQt(Qt::DropAction dropAction)
{
    switch (dropAction) {
    case Qt::CopyAction:
        return makeQOhosOptional(::ARKUI_DROP_OPERATION_COPY);
    case Qt::MoveAction:
        return makeQOhosOptional(::ARKUI_DROP_OPERATION_MOVE);
    default:
        return {};
    }
}

::ArkUI_DropOperation getQOhosDragEventDropOperation(::ArkUI_DragEvent *dragEvent)
{
    ::ArkUI_DropOperation dropOperation = ::ARKUI_DROP_OPERATION_COPY;
    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_DragEvent_GetDropOperation),
        dragEvent, &dropOperation);
    return dropOperation;
}

QT_END_NAMESPACE
