// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoatheme.h"

#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QVariant>

#include "qcocoasystemtrayicon.h"
#include "qcocoamenuitem.h"
#include "qcocoamenu.h"
#include "qcocoamenubar.h"
#include "qcocoahelpers.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/private/qfont_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qcoregraphics_p.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextformat.h>
#include <QtGui/private/qcoretextfontdatabase_p.h>
#include <QtGui/private/qfontengine_coretext_p.h>
#include <QtGui/private/qabstractfileiconengine_p.h>
#include <qpa/qplatformdialoghelper.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

#include "qcocoacolordialoghelper.h"
#include "qcocoafiledialoghelper.h"
#include "qcocoafontdialoghelper.h"
#include "qcocoamessagedialog.h"

#include <CoreServices/CoreServices.h>

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
    const auto inactiveHighlight = qt_mac_toQBrush([NSColor unemphasizedSelectedContentBackgroundColor]);
    palette->setBrush(QPalette::Inactive, QPalette::Highlight, inactiveHighlight);
    palette->setBrush(QPalette::Disabled, QPalette::Highlight, inactiveHighlight);

    palette->setBrush(QPalette::Shadow, qt_mac_toQColor([NSColor shadowColor]));

    qc = qt_mac_toQColor([NSColor controlTextColor]);
    palette->setColor(QPalette::Active, QPalette::Text, qc);
    palette->setColor(QPalette::Active, QPalette::ButtonText, qc);
    palette->setColor(QPalette::Active, QPalette::WindowText, qc);
    palette->setColor(QPalette::Active, QPalette::HighlightedText, qc);
    palette->setColor(QPalette::Inactive, QPalette::Text, qc);
    palette->setColor(QPalette::Inactive, QPalette::WindowText, qc);
    palette->setColor(QPalette::Inactive, QPalette::HighlightedText, qc);

    qc = qt_mac_toQColor([NSColor disabledControlTextColor]);
    palette->setColor(QPalette::Disabled, QPalette::Text, qc);
    palette->setColor(QPalette::Disabled, QPalette::ButtonText, qc);
    palette->setColor(QPalette::Disabled, QPalette::WindowText, qc);
    palette->setColor(QPalette::Disabled, QPalette::HighlightedText, qc);

    palette->setBrush(QPalette::ToolTipBase, qt_mac_toQBrush([NSColor controlColor]));

    palette->setColor(QPalette::Normal, QPalette::Link, qt_mac_toQColor([NSColor linkColor]));

    qc = qt_mac_toQColor([NSColor placeholderTextColor]);
    palette->setColor(QPalette::Active, QPalette::PlaceholderText, qc);
    palette->setColor(QPalette::Inactive, QPalette::PlaceholderText, qc);
    palette->setColor(QPalette::Disabled, QPalette::PlaceholderText, qc);

    qc = qt_mac_toQColor([NSColor controlAccentColor]);
    palette->setColor(QPalette::Accent, qc);

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
            // Cheap approximation for NSVisualEffectView (see deprecation note for selectedMenuItemTextColor)
            auto selectedMenuItemColor = [[NSColor controlAccentColor] highlightWithLevel:0.3];
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
            baseColors = [NSColor alternatingContentBackgroundColors];
            activeHighlightColor = [NSColor selectedContentBackgroundColor];
            pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                         qt_mac_toQBrush([NSColor unemphasizedSelectedTextColor]));
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
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave) {
        m_appearanceObserver = QMacKeyValueObserver(NSApp, @"effectiveAppearance", [this] {
            NSAppearance.currentAppearance = NSApp.effectiveAppearance;
            handleSystemThemeChange();
        });
    }

    m_systemColorObserver = QMacNotificationObserver(nil,
        NSSystemColorsDidChangeNotification, [this] {
            handleSystemThemeChange();
    });

    updateColorScheme();
}

QCocoaTheme::~QCocoaTheme()
{
    reset();
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

    updateColorScheme();

    m_systemPalette = qt_mac_createSystemPalette();
    m_palettes = qt_mac_createRolePalettes();

    if (QCoreTextFontEngine::fontSmoothing() == QCoreTextFontEngine::FontSmoothing::Grayscale) {
        // Re-populate glyph caches based on the new appearance's assumed text fill color
        QFontCache::instance()->clear();
    }

    QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
}

