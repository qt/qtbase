// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qappleiconengine_p.h"

#if defined(Q_OS_MACOS)
# include <AppKit/AppKit.h>
#elif defined(QT_PLATFORM_UIKIT)
# include <UIKit/UIKit.h>
#endif

#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpalette.h>
#include <QtGui/qstylehints.h>

#include <QtGui/private/qcoregraphics_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
auto *loadImage(const QString &iconName)
{
    static constexpr std::pair<QLatin1StringView, NSString *> iconMap[] = {
        {"address-book-new"_L1, @"book.closed"},
        {"application-exit"_L1, @"xmark.circle"},
        {"appointment-new"_L1, @"calendar.badge.plus"},
        {"call-start"_L1, @"phone.arrow.up.right"},
        {"call-stop"_L1, @"phone.down"},
        {"contact-new"_L1, @"person.crop.circle.badge.plus"},
        {"document-new"_L1, @"doc.badge.plus"},
        {"document-open"_L1, @"folder"},
        {"document-open-recent"_L1, @"doc.badge.clock"},
        {"document-page-setup"_L1, @"doc.badge.gearshape"},
        {"document-print"_L1, @"printer"},
        //{"document-print-preview"_L1, @""},
        {"document-properties"_L1, @"doc.badge.ellipsis"},
        //{"document-revert"_L1, @""},
        {"document-save"_L1, @"square.and.arrow.down"},
        //{"document-save-as"_L1, @""},
        {"document-send"_L1, @"paperplane"},
        {"edit-clear"_L1, @"xmark.circle"},
        {"edit-copy"_L1, @"doc.on.doc"},
        {"edit-cut"_L1, @"scissors"},
        {"edit-delete"_L1, @"delete.left"},
        {"edit-find"_L1, @"magnifyingglass"},
        //{"edit-find-replace"_L1, @"arrow.up.left.and.down.right.magnifyingglass"},
        {"edit-paste"_L1, @"clipboard"},
        {"edit-redo"_L1, @"arrowshape.turn.up.right"},
         //{"edit-select-all"_L1, @""},
        {"edit-undo"_L1, @"arrowshape.turn.up.left"},
        {"folder-new"_L1, @"folder.badge.plus"},
        {"format-indent-less"_L1, @"decrease.indent"},
        {"format-indent-more"_L1, @"increase.indent"},
        {"format-justify-center"_L1, @"text.aligncenter"},
        {"format-justify-fill"_L1, @"text.justify"},
        {"format-justify-left"_L1, @"text.justify.left"},
        {"format-justify-right"_L1, @"text.justify.right"},
        {"format-text-direction-ltr"_L1, @"text.justify.leading"},
        {"format-text-direction-rtl"_L1, @"text.justify.trailing"},
        {"format-text-bold"_L1, @"bold"},
        {"format-text-italic"_L1, @"italic"},
        {"format-text-underline"_L1, @"underline"},
        {"format-text-strikethrough"_L1, @"strikethrough"},
        //{"go-bottom"_L1, @""},
        {"go-down"_L1, @"arrowshape.down"},
        {"go-first"_L1, @"increase.indent"},
        {"go-home"_L1, @"house"},
        //{"go-jump"_L1, @""},
        //{"go-last"_L1, @""},
        {"go-next"_L1, @"arrowshape.right"},
        {"go-previous"_L1, @"arrowshape.left"},
        //{"go-top"_L1, @""},
        {"go-up"_L1, @"arrowshape.up"},
        {"help-about"_L1, @"info.circle"},
        //{"help-contents"_L1, @""},
        {"help-faq"_L1, @"questionmark.app"},
        {"insert-image"_L1, @"photo.badge.plus"},
        {"insert-link"_L1, @"link.badge.plus"},
        //{"insert-object"_L1, @""},
        {"insert-text"_L1, @"textformat"},
        {"list-add"_L1, @"plus.circle"},
        {"list-remove"_L1, @"minus.circle"},
        {"mail-forward"_L1, @"arrowshape.turn.up.right"},
        {"mail-mark-important"_L1, @"star"},
        {"mail-mark-junk"_L1, @"xmark.bin"},
        {"mail-mark-notjunk"_L1, @"trash.slash"},
        {"mail-mark-read"_L1, @"envelope.open"},
        {"mail-mark-unread"_L1, @"envelope.fill"},
        {"mail-message-new"_L1, @"square.and.pencil"},
        {"mail-reply-all"_L1, @"arrowshape.turn.up.left.2"},
        {"mail-reply-sender"_L1, @"arrowshape.turn.up.left"},
        {"mail-send"_L1, @"paperplane"},
        {"mail-send-receive"_L1, @"envelope.arrow.triangle.branch"},
        {"media-eject"_L1, @"eject"},
        {"media-playback-pause"_L1, @"pause"},
        {"media-playback-start"_L1, @"play"},
        {"media-playback-stop"_L1, @"stop"},
        {"media-record"_L1, @"record.circle"},
        {"media-seek-backward"_L1, @"backward"},
        {"media-seek-forward"_L1, @"forward"},
        {"media-skip-backward"_L1, @"backward.end.alt"},
        {"media-skip-forward"_L1, @"forward.end.alt"},
        {"object-flip-horizontal"_L1, @"rectangle.landscape.rotate"},
        {"object-flip-vertical"_L1, @"rectangle.portrait.rotate"},
        {"object-rotate-left"_L1, @"rotate.left"},
        {"object-rotate-right"_L1, @"rotate.right"},
        {"process-stop"_L1, @"stop.circle"},
        {"system-lock-screen"_L1, @"lock.display"},
        {"system-log-out"_L1, @"door.left.hand.open"},
        //{"system-run"_L1, @""},
        {"system-search"_L1, @"magnifyingglass"},
        //{"system-reboot"_L1, @""},
        {"system-shutdown"_L1, @"power"},
        //{"tools-check-spelling"_L1, @""},
        {"view-fullscreen"_L1, @"arrow.up.left.and.arrow.down.right"},
        {"view-refresh"_L1, @"arrow.clockwise"},
        {"view-restore"_L1, @"arrow.down.right.and.arrow.up.left"},
        //{"view-sort-ascending"_L1, @""},
        //{"view-sort-descending"_L1, @""},
        {"window-close"_L1, @"xmark.circle"},
        {"window-new"_L1, @"macwindow.badge.plus"},
        {"zoom-fit-best"_L1, @"square.arrowtriangle.4.outward"},
        {"zoom-in"_L1, @"plus.magnifyingglass"},
        //{"zoom-original"_L1, @""},
        {"zoom-out"_L1, @"minus.magnifyingglass"},
        {"process-working"_L1, @"circle.dotted"},
        //{"accessories-calculator"_L1, @""},
        //{"accessories-character-map"_L1, @""},
        {"accessories-dictionary"_L1, @"character.book.closed"},
        {"accessories-text-editor"_L1, @"textformat"},
        {"help-browser"_L1, @"folder.badge.questionmark"},
        {"multimedia-volume-control"_L1, @"speaker.wave.3"},
        {"preferences-desktop-accessibility"_L1, @"accessibility"},
        //{"preferences-desktop-font"_L1, @""},
        {"preferences-desktop-keyboard"_L1, @"keyboard.badge.ellipsis"},
        //{"preferences-desktop-locale"_L1, @""},
        //{"preferences-desktop-multimedia"_L1, @""},
        //{"preferences-desktop-screensaver"_L1, @""},
        //{"preferences-desktop-theme"_L1, @""},
        //{"preferences-desktop-wallpaper"_L1, @""},
        {"system-file-manager"_L1, @"folder.badge.gearshape"},
        //{"system-software-install"_L1, @""},
        //{"system-software-update"_L1, @""}, d
        //{"utilities-system-monitor"_L1, @""},
        {"utilities-terminal"_L1, @"apple.terminal"},
        //{"applications-accessories"_L1, @""},
        //{"applications-development"_L1, @""},
        //{"applications-engineering"_L1, @""},
        {"applications-games"_L1, @"gamecontroller"},
        //{"applications-graphics"_L1, @""},
        {"applications-internet"_L1, @"network"},
        {"applications-multimedia"_L1, @"tv.and.mediabox"},
        //{"applications-office"_L1, @""},
        //{"applications-other"_L1, @""},
        {"applications-science"_L1, @"atom"},
        //{"applications-system"_L1, @""},
        //{"applications-utilities"_L1, @""},
        {"preferences-desktop"_L1, @"menubar.dock.rectangle"},
        //{"preferences-desktop-peripherals"_L1, @""},
        //{"preferences-desktop-personal"_L1, @""},
        //{"preferences-other"_L1, @""},
        //{"preferences-system"_L1, @""},
        {"preferences-system-network"_L1, @"network"},
        {"system-help"_L1, @"questionmark.diamond"},
        {"audio-card"_L1, @"waveform.circle"},
        {"audio-input-microphone"_L1, @"mic"},
        {"battery"_L1, @"battery.100percent"},
        {"camera-photo"_L1, @"camera"},
        {"camera-video"_L1, @"video"},
        {"camera-web"_L1, @"web.camera"},
        {"computer"_L1, @"desktopcomputer"},
        {"drive-harddisk"_L1, @"internaldrive"},
        {"drive-optical"_L1, @"opticaldiscdrive"},
        {"drive-removable-media"_L1, @"externaldrive"},
        {"input-gaming"_L1, @"gamecontroller"}, // "games" also using this one
        {"input-keyboard"_L1, @"keyboard"},
        {"input-mouse"_L1, @"computermouse"},
        {"input-tablet"_L1, @"ipad"},
        {"media-flash"_L1, @"mediastick"},
        //{"media-floppy"_L1, @""},
        //{"media-optical"_L1, @""},
        {"media-tape"_L1, @"recordingtape"},
        //{"modem"_L1, @""},
        {"multimedia-player"_L1, @"play.rectangle"},
        {"network-wired"_L1, @"app.connected.to.app.below.fill"},
        {"network-wireless"_L1, @"wifi"},
        {"network-workgroup"_L1, @"network"},
        //{"pda"_L1, @""},
        {"phone"_L1, @"iphone"},
        {"printer"_L1, @"printer"},
        {"scanner"_L1, @"scanner"},
        {"video-display"_L1, @"play.display"},
        //{"emblem-default"_L1, @""},
        {"emblem-documents"_L1, @"doc.circle"},
        {"emblem-downloads"_L1, @"arrow.down.circle"},
        {"emblem-favorite"_L1, @"star"},
        {"emblem-important"_L1, @"exclamationmark.bubble.circle"},
        {"emblem-mail"_L1, @"envelope"},
        {"emblem-photos"_L1, @"photo.stack"},
        //{"emblem-readonly"_L1, @""},
        {"emblem-shared"_L1, @"folder.badge.person.crop"},
        {"emblem-symbolic-link"_L1, @"link.circle"},
        {"emblem-synchronized"_L1, @"arrow.triangle.2.circlepath.circle"},
        {"emblem-system"_L1, @"gear"},
        //{"emblem-unreadable"_L1, @""},
        {"text-x-generic"_L1, @"doc"}, // until iOS 18/macOS 15; @"document" after that
        {"folder"_L1, @"folder"},
        //{"folder-remote"_L1, @""},
        {"network-server"_L1, @"server.rack"},
        //{"start-here"_L1, @""},
        {"user-bookmarks"_L1, @"bookmark.circle"},
        {"user-desktop"_L1, @"menubar.dock.rectangle"}, // used by finder
        {"user-home"_L1, @"house"}, //"go-home" also using this one
        {"user-trash"_L1, @"trash"},
        {"appointment-missed"_L1, @"calendar.badge.exclamationmark"},
        {"appointment-soon"_L1, @"calendar.badge.clock"},
        {"audio-volume-high"_L1, @"speaker.wave.3"},
        {"audio-volume-low"_L1, @"speaker.wave.1"},
        {"audio-volume-medium"_L1, @"speaker.wave.2"},
        {"audio-volume-muted"_L1, @"speaker.slash"},
        {"battery-caution"_L1, @"minus.plus.batteryblock.exclamationmark"},
        {"battery-low"_L1, @"battery.25percent"}, // there are different levels that can be low battery
        {"dialog-error"_L1, @"exclamationmark.bubble"},
        {"dialog-information"_L1, @"info.circle"},
        {"dialog-password"_L1, @"lock"},
        {"dialog-question"_L1, @"questionmark.circle"},
        {"dialog-warning"_L1, @"exclamationmark.octagon"},
        {"folder-drag-accept"_L1, @"plus.rectangle.on.folder"},
        //{"folder-open"_L1, @""},
        {"folder-visiting"_L1, @"folder.circle"},
        {"image-loading"_L1, @"photo.circle"},
        {"image-missing"_L1, @"photo"},
        {"mail-attachment"_L1, @"paperclip"},
        {"mail-unread"_L1, @"envelope.badge"},
        {"mail-read"_L1, @"envelope.open"},
        {"mail-replied"_L1, @"arrowshape.turn.up.left"},
        //{"mail-signed"_L1, @""},
        //{"mail-signed-verified"_L1, @""},
        {"media-playlist-repeat"_L1, @"repet"},
        {"media-playlist-shuffle"_L1, @"shuffle"},
        //{"network-error"_L1, @""},
        //{"network-idle"_L1, @""},
        {"network-offline"_L1, @"network.slash"},
        //{"network-receive"_L1, @""},
        //{"network-transmit"_L1, @""},
        //{"network-transmit-receive"_L1, @""},
        //{"printer-error"_L1, @""},
        {"printer-printing"_L1, @"printer.dotmatrix.filled.and.paper"}, // not sure
        {"security-high"_L1, @"lock.shield"},
        //{"security-medium"_L1, @""},
        {"security-low"_L1, @"lock.trianglebadge.exclamationmark"},
        {"software-update-available"_L1, @"arrowshape.up.circle"},
        {"software-update-urgent"_L1, @"exclamationmark.transmission"},
        {"sync-error"_L1, @"exclamationmark.arrow.triangle.2.circlepath"},
        {"sync-synchronizing"_L1, @"arrow.triangle.2.circlepath"},
        {"task-due"_L1, @"clock.badge.exclamationmark"},
        {"task-past-due"_L1, @"clock.badge.xmark"},
        {"user-available"_L1, @"person.crop.circle.badge.checkmark"},
        {"user-away"_L1, @"person.crop.circle.badge.clock"},
        //{"user-idle"_L1, @""},
        {"user-offline"_L1, @"person.crop.circle.badge.xmark"},
        //{"user-trash-full"_L1, @""},
        {"weather-clear"_L1, @"sun.max"},
        {"weather-clear-night"_L1, @"moon"},
        {"weather-few-clouds"_L1, @"cloud.sun"},
        {"weather-few-clouds-night"_L1, @"cloud.moon"},
        {"weather-fog"_L1, @"cloud.fog"},
        {"weather-overcast"_L1, @"cloud"},
        //{"weather-severe-alert"_L1, @""},
        {"weather-showers"_L1, @"cloud.rain"},
        //{"weather-showers-scattered"_L1, @""},
        {"weather-snow"_L1, @"cloud.snow"},
        {"weather-storm"_L1, @"tropicalstorm"},
    };
    const auto it = std::find_if(std::begin(iconMap), std::end(iconMap), [iconName](const auto &c){
        return c.first == iconName;
    });
    NSString *systemIconName = it != std::end(iconMap) ? it->second : iconName.toNSString();
#if defined(Q_OS_MACOS)
    return [NSImage imageWithSystemSymbolName:systemIconName accessibilityDescription:nil];
#elif defined(QT_PLATFORM_UIKIT)
    return [UIImage systemImageNamed:systemIconName];
#endif
}
}

