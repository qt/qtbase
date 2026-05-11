// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSHOVEREVENTSGENERATOR_H
#define QOHOSHOVEREVENTSGENERATOR_H

#include <QtCore/qglobal.h>
#include <QtGui/qwindow.h>
#include <memory>
#include <qohosinputmethodeventhandler.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

class QOhosHoverEventsGenerator
{
public:
    virtual ~QOhosHoverEventsGenerator();

    virtual void handleQOhosMouseEvent(const QOhosMouseEvent &mouseEvent) = 0;
    virtual void handleQOhosHoverEvent(bool hovered) = 0;

protected:
    QOhosHoverEventsGenerator();
};

std::shared_ptr<QOhosHoverEventsGenerator> makeQOhosHoverEventsGenerator(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef);

QT_END_NAMESPACE

#endif // QOHOSHOVEREVENTSGENERATOR_H
