// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwindowsdirectmanipulation.h"
#include "qwindowscontext.h"
#include "qwindowscursor.h"
#include "qwindowskeymapper.h"
#include "qwindowswindow.h"

#include <QtCore/private/qcomobject_p.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <limits>
#include <memory>
#include <utility>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Direct Manipulation only reports transforms while it is being pumped; 8ms
// keeps up with a 120Hz display without spinning needlessly.
static constexpr int updateIntervalMs = 8;

// How long to keep pumping after the last viewport stopped interacting, so
// that a contact which Direct Manipulation ends up not manipulating at all
// cannot leave the timer running forever.
static constexpr int maxIdleUpdates = 25;

// We never display the content Direct Manipulation manipulates, we only read
// the transform it computes for it, so the viewport rectangle is arbitrary. It
// does have to cover the whole window, since contacts outside of it are not
// manipulated, and a generous fixed size saves us from tracking resizes.
static constexpr LONG viewportSize = 1 << 20;

// Bounds on the accumulated scale within a single gesture. The transform is
// reset to the identity after every gesture, so these only need to be wide
// enough that a single pinch never runs into them and gets stuck.
static constexpr float minimumZoom = 0.01f;
static constexpr float maximumZoom = 100.0f;

// Qt expects wheel angle deltas in eighths of a degree. Interpret a pixel as a
// quarter of a degree, as the Cocoa plugin does for trackpads, so that
// applications reading only angleDelta() scroll by a comparable amount.
static constexpr int pixelsToDegrees = 2;

// A pinch is the only gesture Direct Manipulation reports a scale for.
static constexpr int pinchFingerCount = 2;

static const char *statusName(DIRECTMANIPULATION_STATUS status)
{
    switch (status) {
    case DIRECTMANIPULATION_BUILDING:  return "Building";
    case DIRECTMANIPULATION_ENABLED:   return "Enabled";
    case DIRECTMANIPULATION_DISABLED:  return "Disabled";
    case DIRECTMANIPULATION_RUNNING:   return "Running";
    case DIRECTMANIPULATION_INERTIA:   return "Inertia";
    case DIRECTMANIPULATION_READY:     return "Ready";
    case DIRECTMANIPULATION_SUSPENDED: return "Suspended";
    }
    return "Unknown";
}

static Qt::KeyboardModifiers keyboardModifiers()
{
    return QWindowsContext::instance()->keyMapper()->queryKeyboardModifiers();
}

static void warnFailure(const char *what, HRESULT hr)
{
    qCWarning(lcQpaGestures, "%s: %s", what, qPrintable(QSystemError::windowsComString(hr)));
}