QAppleIconEngine::QAppleIconEngine(const QString &iconName)
    : m_iconName(iconName), m_image(loadImage(iconName))
{
    if (m_image)
        [m_image retain];
}

QAppleIconEngine::~QAppleIconEngine()
{
    if (m_image)
        [m_image release];
}

QIconEngine *QAppleIconEngine::clone() const
{
    return new QAppleIconEngine(m_iconName);
}

QString QAppleIconEngine::key() const
{
    return u"QAppleIconEngine"_s;
}

QString QAppleIconEngine::iconName()
{
    return m_iconName;
}

bool QAppleIconEngine::isNull()
{
    return m_image == nullptr;
}

QList<QSize> QAppleIconEngine::availableIconSizes(double aspectRatio)
{
    const qreal devicePixelRatio = qGuiApp->devicePixelRatio();
    const QList<QSize> sizes = {
        {qRound(16 * devicePixelRatio), qRound(16. * devicePixelRatio / aspectRatio)},
        {qRound(32 * devicePixelRatio), qRound(32. * devicePixelRatio / aspectRatio)},
        {qRound(64 * devicePixelRatio), qRound(64. * devicePixelRatio / aspectRatio)},
        {qRound(128 * devicePixelRatio), qRound(128. * devicePixelRatio / aspectRatio)},
        {qRound(256 * devicePixelRatio), qRound(256. * devicePixelRatio / aspectRatio)},
    };
    return sizes;
}

