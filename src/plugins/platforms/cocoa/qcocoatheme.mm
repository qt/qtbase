/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import <AppKit/AppKit.h>

#include "qcocoatheme.h"

#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QVariant>

#include "qcocoasystemtrayicon.h"
#include "qcocoamenuitem.h"
#include "qcocoamenu.h"
#include "qcocoamenubar.h"
#include "qcocoahelpers.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/private/qfont_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qcoregraphics_p.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextformat.h>
#include <QtFontDatabaseSupport/private/qcoretextfontdatabase_p.h>
#include <QtFontDatabaseSupport/private/qfontengine_coretext_p.h>
#include <QtThemeSupport/private/qabstractfileiconengine_p.h>
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qtwidgetsglobal.h>
#if QT_CONFIG(colordialog)
#include "qcocoacolordialoghelper.h"
#endif
#if QT_CONFIG(filedialog)
#include "qcocoafiledialoghelper.h"
#endif
#if QT_CONFIG(fontdialog)
#include "qcocoafontdialoghelper.h"
#endif
#endif

#include <CoreServices/CoreServices.h>

#if !QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_14)
@interface NSColor (MojaveForwardDeclarations)
@property (class, strong, readonly) NSColor *selectedContentBackgroundColor NS_AVAILABLE_MAC(10_14);
@property (class, strong, readonly) NSColor *unemphasizedSelectedTextBackgroundColor NS_AVAILABLE_MAC(10_14);
@property (class, strong, readonly) NSColor *unemphasizedSelectedTextColor NS_AVAILABLE_MAC(10_14);
@property (class, strong, readonly) NSColor *unemphasizedSelectedContentBackgroundColor NS_AVAILABLE_MAC(10_14);
@property (class, strong, readonly) NSArray<NSColor *> *alternatingContentBackgroundColors NS_AVAILABLE_MAC(10_14);
// Missing from non-Mojave SDKs, even if introduced in 10.10
@property (class, strong, readonly) NSColor *linkColor NS_AVAILABLE_MAC(10_10);
@end
#endif

QT_BEGIN_NAMESPACE

static QPalette *qt_mac_createSystemPalette()
{
    QColor qc;

    // Standard palette initialization (copied from Qt 4 styles)
    QBrush backgroundBrush = qt_mac_toQBrush([NSColor windowBackgroundColor]);
    QColor background = backgroundBrush.color();
    QColor light(background.lighter(110));
    QColor dark(background.darker(160));
    QColor mid(background.darker(140));
    QPalette *palette = new QPalette(Qt::black, background, light, dark, mid, Qt::black, Qt::white);

    palette->setBrush(QPalette::Window, backgroundBrush);

    palette->setBrush(QPalette::Disabled, QPalette::WindowText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Text, dark);
    palette->setBrush(QPalette::Disabled, QPalette::ButtonText, dark);
    palette->setBrush(QPalette::Disabled, QPalette::Base, backgroundBrush);
    QBrush textBackgroundBrush = qt_mac_toQBrush([NSColor textBackgroundColor]);
    palette->setBrush(QPalette::Active, QPalette::Base, textBackgroundBrush);
    palette->setBrush(QPalette::Inactive, QPalette::Base, textBackgroundBrush);
    palette->setColor(QPalette::Disabled, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Active, QPalette::Dark, QColor(191, 191, 191));
    palette->setColor(QPalette::Inactive, QPalette::Dark, QColor(191, 191, 191));

    // System palette initialization:
    QBrush br = qt_mac_toQBrush([NSColor selectedControlColor]);
    palette->setBrush(QPalette::Active, QPalette::Highlight, br);
    if (__builtin_available(macOS 10.14, *)) {
        const auto inactiveHighlight = qt_mac_toQBrush([NSColor unemphasizedSelectedContentBackgroundColor]);
        palette->setBrush(QPalette::Inactive, QPalette::Highlight, inactiveHighlight);
        palette->setBrush(QPalette::Disabled, QPalette::Highlight, inactiveHighlight);
    } else {
        palette->setBrush(QPalette::Inactive, QPalette::Highlight, br);
        palette->setBrush(QPalette::Disabled, QPalette::Highlight, br);
    }

    palette->setBrush(QPalette::Shadow, qt_mac_toQColor([NSColor shadowColor]));

    qc = qt_mac_toQColor([NSColor controlTextColor]);
    palette->setColor(QPalette::Active, QPalette::Text, qc);
    palette->setColor(QPalette::Active, QPalette::WindowText, qc);
    palette->setColor(QPalette::Active, QPalette::HighlightedText, qc);
    palette->setColor(QPalette::Inactive, QPalette::Text, qc);
    palette->setColor(QPalette::Inactive, QPalette::WindowText, qc);
    palette->setColor(QPalette::Inactive, QPalette::HighlightedText, qc);

    qc = qt_mac_toQColor([NSColor disabledControlTextColor]);
    palette->setColor(QPalette::Disabled, QPalette::Text, qc);
    palette->setColor(QPalette::Disabled, QPalette::WindowText, qc);
    palette->setColor(QPalette::Disabled, QPalette::HighlightedText, qc);

    palette->setBrush(QPalette::ToolTipBase, qt_mac_toQBrush([NSColor controlColor]));

    palette->setColor(QPalette::Normal, QPalette::Link, qt_mac_toQColor([NSColor linkColor]));

    return palette;
}

