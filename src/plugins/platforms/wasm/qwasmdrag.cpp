// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmdrag.h"

#include "qwasmdom.h"
#include "qwasmeventtranslator.h"
#include <qpa/qwindowsysteminterface.h>
#include <QMimeData>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

#include <memory>
#include <string>

QT_BEGIN_NAMESPACE

namespace {
Qt::DropAction parseDropActions(emscripten::val event)
{
    const std::string dEffect = event["dataTransfer"]["dropEffect"].as<std::string>();

    if (dEffect == "copy")
        return Qt::CopyAction;
    else if (dEffect == "move")
        return Qt::MoveAction;
    else if (dEffect == "link")
        return Qt::LinkAction;
    return Qt::IgnoreAction;
}
} // namespace

void dropEvent(emscripten::val event)
{
    // someone dropped a file into the browser window
    // event is dataTransfer object
    // if drop event from outside browser, we do not get any mouse release, maybe mouse move
    // after the drop event
    event.call<void>("preventDefault"); // prevent browser from handling drop event

    static std::shared_ptr<qstdweb::CancellationFlag> readDataCancellation = nullptr;
    readDataCancellation = qstdweb::readDataTransfer(
            event["dataTransfer"],
            [](QByteArray fileContent) {
                QImage image;
                image.loadFromData(fileContent, nullptr);
                return image;
            },
            [wasmScreen = reinterpret_cast<QWasmScreen *>(
                     event["currentTarget"]["data-qtdropcontext"].as<quintptr>()),
             event](std::unique_ptr<QMimeData> data) {
                const auto mouseDropPoint = QPoint(event["x"].as<int>(), event["y"].as<int>());
                const auto button = MouseEvent::buttonFromWeb(event["button"].as<int>());
                const Qt::KeyboardModifiers modifiers = KeyboardModifier::getForEvent(event);
                const auto dropAction = parseDropActions(event);
                auto *window = wasmScreen->topLevelAt(mouseDropPoint);

                QWindowSystemInterface::handleDrag(window, data.get(), mouseDropPoint, dropAction,
                                                   button, modifiers);

                // drag drop
                QWindowSystemInterface::handleDrop(window, data.get(), mouseDropPoint, dropAction,
                                                   button, modifiers);

                // drag leave
                QWindowSystemInterface::handleDrag(window, nullptr, QPoint(), Qt::IgnoreAction, {},
                                                   {});
            });
}

EMSCRIPTEN_BINDINGS(drop_module)
{
    function("qtDrop", &dropEvent);
}

QT_END_NAMESPACE