QList<QSize> QAppleIconEngine::availableSizes(QIcon::Mode, QIcon::State)
{
    const double aspectRatio = isNull() ? 1.0 : m_image.size.width / m_image.size.height;
    return availableIconSizes(aspectRatio);
}

QSize QAppleIconEngine::actualSize(const QSize &size, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
    const double inputAspectRatio = isNull() ? 1.0 : m_image.size.width / m_image.size.height;
    const double outputAspectRatio = size.width() / size.height();
    QSize result = size;
    if (outputAspectRatio > inputAspectRatio)
        result.rwidth() = result.height() * inputAspectRatio;
    else
        result.rheight() = result.width() / inputAspectRatio;
    return result;
}

QPixmap QAppleIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

namespace {
#if defined(Q_OS_MACOS)
auto *configuredImage(const NSImage *image, const QColor &color)
{
    auto *config = [NSImageSymbolConfiguration configurationWithPointSize:48
                                               weight:NSFontWeightRegular
                                               scale:NSImageSymbolScaleLarge];

    NSImage *configuredImage = [image imageWithSymbolConfiguration:config];

    auto *primaryColor = [NSColor colorWithSRGBRed:color.redF()
                                             green:color.greenF()
                                              blue:color.blueF()
                                             alpha:color.alphaF()];

    NSImage *tintedImage = [NSImage imageWithSize:configuredImage.size flipped:NO
        drawingHandler:^BOOL(NSRect) {
            [primaryColor set];
            NSRect imageRect = {NSZeroPoint, configuredImage.size};
            [configuredImage drawInRect:imageRect];
            NSRectFillUsingOperation(imageRect, NSCompositingOperationSourceIn);
            return YES;
        }];
    return tintedImage;
}
#elif defined(QT_PLATFORM_UIKIT)
auto *configuredImage(const UIImage *image, const QColor &color)
{
    auto *config = [UIImageSymbolConfiguration configurationWithPointSize:48
                                               weight:UIImageSymbolWeightRegular
                                               scale:UIImageSymbolScaleLarge];

    auto *primaryColor = [UIColor colorWithRed:color.redF()
                                     green:color.greenF()
                                      blue:color.blueF()
                                     alpha:color.alphaF()];

    return [[image imageByApplyingSymbolConfiguration:config] imageWithTintColor:primaryColor];
}
#endif
}

