// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSDRAGEVENTUTILS_H
#define QOHOSDRAGEVENTUTILS_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>
#include <arkui/drag_and_drop.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

Qt::DropAction mapQOhosArkUiDropOperationToQt(::ArkUI_DropOperation dropOperation);

QOhosOptional<::ArkUI_DropOperation> tryMapQOhosArkUiDropOperationFromQt(Qt::DropAction dropAction);

::ArkUI_DropOperation getQOhosDragEventDropOperation(::ArkUI_DragEvent *dragEvent);

QT_END_NAMESPACE

#endif
