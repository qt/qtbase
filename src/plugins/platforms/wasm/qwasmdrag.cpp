// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

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
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QFile>

#include <private/qshapedpixmapdndwindow_p.h>
#include <private/qdnd_p.h>

#include <functional>
#include <string>
#include <utility>

QT_BEGIN_NAMESPACE

namespace {

QWindow *windowForDrag(QDrag *drag)
{
    QObject *source = drag->source();
    QWindow *window = nullptr;
    while (source && !window) {
        window = qobject_cast<QWindow *>(source);
        if (!window && source->metaObject()->indexOfMethod("_q_closestWindowHandle()") != -1)
            QMetaObject::invokeMethod(source, "_q_closestWindowHandle",
                              Q_RETURN_ARG(QWindow *, window));
        if (!window)
            source = source->parent();
    }
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

        emscripten::val htmlElement() { return m_imageDomElement; }
        QPixmap pixmap() const { return m_pixmap; }

    private:
        void generateDragImage(const QPixmap &pixmap, const QMimeData *mimeData);
        void generateDragImageFromText(const QMimeData *mimeData);
        void generateDefaultDragImage();
        void generateDragImageFromPixmap(const QPixmap &pixmap);

        emscripten::val m_imageDomElement = emscripten::val::undefined();
        emscripten::val m_temporaryImageElementParent = emscripten::val::undefined();
        QPixmap         m_pixmap;
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

    Qt::DropAction dragResult = Qt::IgnoreAction;
    if (!qstdweb::haveAsyncify())
        return dragResult;

    auto dragState = std::make_shared<DragState>(drag, window, [this]() { QSimpleDrag::cancelDrag();  });

    if (!m_isInEnterDrag)
        m_dragState = dragState;

    if (m_isInEnterDrag)
        drag->setPixmap(QPixmap());
    else if (drag->pixmap().size() == QSize(0, 0))
        drag->setPixmap(dragState->dragImage->pixmap());

    dragResult = QSimpleDrag::drag(drag);
    m_dragState.reset();

    return dragResult;
}

void QWasmDrag::onNativeDragStarted(DragEvent *event)
{
    Q_ASSERT_X(event->type == EventType::DragStart, Q_FUNC_INFO,
               "The event is not a DragStart event");

    // It is possible for a drag start event to arrive from another window.
    if (!m_dragState || m_dragState->window != event->targetWindow) {
        event->cancelDragStart();
        return;
    }
    setExecutedDropAction(event->dropAction);

    // We have our own window
    if (shapedPixmapWindow())
        shapedPixmapWindow()->setVisible(false);

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
        setExecutedDropAction(dragResponse.acceptedAction());
        event->dataTransfer.setDropAction(dragResponse.acceptedAction());
    } else {
        setExecutedDropAction(Qt::DropAction::IgnoreAction);
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
    setExecutedDropAction(event->dropAction);
    std::shared_ptr<DragState> dragState = m_dragState;

    const auto dropCallback = [this, dragState, wasmWindow, targetWindowPos,
        actions, mouseButton, modifiers](QMimeData *mimeData) {

        if (mimeData) {
            const QPlatformDropQtResponse dropResponse =
                QWindowSystemInterface::handleDrop(wasmWindow->window(), mimeData,
                                                           targetWindowPos, actions,
                                                           mouseButton, modifiers);

            if (dragState && dropResponse.isAccepted())
                dragState->dropAction = dropResponse.acceptedAction();

            delete mimeData;
        }

        if (!dragState)
            QSimpleDrag::cancelDrag();
    };

    event->dataTransfer.toMimeDataWithFile(dropCallback);
}

void QWasmDrag::onNativeDragFinished(DragEvent *event, QWasmScreen *platformScreen)
{
    // Keep sourcewindow before it is reset
    QPointer<QWindow> sourceWindow = m_sourceWindow;

    event->webEvent.call<void>("preventDefault");

    if (m_dragState)
        m_dragState->dropAction = event->dropAction;

    setExecutedDropAction(event->dropAction);

    if (m_dragState)
        m_dragState->quitEventLoopClosure();


    // Send synthetic mouserelease event
    const MouseEvent mouseEvent(*event);

    const auto pointInScreen = platformScreen->mapFromLocal(
        dom::mapPoint(mouseEvent.target(), platformScreen->element(), mouseEvent.localPoint));
    const auto geometryF = platformScreen->geometry().toRectF();
    QPointF targetPointClippedToScreen(
            qBound(geometryF.left(), pointInScreen.x(), geometryF.right()),
            qBound(geometryF.top(), pointInScreen.y(), geometryF.bottom()));

    QTimer::singleShot(0, [sourceWindow, targetPointClippedToScreen, mouseEvent]() {
        if (sourceWindow) {
            const QEvent::Type eventType = QEvent::MouseButtonRelease;
            QWindowSystemInterface::handleMouseEvent(
                sourceWindow, QWasmIntegration::getTimestamp(),
                sourceWindow->mapFromGlobal(targetPointClippedToScreen),
                targetPointClippedToScreen, mouseEvent.mouseButtons, mouseEvent.mouseButton,
                eventType, mouseEvent.modifiers);
        }
    });
}

void QWasmDrag::onNativeDragEnter(DragEvent *event)
{
    event->webEvent.call<void>("preventDefault");

    // Already dragging
    if (QDragManager::self() && QDragManager::self()->object())
        return;

    // Event coming from external browser, start a drag
    if (m_dragState)
        m_dragState->dropAction = event->dropAction;

    setExecutedDropAction(event->dropAction);

    m_isInEnterDrag = true;
    QDrag *drag = new QDrag(this);
    drag->setMimeData(event->dataTransfer.toMimeDataPreview());
    drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
    m_isInEnterDrag = false;
}

void QWasmDrag::onNativeDragLeave(DragEvent *event)
{
    event->webEvent.call<void>("preventDefault");
    if (m_dragState)
        m_dragState->dropAction = event->dropAction;
    setExecutedDropAction(event->dropAction);

    event->dataTransfer.setDropAction(Qt::DropAction::IgnoreAction);

    // If we started the drag from  onNativeDragEnter
    // it is correct to cancel the drag.
    if (m_isInEnterDrag)
        cancelDrag();
}

QWasmDrag::DragState::DragImage::DragImage(const QPixmap &pixmap, const QMimeData *mimeData,
                                           QWindow *window)
{
    if (window)
        m_temporaryImageElementParent = QWasmWindow::fromWindow(window)->containerElement();
    generateDragImage(pixmap, mimeData);

    m_imageDomElement.set("className", "hidden-drag-image");

    // chromium requires the image to be the first child
    if (m_temporaryImageElementParent.isUndefined())
        ;
    else if (m_temporaryImageElementParent["childElementCount"].as<int>() == 0)
        m_temporaryImageElementParent.call<void>("appendChild", m_imageDomElement);
    else
        m_temporaryImageElementParent.call<void>("insertBefore", m_imageDomElement, m_temporaryImageElementParent["children"][0]);
}

QWasmDrag::DragState::DragImage::~DragImage()
{
    if (!m_temporaryImageElementParent.isUndefined())
        m_temporaryImageElementParent.call<void>("removeChild", m_imageDomElement);
}

void QWasmDrag::DragState::DragImage::generateDragImage(
    const QPixmap &pixmap,
    const QMimeData *mimeData)
{
    if (!pixmap.isNull())
        generateDragImageFromPixmap(pixmap);
    else if (mimeData && mimeData->hasFormat("text/plain"))
        generateDragImageFromText(mimeData);
    else
        generateDefaultDragImage();
}

void QWasmDrag::DragState::DragImage::generateDragImageFromText(const QMimeData *mimeData)
{
    emscripten::val dragImageElement =
            emscripten::val::global("document")
                    .call<emscripten::val>("createElement", emscripten::val("span"));

    constexpr qsizetype MaxCharactersInDragImage = 100;

    const auto text = QString::fromUtf8(mimeData->data("text/plain"));
    dragImageElement.set(
            "innerText",
            text.first(qMin(qsizetype(MaxCharactersInDragImage), text.length())).toStdString());

    QRect bounds;
    {
        QPixmap image(QSize(200,200));
        if (!image.isNull()) {
            QPainter painter(&image);
            bounds = painter.boundingRect(0, 0, 200, 200, 0, text);
        }
    }
    QImage image(bounds.size(), QImage::Format_RGBA8888);
    if (!image.isNull()) {
        QPainter painter(&image);
        painter.fillRect(bounds, QColor(255,255,255, 255)); // Transparency does not work very well :-(
        painter.setPen(Qt::black);
        painter.drawText(bounds, text);

        m_pixmap = QPixmap::fromImage(image);
    }
    m_imageDomElement = dragImageElement;
}

void QWasmDrag::DragState::DragImage::generateDefaultDragImage()
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

    QByteArray pixmap_ba =
        QByteArray::fromBase64(
            QString::fromLatin1(
                Base64IconStore::get()->getIcon(
                    Base64IconStore::IconType::QtLogo)).toLocal8Bit());

    if (!m_pixmap.loadFromData(pixmap_ba))
        qWarning() << " Load of Qt logo failed";

    m_pixmap = m_pixmap.scaled(50, 50);
    m_imageDomElement = dragImageElement;
}

void QWasmDrag::DragState::DragImage::generateDragImageFromPixmap(const QPixmap &pixmap)
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

    m_pixmap = pixmap;
    m_imageDomElement = dragImageElement;
}

QWasmDrag::DragState::DragState(QDrag *drag, QWindow *window,
                                std::function<void()> quitEventLoopClosure)
    : drag(drag), window(window), quitEventLoopClosure(std::move(quitEventLoopClosure))
{
    dragImage =
        std::make_unique<DragState::DragImage>(
            drag->pixmap(),
            drag->mimeData(),
            window);
}

QWasmDrag::DragState::~DragState() = default;

QT_END_NAMESPACE
