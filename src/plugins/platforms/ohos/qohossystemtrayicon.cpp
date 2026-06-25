// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohossystemtrayicon.h"
#include "qohosdisplayinfo.h"
#include "qohosenums.h"
#include "qohosimageformat.h"
#include "qohosjsutils.h"
#include "qohospixelmapconversions.h"
#include "qohosstatusbarmenu.h"
#include <QtCore/qdatetime.h>
#include <QtCore/qobject.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qicon.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <QtGui/qscreen.h>
#include <algorithm>
#include <cmath>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

namespace {

constexpr int quickOperationHeight = 300;

const std::string ohosSystemTrayItemTitle = "Qt Application";

const auto trayIconGeometry = QRect(0, 0, 24, 24);

const auto notificationIconSize = QSize(128,128);

std::string applyWorkaroundForEmptyHoverTips(const std::string &hoverTips)
{
    return !hoverTips.empty() ? hoverTips : " ";
}

std::function<QNapi::Array(QtOhos::JsState &)> makeEmptyJsArrayFactory()
{
    return [](QtOhos::JsState &jsState) {
        return QNapi::Array::New(jsState.env());
    };
}

QNapi::Object getContextForStatusBarManager(QtOhos::JsState &jsState)
{
    auto qUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(jsState.defaultQAbilityPeer());
    if (!qUiAbilityPeer)
        qOhosReportFatalErrorAndAbort("%s: Default QAbilityPeer is not a QUiAbilityPeer", Q_FUNC_INFO);

    return qUiAbilityPeer->qAbility().eval<QNapi::Object>("context");
}

std::shared_ptr<void> registerOhosIconLeftClickListener(
    QtOhos::JsState &jsState, std::function<void()> leftClickListener)
{
    return QtOhos::registerOnOffMethodsBasedEventHandler(
        jsState.eval<QNapi::Object>("@kit.StatusBarExtensionKit.statusBarManager"),
        "statusBarIconClick",
        [leftClickListener = std::move(leftClickListener)](const QtOhos::CallbackInfo &cbInfo) {
            auto eventData = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
            auto optIconClickType = QNapi::getOptionalPropOrEmpty<QNapi::String>(
                QNapi::getOptionalPropOrEmpty<QNapi::Object>(eventData, "data"),
                "iconClickType");

            if (optIconClickType.IsEmpty()) {
                qOhosPrintfDebug(
                    "%s: no 'iconClickType' in the event (%s), ignoring it",
                    Q_FUNC_INFO, QNapi::toJsonString(eventData).c_str());
                return;
            }

            if (optIconClickType.Utf8Value() == "leftClick")
                leftClickListener();
        });
}

void removeIconFromOhosStatusBar(QtOhos::JsState &jsState)
{
    jsState.eval(
        "@kit.StatusBarExtensionKit.statusBarManager.removeFromStatusBar(*)",
        {getContextForStatusBarManager(jsState)});

    qOhosPrintfDebug("%s: Successfully removed icon from status bar", Q_FUNC_INFO);
}

std::shared_ptr<void> addItemToOhosStatusBar(QtOhos::JsState &jsState, QNapi::Object statusBarItem)
{
    jsState.eval(
        "@kit.StatusBarExtensionKit.statusBarManager.addToStatusBar(*)",
        {getContextForStatusBarManager(jsState), statusBarItem});

    qOhosPrintfDebug("%s: Successfully added icon to status bar [Statusbar] ", Q_FUNC_INFO);

    return QtOhos::makeDestroyNotifier(
        []() {
            QtOhos::runInJsThreadAndWait(&removeIconFromOhosStatusBar, Q_FUNC_INFO);
        });
}

void updateOhosStatusBarIcon(QtOhos::JsState &jsState, QNapi::Object iconData)
{
    qOhosPrintfDebug("%s", Q_FUNC_INFO);

    if (iconData.IsEmpty()) {
        qOhosPrintfError("%s: Icon data is empty", Q_FUNC_INFO);
        return;
    }

    jsState.eval(
        "@kit.StatusBarExtensionKit.statusBarManager.updateStatusBarIcon(*)",
        {getContextForStatusBarManager(jsState), iconData});

    qOhosPrintfDebug("%s: Successfully updated status bar icon", Q_FUNC_INFO);
}

void updateOhosStatusBarMenu(QtOhos::JsState &jsState, QNapi::Array statusBarGroupMenus)
{
    jsState.eval(
        "@kit.StatusBarExtensionKit.statusBarManager.updateStatusBarMenu(*)",
        {getContextForStatusBarManager(jsState), statusBarGroupMenus});

    qOhosPrintfDebug("%s: Successfully updated status bar menu", Q_FUNC_INFO);
}

QNapi::Object makeJsStatusBarItem(
    QtOhos::JsState &jsState, QNapi::Object statusBarIcon, const std::string &title,
    QNapi::Array statusBarGroupMenus, QOhosOptional<std::string> hoverTips)
{
    auto *env = jsState.env();

    auto quickOperation = QNapi::makeObject(
        env,
        {
            {"title", title},
            {"height", quickOperationHeight},
            {"abilityName", ""},
        });

    auto statusBarItem = QNapi::makeObject(
        env,
        {
            {"icons", statusBarIcon},
            {"quickOperation", quickOperation},
            {"statusBarGroupMenu", statusBarGroupMenus},
        });

    if (hoverTips.has_value())
        statusBarItem.set("hoverTips", applyWorkaroundForEmptyHoverTips(hoverTips.value()));

    return statusBarItem;
}

double getPrimaryDisplayPixelDensity(QtOhos::JsState &jsState)
{
    constexpr auto fallbackDisplayDensity = 1.0;

    auto primaryDisplay = jsState.eval<QNapi::Object>("@ohos.display.getPrimaryDisplaySync()");
    auto displayInfo = QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, primaryDisplay);

