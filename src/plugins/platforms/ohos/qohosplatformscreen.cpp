// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>
#include <QTime>

#include <qpa/qwindowsysteminterface.h>

#include <qohosdisplayinfo.h>
#include "qohosplatformscreen.h"
#include "qohosplatformbackingstore.h"
#include "qohosplatformintegration.h"
#include "qohosplatformwindow.h"
#include "qohosjsmain.h"
#include "qohosdeadlockprotector.h"
#include "render/qohosview.h"
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <qarkui/qarkuiutils.h>
#include <qguiapplication.h>
#include <qohosapppermissions_p.h>
#include <qohosjsutils.h>
#include <qohospixelmapconversions.h>
#include <qohosutils.h>
#include <render/qwindowproxyregistry.h>
#include <window_manager/oh_display_capture.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/private/qwindow_p.h>

#include <vector>

QT_BEGIN_NAMESPACE

namespace {

static const int ohosLogicalDpi = 72;

std::shared_ptr<::OH_PixelmapNative> captureScreenPixelmap(
    QtOhos::JsState &, QOhosDisplayInfo::JsDisplayId displayId)
{
    ::OH_PixelmapNative *pixelMapNativePtr;

    QArkUi::callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_NativeDisplayManager_CaptureScreenPixelmap),
        static_cast<std::uint32_t>(displayId.value()), &pixelMapNativePtr);

    return wrapNativePixelMapPtr(pixelMapNativePtr);
}

void tryCaptureScreenPixelmapWithPermissionCheck(
    QtOhos::JsState &jsState, QOhosDisplayInfo::JsDisplayId displayId,
    QOhosConsumer<std::shared_ptr<::OH_PixelmapNative>> pixelMapOrNullConsumer)
{
    static constexpr const char *ohosCustomScreenCapturePermission =
        "ohos.permission.CUSTOM_SCREEN_CAPTURE";

    QOhosAppPermissions::checkAppPermissionGrantedWithConsumer(
        jsState, ohosCustomScreenCapturePermission,
        [pixelMapOrNullConsumer = std::move(pixelMapOrNullConsumer), displayId](
            auto &jsState, bool permissionGranted) {
            if (permissionGranted) {
                pixelMapOrNullConsumer(captureScreenPixelmap(jsState, displayId));
            } else {
                qOhosPrintfError(
                    "%s: %s hasn't been granted by user. Cannot grab window.", Q_FUNC_INFO,
                    ohosCustomScreenCapturePermission);
                pixelMapOrNullConsumer(nullptr);
            }
        });
}

QWindow *tryFindWindowByWIdOrNull(WId wId)
{
    auto allWindows = qApp->allWindows();
    auto windowItByWId = std::find_if(
        std::begin(allWindows), std::end(allWindows),
        [&](QWindow *w) {
            return w->winId() == wId;
        });

    if (windowItByWId == allWindows.end())
        return nullptr;

    return *windowItByWId;
}

QSize calculateResultWindowCapturePixmapSize(
    const QSize &windowSnapshotSize, const QRect &windowSpaceCaptureRect)
{
    return {
        windowSpaceCaptureRect.width() < 0
            ? qMax(0, windowSnapshotSize.width() - windowSpaceCaptureRect.x())
            : windowSpaceCaptureRect.width(),
        windowSpaceCaptureRect.height() < 0
            ? qMax(0, windowSnapshotSize.height() - windowSpaceCaptureRect.y())
            : windowSpaceCaptureRect.height()
    };
}

QRect calculateResultPixmapWindowFragmentRect(
    const QRect &windowSpaceCaptureRect, const QRect &windowFragmentRect)
{
    return {
        qAbs(windowSpaceCaptureRect.x() - windowFragmentRect.x()),
        qAbs(windowSpaceCaptureRect.y() - windowFragmentRect.y()),
        windowFragmentRect.width(),
        windowFragmentRect.height()
    };
}

QPixmap grabWindowFromCapturedScreenPixmap(
    const QOhosPlatformScreen *platformScreen, const QPixmap &capturedWindowPixmap,
    const QRect &windowSpaceCaptureRect)
{
    auto resultSize = calculateResultWindowCapturePixmapSize(
        QHighDpi::fromNativePixels(capturedWindowPixmap.size(), platformScreen),
        windowSpaceCaptureRect);

    if (resultSize.isEmpty())
        return {};

    auto windowFragmentRect = capturedWindowPixmap.rect().intersected(
        QRect(windowSpaceCaptureRect.topLeft(), resultSize));

    if (windowFragmentRect.isEmpty())
        return QPixmap(resultSize);

    auto resultSpaceWindowFragmentRect =
        calculateResultPixmapWindowFragmentRect(windowSpaceCaptureRect, windowFragmentRect);

    QPixmap result(resultSize);
    QPainter p(&result);
    p.drawPixmap(
        QHighDpi::toNativePixels(resultSpaceWindowFragmentRect, platformScreen),
        capturedWindowPixmap,
        QHighDpi::toNativePixels(windowFragmentRect, platformScreen));

    return result;
}

