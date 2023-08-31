// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmclipboard.h"
#include "qwasmdom.h"
#include "qwasmevent.h"
#include "qwasmwindow.h"

#include <private/qstdweb_p.h>

#include <QCoreApplication>
#include <qpa/qwindowsysteminterface.h>
#include <QBuffer>
#include <QString>

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE
using namespace emscripten;

static void commonCopyEvent(val event)
{
    QMimeData *_mimes = QWasmIntegration::get()->getWasmClipboard()->mimeData(QClipboard::Clipboard);
    if (!_mimes)
      return;

    // doing it this way seems to sanitize the text better that calling data() like down below
    if (_mimes->hasText()) {
        event["clipboardData"].call<void>("setData", val("text/plain"),
                                          _mimes->text().toEcmaString());
    }
    if (_mimes->hasHtml()) {
        event["clipboardData"].call<void>("setData", val("text/html"), _mimes->html().toEcmaString());
    }

    for (auto mimetype : _mimes->formats()) {
        if (mimetype.contains("text/"))
            continue;
        QByteArray ba = _mimes->data(mimetype);
        if (!ba.isEmpty())
            event["clipboardData"].call<void>("setData", mimetype.toEcmaString(),
                                              val(ba.constData()));
    }

    event.call<void>("preventDefault");
}

static void qClipboardCutTo(val event)
{
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi()) {
        // Send synthetic Ctrl+X to make the app cut data to Qt's clipboard
         QWindowSystemInterface::handleKeyEvent(
                     0, QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier, "X");
   }

    commonCopyEvent(event);
}

static void qClipboardCopyTo(val event)
{
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi()) {
        // Send synthetic Ctrl+C to make the app copy data to Qt's clipboard
            QWindowSystemInterface::handleKeyEvent(
                        0, QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, "C");
    }
    commonCopyEvent(event);
}

static void qClipboardPasteTo(val event)
{
    event.call<void>("preventDefault"); // prevent browser from handling drop event

    static std::shared_ptr<qstdweb::CancellationFlag> readDataCancellation = nullptr;
    readDataCancellation = qstdweb::readDataTransfer(
            event["clipboardData"],
            [](QByteArray fileContent) {
                QImage image;
                image.loadFromData(fileContent, nullptr);
                return image;
            },
            [event](std::unique_ptr<QMimeData> data) {
                if (data->formats().isEmpty())
                    return;

                // Persist clipboard data so that the app can read it when handling the CTRL+V
                QWasmIntegration::get()->clipboard()->QPlatformClipboard::setMimeData(
                        data.release(), QClipboard::Clipboard);

                QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyPress, Qt::Key_V,
                                                       Qt::ControlModifier, "V");
            });
}

EMSCRIPTEN_BINDINGS(qtClipboardModule) {
    function("qtClipboardCutTo", &qClipboardCutTo);
    function("qtClipboardCopyTo", &qClipboardCopyTo);
    function("qtClipboardPasteTo", &qClipboardPasteTo);
}

QWasmClipboard::QWasmClipboard()
{
    val clipboard = val::global("navigator")["clipboard"];

    const bool hasPermissionsApi = !val::global("navigator")["permissions"].isUndefined();
    m_hasClipboardApi = !clipboard.isUndefined() && !clipboard["readText"].isUndefined();

    if (m_hasClipboardApi && hasPermissionsApi)
        initClipboardPermissions();
}

QWasmClipboard::~QWasmClipboard()
{
}

QMimeData *QWasmClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return nullptr;

    return QPlatformClipboard::mimeData(mode);
}

void QWasmClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    // handle setText/ setData programmatically
    QPlatformClipboard::setMimeData(mimeData, mode);
    if (m_hasClipboardApi)
        writeToClipboardApi();
    else
        writeToClipboard();
}

QWasmClipboard::ProcessKeyboardResult QWasmClipboard::processKeyboard(const KeyEvent &event)
{
    if (event.type != EventType::KeyDown || !event.modifiers.testFlag(Qt::ControlModifier))
        return ProcessKeyboardResult::Ignored;

    if (event.key != Qt::Key_C && event.key != Qt::Key_V && event.key != Qt::Key_X)
        return ProcessKeyboardResult::Ignored;

    const bool isPaste = event.key == Qt::Key_V;

    return m_hasClipboardApi && !isPaste
            ? ProcessKeyboardResult::NativeClipboardEventAndCopiedDataNeeded
            : ProcessKeyboardResult::NativeClipboardEventNeeded;
}

bool QWasmClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QWasmClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