struct QMacPaletteMap {
    inline QMacPaletteMap(QPlatformTheme::Palette p, NSColor *a, NSColor *i) :
        active(a), inactive(i), paletteRole(p) { }

    NSColor *active;
    NSColor *inactive;
    QPlatformTheme::Palette paletteRole;
};

#define MAC_PALETTE_ENTRY(pal, active, inactive) \
    QMacPaletteMap(pal, [NSColor active], [NSColor inactive])
static QMacPaletteMap mac_widget_colors[] = {
    MAC_PALETTE_ENTRY(QPlatformTheme::ToolButtonPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::ButtonPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::HeaderPalette, headerTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::ComboBoxPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::ItemViewPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::MessageBoxLabelPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::TabBarPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::LabelPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::GroupBoxPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::MenuPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::MenuBarPalette, controlTextColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::TextEditPalette, textColor, disabledControlTextColor),
    MAC_PALETTE_ENTRY(QPlatformTheme::TextLineEditPalette, textColor, disabledControlTextColor)
};
#undef MAC_PALETTE_ENTRY

static const int mac_widget_colors_count = sizeof(mac_widget_colors) / sizeof(mac_widget_colors[0]);

static QHash<QPlatformTheme::Palette, QPalette*> qt_mac_createRolePalettes()
{
    QHash<QPlatformTheme::Palette, QPalette*> palettes;
    QColor qc;
    for (int i = 0; i < mac_widget_colors_count; i++) {
        QPalette &pal = *qt_mac_createSystemPalette();
        if (mac_widget_colors[i].active) {
            qc = qt_mac_toQColor(mac_widget_colors[i].active);
            pal.setColor(QPalette::Active, QPalette::Text, qc);
            pal.setColor(QPalette::Inactive, QPalette::Text, qc);
            pal.setColor(QPalette::Active, QPalette::WindowText, qc);
            pal.setColor(QPalette::Inactive, QPalette::WindowText, qc);
            pal.setColor(QPalette::Active, QPalette::HighlightedText, qc);
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText, qc);
            pal.setColor(QPalette::Active, QPalette::ButtonText, qc);
            pal.setColor(QPalette::Inactive, QPalette::ButtonText, qc);
            qc = qt_mac_toQColor(mac_widget_colors[i].inactive);
            pal.setColor(QPalette::Disabled, QPalette::Text, qc);
            pal.setColor(QPalette::Disabled, QPalette::WindowText, qc);
            pal.setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
            pal.setColor(QPalette::Disabled, QPalette::ButtonText, qc);
        }
        if (mac_widget_colors[i].paletteRole == QPlatformTheme::MenuPalette
                || mac_widget_colors[i].paletteRole == QPlatformTheme::MenuBarPalette) {
            NSColor *selectedMenuItemColor = nil;
            if (__builtin_available(macOS 10.14, *)) {
                // Cheap approximation for NSVisualEffectView (see deprecation note for selectedMenuItemTextColor)
                selectedMenuItemColor = [[NSColor selectedContentBackgroundColor] highlightWithLevel:0.4];
            } else {
                // selectedMenuItemColor would presumably be the correct color to use as the background
                // for selected menu items. But that color is always blue, and doesn't follow the
                // appearance color in system preferences. So we therefore deliberatly choose to use
                // keyboardFocusIndicatorColor instead, which appears to have the same color value.
                selectedMenuItemColor = [NSColor keyboardFocusIndicatorColor];
            }
            pal.setBrush(QPalette::Highlight, qt_mac_toQColor(selectedMenuItemColor));
            qc = qt_mac_toQColor([NSColor labelColor]);
            pal.setBrush(QPalette::ButtonText, qc);
            pal.setBrush(QPalette::Text, qc);
            qc = qt_mac_toQColor([NSColor selectedMenuItemTextColor]);
            pal.setBrush(QPalette::HighlightedText, qc);
            qc = qt_mac_toQColor([NSColor disabledControlTextColor]);
            pal.setBrush(QPalette::Disabled, QPalette::Text, qc);
        } else if ((mac_widget_colors[i].paletteRole == QPlatformTheme::ButtonPalette)
                || (mac_widget_colors[i].paletteRole == QPlatformTheme::HeaderPalette)
                || (mac_widget_colors[i].paletteRole == QPlatformTheme::TabBarPalette)) {
            pal.setColor(QPalette::Disabled, QPalette::ButtonText,
                         pal.color(QPalette::Disabled, QPalette::Text));
            pal.setColor(QPalette::Inactive, QPalette::ButtonText,
                         pal.color(QPalette::Inactive, QPalette::Text));
            pal.setColor(QPalette::Active, QPalette::ButtonText,
                         pal.color(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::ItemViewPalette) {
            NSArray<NSColor *> *baseColors = nil;
            NSColor *activeHighlightColor = nil;
            if (__builtin_available(macOS 10.14, *)) {
                baseColors = [NSColor alternatingContentBackgroundColors];
                activeHighlightColor = [NSColor selectedContentBackgroundColor];
                pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                             qt_mac_toQBrush([NSColor unemphasizedSelectedTextColor]));
            } else {
                baseColors = [NSColor controlAlternatingRowBackgroundColors];
                activeHighlightColor = [NSColor alternateSelectedControlColor];
                pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                             pal.brush(QPalette::Active, QPalette::Text));
            }
            pal.setBrush(QPalette::Base, qt_mac_toQBrush(baseColors[0]));
            pal.setBrush(QPalette::AlternateBase, qt_mac_toQBrush(baseColors[1]));
            pal.setBrush(QPalette::Active, QPalette::Highlight,
                         qt_mac_toQBrush(activeHighlightColor));
            pal.setBrush(QPalette::Active, QPalette::HighlightedText,
                         qt_mac_toQBrush([NSColor alternateSelectedControlTextColor]));
            pal.setBrush(QPalette::Inactive, QPalette::Text,
                         pal.brush(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::TextEditPalette) {
            pal.setBrush(QPalette::Active, QPalette::Base, qt_mac_toQColor([NSColor textBackgroundColor]));
            pal.setBrush(QPalette::Inactive, QPalette::Text,
                          pal.brush(QPalette::Active, QPalette::Text));
            pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                          pal.brush(QPalette::Active, QPalette::Text));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::TextLineEditPalette
                   || mac_widget_colors[i].paletteRole == QPlatformTheme::ComboBoxPalette) {
            pal.setBrush(QPalette::Active, QPalette::Base, qt_mac_toQColor([NSColor textBackgroundColor]));
            pal.setBrush(QPalette::Disabled, QPalette::Base,
                         pal.brush(QPalette::Active, QPalette::Base));
        } else if (mac_widget_colors[i].paletteRole == QPlatformTheme::LabelPalette) {
            qc = qt_mac_toQColor([NSColor labelColor]);
            pal.setBrush(QPalette::Inactive, QPalette::ToolTipText, qc);
        }
        palettes.insert(mac_widget_colors[i].paletteRole, &pal);
    }
    return palettes;
}