QOhosOptional<Qt::ScreenOrientation> tryMapJsDisplayOrientationToQt(QOhosDisplayInfo::JsDisplayOrientation jsDisplayOrientation)
{
    using JsDisplayOrientation = QOhosDisplayInfo::JsDisplayOrientation;

    switch (jsDisplayOrientation) {
    case JsDisplayOrientation::PORTRAIT:
        return makeQOhosOptional(Qt::ScreenOrientation::PortraitOrientation);
    case JsDisplayOrientation::LANDSCAPE:
        return makeQOhosOptional(Qt::ScreenOrientation::LandscapeOrientation);
    case JsDisplayOrientation::PORTRAIT_INVERTED:
        return makeQOhosOptional(Qt::ScreenOrientation::InvertedPortraitOrientation);
    case JsDisplayOrientation::LANDSCAPE_INVERTED:
        return makeQOhosOptional(Qt::ScreenOrientation::InvertedLandscapeOrientation);
    }

    return {};
}

}

QOhosPlatformScreen::QOhosPlatformScreen(const QOhosDisplayInfo &displayInfo)
    : QObject()
    , QPlatformScreen()
    , m_format(QImage::Format_ARGB32_Premultiplied)
    , m_depth(32)
    , m_platformCursor(new QOhosPlatformCursor())
    , m_displayInfo(displayInfo)
    , m_availableGeometry(getAvailableArea())
{
    auto __dbg = make_QCScopedDebug("QOhosPlatformScreen::QOhosPlatformScreen");
}

QOhosPlatformScreen::~QOhosPlatformScreen() = default;

QOhosPlatformScreen *QOhosPlatformScreen::fromQScreen(QScreen *screen)
{
    auto *platformScreen = screen->handle();
    if (Q_UNLIKELY(platformScreen == nullptr))
        qOhosReportFatalErrorAndAbort("QScreen::handle() returned null");
    return static_cast<QOhosPlatformScreen *>(platformScreen);
}

QWindow *QOhosPlatformScreen::topLevelAt(const QPoint &p) const
{
    for (auto *platformScreen : virtualSiblings()) {
        auto *ohosPlatformScreen = static_cast<QOhosPlatformScreen *>(platformScreen);

        auto windowIds = QOhosWindowProxy::queryWindowIdsByCoordinate(ohosPlatformScreen->m_displayInfo.id, p);
        if (!windowIds.empty())
            return QWindowProxyRegistry::instance().findQWindowByJsWindowIdOrNull(windowIds.front());
    }

    return nullptr;
}

QPlatformCursor *QOhosPlatformScreen::cursor() const
{
    return m_platformCursor.data();
}

void QOhosPlatformScreen::setDisplayInfo(const QOhosDisplayInfo &displayInfo)
{
    auto geometryChanged = displayInfo.displayGeometryPixels() != m_displayInfo.displayGeometryPixels();
    auto logicalDpiChanged = displayInfo.densityDPI != m_displayInfo.densityDPI;

    QVector<QPair<QWindow *, QOhosOptional<QRect>>> windowRectPairs;

    if (logicalDpiChanged) {
        for (auto *window : qGuiApp->allWindows()) {
            auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(window);
            if (platformWindow == nullptr || platformWindow->screen() != this || !window->isVisible())
                continue;

            auto *ownedView = platformWindow->ownedViewOrNull();
            if (ownedView == nullptr)
                continue;

            windowRectPairs.push_back({
                window,
                ownedView->viewType() == QOhosView::ViewType::EmbeddedWindow
                    ? makeQOhosOptional(window->geometry())
                    : makeEmptyQOhosOptional()});
        }
    }

    m_displayInfo = displayInfo;

    if (geometryChanged)
        QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());

    if (logicalDpiChanged) {
        auto ldpi = logicalDpi();
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(
            screen(), ldpi.first, ldpi.second);

        QWindowSystemInterface::flushWindowSystemEvents();

        for (const auto &windowRectPair : windowRectPairs) {
            auto *qWindow = windowRectPair.first;
            auto *platformWindow = QOhosPlatformWindow::fromQWindow(qWindow);
            if (windowRectPair.second.hasValue())
                qWindow->setGeometry(windowRectPair.second.value());
            platformWindow->handleDpiChange();
        }
    }
}

QDpi QOhosPlatformScreen::logicalDpi() const
{
    qreal lDpi = m_displayInfo.densityPixels * ohosLogicalDpi;
    return QDpi(lDpi, lDpi);
}