/*!
    \internal

    Handles the Direct Manipulation callbacks for a single window and turns the
    content transforms into Qt events.

    Direct Manipulation reports an absolute transform for the (notional)
    content of its viewport, so an interaction is a stream of transforms which
    we differentiate to get per-event deltas. A gesture either pans or zooms,
    but never both, so whichever of the two deltas changes tells us which one
    the user is performing.
*/
class QWindowsDirectManipulationViewport :
        public QComObject<IDirectManipulationViewportEventHandler,
                          IDirectManipulationInteractionEventHandler>
{
public:
    QWindowsDirectManipulationViewport(QWindowsDirectManipulation *dm, QWindow *window, HWND hwnd)
        : m_dm(dm), m_window(window), m_hwnd(hwnd) {}

    bool init(IDirectManipulationManager *manager);
    void destroy(IDirectManipulationManager *manager);

    bool setContact(UINT32 pointerId);

    bool isInteracting() const
    {
        return m_status == DIRECTMANIPULATION_RUNNING || m_status == DIRECTMANIPULATION_INERTIA;
    }

    // IDirectManipulationViewportEventHandler
    HRESULT STDMETHODCALLTYPE OnViewportStatusChanged(IDirectManipulationViewport *viewport,
                                                      DIRECTMANIPULATION_STATUS current,
                                                      DIRECTMANIPULATION_STATUS previous) override;
    HRESULT STDMETHODCALLTYPE OnViewportUpdated(IDirectManipulationViewport *viewport) override;
    HRESULT STDMETHODCALLTYPE OnContentUpdated(IDirectManipulationViewport *viewport,
                                               IDirectManipulationContent *content) override;

    // IDirectManipulationInteractionEventHandler
    HRESULT STDMETHODCALLTYPE OnInteraction(IDirectManipulationViewport2 *viewport,
                                            DIRECTMANIPULATION_INTERACTION_TYPE interaction) override;

private:
    ~QWindowsDirectManipulationViewport() override = default;

    enum class Gesture { None, Scroll, Zoom };

    void deliverScroll(QPointF delta);
    void deliverZoom(qreal scaleDelta);
    void endGesture();
    void endInteraction(IDirectManipulationViewport *viewport);
    QPoint globalPosition() const { return QWindowsCursor::mousePosition(); }
    QPoint localPosition(QPoint globalPos) const
    {
        return QWindowsGeometryHint::mapFromGlobal(m_hwnd, globalPos);
    }

    QWindowsDirectManipulation *m_dm;
    QPointer<QWindow> m_window;
    const HWND m_hwnd;
    ComPtr<IDirectManipulationViewport2> m_viewport;
    DWORD m_eventHandlerCookie = 0;
    DIRECTMANIPULATION_STATUS m_status = DIRECTMANIPULATION_BUILDING;
    // Which kind of gesture the deltas of this interaction are being reported
    // as, decided by the first delta which is not zero.
    Gesture m_gesture = Gesture::None;
    // The transform reported by the previous content update, and whether there
    // has been one since this interaction started.
    bool m_haveReference = false;
    float m_referenceScale = 1;
    QPointF m_referenceTranslation;
    // Sub-pixel scroll movement which has not been delivered yet, so that slow
    // dragging is not lost to rounding.
    QPointF m_pendingScroll;
};

bool QWindowsDirectManipulationViewport::init(IDirectManipulationManager *manager)
{
    HRESULT hr = manager->CreateViewport(nullptr, m_hwnd, IID_PPV_ARGS(&m_viewport));
    if (FAILED(hr)) {
        warnFailure("Failed to create a Direct Manipulation viewport", hr);
        return false;
    }

    const auto configuration =
            DIRECTMANIPULATION_CONFIGURATION(DIRECTMANIPULATION_CONFIGURATION_INTERACTION
                                             | DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X
                                             | DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y
                                             | DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA
                                             | DIRECTMANIPULATION_CONFIGURATION_SCALING
                                             | DIRECTMANIPULATION_CONFIGURATION_SCALING_INERTIA);
    hr = m_viewport->ActivateConfiguration(configuration);
    if (FAILED(hr)) {
        warnFailure("Failed to configure a Direct Manipulation viewport", hr);
        return false;
    }

    // Without MANUALUPDATE, Direct Manipulation drives itself, but then it
    // oscillates between running and inertia and never settles back to
    // DIRECTMANIPULATION_READY, so gestures would never end.
    hr = m_viewport->SetViewportOptions(DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE);
    if (FAILED(hr)) {
        warnFailure("Failed to set Direct Manipulation viewport options", hr);
        return false;
    }

    const RECT rect = { 0, 0, viewportSize, viewportSize };
    hr = m_viewport->SetViewportRect(&rect);
    if (FAILED(hr)) {
        warnFailure("Failed to set a Direct Manipulation viewport rect", hr);
        return false;
    }

    ComPtr<IDirectManipulationPrimaryContent> primaryContent;
    if (SUCCEEDED(m_viewport->GetPrimaryContent(IID_PPV_ARGS(&primaryContent))))
        primaryContent->SetZoomBoundaries(minimumZoom, maximumZoom);

    hr = m_viewport->AddEventHandler(m_hwnd, this, &m_eventHandlerCookie);
    if (FAILED(hr)) {
        warnFailure("Failed to install a Direct Manipulation event handler", hr);
        return false;
    }

    hr = m_viewport->Enable();
    if (FAILED(hr)) {
        warnFailure("Failed to enable a Direct Manipulation viewport", hr);
        return false;
    }

    qCDebug(lcQpaGestures) << "Direct Manipulation enabled for" << m_window.data();
    return true;
}