const char *QCocoaTheme::name = "cocoa";

QCocoaTheme::QCocoaTheme()
    : m_systemPalette(nullptr)
{
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_14)
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave) {
        m_appearanceObserver = QMacKeyValueObserver(NSApp, @"effectiveAppearance", [this] {
            if (__builtin_available(macOS 10.14, *))
                NSAppearance.currentAppearance = NSApp.effectiveAppearance;

            handleSystemThemeChange();
        });
    }
#endif

    m_systemColorObserver = QMacNotificationObserver(nil,
        NSSystemColorsDidChangeNotification, [this] {
            handleSystemThemeChange();
    });
}

QCocoaTheme::~QCocoaTheme()
{
    reset();
    qDeleteAll(m_fonts);
}

void QCocoaTheme::reset()
{
    delete m_systemPalette;
    m_systemPalette = nullptr;
    qDeleteAll(m_palettes);
    m_palettes.clear();
}

void QCocoaTheme::handleSystemThemeChange()
{
    reset();
    m_systemPalette = qt_mac_createSystemPalette();
    m_palettes = qt_mac_createRolePalettes();

    if (QCoreTextFontEngine::fontSmoothing() == QCoreTextFontEngine::FontSmoothing::Grayscale) {
        // Re-populate glyph caches based on the new appearance's assumed text fill color
        QFontCache::instance()->clear();
    }

    QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>(nullptr);
}

