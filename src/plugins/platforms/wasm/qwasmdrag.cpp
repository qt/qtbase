// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmdrag.h"

#include "qwasmbase64iconstore.h"
#include "qwasmdom.h"
#include "qwasmevent.h"
#include "qwasmintegration.h"

#include <qpa/qwindowsysteminterface.h>

#include <QtCore/private/qstdweb_p.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qtimer.h>
#include <QFile>

#include <functional>
#include <string>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

QWindow *windowForDrag(QDrag *drag)
{
    QWindow *window = qobject_cast<QWindow *>(drag->source());
    if (window)
        return window;
    if (drag->source()->metaObject()->indexOfMethod("_q_closestWindowHandle()") == -1)
        return nullptr;

    QMetaObject::invokeMethod(drag->source(), "_q_closestWindowHandle",
                              Q_RETURN_ARG(QWindow *, window));
    return window;
}

} // namespace

struct QWasmDrag::DragState
{
    class DragImage
    {
    public:
        DragImage(const QPixmap &pixmap, const QMimeData *mimeData, QWindow *window);
        ~DragImage();

        emscripten::val htmlElement();

    private:
        emscripten::val generateDragImage(const QPixmap &pixmap, const QMimeData *mimeData);
        emscripten::val generateDragImageFromText(const QMimeData *mimeData);
        emscripten::val generateDefaultDragImage();
        emscripten::val generateDragImageFromPixmap(const QPixmap &pixmap);

        emscripten::val m_imageDomElement;
        emscripten::val m_temporaryImageElementParent;
    };

    DragState(QDrag *drag, QWindow *window, std::function<void()> quitEventLoopClosure);
    ~DragState();
    DragState(const QWasmDrag &other) = delete;
    DragState(QWasmDrag &&other) = delete;
    DragState &operator=(const QWasmDrag &other) = delete;
    DragState &operator=(QWasmDrag &&other) = delete;

    QDrag *drag;
    QWindow *window;
    std::function<void()> quitEventLoopClosure;
    std::unique_ptr<DragImage> dragImage;
    Qt::DropAction dropAction = Qt::DropAction::IgnoreAction;
};

QWasmDrag::QWasmDrag() = default;

QWasmDrag::~QWasmDrag() = default;

QWasmDrag *QWasmDrag::instance()
{
    return static_cast<QWasmDrag *>(QWasmIntegration::get()->drag());
}

Qt::DropAction QWasmDrag::drag(QDrag *drag)
{
    Q_ASSERT_X(!m_dragState, Q_FUNC_INFO, "Drag already in progress");

    QWindow *window = windowForDrag(drag);
    if (!window)
        return Qt::IgnoreAction;

    Qt::DropAction dragResult = Qt::IgnoreAction;
    if (qstdweb::haveJspi()) {
        QEventLoop loop;
        m_dragState = std::make_unique<DragState>(drag, window, [&loop]() { loop.quit(); });
        loop.exec();
        dragResult = m_dragState->dropAction;
        m_dragState.reset();
    }

    if (dragResult == Qt::IgnoreAction)
        dragResult = QBasicDrag::drag(drag);

    return dragResult;
}

void QWasmDrag::onNativeDragStarted(DragEvent *event)
{
    Q_ASSERT_X(event->type == EventType::DragStart, Q_FUNC_INFO,
               "The event is not a DragStart event");

    event->webEvent.call<void>("preventDefault");

    // It is possible for a drag start event to arrive from another window.
    if (!m_dragState || m_dragState->window != event->targetWindow) {
        event->cancelDragStart();
        return;
    }

    m_dragState->dragImage = std::make_unique<DragState::DragImage>(
            m_dragState->drag->pixmap(), m_dragState->drag->mimeData(), event->targetWindow);
    event->dataTransfer.setDragImage(m_dragState->dragImage->htmlElement(),
                                     m_dragState->drag->hotSpot());
    event->dataTransfer.setDataFromMimeData(*m_dragState->drag->mimeData());
}

void QWasmDrag::onNativeDragOver(DragEvent *event)
{
    event->webEvent.call<void>("preventDefault");

    auto mimeDataPreview = event->dataTransfer.toMimeDataPreview();

    const Qt::DropActions actions = m_dragState
            ? m_dragState->drag->supportedActions()
            : (Qt::DropAction::CopyAction | Qt::DropAction::MoveAction
               | Qt::DropAction::LinkAction);

    const auto dragResponse = QWindowSystemInterface::handleDrag(
            event->targetWindow, &*mimeDataPreview, event->pointInPage.toPoint(), actions,
            event->mouseButton, event->modifiers);
    event->acceptDragOver();
    if (dragResponse.isAccepted()) {
        event->dataTransfer.setDropAction(dragResponse.acceptedAction());
    } else {
        event->dataTransfer.setDropAction(Qt::DropAction::IgnoreAction);
    }
}