void QWindowsDirectManipulationViewport::destroy(IDirectManipulationManager *manager)
{
    if (!m_viewport)
        return;
    m_viewport->Stop();
    if (m_eventHandlerCookie)
        m_viewport->RemoveEventHandler(m_eventHandlerCookie);
    m_viewport->Abandon();
    m_viewport.Reset();
    manager->Deactivate(m_hwnd);
}

bool QWindowsDirectManipulationViewport::setContact(UINT32 pointerId)
{
    if (!m_viewport)
        return false;
    const HRESULT hr = m_viewport->SetContact(pointerId);
    if (FAILED(hr)) {
        qCDebug(lcQpaGestures, "Direct Manipulation refused contact %u: %s", pointerId,
                qPrintable(QSystemError::windowsComString(hr)));
        return false;
    }
    // The interaction only produces transforms while Direct Manipulation is
    // pumped, and the first of them arrive before it reports RUNNING.
    m_dm->startUpdates();
    return true;
}

HRESULT QWindowsDirectManipulationViewport::OnViewportStatusChanged(IDirectManipulationViewport *viewport,
                                                                    DIRECTMANIPULATION_STATUS current,
                                                                    DIRECTMANIPULATION_STATUS previous)
{
    qCDebug(lcQpaGestures, "Direct Manipulation status %s -> %s", statusName(previous),
            statusName(current));
    m_status = current;

    switch (current) {
    case DIRECTMANIPULATION_READY:
        // The interaction, including any inertia, is over. Reset the transform
        // so that the next one starts from a known scale and translation
        // instead of accumulating indefinitely.
        endInteraction(viewport);
        break;
    case DIRECTMANIPULATION_ENABLED:
    case DIRECTMANIPULATION_DISABLED:
    case DIRECTMANIPULATION_SUSPENDED:
        endInteraction(nullptr);
        break;
    case DIRECTMANIPULATION_RUNNING:
    case DIRECTMANIPULATION_INERTIA:
    case DIRECTMANIPULATION_BUILDING:
        break;
    }
    return S_OK;
}

HRESULT QWindowsDirectManipulationViewport::OnViewportUpdated(IDirectManipulationViewport *)
{
    return S_OK;
}

HRESULT QWindowsDirectManipulationViewport::OnContentUpdated(IDirectManipulationViewport *,
                                                             IDirectManipulationContent *content)
{
    float transform[6];
    if (FAILED(content->GetContentTransform(transform, 6)))
        return S_OK;

    // A 3x2 matrix. Direct Manipulation only ever scales uniformly and
    // translates, so transform[1] and transform[2] are always zero.
    const float scale = transform[0];
    const QPointF translation(transform[4], transform[5]);

    if (!m_haveReference) {
        // The transform is not reliably the identity at the start of an
        // interaction, so take the first update as the reference to
        // differentiate the following ones against.
        m_haveReference = true;
        m_referenceScale = scale;
        m_referenceTranslation = translation;
        return S_OK;
    }

    const float scaleDelta = scale - m_referenceScale;
    const QPointF translationDelta = translation - m_referenceTranslation;
    m_referenceScale = scale;
    m_referenceTranslation = translation;

    // While zooming, the translation changes as well, because Direct
    // Manipulation scales the content about the point between the fingers. The
    // scale is what the user is driving, so let it take precedence.
    if (scaleDelta != 0)
        deliverZoom(scaleDelta);
    else if (!translationDelta.isNull())
        deliverScroll(translationDelta);

    return S_OK;
}

HRESULT QWindowsDirectManipulationViewport::OnInteraction(IDirectManipulationViewport2 *,
                                                          DIRECTMANIPULATION_INTERACTION_TYPE interaction)
{
    // The status changes tell us all we need about the lifetime of an
    // interaction; this is only interesting while debugging.
    qCDebug(lcQpaGestures, "Direct Manipulation interaction type %d", int(interaction));
    return S_OK;
}