bool QCocoaTheme::usePlatformNativeDialog(DialogType dialogType) const
{
    if (dialogType == QPlatformTheme::FileDialog)
        return true;
#if defined(QT_WIDGETS_LIB) && QT_CONFIG(colordialog)
    if (dialogType == QPlatformTheme::ColorDialog)
        return true;
#endif
#if defined(QT_WIDGETS_LIB) && QT_CONFIG(fontdialog)
    if (dialogType == QPlatformTheme::FontDialog)
        return true;
#endif
    return false;
}

QPlatformDialogHelper *QCocoaTheme::createPlatformDialogHelper(DialogType dialogType) const
{
    switch (dialogType) {
#if defined(QT_WIDGETS_LIB) && QT_CONFIG(filedialog)
    case QPlatformTheme::FileDialog:
        return new QCocoaFileDialogHelper();
#endif
#if defined(QT_WIDGETS_LIB) && QT_CONFIG(colordialog)
    case QPlatformTheme::ColorDialog:
        return new QCocoaColorDialogHelper();
#endif
#if defined(QT_WIDGETS_LIB) && QT_CONFIG(fontdialog)
    case QPlatformTheme::FontDialog:
        return new QCocoaFontDialogHelper();
#endif
    default:
        return nullptr;
    }
}

#ifndef QT_NO_SYSTEMTRAYICON
QPlatformSystemTrayIcon *QCocoaTheme::createPlatformSystemTrayIcon() const
{
    return new QCocoaSystemTrayIcon;
}
#endif

const QPalette *QCocoaTheme::palette(Palette type) const
{
    if (type == SystemPalette) {
        if (!m_systemPalette)
            m_systemPalette = qt_mac_createSystemPalette();
        return m_systemPalette;
    } else {
        if (m_palettes.isEmpty())
            m_palettes = qt_mac_createRolePalettes();
        return m_palettes.value(type, nullptr);
    }
    return nullptr;
}

const QFont *QCocoaTheme::font(Font type) const
{
    if (m_fonts.isEmpty()) {
        const auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
        const auto *coreTextFontDb = static_cast<QCoreTextFontDatabase *>(platformIntegration->fontDatabase());
        m_fonts = coreTextFontDb->themeFonts();
    }
    return m_fonts.value(type, nullptr);
}

//! \internal
QPixmap qt_mac_convert_iconref(const IconRef icon, int width, int height)
{
    QPixmap ret(width, height);
    ret.fill(QColor(0, 0, 0, 0));

    CGRect rect = CGRectMake(0, 0, width, height);

    QMacCGContext ctx(&ret);
    CGAffineTransform old_xform = CGContextGetCTM(ctx);
    CGContextConcatCTM(ctx, CGAffineTransformInvert(old_xform));
    CGContextConcatCTM(ctx, CGAffineTransformIdentity);

    ::RGBColor b;
    b.blue = b.green = b.red = 255*255;
    PlotIconRefInContext(ctx, &rect, kAlignNone, kTransformNone, &b, kPlotIconRefNormalFlags, icon);
    return ret;
}

QPixmap QCocoaTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    OSType iconType = 0;
    switch (sp) {
    case MessageBoxQuestion:
        iconType = kQuestionMarkIcon;
        break;
    case MessageBoxInformation:
        iconType = kAlertNoteIcon;
        break;
    case MessageBoxWarning:
        iconType = kAlertCautionIcon;
        break;
    case MessageBoxCritical:
        iconType = kAlertStopIcon;
        break;
    case DesktopIcon:
        iconType = kDesktopIcon;
        break;
    case TrashIcon:
        iconType = kTrashIcon;
        break;
    case ComputerIcon:
        iconType = kComputerIcon;
        break;
    case DriveFDIcon:
        iconType = kGenericFloppyIcon;
        break;
    case DriveHDIcon:
        iconType = kGenericHardDiskIcon;
        break;
    case DriveCDIcon:
    case DriveDVDIcon:
        iconType = kGenericCDROMIcon;
        break;
    case DriveNetIcon:
        iconType = kGenericNetworkIcon;
        break;
    case DirOpenIcon:
        iconType = kOpenFolderIcon;
        break;
    case DirClosedIcon:
    case DirLinkIcon:
        iconType = kGenericFolderIcon;
        break;
    case FileLinkIcon:
    case FileIcon:
        iconType = kGenericDocumentIcon;
        break;
    default:
        break;
    }
    if (iconType != 0) {
        QPixmap pixmap;
        IconRef icon = nullptr;
        GetIconRef(kOnSystemDisk, kSystemIconsCreator, iconType, &icon);

        if (icon) {
            pixmap = qt_mac_convert_iconref(icon, size.width(), size.height());
            ReleaseIconRef(icon);
        }

        return pixmap;
    }

    return QPlatformTheme::standardPixmap(sp, size);
}