void QWasmClipboard::initClipboardPermissions()
{
    val permissions = val::global("navigator")["permissions"];

    qstdweb::Promise::make(permissions, "query", { .catchFunc = [](emscripten::val) {} }, ([]() {
                               val readPermissionsMap = val::object();
                               readPermissionsMap.set("name", val("clipboard-read"));
                               return readPermissionsMap;
                           })());
    qstdweb::Promise::make(permissions, "query", { .catchFunc = [](emscripten::val) {} }, ([]() {
                               val readPermissionsMap = val::object();
                               readPermissionsMap.set("name", val("clipboard-write"));
                               return readPermissionsMap;
                           })());
}

void QWasmClipboard::installEventHandlers(const emscripten::val &target)
{
    emscripten::val cContext = val::undefined();
    emscripten::val isChromium = val::global("window")["chrome"];
    if (!isChromium.isUndefined()) {
        cContext = val::global("document");
    } else {
        cContext = target;
    }
    // Fallback path for browsers which do not support direct clipboard access
    cContext.call<void>("addEventListener", val("cut"),
                        val::module_property("qtClipboardCutTo"), true);
    cContext.call<void>("addEventListener", val("copy"),
                        val::module_property("qtClipboardCopyTo"), true);
    cContext.call<void>("addEventListener", val("paste"),
                        val::module_property("qtClipboardPasteTo"), true);
}

bool QWasmClipboard::hasClipboardApi()
{
    return m_hasClipboardApi;
}

void QWasmClipboard::writeToClipboardApi()
{
    Q_ASSERT(m_hasClipboardApi);

    // copy event
    // browser event handler detected ctrl c if clipboard API
    // or Qt call from keyboard event handler

    QMimeData *_mimes = mimeData(QClipboard::Clipboard);
    if (!_mimes)
        return;

    emscripten::val clipboardWriteArray = emscripten::val::array();
    QByteArray ba;

    for (auto mimetype : _mimes->formats()) {
        // we need to treat binary and text differently, as the blob method below
        // fails for text mimetypes
        // ignore text types

        if (mimetype.contains("STRING", Qt::CaseSensitive) || mimetype.contains("TEXT", Qt::CaseSensitive))
            continue;

        if (_mimes->hasHtml()) { // prefer html over text
            ba = _mimes->html().toLocal8Bit();
            // force this mime
            mimetype = "text/html";
        } else if (mimetype.contains("text/plain")) {
            ba = _mimes->text().toLocal8Bit();
        } else if (mimetype.contains("image")) {
            QImage img = qvariant_cast<QImage>( _mimes->imageData());
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            mimetype = "image/png"; // chrome only allows png
            // clipboard error "NotAllowedError" "Type application/x-qt-image not supported on write."
            // safari silently fails
            // so we use png internally for now
        } else {
            // DATA
            ba = _mimes->data(mimetype);
        }
        // Create file data Blob

        const char *content = ba.data();
        int dataLength = ba.length();
        if (dataLength < 1) {
            qDebug() << "no content found";
            return;
        }

        emscripten::val document = emscripten::val::global("document");
        emscripten::val window = emscripten::val::global("window");

        emscripten::val fileContentView =
                emscripten::val(emscripten::typed_memory_view(dataLength, content));
        emscripten::val fileContentCopy = emscripten::val::global("ArrayBuffer").new_(dataLength);
        emscripten::val fileContentCopyView =
                emscripten::val::global("Uint8Array").new_(fileContentCopy);
        fileContentCopyView.call<void>("set", fileContentView);

        emscripten::val contentArray = emscripten::val::array();
        contentArray.call<void>("push", fileContentCopyView);

        // we have a blob, now create a ClipboardItem
        emscripten::val type = emscripten::val::array();
        type.set("type", mimetype.toEcmaString());

        emscripten::val contentBlob = emscripten::val::global("Blob").new_(contentArray, type);

        emscripten::val clipboardItemObject = emscripten::val::object();
        clipboardItemObject.set(mimetype.toEcmaString(), contentBlob);

        val clipboardItemData = val::global("ClipboardItem").new_(clipboardItemObject);

        clipboardWriteArray.call<void>("push", clipboardItemData);

        // Clipboard write is only supported with one ClipboardItem at the moment
        // but somehow this still works?
        // break;
    }

    val navigator = val::global("navigator");

    qstdweb::Promise::make(
        navigator["clipboard"], "write",
        {
            .catchFunc = [](emscripten::val error) {
                qWarning() << "clipboard error"
                    << QString::fromStdString(error["name"].as<std::string>())
                    << QString::fromStdString(error["message"].as<std::string>());
            }
        },
        clipboardWriteArray);
}

void QWasmClipboard::writeToClipboard()
{
    // this works for firefox, chrome by generating
    // copy event, but not safari
    // execCommand has been deemed deprecated in the docs, but browsers do not seem
    // interested in removing it. There is no replacement, so we use it here.
    val document = val::global("document");
    document.call<val>("execCommand", val("copy"));
}
QT_END_NAMESPACE