bool QCocoaTheme::usePlatformNativeDialog(DialogType dialogType) const
{
    switch (dialogType) {
    case QPlatformTheme::FileDialog:
    case QPlatformTheme::ColorDialog:
    case QPlatformTheme::FontDialog:
    case QPlatformTheme::MessageDialog:
        return true;
    default:
        return false;
    }
}

QPlatformDialogHelper *QCocoaTheme::createPlatformDialogHelper(DialogType dialogType) const
{
    switch (dialogType) {
    case QPlatformTheme::FileDialog:
        return new QCocoaFileDialogHelper();
    case QPlatformTheme::ColorDialog:
        return new QCocoaColorDialogHelper();
    case QPlatformTheme::FontDialog:
        return new QCocoaFontDialogHelper();
    case QPlatformTheme::MessageDialog:
        return new QCocoaMessageDialog;
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
    const auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    const auto *coreTextFontDatabase = static_cast<QCoreTextFontDatabase *>(platformIntegration->fontDatabase());
    return coreTextFontDatabase->themeFont(type);
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
        QT_IGNORE_DEPRECATIONS(GetIconRef(kOnSystemDisk, kSystemIconsCreator, iconType, &icon));

        if (icon) {
            pixmap = qt_mac_convert_iconref(icon, size.width(), size.height());
            QT_IGNORE_DEPRECATIONS(ReleaseIconRef(icon));
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

    QList<QSize> availableSizes(QIcon::Mode = QIcon::Normal, QIcon::State = QIcon::Off) override
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
        return QStringList(QStringLiteral("macOS"));
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
    case QPlatformTheme::InteractiveResizeAcrossScreens:
        return !NSScreen.screensHaveSeparateSpaces;
    case QPlatformTheme::ShowDirectoriesFirst:
        return false;
    case QPlatformTheme::MouseDoubleClickInterval:
        return NSEvent.doubleClickInterval * 1000;
    case QPlatformTheme::KeyboardInputInterval:
        return NSEvent.keyRepeatDelay * 1000;
    case QPlatformTheme::KeyboardAutoRepeatRate:
        return 1.0 / NSEvent.keyRepeatInterval;
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

Qt::ColorScheme QCocoaTheme::colorScheme() const
{
    return m_colorScheme;
}

/*
    Update the theme's color scheme based on the current appearance.

    We can only reference the appearance on the main thread, but the
    CoreText font engine needs to know the color scheme, and might be
    used from secondary threads, so we cache the color scheme.
*/
void QCocoaTheme::updateColorScheme()
{
    m_colorScheme = qt_mac_applicationIsInDarkMode() ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
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

#ifndef QT_NO_SHORTCUT
QList<QKeySequence> QCocoaTheme::keyBindings(QKeySequence::StandardKey key) const
{
    // The default key bindings in QPlatformTheme all hard-coded to use the Ctrl
    // modifier, to match other platforms. In the normal case, when translating
    // those to key sequences, we'll end up with Qt::ControlModifier+X, which is
    // then matched against incoming key events that have been mapped from the
    // command key to Qt::ControlModifier, and we'll get a match. If, however,
    // the AA_MacDontSwapCtrlAndMeta application attribute is set, we need to
    // fix the resulting key sequence so that it will match against unmapped
    // key events that contain Qt::MetaModifier.
    auto bindings = QPlatformTheme::keyBindings(key);

    if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
        static auto swapCtrlMeta = [](QKeyCombination keyCombination) {
            const auto originalKeyModifiers = keyCombination.keyboardModifiers();
            auto newKeyboardModifiers = originalKeyModifiers & ~(Qt::ControlModifier | Qt::MetaModifier);
            if (originalKeyModifiers & Qt::ControlModifier)
                newKeyboardModifiers |= Qt::MetaModifier;
            if (originalKeyModifiers & Qt::MetaModifier)
                newKeyboardModifiers |= Qt::ControlModifier;
            return QKeyCombination(newKeyboardModifiers, keyCombination.key());
        };

        QList<QKeySequence> swappedBindings;
        for (auto binding : bindings) {
            Q_ASSERT(binding.count() == 1);
            swappedBindings.append(QKeySequence(swapCtrlMeta(binding[0])));
        }

        bindings = swappedBindings;
    }

    return bindings;
}
#endif

QT_END_NAMESPACE