    double density;
    if (displayInfo.densityPixels > 0.0) {
        density = std::lround(displayInfo.densityPixels);
    } else {
        qOhosPrintfDebug("%s: invalid display densityPixels: %.3f", Q_FUNC_INFO, displayInfo.densityPixels);
        density = 0.0;
    }

    return density > 0.0 ? density : fallbackDisplayDensity;
}

QNapi::Object makeDisplayDensityScaledJsPixelMapFromQImage(QtOhos::JsState &jsState, const QImage &image)
{
    qOhosPrintfDebug(
        "%s: image dimensions: %dx%d, format: %d, bytes: %lld",
        Q_FUNC_INFO, image.width(), image.height(), static_cast<int>(image.format()), image.sizeInBytes());

    const int sourceWidth = image.width();
    const int sourceHeight = image.height();

    double density = getPrimaryDisplayPixelDensity(jsState);

    const double widthVp = std::lround(sourceWidth / density);
    const double heightVp = std::lround(sourceHeight / density);

    QImage newImage = image.scaled(widthVp, heightVp, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    const int width = newImage.width();
    const int height = newImage.height();
    const double finalWidthVp = width / density;
    const double finalHeightVp = height / density;

    qOhosPrintfDebug(
        "%s: density=%.3f, vp %.2f x %.2f -> %.2f x %.2f, px %dx%d -> %dx%d",
        Q_FUNC_INFO, density, widthVp, heightVp, finalWidthVp, finalHeightVp, sourceWidth, sourceHeight, width, height);

    return createNapiPixelMapFromQImage(jsState, newImage);
}

QImage convertIconToScaledImage(
    const QIcon &icon, const QSize &imageSize, const QColor &fallbackContentColor)
{
    auto iconImage = icon.pixmap(imageSize).toImage();
    if (!iconImage.isNull())
        return iconImage;

    auto fallbackImage = QImage(imageSize, QImage::Format_RGBA8888);
    fallbackImage.fill(fallbackContentColor);
    return fallbackImage;
}

QImage convertToMonochromeIcon(const QImage &sourceImage, bool useWhite)
{
    QImage monochromeImage = sourceImage.copy();
    const int colorValue = useWhite ? 255 : 0;

    for (int y = 0; y < monochromeImage.height(); ++y) {
        for (int x = 0; x < monochromeImage.width(); ++x) {
            const QRgb pixel = monochromeImage.pixel(x, y);
            const int alpha = qAlpha(pixel);

            if (alpha > 0)
                monochromeImage.setPixel(x, y, qRgba(colorValue, colorValue, colorValue, alpha));
        }
    }

    return monochromeImage;
}

QNapi::Object makeJsPixelMapFromIcon(QtOhos::JsState &jsState, const QIcon &icon, bool isWhiteIcon)
{
    const QSize iconSize(48, 48);

    QImage iconImage = icon.pixmap(iconSize).toImage();
    if (iconImage.isNull()) {
        QImage defaultImage(iconSize, QImage::Format_RGBA8888);
        defaultImage.fill(isWhiteIcon ? Qt::white : Qt::black);
        return makeDisplayDensityScaledJsPixelMapFromQImage(jsState, defaultImage);
    }

    qOhosPrintfDebug(
        "%s: original Icon dimensions: %dx%d, format: %d, bytes: %lld",
        Q_FUNC_INFO, iconImage.width(), iconImage.height(),
        static_cast<int>(iconImage.format()), iconImage.sizeInBytes());

    return makeDisplayDensityScaledJsPixelMapFromQImage(
        jsState, convertToMonochromeIcon(iconImage, isWhiteIcon));
}

QNapi::Object makeJsStatusBarIcon(QtOhos::JsState &jsState, const QIcon &icon)
{
    auto *env = jsState.env();

    auto whitePixelMap = makeJsPixelMapFromIcon(jsState, icon, true);
    auto blackPixelMap = makeJsPixelMapFromIcon(jsState, icon, false);

    auto imageSize = whitePixelMap.eval<QNapi::Object>("getImageInfoSync().size");;

    int imageWidth = imageSize.get<QNapi::Number>("width");
    int imageHeight = imageSize.get<QNapi::Number>("height");
    qOhosPrintfDebug("%s: ceated PixelMap size: %dx%d", Q_FUNC_INFO, imageWidth, imageHeight);

    return QNapi::makeObject(
        env,
        {
            {"white", whitePixelMap},
            {"black", blackPixelMap},
        });
}

QNapi::Object makeJsNotificationContent(
    QtOhos::JsState &jsState, const std::string &title, const std::string &text)
{
    using ContentType = QtOhos::enums::ohos::notificationManager::ContentType;

    return QNapi::makeObject(
        jsState.env(),
        {
            {"notificationContentType", jsState.mapOhosEnumToJs(ContentType::NOTIFICATION_CONTENT_BASIC_TEXT)},
            {
                "normal",
                QNapi::makeObject(
                    jsState.env(),
                    {
                        {"title", title},
                        {"text", text},
                    }),
            },
        });
}

QNapi::Object makeJsNotificationRequest(
    QtOhos::JsState &jsState, const std::string &title, const std::string &content,
    const QIcon &icon, QOhosOptional<int> optAutoDeletedDelayMs)
{
    auto iconPixelMap = makeDisplayDensityScaledJsPixelMapFromQImage(
        jsState, convertIconToScaledImage(icon, notificationIconSize, Qt::transparent));
    auto notificationRequest = QNapi::makeObject(
        jsState.env(),
        {
            {"content", makeJsNotificationContent(jsState, title, content)},
            {"smallIcon", iconPixelMap},
        });
    if (optAutoDeletedDelayMs.has_value()) {
        notificationRequest.set(
            "autoDeletedTime",
            QDateTime::currentMSecsSinceEpoch() + optAutoDeletedDelayMs.value());
    }

    return notificationRequest;
}

class QOhosSystemTrayIcon final : public QPlatformSystemTrayIcon
{
public:
    QOhosSystemTrayIcon();

    void init() override;
    void cleanup() override;
    void updateIcon(const QIcon &icon) override;
    void updateToolTip(const QString &tooltip) override;
    void updateMenu(QPlatformMenu *menu) override;
    QRect geometry() const override;
    void showMessage(
        const QString &title, const QString &msg, const QIcon &icon,
        MessageIcon iconType, int msecs) override;

    bool isSystemTrayAvailable() const override;
    bool supportsMessages() const override;

    QPlatformMenu *createMenu() const override;

private:
    QIcon m_icon;
    QOhosOptional<QString> m_optToolTip;
    QPlatformMenu *m_menu = nullptr;

    struct JsScopeData
    {
        std::shared_ptr<void> m_statusBarItemHandle;
        std::shared_ptr<void> m_iconLeftClickListenerHandle;
    };

    std::shared_ptr<JsScopeData> m_jsScopeData;
};

QOhosSystemTrayIcon::QOhosSystemTrayIcon() = default;

void QOhosSystemTrayIcon::init()
{
    if (m_jsScopeData)
        return;

    auto jsStatusBarGroupMenusFactory =
        m_menu != nullptr
            ? static_cast<QOhosStatusBarMenu *>(m_menu)->makeJsStatusBarGroupMenusFactory()
            : makeEmptyJsArrayFactory();

    auto selfRef = QtOhos::makeQThreadSafeRef(this);
    m_jsScopeData = QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            auto jsStatusBarIcon = makeJsStatusBarIcon(jsState, m_icon);
            auto jsStatusBarGroupMenus = jsStatusBarGroupMenusFactory(jsState);
            auto optHoverTipsString = qTransform(
                m_optToolTip,
                [](const auto &toolTip) {
                    return toolTip.toStdString();
                });
            auto jsStatusBarItem = makeJsStatusBarItem(
                jsState, jsStatusBarIcon, ohosSystemTrayItemTitle, jsStatusBarGroupMenus, optHoverTipsString);
            return QtOhos::makeProxyWithJsThreadDeleter(
                QtOhos::moveToSharedPtr(
                    JsScopeData {
                        .m_statusBarItemHandle = addItemToOhosStatusBar(jsState, jsStatusBarItem),
                        .m_iconLeftClickListenerHandle = registerOhosIconLeftClickListener(
                            jsState,
                            [selfRef]() {
                                selfRef.visitInQtThreadIfAlive(
                                    [](auto &self) {
                                        Q_EMIT self.activated(QPlatformSystemTrayIcon::Trigger);
                                    });
                            }),
                    }));
        },
        Q_FUNC_INFO);

    qOhosPrintfDebug("%s: System tray icon initialized", Q_FUNC_INFO);
}