QPixmap QAppleIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    const CacheKey key(mode, state, size, scale);
    QPixmap pixmap = m_cache.value(key);
    if (pixmap.isNull()) {
        const QSize paintSize = actualSize(size, mode, state);
        const QSize paintOffset = paintSize != size
                                ? (QSizeF(size - paintSize) * 0.5).toSize()
                                : QSize();

        pixmap = QPixmap(size * scale);
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);

        if (!pixmap.isNull()) {
            QPainter painter(&pixmap);
            paint(&painter, QRect(paintOffset.width(), paintOffset.height(),
                                  paintSize.width(), paintSize.height()), mode, state);
            m_cache.insert(key, pixmap);
        }
    }
    return pixmap;
}

void QAppleIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);

    QColor color;
    const QPalette palette;
    switch (mode) {
    case QIcon::Normal:
        color = palette.color(QPalette::Inactive, QPalette::Text);
        break;
    case QIcon::Disabled:
        color = palette.color(QPalette::Disabled, QPalette::Text);
        break;
    case QIcon::Active:
        color = palette.color(QPalette::Active, QPalette::Text);
        break;
    case QIcon::Selected:
        color = palette.color(QPalette::Active, QPalette::HighlightedText);
        break;
    }
    const auto *image = configuredImage(m_image, color);

    QMacCGContext ctx(painter);

#if defined(Q_OS_MACOS)
    NSGraphicsContext *gc = [NSGraphicsContext graphicsContextWithCGContext:ctx flipped:YES];
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:gc];

    const NSSize pixmapSize = NSMakeSize(rect.width(), rect.height());
    [image setSize:pixmapSize];
    const NSRect sourceRect = NSMakeRect(0, 0, pixmapSize.width, pixmapSize.height);
    const NSRect iconRect = NSMakeRect(rect.x(), rect.y(), pixmapSize.width, pixmapSize.height);

    [image drawInRect:iconRect fromRect:sourceRect operation:NSCompositingOperationSourceOver fraction:1.0 respectFlipped:YES hints:nil];
    [NSGraphicsContext restoreGraphicsState];
#elif defined(QT_PLATFORM_UIKIT)
    UIGraphicsPushContext(ctx);
    const CGRect cgrect = CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
    [image drawInRect:cgrect];
    UIGraphicsPopContext();
#endif
}

QT_END_NAMESPACE
