// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmclipboard.h"
#include "qwasminputcontext.h"
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

void QWasmClipboard::cut(val event)
{
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi()) {
        // Send synthetic Ctrl+X to make the app cut data to Qt's clipboard
         QWindowSystemInterface::handleKeyEvent(
                     0, QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier, "X");
   }

    commonCopyEvent(event);
}

void QWasmClipboard::copy(val event)
{
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi()) {
        // Send synthetic Ctrl+C to make the app copy data to Qt's clipboard
            QWindowSystemInterface::handleKeyEvent(
                        0, QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, "C");
    }
    commonCopyEvent(event);
}

void QWasmClipboard::paste(val event)
{
    event.call<void>("preventDefault"); // prevent browser from handling drop event

    QWasmIntegration::get()->getWasmClipboard()->sendClipboardData(event);
}

QWasmClipboard::QWasmClipboard()
{
    val clipboard = val::global("navigator")["clipboard"];

    const bool hasPermissionsApi = !val::global("navigator")["permissions"].isUndefined();
    m_hasClipboardApi = !clipboard.isUndefined() && !clipboard["readText"].isUndefined();

    if (m_hasClipboardApi && hasPermissionsApi)
        initClipboardPermissions();

    if (!shouldInstallWindowEventHandlers()) {
        val document = val::global("document");
        m_documentCut = QWasmEventHandler(document, "cut", QWasmClipboard::cut);
        m_documentCopy = QWasmEventHandler(document, "copy", QWasmClipboard::copy);
        m_documentPaste = QWasmEventHandler(document, "paste", QWasmClipboard::paste);
    }
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

bool QWasmClipboard::hasClipboardApi()
{
    return m_hasClipboardApi;
}

bool QWasmClipboard::shouldInstallWindowEventHandlers()
{
    // Chrome uses global handlers
    return val::global("window")["chrome"].isUndefined();
}

void QWasmClipboard::writeToClipboardApi()
{
    Q_ASSERT(m_hasClipboardApi);

    QMimeData *mimeData = this->mimeData(QClipboard::Clipboard);
    if (!mimeData)
        return;

    // Support for plain text, html and images (png) are standardized,
    // copy those to the clipboard data object.
    emscripten::val clipboardData = emscripten::val::object();
    for (const QString &mimetype: mimeData->formats()) {
        if (mimetype == QLatin1String("text/plain")) {
            emscripten::val text = mimeData->text().toEcmaString();
            clipboardData.set(mimetype.toEcmaString(), text);
        } else if (mimetype == QLatin1String("text/html")) {
            emscripten::val html = mimeData->html().toEcmaString();
            clipboardData.set(mimetype.toEcmaString(), html);
        } else if (mimetype.contains(QLatin1String("image"))) {
            // Serialize the Qt image data to browser supported png
            QImage img = qvariant_cast<QImage>(mimeData->imageData());
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");

            qstdweb::Blob blob = qstdweb::Blob::fromArrayBuffer(qstdweb::Uint8Array::copyFrom(ba).buffer());
            clipboardData.set(std::string("image/png"), blob.val());
        }
    }

    // Return if there is no data (creating an empty ClipboardItem is an error)
    if (val::global("Object").call<val>("keys", clipboardData)["length"].as<int>() == 0)
        return;

    // Write a single clipboard item containing the data formats to the clipboard
    emscripten::val clipboardItem = val::global("ClipboardItem").new_(clipboardData);
    emscripten::val clipboardItemArray = emscripten::val::array();
    clipboardItemArray.call<void>("push", clipboardItem);
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
        clipboardItemArray);
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

void QWasmClipboard::sendClipboardData(emscripten::val event)
{
    dom::DataTransfer *transfer = new dom::DataTransfer(event["clipboardData"]);
    const auto mimeCallback = std::function([transfer](QMimeData *data) {

        // Persist clipboard data so that the app can read it when handling the CTRL+V
        QWasmIntegration::get()->clipboard()->QPlatformClipboard::setMimeData(data, QClipboard::Clipboard);
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyPress, Qt::Key_V,
                                               Qt::ControlModifier, "V");
        QWindowSystemInterface::handleKeyEvent(0, QEvent::KeyRelease, Qt::Key_V,
                                               Qt::ControlModifier, "V");
        delete transfer;
    });

    transfer->toMimeDataWithFile(mimeCallback);
}
QT_END_NAMESPACE