void QOhosSystemTrayIcon::cleanup()
{
    m_jsScopeData.reset();
}

void QOhosSystemTrayIcon::updateIcon(const QIcon &icon)
{
    if (!m_jsScopeData)
        return;

    m_icon = icon;

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &jsState) {
            updateOhosStatusBarIcon(jsState, makeJsStatusBarIcon(jsState, icon));
        },
        Q_FUNC_INFO);
}

void QOhosSystemTrayIcon::updateToolTip(const QString &tooltip)
{
    m_optToolTip = tooltip;

    if (m_jsScopeData) {
        QtOhos::invokeInJsThreadAndWaitForContinue(
            [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
                jsState.evalToPromiseOrRejectOnThrow(
                    "@kit.StatusBarExtensionKit.statusBarManager.updateStatusBarHoverTips(*)",
                    {getContextForStatusBarManager(jsState), applyWorkaroundForEmptyHoverTips(tooltip.toStdString())})
                .onCatch(QtOhos::makeErrorLoggingJsCallback("updateStatusBarHoverTips()"))
                .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
            },
            Q_FUNC_INFO);
    }
}

QRect QOhosSystemTrayIcon::geometry() const
{
    return trayIconGeometry;
}

void QOhosSystemTrayIcon::showMessage(
    const QString &title, const QString &msg, const QIcon &icon, MessageIcon iconType, int msecs)
{
    Q_UNUSED(iconType);
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<> taskPromise) {
            auto notificationRequest = makeJsNotificationRequest(
                jsState, title.toStdString(), msg.toStdString(), icon,
                msecs > 0 ? makeQOhosOptional(msecs) : makeEmptyQOhosOptional());
            jsState.evalToPromiseOrRejectOnThrow("@ohos.notificationManager.publish(*)", {notificationRequest})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("publish()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

bool QOhosSystemTrayIcon::isSystemTrayAvailable() const
{
    return true;
}

bool QOhosSystemTrayIcon::supportsMessages() const
{
    return true;
}

QPlatformMenu *QOhosSystemTrayIcon::createMenu() const
{
    if (m_menu == nullptr)
        return makeQOhosStatusBarMenu().release();
    return m_menu;
}

void QOhosSystemTrayIcon::updateMenu(QPlatformMenu *menu)
{
    qOhosPrintfDebug("%s: Updating status bar menu", Q_FUNC_INFO);

    m_menu = menu;

    if (!m_jsScopeData) {
        qOhosPrintfDebug("%s: System tray not initialized yet", Q_FUNC_INFO);
        return;
    }

    auto jsStatusBarGroupMenusFactory =
        m_menu != nullptr
            ? static_cast<QOhosStatusBarMenu *>(m_menu)->makeJsStatusBarGroupMenusFactory()
            : makeEmptyJsArrayFactory();

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &jsState) {
            updateOhosStatusBarMenu(jsState, jsStatusBarGroupMenusFactory(jsState));
        },
        Q_FUNC_INFO);
}

}

std::unique_ptr<QPlatformSystemTrayIcon> makeQOhosSystemTrayIcon()
{
    return std::make_unique<QOhosSystemTrayIcon>();
}

QT_END_NAMESPACE
