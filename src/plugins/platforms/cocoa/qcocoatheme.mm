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
#include "messages.h"

#include <QtCore/QVariant>

#include "qcocoasystemsettings.h"
#include "qcocoasystemtrayicon.h"
#include "qcocoamenuitem.h"
#include "qcocoamenu.h"
#include "qcocoamenubar.h"
#include "qcocoahelpers.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qcoregraphics_p.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtextformat.h>
#include <QtFontDatabaseSupport/private/qcoretextfontdatabase_p.h>
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

#include <Carbon/Carbon.h>

@interface QT_MANGLE_NAMESPACE(QCocoaThemeNotificationReceiver) : NSObject {
QCocoaTheme *mPrivate;
}
- (id)initWithPrivate:(QCocoaTheme *)priv;
- (void)systemColorsDidChange:(NSNotification *)notification;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QCocoaThemeNotificationReceiver);

@implementation QCocoaThemeNotificationReceiver
- (id)initWithPrivate:(QCocoaTheme *)priv
{
    self = [super init];
    mPrivate = priv;
    return self;
}

- (void)systemColorsDidChange:(NSNotification *)notification
{
    Q_UNUSED(notification);
    mPrivate->reset();
    QWindowSystemInterface::handleThemeChange(nullptr);
}
@end

QT_BEGIN_NAMESPACE

const char *QCocoaTheme::name = "cocoa";

QCocoaTheme::QCocoaTheme()
    :m_systemPalette(0)
{
    m_notificationReceiver = [[QT_MANGLE_NAMESPACE(QCocoaThemeNotificationReceiver) alloc] initWithPrivate:this];
    [[NSNotificationCenter defaultCenter] addObserver:m_notificationReceiver
                                             selector:@selector(systemColorsDidChange:)
                                                 name:NSSystemColorsDidChangeNotification
                                               object:nil];
}

QCocoaTheme::~QCocoaTheme()
{
    [[NSNotificationCenter defaultCenter] removeObserver:m_notificationReceiver];
    [m_notificationReceiver release];
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

QPlatformDialogHelper * QCocoaTheme::createPlatformDialogHelper(DialogType dialogType) const
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
        return 0;
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
        return m_palettes.value(type, 0);
    }
    return 0;
}

QHash<QPlatformTheme::Font, QFont *> qt_mac_createRoleFonts()
{
    QCoreTextFontDatabase *ctfd = static_cast<QCoreTextFontDatabase *>(QGuiApplicationPrivate::platformIntegration()->fontDatabase());
    return ctfd->themeFonts();
}

const QFont *QCocoaTheme::font(Font type) const
{
    if (m_fonts.isEmpty()) {
        m_fonts = qt_mac_createRoleFonts();
    }
    return m_fonts.value(type, 0);
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
    return button == QPlatformDialogHelper::Discard ? msgDialogButtonDiscard() : QPlatformTheme::standardButtonText(button);
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