qreal QOhosPlatformScreen::pixelScalingCoefficient() const
{
    // densityPixels is the scaling coefficient between the virtual pixel
    // and the physical pixels.
    return m_displayInfo.densityPixels;
}

QDpi QOhosPlatformScreen::logicalBaseDpi() const
{
    return QDpi(ohosLogicalDpi, ohosLogicalDpi);
}

Qt::ScreenOrientation QOhosPlatformScreen::orientation() const
{
    return
        m_displayInfo.orientation
        .andThen(tryMapJsDisplayOrientationToQt)
        .valueOr(Qt::ScreenOrientation::PrimaryOrientation);
}

Qt::ScreenOrientation QOhosPlatformScreen::nativeOrientation() const
{
    return Qt::ScreenOrientation::PrimaryOrientation;
}

QRect QOhosPlatformScreen::getAvailableArea() const
{
    return QtOhos::evalInJsThreadWithPromise<QRect>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<QRect> evalPromise) {

            QNapi::Object display;
            try {
                display = jsState.eval<QNapi::Object>(
                    "@ohos.display.getDisplayByIdSync(*)", {m_displayInfo.id.value()});
            } catch (const Napi::Error &error) {
                qOhosPrintfError(
                    "%s: getDisplayByIdSync(%f) failed with error: %s",
                    Q_FUNC_INFO, m_displayInfo.id.value(), error.Message().c_str());
                evalPromise(QRect());
                return;
            }

            display.call<QNapi::Promise>("getAvailableArea")
            .withContext(std::move(evalPromise))
            .onThenWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, QOhosTaskPromise<QRect> &evalPromise) {
                    auto availableArea = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    evalPromise(
                        QRect(
                            availableArea.get<QNapi::Number>("left"),
                            availableArea.get<QNapi::Number>("top"),
                            availableArea.get<QNapi::Number>("width"),
                            availableArea.get<QNapi::Number>("height")));
                })
            .onCatchWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, QOhosTaskPromise<QRect> &evalPromise) {
                    QtOhos::logJsCallbackError(cbInfo, "Error occurred in JS getAvailableArea()");
                    evalPromise(QRect());
                });
        });
}

void QOhosPlatformScreen::releaseSurface()
{
}

const QOhosDisplayInfo &QOhosPlatformScreen::displayInfo() const
{
    return m_displayInfo;
}

QRect QOhosPlatformScreen::geometry() const
{
    return m_displayInfo.displayGeometryPixels();
}

QRect QOhosPlatformScreen::availableGeometry() const
{
    auto screenGeometry = geometry();
    return !m_availableGeometry.isEmpty()
        ? m_availableGeometry.translated(screenGeometry.topLeft())
        : screenGeometry;
}

int QOhosPlatformScreen::depth() const
{
    return m_depth;
}

QImage::Format QOhosPlatformScreen::format() const
{
    return m_format;
}

QSizeF QOhosPlatformScreen::physicalSize() const
{
    return m_displayInfo.physicalSize();
}

void QOhosPlatformScreen::setAvailableGeometry(const QRect &rect)
{
    if (rect != m_availableGeometry) {
        m_availableGeometry = rect;
        QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());
    }
}

QPixmap QOhosPlatformScreen::grabWindow(WId wId, int x, int y, int width, int height) const
{
    auto captureRect = QRect(x, y, width, height);

    if (wId != 0) {
        auto *window = tryFindWindowByWIdOrNull(wId);
        if (window == nullptr) {
            qOhosPrintfError(
                "%s: Cannot find window with the given WId: %lld.", Q_FUNC_INFO, wId);
            return {};
        }
        return grabWindowFromCapturedScreenPixmap(
            this, QOhosPlatformWindow::fromQWindow(window)->makeSnapshot(), captureRect);
    }

    auto capturedScreenPixmap = QtOhos::evalInJsThreadWithPromise<QPixmap>(
        [displayId = m_displayInfo.id](
            QtOhos::JsState &jsState, QOhosTaskPromise<QPixmap> evalPromise) {
            auto sharedEvalPromise = QtOhos::moveToSharedPtr(std::move(evalPromise));
            tryCaptureScreenPixelmapWithPermissionCheck(
                jsState, displayId,
                [sharedEvalPromise](
                    std::shared_ptr<::OH_PixelmapNative> optPixelMap) {
                    (*sharedEvalPromise)(
                        optPixelMap
                            ? QPixmap::fromImage(createQImageFromNativePixelMap(optPixelMap.get()))
                            : QPixmap());
                });
        });

    return capturedScreenPixmap.copy(captureRect);
}

QString QOhosPlatformScreen::name() const
{
    return m_displayInfo.name;
}

QT_END_NAMESPACE
