// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiostheme.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/QFont>
#include <QtGui/private/qcoregraphics_p.h>

#include <QtGui/private/qcoretextfontdatabase_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <UIKit/UIFont.h>
#include <UIKit/UIInterface.h>

#ifndef Q_OS_TVOS
#include "qiosmenu.h"
#include "qiosfiledialog.h"
#include "qiosmessagedialog.h"
#include "qioscolordialog.h"
#include "qiosfontdialog.h"
#include "qiosscreen.h"
#endif

QT_BEGIN_NAMESPACE

const char *QIOSTheme::name = "ios";

QIOSTheme::QIOSTheme()
{
    initializeSystemPalette();

    m_contentSizeCategoryObserver = QMacNotificationObserver(nil,
        UIContentSizeCategoryDidChangeNotification, [] {
        qCDebug(lcQpaFonts) << "Contents size category changed to" << UIApplication.sharedApplication.preferredContentSizeCategory;
        QPlatformFontDatabase::repopulateFontDatabase();
    });
}

QIOSTheme::~QIOSTheme()
{
}

QPalette QIOSTheme::s_systemPalette;

void QIOSTheme::initializeSystemPalette()
{
    Q_DECL_IMPORT QPalette qt_fusionPalette(void);
    s_systemPalette = qt_fusionPalette();

    s_systemPalette.setBrush(QPalette::Window, qt_mac_toQBrush(UIColor.systemGroupedBackgroundColor.CGColor));
    s_systemPalette.setBrush(QPalette::Active, QPalette::WindowText, qt_mac_toQBrush(UIColor.labelColor.CGColor));

    s_systemPalette.setBrush(QPalette::Base, qt_mac_toQBrush(UIColor.secondarySystemGroupedBackgroundColor.CGColor));
    s_systemPalette.setBrush(QPalette::Active, QPalette::Text, qt_mac_toQBrush(UIColor.labelColor.CGColor));

    s_systemPalette.setBrush(QPalette::Button, qt_mac_toQBrush(UIColor.secondarySystemBackgroundColor.CGColor));
    s_systemPalette.setBrush(QPalette::Active, QPalette::ButtonText, qt_mac_toQBrush(UIColor.labelColor.CGColor));

    s_systemPalette.setBrush(QPalette::Active, QPalette::BrightText, qt_mac_toQBrush(UIColor.lightTextColor.CGColor));
    s_systemPalette.setBrush(QPalette::Active, QPalette::PlaceholderText, qt_mac_toQBrush(UIColor.placeholderTextColor.CGColor));

    s_systemPalette.setBrush(QPalette::Active, QPalette::Link, qt_mac_toQBrush(UIColor.linkColor.CGColor));
    s_systemPalette.setBrush(QPalette::Active, QPalette::LinkVisited, qt_mac_toQBrush(UIColor.linkColor.CGColor));

    s_systemPalette.setBrush(QPalette::Highlight, QColor(11, 70, 150, 60));
    s_systemPalette.setBrush(QPalette::HighlightedText, qt_mac_toQBrush(UIColor.labelColor.CGColor));

    if (@available(ios 15.0, *))
        s_systemPalette.setBrush(QPalette::Accent, qt_mac_toQBrush(UIColor.tintColor.CGColor));
}

const QPalette *QIOSTheme::palette(QPlatformTheme::Palette type) const
{
    if (type == QPlatformTheme::SystemPalette)
        return &s_systemPalette;
    return 0;
}

QPlatformMenuItem* QIOSTheme::createPlatformMenuItem() const
{
#ifdef Q_OS_TVOS
    return 0;
#else
    return new QIOSMenuItem();
#endif
}

QPlatformMenu* QIOSTheme::createPlatformMenu() const
{
#ifdef Q_OS_TVOS
    return 0;
#else
    return new QIOSMenu();
#endif
}

bool QIOSTheme::usePlatformNativeDialog(QPlatformTheme::DialogType type) const
{
    switch (type) {
    case FileDialog:
    case MessageDialog:
    case ColorDialog:
    case FontDialog:
        return !qt_apple_isApplicationExtension();
    default:
        return false;
    }
}

QPlatformDialogHelper *QIOSTheme::createPlatformDialogHelper(QPlatformTheme::DialogType type) const
{
    switch (type) {
#ifndef Q_OS_TVOS
    case FileDialog:
        return new QIOSFileDialog();
        break;
    case MessageDialog:
        return new QIOSMessageDialog();
        break;
    case ColorDialog:
        return new QIOSColorDialog();
        break;
    case FontDialog:
        return new QIOSFontDialog();
        break;
#endif
    default:
        return 0;
    }
}

QVariant QIOSTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::StyleNames:
        return QStringList(QStringLiteral("Fusion"));
    case KeyboardScheme:
        return QVariant(int(MacKeyboardScheme));
    default:
        return QPlatformTheme::themeHint(hint);
    }
}

Qt::ColorScheme QIOSTheme::colorScheme() const
{
    // Set the appearance based on the QUIWindow
    // Fallback to the UIScreen if no window is created yet
    UIUserInterfaceStyle appearance = UIScreen.mainScreen.traitCollection.userInterfaceStyle;
    NSArray<UIWindow *> *windows = qt_apple_sharedApplication().windows;
    for (UIWindow *window in windows) {
        if ([window isKindOfClass:[QUIWindow class]]) {
            appearance = static_cast<QUIWindow*>(window).traitCollection.userInterfaceStyle;
            break;
        }
    }

    return appearance == UIUserInterfaceStyleDark
                       ? Qt::ColorScheme::Dark
                       : Qt::ColorScheme::Light;
}

const QFont *QIOSTheme::font(Font type) const
{
    const auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    const auto *coreTextFontDatabase = static_cast<QCoreTextFontDatabase *>(platformIntegration->fontDatabase());
    return coreTextFontDatabase->themeFont(type);
}

QT_END_NAMESPACE
