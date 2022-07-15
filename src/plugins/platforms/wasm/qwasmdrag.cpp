// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmdrag.h"

#include <iostream>
#include <QMimeDatabase>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <private/qstdweb_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qsimpledrag_p.h>
#include "qwasmcompositor.h"
#include "qwasmeventtranslator.h"
#include <QtCore/QEventLoop>
#include <QMimeData>
#include <private/qshapedpixmapdndwindow_p.h>

QT_BEGIN_NAMESPACE

using namespace emscripten;

static void getTextPlainCallback(val m_string)
{
    QWasmDrag *thisDrag = static_cast<QWasmDrag*>(QWasmIntegration::get()->drag());
    thisDrag->m_mimeData->setText(QString::fromStdString(m_string.as<std::string>()));
    thisDrag->qWasmDrop();
}

static void getTextUrlCallback(val m_string)
{
    QWasmDrag *thisDrag = static_cast<QWasmDrag*>(QWasmIntegration::get()->drag());
    thisDrag->m_mimeData->setData(QStringLiteral("text/uri-list"),
                                          QByteArray::fromStdString(m_string.as<std::string>()));

    thisDrag->qWasmDrop();
}

static void getTextHtmlCallback(val m_string)
{
    QWasmDrag *thisDrag = static_cast<QWasmDrag*>(QWasmIntegration::get()->drag());
    thisDrag->m_mimeData->setHtml(QString::fromStdString(m_string.as<std::string>()));

    thisDrag->qWasmDrop();
}

static void dropEvent(val event)
{
    // someone dropped a file into the browser window
    // event is dataTransfer object
    // if drop event from outside browser, we do not get any mouse release, maybe mouse move
    // after the drop event

    // data-context thing was not working here :(
    QWasmDrag *wasmDrag = static_cast<QWasmDrag*>(QWasmIntegration::get()->drag());

    wasmDrag->m_wasmScreen =
            reinterpret_cast<QWasmScreen*>(event["target"]["data-qtdropcontext"].as<quintptr>());

    wasmDrag->m_mouseDropPoint = QPoint(event["x"].as<int>(), event["y"].as<int>());
    if (wasmDrag->m_mimeData)
        delete wasmDrag->m_mimeData;
    wasmDrag->m_mimeData = new QMimeData;
    wasmDrag->m_qButton = MouseEvent::buttonFromWeb(event["button"].as<int>());

    wasmDrag->m_keyModifiers = Qt::NoModifier;
    if (event["altKey"].as<bool>())
        wasmDrag->m_keyModifiers |= Qt::AltModifier;
    if (event["ctrlKey"].as<bool>())
        wasmDrag->m_keyModifiers |= Qt::ControlModifier;
    if (event["metaKey"].as<bool>())
        wasmDrag->m_keyModifiers |= Qt::MetaModifier;

    event.call<void>("preventDefault"); // prevent browser from handling drop event

    std::string dEffect = event["dataTransfer"]["dropEffect"].as<std::string>();

    wasmDrag->m_dropActions = Qt::IgnoreAction;
    if (dEffect == "copy")
         wasmDrag->m_dropActions = Qt::CopyAction;
    if (dEffect == "move")
         wasmDrag->m_dropActions = Qt::MoveAction;
    if (dEffect == "link")
         wasmDrag->m_dropActions = Qt::LinkAction;

    val dt = event["dataTransfer"]["items"]["length"];

    val typesCount = event["dataTransfer"]["types"]["length"];

    // handle mimedata
    int count = dt.as<int>();
    wasmDrag->m_mimeTypesCount = count;
    // kind is file type: file or string
    for (int i=0; i < count; i++) {
        val item = event["dataTransfer"]["items"][i];
        val kind = item["kind"];
        val fileType = item["type"];

        if (kind.as<std::string>() == "file") {
            val m_file = item.call<val>("getAsFile");
            if (m_file.isUndefined()) {
                continue;
            }

            qstdweb::File file(m_file);

            QString mimeFormat = QString::fromStdString(file.type());
            QByteArray fileContent;
            fileContent.resize(file.size());

            file.stream(fileContent.data(), [=]() {
                if (!fileContent.isEmpty()) {

                    if (mimeFormat.contains("image")) {
                        QImage image;
                        image.loadFromData(fileContent, nullptr);
                        wasmDrag->m_mimeData->setImageData(image);
                    } else {
                        wasmDrag->m_mimeData->setData(mimeFormat, fileContent.data());
                    }
                     wasmDrag->qWasmDrop();
                }
            });

        } else { // string

            if (fileType.as<std::string>() == "text/uri-list"
                    || fileType.as<std::string>() == "text/x-moz-url") {
                item.call<val>("getAsString", val::module_property("qtgetTextUrl"));
            } else if (fileType.as<std::string>() == "text/html") {
                item.call<val>("getAsString", val::module_property("qtgetTextHtml"));
            } else { // handle everything else here as plain text
                item.call<val>("getAsString", val::module_property("qtgetTextPlain"));
            }
        }
    }
}

EMSCRIPTEN_BINDINGS(drop_module) {
    function("qtDrop", &dropEvent);
    function("qtgetTextPlain", &getTextPlainCallback);
    function("qtgetTextUrl", &getTextUrlCallback);
    function("qtgetTextHtml", &getTextHtmlCallback);
}


QWasmDrag::QWasmDrag()
{
    init();
}

QWasmDrag::~QWasmDrag()
{
    if (m_mimeData)
        delete m_mimeData;
}

void QWasmDrag::init()
{
}

void QWasmDrag::drop(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    QSimpleDrag::drop(globalPos, b, mods);
}

void QWasmDrag::move(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    QSimpleDrag::move(globalPos, b, mods);
}

void QWasmDrag::qWasmDrop()
{
    // collect mime
    QWasmDrag *thisDrag = static_cast<QWasmDrag*>(QWasmIntegration::get()->drag());

    if (thisDrag->m_mimeTypesCount != thisDrag->m_mimeData->formats().size())
        return; // keep collecting mimetypes

    // start drag enter
    QWindowSystemInterface::handleDrag(thisDrag->m_wasmScreen->topLevelAt(thisDrag->m_mouseDropPoint),
                                       thisDrag->m_mimeData,
                                       thisDrag->m_mouseDropPoint,
                                       thisDrag->m_dropActions,
                                       thisDrag->m_qButton,
                                       thisDrag->m_keyModifiers);

    // drag drop
    QWindowSystemInterface::handleDrop(thisDrag->m_wasmScreen->topLevelAt(thisDrag->m_mouseDropPoint),
                                       thisDrag->m_mimeData,
                                       thisDrag->m_mouseDropPoint,
                                       thisDrag->m_dropActions,
                                       thisDrag->m_qButton,
                                       thisDrag->m_keyModifiers);

    // drag leave
    QWindowSystemInterface::handleDrag(thisDrag->m_wasmScreen->topLevelAt(thisDrag->m_mouseDropPoint),
                                       nullptr,
                                       QPoint(),
                                       Qt::IgnoreAction, { }, { });

    thisDrag->m_mimeData->clear();
    thisDrag->m_mimeTypesCount = 0;
}

QT_END_NAMESPACE