void QWasmDrag::onNativeDrop(DragEvent *event)
{
    event->webEvent.call<void>("preventDefault");

    QWasmWindow *wasmWindow = QWasmWindow::fromWindow(event->targetWindow);

    const auto screenElementPos = dom::mapPoint(
        event->target(), wasmWindow->platformScreen()->element(), event->localPoint);
    const auto screenPos =
            wasmWindow->platformScreen()->mapFromLocal(screenElementPos);
    const QPoint targetWindowPos = event->targetWindow->mapFromGlobal(screenPos).toPoint();

    const Qt::DropActions actions = m_dragState
                                        ? m_dragState->drag->supportedActions()
                                        : (Qt::DropAction::CopyAction | Qt::DropAction::MoveAction
                                           | Qt::DropAction::LinkAction);
    Qt::MouseButton mouseButton = event->mouseButton;
    QFlags<Qt::KeyboardModifier> modifiers = event->modifiers;

    // Accept the native drop event: We are going to async read any dropped
    // files, but the browser expects that accepted state is set before any
    // async calls.
    event->acceptDrop();

    const auto dropCallback = [&m_dragState = m_dragState, wasmWindow, targetWindowPos,
        actions, mouseButton, modifiers](QMimeData *mimeData) {

        auto dropResponse = std::make_shared<QPlatformDropQtResponse>(true, Qt::DropAction::CopyAction);
        *dropResponse = QWindowSystemInterface::handleDrop(wasmWindow->window(), mimeData,
                                                           targetWindowPos, actions,
                                                           mouseButton, modifiers);

        if (dropResponse->isAccepted())
            m_dragState->dropAction = dropResponse->acceptedAction();

        delete mimeData;
    };

     event->dataTransfer.toMimeDataWithFile(dropCallback);
}

void QWasmDrag::onNativeDragFinished(DragEvent *event)
{
    event->webEvent.call<void>("preventDefault");
    m_dragState->dropAction = event->dropAction;
    m_dragState->quitEventLoopClosure();
}

void QWasmDrag::onNativeDragLeave(DragEvent *event)
{
    event->webEvent.call<void>("preventDefault");
    m_dragState->dropAction = event->dropAction;
    event->dataTransfer.setDropAction(Qt::DropAction::IgnoreAction);
}

QWasmDrag::DragState::DragImage::DragImage(const QPixmap &pixmap, const QMimeData *mimeData,
                                           QWindow *window)
    : m_temporaryImageElementParent(QWasmWindow::fromWindow(window)->containerElement())
{
    m_imageDomElement = generateDragImage(pixmap, mimeData);

    m_imageDomElement.set("className", "hidden-drag-image");
    m_temporaryImageElementParent.call<void>("appendChild", m_imageDomElement);
}

QWasmDrag::DragState::DragImage::~DragImage()
{
    m_temporaryImageElementParent.call<void>("removeChild", m_imageDomElement);
}

emscripten::val QWasmDrag::DragState::DragImage::generateDragImage(const QPixmap &pixmap,
                                                                   const QMimeData *mimeData)
{
    if (!pixmap.isNull())
        return generateDragImageFromPixmap(pixmap);
    if (mimeData->hasFormat("text/plain"))
        return generateDragImageFromText(mimeData);
    return generateDefaultDragImage();
}

emscripten::val
QWasmDrag::DragState::DragImage::generateDragImageFromText(const QMimeData *mimeData)
{
    emscripten::val dragImageElement =
            emscripten::val::global("document")
                    .call<emscripten::val>("createElement", emscripten::val("span"));

    constexpr qsizetype MaxCharactersInDragImage = 100;

    const auto text = QString::fromUtf8(mimeData->data("text/plain"));
    dragImageElement.set(
            "innerText",
            text.first(qMin(qsizetype(MaxCharactersInDragImage), text.length())).toStdString());
    return dragImageElement;
}

emscripten::val QWasmDrag::DragState::DragImage::generateDefaultDragImage()
{
    emscripten::val dragImageElement =
            emscripten::val::global("document")
                    .call<emscripten::val>("createElement", emscripten::val("div"));

    auto innerImgElement = emscripten::val::global("document")
                                   .call<emscripten::val>("createElement", emscripten::val("img"));
    innerImgElement.set("src",
                        "data:image/" + std::string("svg+xml") + ";base64,"
                                + std::string(Base64IconStore::get()->getIcon(
                                        Base64IconStore::IconType::QtLogo)));

    constexpr char DragImageSize[] = "50px";

    dragImageElement["style"].set("width", DragImageSize);
    innerImgElement["style"].set("width", DragImageSize);
    dragImageElement["style"].set("display", "flex");

    dragImageElement.call<void>("appendChild", innerImgElement);
    return dragImageElement;
}

emscripten::val QWasmDrag::DragState::DragImage::generateDragImageFromPixmap(const QPixmap &pixmap)
{
    emscripten::val dragImageElement =
            emscripten::val::global("document")
                    .call<emscripten::val>("createElement", emscripten::val("canvas"));
    dragImageElement.set("width", pixmap.width());
    dragImageElement.set("height", pixmap.height());

    dragImageElement["style"].set(
            "width", std::to_string(pixmap.width() / pixmap.devicePixelRatio()) + "px");
    dragImageElement["style"].set(
            "height", std::to_string(pixmap.height() / pixmap.devicePixelRatio()) + "px");

    auto context2d = dragImageElement.call<emscripten::val>("getContext", emscripten::val("2d"));
    auto imageData = context2d.call<emscripten::val>(
            "createImageData", emscripten::val(pixmap.width()), emscripten::val(pixmap.height()));

    dom::drawImageToWebImageDataArray(pixmap.toImage().convertedTo(QImage::Format::Format_RGBA8888),
                                      imageData, QRect(0, 0, pixmap.width(), pixmap.height()));
    context2d.call<void>("putImageData", imageData, emscripten::val(0), emscripten::val(0));

    return dragImageElement;
}

emscripten::val QWasmDrag::DragState::DragImage::htmlElement()
{
    return m_imageDomElement;
}

QWasmDrag::DragState::DragState(QDrag *drag, QWindow *window,
                                std::function<void()> quitEventLoopClosure)
    : drag(drag), window(window), quitEventLoopClosure(std::move(quitEventLoopClosure))
{
}

QWasmDrag::DragState::~DragState() = default;

QT_END_NAMESPACE