void QWindowsDirectManipulationViewport::deliverScroll(QPointF delta)
{
    if (m_window.isNull())
        return;

    if (m_gesture == Gesture::Zoom)
        endGesture();

    // Direct Manipulation works in physical pixels, QWheelEvent::pixelDelta()
    // in device independent ones.
    m_pendingScroll += delta / QHighDpiScaling::factor(m_window.data());
    const QPoint pixelDelta = m_pendingScroll.toPoint();
    if (pixelDelta.isNull())
        return;
    m_pendingScroll -= QPointF(pixelDelta);

    Qt::ScrollPhase phase = Qt::ScrollUpdate;
    if (m_gesture != Gesture::Scroll) {
        m_gesture = Gesture::Scroll;
        phase = Qt::ScrollBegin;
    } else if (m_status == DIRECTMANIPULATION_INERTIA) {
        phase = Qt::ScrollMomentum;
    }

    const QPoint globalPos = globalPosition();
    qCDebug(lcQpaGestures) << "scroll" << pixelDelta << "phase" << phase << "to" << m_window.data();
    QWindowSystemInterface::handleWheelEvent(m_window.data(), ::GetTickCount(),
                                             m_dm->device().data(), localPosition(globalPos),
                                             globalPos, pixelDelta,
                                             pixelDelta * pixelsToDegrees,
                                             keyboardModifiers(), phase,
                                             Qt::MouseEventSynthesizedBySystem);
}

void QWindowsDirectManipulationViewport::deliverZoom(qreal scaleDelta)
{
    if (m_window.isNull())
        return;

    if (m_gesture == Gesture::Scroll)
        endGesture();

    const QPoint globalPos = globalPosition();
    const QPoint localPos = localPosition(globalPos);

    if (m_gesture != Gesture::Zoom) {
        m_gesture = Gesture::Zoom;
        QWindowSystemInterface::handleGestureEvent(m_window.data(), ::GetTickCount(),
                                                   m_dm->device().data(), Qt::BeginNativeGesture,
                                                   localPos, globalPos, pinchFingerCount);
    }

    qCDebug(lcQpaGestures) << "zoom by" << scaleDelta << "to" << m_window.data();
    QWindowSystemInterface::handleGestureEventWithRealValue(m_window.data(), ::GetTickCount(),
                                                            m_dm->device().data(),
                                                            Qt::ZoomNativeGesture, scaleDelta,
                                                            localPos, globalPos, pinchFingerCount);
}

void QWindowsDirectManipulationViewport::endGesture()
{
    const Gesture gesture = std::exchange(m_gesture, Gesture::None);
    m_pendingScroll = {};

    if (gesture == Gesture::None || m_window.isNull())
        return;

    const QPoint globalPos = globalPosition();
    const QPoint localPos = localPosition(globalPos);

    if (gesture == Gesture::Scroll) {
        qCDebug(lcQpaGestures) << "scroll ended for" << m_window.data();
        QWindowSystemInterface::handleWheelEvent(m_window.data(), ::GetTickCount(),
                                                 m_dm->device().data(), localPos, globalPos,
                                                 QPoint(), QPoint(),
                                                 keyboardModifiers(),
                                                 Qt::ScrollEnd,
                                                 Qt::MouseEventSynthesizedBySystem);
    } else {
        qCDebug(lcQpaGestures) << "zoom ended for" << m_window.data();
        QWindowSystemInterface::handleGestureEvent(m_window.data(), ::GetTickCount(),
                                                   m_dm->device().data(), Qt::EndNativeGesture,
                                                   localPos, globalPos, pinchFingerCount);
    }
}

void QWindowsDirectManipulationViewport::endInteraction(IDirectManipulationViewport *viewport)
{
    endGesture();
    m_haveReference = false;

    ComPtr<IDirectManipulationContent> content;
    if (viewport && SUCCEEDED(viewport->GetPrimaryContent(IID_PPV_ARGS(&content)))) {
        static constexpr float identity[6] = { 1, 0, 0, 1, 0, 0 };
        content->SyncContentTransform(identity, 6);
    }
}

/*!
    \internal
    Pumps Direct Manipulation while an interaction is in progress.
*/
class QWindowsDirectManipulation::UpdatePump : public QObject
{
public:
    explicit UpdatePump(QWindowsDirectManipulation *dm) : m_dm(dm) {}

    void start()
    {
        if (!m_timer.isActive())
            m_timer.start(updateIntervalMs, Qt::PreciseTimer, this);
    }
    void stop() { m_timer.stop(); }

protected:
    void timerEvent(QTimerEvent *) override { m_dm->update(); }

private:
    QWindowsDirectManipulation *m_dm;
    QBasicTimer m_timer;
};