class QCocoaFileIconEngine : public QAbstractFileIconEngine
{
public:
    explicit QCocoaFileIconEngine(const QFileInfo &info,
                                  QPlatformTheme::IconOptions opts) :
        QAbstractFileIconEngine(info, opts) {}

    static QList<QSize> availableIconSizes()
    {
        const qreal devicePixelRatio = qGuiApp->devicePixelRatio();
        const int sizes[] = {
            qRound(16 * devicePixelRatio), qRound(32 * devicePixelRatio),
            qRound(64 * devicePixelRatio), qRound(128 * devicePixelRatio),
            qRound(256 * devicePixelRatio)
        };
        return QAbstractFileIconEngine::toSizeList(sizes, sizes + sizeof(sizes) / sizeof(sizes[0]));
    }

    QList<QSize> availableSizes(QIcon::Mode = QIcon::Normal, QIcon::State = QIcon::Off) const override
    { return QCocoaFileIconEngine::availableIconSizes(); }

protected:
    QPixmap filePixmap(const QSize &size, QIcon::Mode, QIcon::State) override
    {
        QMacAutoReleasePool pool;

        NSImage *iconImage = [[NSWorkspace sharedWorkspace] iconForFile:fileInfo().canonicalFilePath().toNSString()];
        if (!iconImage)
            return QPixmap();
        return qt_mac_toQPixmap(iconImage, size);
    }
};

QIcon QCocoaTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions) const
{
    return QIcon(new QCocoaFileIconEngine(fileInfo, iconOptions));
}

QVariant QCocoaTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::StyleNames:
        return QStringList(QStringLiteral("macintosh"));
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(QPlatformDialogHelper::MacLayout);
    case KeyboardScheme:
        return QVariant(int(MacKeyboardScheme));
    case TabFocusBehavior:
        return QVariant([[NSApplication sharedApplication] isFullKeyboardAccessEnabled] ?
                    int(Qt::TabFocusAllControls) : int(Qt::TabFocusTextControls | Qt::TabFocusListControls));
    case IconPixmapSizes:
        return QVariant::fromValue(QCocoaFileIconEngine::availableIconSizes());
    case QPlatformTheme::PasswordMaskCharacter:
        return QVariant(QChar(0x2022));
    case QPlatformTheme::UiEffects:
        return QVariant(int(HoverEffect));
    case QPlatformTheme::SpellCheckUnderlineStyle:
        return QVariant(int(QTextCharFormat::DotLine));
    case QPlatformTheme::UseFullScreenForPopupMenu:
        return QVariant(bool([[NSApplication sharedApplication] presentationOptions] & NSApplicationPresentationFullScreen));
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

QString QCocoaTheme::standardButtonText(int button) const
{
    return button == QPlatformDialogHelper::Discard ?
        QCoreApplication::translate("QCocoaTheme", "Don't Save")
      : QPlatformTheme::standardButtonText(button);
}

QKeySequence QCocoaTheme::standardButtonShortcut(int button) const
{
    return button == QPlatformDialogHelper::Discard ? QKeySequence(Qt::CTRL | Qt::Key_Delete)
                                                    : QPlatformTheme::standardButtonShortcut(button);
}

QPlatformMenuItem *QCocoaTheme::createPlatformMenuItem() const
{
    return new QCocoaMenuItem();
}

QPlatformMenu *QCocoaTheme::createPlatformMenu() const
{
    return new QCocoaMenu();
}

QPlatformMenuBar *QCocoaTheme::createPlatformMenuBar() const
{
    static bool haveMenubar = false;
    if (!haveMenubar) {
        haveMenubar = true;
        QObject::connect(qGuiApp, SIGNAL(focusWindowChanged(QWindow*)),
            QGuiApplicationPrivate::platformIntegration()->nativeInterface(),
                SLOT(onAppFocusWindowChanged(QWindow*)));
    }

    return new QCocoaMenuBar();
}

QT_END_NAMESPACE