QWindowsDirectManipulation *QWindowsDirectManipulation::create()
{
    std::unique_ptr<QWindowsDirectManipulation> dm(new QWindowsDirectManipulation);
    return dm->init() ? dm.release() : nullptr;
}

bool QWindowsDirectManipulation::init()
{
    HRESULT hr = CoCreateInstance(CLSID_DirectManipulationManager, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&m_manager));
    if (FAILED(hr)) {
        warnFailure("Failed to create the Direct Manipulation manager", hr);
        return false;
    }

    hr = m_manager->GetUpdateManager(IID_PPV_ARGS(&m_updateManager));
    if (FAILED(hr)) {
        warnFailure("Failed to get the Direct Manipulation update manager", hr);
        return false;
    }

    m_pump.reset(new UpdatePump(this));
    return true;
}

QWindowsDirectManipulation::~QWindowsDirectManipulation()
{
    for (auto it = m_viewports.cbegin(), end = m_viewports.cend(); it != end; ++it)
        it.value()->destroy(m_manager.Get());
    m_viewports.clear();
}

void QWindowsDirectManipulation::registerWindow(QWindow *window, HWND hwnd)
{
    if (m_viewports.contains(hwnd))
        return;

    // Activating the window is what makes Windows send it DM_POINTERHITTEST.
    const HRESULT hr = m_manager->Activate(hwnd);
    if (FAILED(hr)) {
        warnFailure("Failed to activate Direct Manipulation for a window", hr);
        return;
    }

    auto viewport = makeComObject<QWindowsDirectManipulationViewport>(this, window, hwnd);
    if (!viewport->init(m_manager.Get())) {
        m_manager->Deactivate(hwnd);
        return;
    }
    m_viewports.insert(hwnd, viewport);
}

void QWindowsDirectManipulation::unregisterWindow(HWND hwnd)
{
    if (auto viewport = m_viewports.take(hwnd))
        viewport->destroy(m_manager.Get());
}

bool QWindowsDirectManipulation::handlePointerHitTest(HWND hwnd, WPARAM wParam)
{
    const UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
    POINTER_INPUT_TYPE pointerType = PT_POINTER;
    // Leave touch screens and pens to QWindowsPointerHandler, which turns them
    // into touch and tablet events; only touchpads are gesture sources here.
    if (!GetPointerType(pointerId, &pointerType) || pointerType != PT_TOUCHPAD)
        return false;

    const auto it = m_viewports.constFind(hwnd);
    return it != m_viewports.cend() && it.value()->setContact(pointerId);
}

const QWindowsDirectManipulation::QPointingDevicePtr &QWindowsDirectManipulation::device()
{
    if (!m_device) {
        // Direct Manipulation does not tell us which touchpad it is reporting
        // for, so this stands in for all of them. QInputDevicePrivate::fromId()
        // looks devices up by system ID alone, so use one which cannot collide
        // with the small IDs QWindowsPointerHandler hands out.
        // TODO: report one device per touchpad, with its system-provided name
        const qint64 systemId = std::numeric_limits<qint64>::max();
        m_device.reset(new QPointingDevice("touchpad"_L1, systemId,
                                           QInputDevice::DeviceType::TouchPad,
                                           QPointingDevice::PointerType::Finger,
                                           QInputDevice::Capability::Position
                                                   | QInputDevice::Capability::Scroll,
                                           pinchFingerCount, 3));
        QWindowSystemInterface::registerInputDevice(m_device.data());
    }
    return m_device;
}

void QWindowsDirectManipulation::startUpdates()
{
    m_idleUpdates = 0;
    m_pump->start();
}

void QWindowsDirectManipulation::update()
{
    // Any status change and content transform is reported from within this
    // call, so the state checked below is up to date.
    m_updateManager->Update(nullptr);

    if (isInteracting())
        m_idleUpdates = 0;
    else if (++m_idleUpdates > maxIdleUpdates)
        m_pump->stop();
}

bool QWindowsDirectManipulation::isInteracting() const
{
    for (auto it = m_viewports.cbegin(), end = m_viewports.cend(); it != end; ++it) {
        if (it.value()->isInteracting())
            return true;
    }
    return false;
}

QT_END_NAMESPACE
