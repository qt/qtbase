/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#include <Cocoa/Cocoa.h>

#include "qapplication.h"
#include "qbitarray.h"
#include "qclipboard.h"
#include "qcursor.h"
#include "qdatastream.h"
#include "qdatetime.h"
#include "qdesktopwidget.h"
#include "qdockwidget.h"
#include "qevent.h"
#include "qhash.h"
#include "qlayout.h"
#include "qmenubar.h"
#include "qmessagebox.h"
#include "qmime.h"
#include "qpixmapcache.h"
#include "qpointer.h"
#include "qsessionmanager.h"
#include "qsettings.h"
#include "qsocketnotifier.h"
#include "qstyle.h"
#include "qstylefactory.h"
#include "qtextcodec.h"
#include "qtoolbar.h"
#include "qvariant.h"
#include "qwidget.h"
#include "qcolormap.h"
#include "qdir.h"
#include "qdebug.h"
#include "qtimer.h"
#include "qurl.h"
#include "private/qmacinputcontext_p.h"
#include "private/qpaintengine_mac_p.h"
#include "private/qcursor_p.h"
#include "private/qapplication_p.h"
#include "private/qcolor_p.h"
#include "private/qwidget_p.h"
#include "private/qkeymapper_p.h"
#include "private/qeventdispatcher_mac_p.h"
#include "private/qeventdispatcher_unix_p.h"
#include <private/qcocoamenuloader_mac_p.h>
#include <private/qcocoaapplication_mac_p.h>
#include <private/qcocoaapplicationdelegate_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qcocoawindow_mac_p.h>
#include <private/qpixmap_mac_p.h>
#include <private/qdesktopwidget_mac_p.h>
#include <private/qeventdispatcher_mac_p.h>
#include <qvarlengtharray.h>

#ifndef QT_NO_ACCESSIBILITY
#  include "qaccessible.h"
#endif

#ifndef QT_NO_THREAD
#  include "qmutex.h"
#endif

#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

/*****************************************************************************
  QApplication debug facilities
 *****************************************************************************/
//#define DEBUG_EVENTS //like EventDebug but more specific to Qt
//#define DEBUG_DROPPED_EVENTS
//#define DEBUG_MOUSE_MAPS
//#define DEBUG_MODAL_EVENTS
//#define DEBUG_PLATFORM_SETTINGS

#define QMAC_SPEAK_TO_ME
#ifdef QMAC_SPEAK_TO_ME
#include "qregexp.h"
#endif

#ifndef kThemeBrushAlternatePrimaryHighlightColor
#define kThemeBrushAlternatePrimaryHighlightColor -5
#endif

#define kCMDeviceUnregisteredNotification CFSTR("CMDeviceUnregisteredNotification")
#define kCMDefaultDeviceNotification CFSTR("CMDefaultDeviceNotification")
#define kCMDeviceProfilesNotification CFSTR("CMDeviceProfilesNotification")
#define kCMDefaultDeviceProfileNotification CFSTR("CMDefaultDeviceProfileNotification")

QT_BEGIN_NAMESPACE

//for qt_mac.h
QPaintDevice *qt_mac_safe_pdev = 0;
QList<QMacWindowChangeEvent*> *QMacWindowChangeEvent::change_events = 0;
QPointer<QWidget> topLevelAt_cache = 0;

/*****************************************************************************
  Internal variables and functions
 *****************************************************************************/
static struct {
    bool use_qt_time_limit;
    QPointer<QWidget> last_widget;
    int last_x, last_y;
    int last_modifiers, last_button;
    EventTime last_time;
} qt_mac_dblclick = { false, 0, -1, -1, 0, 0, -2 };

static bool app_do_modal = false;       // modal mode
extern QWidgetList *qt_modal_stack;     // stack of modal widgets
extern bool qt_tab_all_widgets;         // from qapplication.cpp
bool qt_scrollbar_jump_to_pos = false;
static bool qt_mac_collapse_on_dblclick = true;
extern int qt_antialiasing_threshold; // from qapplication.cpp
QWidget * qt_button_down;                // widget got last button-down
QPointer<QWidget> qt_last_mouse_receiver;
#if defined(QT_DEBUG)
static bool        appNoGrab        = false;        // mouse/keyboard grabbing
#endif
static AEEventHandlerUPP app_proc_ae_handlerUPP = NULL;
static EventHandlerRef tablet_proximity_handler = 0;
static EventHandlerUPP tablet_proximity_UPP = 0;
bool QApplicationPrivate::native_modal_dialog_active;

Q_GUI_EXPORT bool qt_applefontsmoothing_enabled;

/*****************************************************************************
  External functions
 *****************************************************************************/
extern void qt_mac_beep(); //qsound_mac.mm
extern Qt::KeyboardModifiers qt_mac_get_modifiers(int keys); //qkeymapper_mac.cpp
extern bool qt_mac_can_clickThrough(const QWidget *); //qwidget_mac.cpp
extern bool qt_mac_is_macdrawer(const QWidget *); //qwidget_mac.cpp
extern OSWindowRef qt_mac_window_for(const QWidget*); //qwidget_mac.cpp
extern QWidget *qt_mac_find_window(OSWindowRef); //qwidget_mac.cpp
extern void qt_mac_set_cursor(const QCursor *); //qcursor_mac.cpp
extern bool qt_mac_is_macsheet(const QWidget *); //qwidget_mac.cpp
extern QString qt_mac_from_pascal_string(const Str255); //qglobal.cpp
extern void qt_mac_command_set_enabled(MenuRef, UInt32, bool); //qmenu_mac.cpp
extern bool qt_sendSpontaneousEvent(QObject *obj, QEvent *event); // qapplication.cpp
extern void qt_mac_update_cursor(); // qcursor_mac.mm

// Forward Decls
void onApplicationWindowChangedActivation( QWidget*widget, bool activated );
void onApplicationChangedActivation( bool activated );

static void qt_mac_read_fontsmoothing_settings()
{
    qt_applefontsmoothing_enabled = true;
    int w = 10, h = 10;
    QImage image(w, h, QImage::Format_RGB32);
    image.fill(0xffffffff);
    QPainter p(&image);
    p.drawText(0, h, "X\\");
    p.end();

    const int *bits = (const int *) ((const QImage &) image).bits();
    int bpl = image.bytesPerLine() / 4;
    for (int y=0; y<w; ++y) {
        for (int x=0; x<h; ++x) {
            int r = qRed(bits[x]);
            int g = qGreen(bits[x]);
            int b = qBlue(bits[x]);
            if (r != g || r != b) {
                qt_applefontsmoothing_enabled = true;
                return;
            }
        }
        bits += bpl;
    }
    qt_applefontsmoothing_enabled = false;
}

Q_GUI_EXPORT bool qt_mac_execute_apple_script(const char *script, long script_len, AEDesc *ret) {
    OSStatus err;
    AEDesc scriptTextDesc;
    ComponentInstance theComponent = 0;
    OSAID scriptID = kOSANullScript, resultID = kOSANullScript;

    // set up locals to a known state
    AECreateDesc(typeNull, 0, 0, &scriptTextDesc);
    scriptID = kOSANullScript;
    resultID = kOSANullScript;

    // open the scripting component
    theComponent = OpenDefaultComponent(kOSAComponentType, typeAppleScript);
    if (!theComponent) {
        err = paramErr;
        goto bail;
    }

    // put the script text into an aedesc
    err = AECreateDesc(typeUTF8Text, script, script_len, &scriptTextDesc);
    if (err != noErr)
        goto bail;

    // compile the script
    err = OSACompile(theComponent, &scriptTextDesc, kOSAModeNull, &scriptID);
    if (err != noErr)
        goto bail;

    // run the script
    err = OSAExecute(theComponent, scriptID, kOSANullScript, kOSAModeNull, &resultID);

    // collect the results - if any
    if (ret) {
        AECreateDesc(typeNull, 0, 0, ret);
        if (err == errOSAScriptError)
            OSAScriptError(theComponent, kOSAErrorMessage, typeChar, ret);
        else if (err == noErr && resultID != kOSANullScript)
            OSADisplay(theComponent, resultID, typeChar, kOSAModeNull, ret);
    }
bail:
    AEDisposeDesc(&scriptTextDesc);
    if (scriptID != kOSANullScript)
        OSADispose(theComponent, scriptID);
    if (resultID != kOSANullScript)
        OSADispose(theComponent, resultID);
    if (theComponent)
        CloseComponent(theComponent);
    return err == noErr;
}

Q_GUI_EXPORT bool qt_mac_execute_apple_script(const char *script, AEDesc *ret)
{
    return qt_mac_execute_apple_script(script, qstrlen(script), ret);
}

Q_GUI_EXPORT bool qt_mac_execute_apple_script(const QString &script, AEDesc *ret)
{
    const QByteArray l = script.toUtf8(); return qt_mac_execute_apple_script(l.constData(), l.size(), ret);
}

/* Resolution change magic */
void qt_mac_display_change_callbk(CGDirectDisplayID, CGDisplayChangeSummaryFlags flags, void *)
{
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    const bool resized = flags & kCGDisplayDesktopShapeChangedFlag;
#else
    Q_UNUSED(flags);
    const bool resized = true;
#endif
    if (resized && qApp) {
        if (QDesktopWidget *dw = qApp->desktop()) {
            QResizeEvent *re = new QResizeEvent(dw->size(), dw->size());
            QApplication::postEvent(dw, re);
            QCoreGraphicsPaintEngine::cleanUpMacColorSpaces();
        }
    }
}

#ifdef DEBUG_PLATFORM_SETTINGS
static void qt_mac_debug_palette(const QPalette &pal, const QPalette &pal2, const QString &where)
{
    const char *const groups[] = {"Active", "Disabled", "Inactive" };
    const char *const roles[] = { "WindowText", "Button", "Light", "Midlight", "Dark", "Mid",
                            "Text", "BrightText", "ButtonText", "Base", "Window", "Shadow",
                            "Highlight", "HighlightedText", "Link", "LinkVisited" };
    if (!where.isNull())
        qDebug("qt-internal: %s", where.toLatin1().constData());
    for(int grp = 0; grp < QPalette::NColorGroups; grp++) {
        for(int role = 0; role < QPalette::NColorRoles; role++) {
            QBrush b = pal.brush((QPalette::ColorGroup)grp, (QPalette::ColorRole)role);
            QPixmap pm = b.texture();
            qDebug("  %s::%s %d::%d::%d [%p]%s", groups[grp], roles[role], b.color().red(),
                   b.color().green(), b.color().blue(), pm.isNull() ? 0 : &pm,
                   pal2.brush((QPalette::ColorGroup)grp, (QPalette::ColorRole)role) != b ? " (*)" : "");
        }
    }

}
#else
#define qt_mac_debug_palette(x, y, z)
#endif

//raise a notification
void qt_mac_send_notification()
{
    QMacCocoaAutoReleasePool pool;
    [[NSApplication sharedApplication] requestUserAttention:NSInformationalRequest];
}

void qt_mac_cancel_notification()
{
    QMacCocoaAutoReleasePool pool;
    [[NSApplication sharedApplication] cancelUserAttentionRequest:NSInformationalRequest];
}

void qt_mac_set_app_icon(const QPixmap &pixmap)
{
    QMacCocoaAutoReleasePool pool;
    NSImage *image = NULL;
    if (pixmap.isNull()) {
        // Get Application icon from bundle
        image = [[NSImage imageNamed:@"NSApplicationIcon"] retain]; // released below
    } else {
        image = static_cast<NSImage *>(qt_mac_create_nsimage(pixmap));
    }

    [NSApp setApplicationIconImage:image];
    [image release];
}

Q_GUI_EXPORT void qt_mac_set_press_and_hold_context(bool b)
{
    Q_UNUSED(b);
    qWarning("qt_mac_set_press_and_hold_context: This functionality is no longer available");
}

bool qt_nograb()                                // application no-grab option
{
#if defined(QT_DEBUG)
    return appNoGrab;
#else
    return false;
#endif
}

void qt_mac_update_os_settings()
{
    if (!qApp)
        return;
    if (!QApplication::startingUp()) {
        static bool needToPolish = true;
        if (needToPolish) {
            QApplication::style()->polish(qApp);
            needToPolish = false;
        }
    }
    //focus mode
    /* First worked as of 10.2.3 */
    QSettings appleSettings(QLatin1String("apple.com"));
    QVariant appleValue = appleSettings.value(QLatin1String("AppleKeyboardUIMode"), 0);
    qt_tab_all_widgets = (appleValue.toInt() & 0x2);
    //paging mode
    /* First worked as of 10.2.3 */
    appleValue = appleSettings.value(QLatin1String("AppleScrollerPagingBehavior"), false);
    qt_scrollbar_jump_to_pos = appleValue.toBool();
    //collapse
    /* First worked as of 10.3.3 */
    appleValue = appleSettings.value(QLatin1String("AppleMiniaturizeOnDoubleClick"), true);
    qt_mac_collapse_on_dblclick = appleValue.toBool();

    // Anti-aliasing threshold
    appleValue = appleSettings.value(QLatin1String("AppleAntiAliasingThreshold"));
    if (appleValue.isValid())
        qt_antialiasing_threshold = appleValue.toInt();

#ifdef DEBUG_PLATFORM_SETTINGS
    qDebug("qt_mac_update_os_settings *********************************************************************");
#endif
    { // setup the global palette
        QColor qc;
        (void) QApplication::style();  // trigger creation of application style and system palettes
        QPalette pal = *QApplicationPrivate::sys_pal;

        pal.setBrush( QPalette::Active, QPalette::Highlight, qcolorForTheme(kThemeBrushPrimaryHighlightColor) );
        pal.setBrush( QPalette::Inactive, QPalette::Highlight, qcolorForTheme(kThemeBrushSecondaryHighlightColor) );

        pal.setBrush( QPalette::Disabled, QPalette::Highlight, qcolorForTheme(kThemeBrushSecondaryHighlightColor) );
        pal.setBrush( QPalette::Active, QPalette::Shadow, qcolorForTheme(kThemeBrushButtonActiveDarkShadow) );

        pal.setBrush( QPalette::Inactive, QPalette::Shadow, qcolorForTheme(kThemeBrushButtonInactiveDarkShadow) );
        pal.setBrush( QPalette::Disabled, QPalette::Shadow, qcolorForTheme(kThemeBrushButtonInactiveDarkShadow) );

        qc = qcolorForThemeTextColor(kThemeTextColorDialogActive);
        pal.setColor(QPalette::Active, QPalette::Text, qc);
        pal.setColor(QPalette::Active, QPalette::WindowText, qc);
        pal.setColor(QPalette::Active, QPalette::HighlightedText, qc);

        qc = qcolorForThemeTextColor(kThemeTextColorDialogInactive);
        pal.setColor(QPalette::Inactive, QPalette::Text, qc);
        pal.setColor(QPalette::Inactive, QPalette::WindowText, qc);
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText, qc);
        pal.setColor(QPalette::Disabled, QPalette::Text, qc);
        pal.setColor(QPalette::Disabled, QPalette::WindowText, qc);
        pal.setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
        pal.setBrush(QPalette::ToolTipBase, QColor(255, 255, 199));

        if (!QApplicationPrivate::sys_pal || *QApplicationPrivate::sys_pal != pal) {
            QApplicationPrivate::setSystemPalette(pal);
            QApplication::setPalette(pal);
        }
#ifdef DEBUG_PLATFORM_SETTINGS
        qt_mac_debug_palette(pal, QApplication::palette(), "Global Palette");
#endif
    }

    QFont fnt = qfontForThemeFont(kThemeApplicationFont);
#ifdef DEBUG_PLATFORM_SETTINGS
    qDebug("qt-internal: Font for Application [%s::%d::%d::%d]",
           fnt.family().toLatin1().constData(), fnt.pointSize(), fnt.bold(), fnt.italic());
#endif
    if (!QApplicationPrivate::sys_font || *QApplicationPrivate::sys_font != fnt)
        QApplicationPrivate::setSystemFont(fnt);

    { //setup the fonts
        struct FontMap {
            FontMap(const char *qc, short fk) : qt_class(qc), font_key(fk) { }
            const char *const qt_class;
            short font_key;
        } mac_widget_fonts[] = {
            FontMap("QPushButton", kThemePushButtonFont),
            FontMap("QListView", kThemeViewsFont),
            FontMap("QListBox", kThemeViewsFont),
            FontMap("QTitleBar", kThemeWindowTitleFont),
            FontMap("QMenuBar", kThemeMenuTitleFont),
            FontMap("QMenu", kThemeMenuItemFont),
            FontMap("QComboMenuItem", kThemeSystemFont),
            FontMap("QHeaderView", kThemeSmallSystemFont),
            FontMap("Q3Header", kThemeSmallSystemFont),
            FontMap("QTipLabel", kThemeSmallSystemFont),
            FontMap("QLabel", kThemeSystemFont),
            FontMap("QToolButton", kThemeSmallSystemFont),
            FontMap("QMenuItem", kThemeMenuItemFont),  // It doesn't exist, but its unique.
            FontMap("QComboLineEdit", kThemeViewsFont),  // It doesn't exist, but its unique.
            FontMap("QSmallFont", kThemeSmallSystemFont),  // It doesn't exist, but its unique.
            FontMap("QMiniFont", kThemeMiniSystemFont),  // It doesn't exist, but its unique.
            FontMap(0, 0) };
        for(int i = 0; mac_widget_fonts[i].qt_class; i++) {
            QFont fnt = qfontForThemeFont(mac_widget_fonts[i].font_key);
            bool set_font = true;
            FontHash *hash = qt_app_fonts_hash();
            if (!hash->isEmpty()) {
                FontHash::const_iterator it
                                        = hash->constFind(mac_widget_fonts[i].qt_class);
                if (it != hash->constEnd())
                    set_font = (fnt != *it);
            }
            if (set_font) {
                QApplication::setFont(fnt, mac_widget_fonts[i].qt_class);
#ifdef DEBUG_PLATFORM_SETTINGS
                qDebug("qt-internal: Font for %s [%s::%d::%d::%d]", mac_widget_fonts[i].qt_class,
                       fnt.family().toLatin1().constData(), fnt.pointSize(), fnt.bold(), fnt.italic());
#endif
            }
        }
    }
    QApplicationPrivate::initializeWidgetPaletteHash();
#ifdef DEBUG_PLATFORM_SETTINGS
    qDebug("qt_mac_update_os_settings END !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
#endif
}

void QApplicationPrivate::initializeWidgetPaletteHash()
{
    { //setup the palette
        struct PaletteMap {
            inline PaletteMap(const char *qc, ThemeBrush a, ThemeBrush i) :
                qt_class(qc), active(a), inactive(i) { }
            const char *const qt_class;
            ThemeBrush active, inactive;
        } mac_widget_colors[] = {
            PaletteMap("QToolButton", kThemeTextColorBevelButtonActive, kThemeTextColorBevelButtonInactive),
            PaletteMap("QAbstractButton", kThemeTextColorPushButtonActive, kThemeTextColorPushButtonInactive),
            PaletteMap("QHeaderView", kThemeTextColorPushButtonActive, kThemeTextColorPushButtonInactive),
            PaletteMap("Q3Header", kThemeTextColorPushButtonActive, kThemeTextColorPushButtonInactive),
            PaletteMap("QComboBox", kThemeTextColorPopupButtonActive, kThemeTextColorPopupButtonInactive),
            PaletteMap("QAbstractItemView", kThemeTextColorListView, kThemeTextColorDialogInactive),
            PaletteMap("QMessageBoxLabel", kThemeTextColorAlertActive, kThemeTextColorAlertInactive),
            PaletteMap("QTabBar", kThemeTextColorTabFrontActive, kThemeTextColorTabFrontInactive),
            PaletteMap("QLabel", kThemeTextColorPlacardActive, kThemeTextColorPlacardInactive),
            PaletteMap("QGroupBox", kThemeTextColorPlacardActive, kThemeTextColorPlacardInactive),
            PaletteMap("QMenu", kThemeTextColorPopupLabelActive, kThemeTextColorPopupLabelInactive),
            PaletteMap("QTextEdit", 0, 0),
            PaletteMap("QTextControl", 0, 0),
            PaletteMap("QLineEdit", 0, 0),
            PaletteMap(0, 0, 0) };
        QColor qc;
        for(int i = 0; mac_widget_colors[i].qt_class; i++) {
            QPalette pal;
            if (mac_widget_colors[i].active != 0) {
                qc = qcolorForThemeTextColor(mac_widget_colors[i].active);
                pal.setColor(QPalette::Active, QPalette::Text, qc);
                pal.setColor(QPalette::Active, QPalette::WindowText, qc);
                pal.setColor(QPalette::Active, QPalette::HighlightedText, qc);
                qc = qcolorForThemeTextColor(mac_widget_colors[i].inactive);
                pal.setColor(QPalette::Inactive, QPalette::Text, qc);
                pal.setColor(QPalette::Disabled, QPalette::Text, qc);
                pal.setColor(QPalette::Inactive, QPalette::WindowText, qc);
                pal.setColor(QPalette::Disabled, QPalette::WindowText, qc);
                pal.setColor(QPalette::Inactive, QPalette::HighlightedText, qc);
                pal.setColor(QPalette::Disabled, QPalette::HighlightedText, qc);
            }
            if (!strcmp(mac_widget_colors[i].qt_class, "QMenu")) {
                qc = qcolorForThemeTextColor(kThemeTextColorMenuItemActive);
                pal.setBrush(QPalette::ButtonText, qc);
                qc = qcolorForThemeTextColor(kThemeTextColorMenuItemSelected);
                pal.setBrush(QPalette::HighlightedText, qc);
                qc = qcolorForThemeTextColor(kThemeTextColorMenuItemDisabled);
                pal.setBrush(QPalette::Disabled, QPalette::Text, qc);
            } else if (!strcmp(mac_widget_colors[i].qt_class, "QAbstractButton")
                      || !strcmp(mac_widget_colors[i].qt_class, "QHeaderView")
                      || !strcmp(mac_widget_colors[i].qt_class, "Q3Header")) { //special
                pal.setColor(QPalette::Disabled, QPalette::ButtonText,
                             pal.color(QPalette::Disabled, QPalette::Text));
                pal.setColor(QPalette::Inactive, QPalette::ButtonText,
                             pal.color(QPalette::Inactive, QPalette::Text));
                pal.setColor(QPalette::Active, QPalette::ButtonText,
                             pal.color(QPalette::Active, QPalette::Text));
            } else if (!strcmp(mac_widget_colors[i].qt_class, "QAbstractItemView")) {
                pal.setBrush(QPalette::Active, QPalette::Highlight,
                             qcolorForTheme(kThemeBrushAlternatePrimaryHighlightColor));
                qc = qcolorForThemeTextColor(kThemeTextColorMenuItemSelected);
                pal.setBrush(QPalette::Active, QPalette::HighlightedText, qc);
#if 1
                pal.setBrush(QPalette::Inactive, QPalette::Text,
                              pal.brush(QPalette::Active, QPalette::Text));
                pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                              pal.brush(QPalette::Active, QPalette::Text));
#endif
            } else if (!strcmp(mac_widget_colors[i].qt_class, "QTextEdit")
                       || !strcmp(mac_widget_colors[i].qt_class, "QTextControl")) {
                pal.setBrush(QPalette::Inactive, QPalette::Text,
                              pal.brush(QPalette::Active, QPalette::Text));
                pal.setBrush(QPalette::Inactive, QPalette::HighlightedText,
                              pal.brush(QPalette::Active, QPalette::Text));
            } else if (!strcmp(mac_widget_colors[i].qt_class, "QLineEdit")) {
                pal.setBrush(QPalette::Disabled, QPalette::Base,
                             pal.brush(QPalette::Active, QPalette::Base));
            }

            bool set_palette = true;
            PaletteHash *phash = qt_app_palettes_hash();
            if (!phash->isEmpty()) {
                PaletteHash::const_iterator it
                                    = phash->constFind(mac_widget_colors[i].qt_class);
                if (it != phash->constEnd())
                    set_palette = (pal != *it);
            }
            if (set_palette) {
                QApplication::setPalette(pal, mac_widget_colors[i].qt_class);
#ifdef DEBUG_PLATFORM_SETTINGS
                qt_mac_debug_palette(pal, QApplication::palette(), QLatin1String("Palette for ") + QString::fromLatin1(mac_widget_colors[i].qt_class));
#endif
            }
        }
    }
}

static void qt_mac_event_release(EventRef &event)
{
    ReleaseEvent(event);
    event = 0;
}

/* sheets */
void qt_event_request_showsheet(QWidget *w)
{
    Q_ASSERT(qt_mac_is_macsheet(w));
    [NSApp beginSheet:qt_mac_window_for(w) modalForWindow:qt_mac_window_for(w->parentWidget())
        modalDelegate:nil didEndSelector:nil contextInfo:0];
}

static void qt_post_window_change_event(QWidget *widget)
{
    qt_widget_private(widget)->needWindowChange = true;
    QEvent *glWindowChangeEvent = new QEvent(QEvent::MacGLWindowChange);
    QApplication::postEvent(widget, glWindowChangeEvent);
}

/*
    Posts updates to all child and grandchild OpenGL widgets for the given widget.
*/
static void qt_mac_update_child_gl_widgets(QWidget *widget)
{
    // Update all OpenGL child widgets for the given widget.
    QList<QWidgetPrivate::GlWidgetInfo> &glWidgets = qt_widget_private(widget)->glWidgets;
    QList<QWidgetPrivate::GlWidgetInfo>::iterator end = glWidgets.end();
    QList<QWidgetPrivate::GlWidgetInfo>::iterator it = glWidgets.begin();

    for (;it != end; ++it) {
        qt_post_window_change_event(it->widget);
    }
}

/*
    Sends updates to all child and grandchild gl widgets that have updates pending.
*/
void qt_mac_send_posted_gl_updates(QWidget *widget)
{
    QList<QWidgetPrivate::GlWidgetInfo> &glWidgets = qt_widget_private(widget)->glWidgets;
    QList<QWidgetPrivate::GlWidgetInfo>::iterator end = glWidgets.end();
    QList<QWidgetPrivate::GlWidgetInfo>::iterator it = glWidgets.begin();

    for (;it != end; ++it) {
        QWidget *glWidget = it->widget;
        if (qt_widget_private(glWidget)->needWindowChange) {
            QEvent glChangeEvent(QEvent::MacGLWindowChange);
            QApplication::sendEvent(glWidget, &glChangeEvent);
        }
    }
}

/*
    Posts updates to all OpenGL widgets within the window that the given widget intersects.
*/
static void qt_mac_update_intersected_gl_widgets(QWidget *widget)
{
    Q_UNUSED(widget);
}

/*
    Posts a kEventQtRequestWindowChange event to the main Carbon event queue.
*/
static EventRef request_window_change_pending = 0;
Q_GUI_EXPORT void qt_event_request_window_change()
{
    if(request_window_change_pending)
        return;

    CreateEvent(0, kEventClassQt, kEventQtRequestWindowChange, GetCurrentEventTime(),
                kEventAttributeUserEvent, &request_window_change_pending);
    PostEventToQueue(GetMainEventQueue(), request_window_change_pending, kEventPriorityHigh);
}

/* window changing. This is a hack around Apple's missing functionality, pending the toolbox
   team fix. --Sam */
Q_GUI_EXPORT void qt_event_request_window_change(QWidget *widget)
{
    if (!widget)
        return;

    // Post a kEventQtRequestWindowChange event. This event is semi-public,
    // don't remove this line!
    qt_event_request_window_change();

    // Post update request on gl widgets unconditionally.
    if (qt_widget_private(widget)->isGLWidget == true) {
        qt_post_window_change_event(widget);
        return;
    }

    qt_mac_update_child_gl_widgets(widget);
    qt_mac_update_intersected_gl_widgets(widget);
}

/* activation */
static struct {
    QPointer<QWidget> widget;
    EventRef event;
    EventLoopTimerRef timer;
    EventLoopTimerUPP timerUPP;
} request_activate_pending = { 0, 0, 0, 0 };
bool qt_event_remove_activate()
{
    if (request_activate_pending.timer) {
        RemoveEventLoopTimer(request_activate_pending.timer);
        request_activate_pending.timer = 0;
    }
    if (request_activate_pending.event)
        qt_mac_event_release(request_activate_pending.event);
    return true;
}

void qt_event_activate_timer_callbk(EventLoopTimerRef r, void *)
{
    EventLoopTimerRef otc = request_activate_pending.timer;
    qt_event_remove_activate();
    if (r == otc && !request_activate_pending.widget.isNull()) {
        const QWidget *tlw = request_activate_pending.widget->window();
        Qt::WindowType wt = tlw->windowType();
        if (tlw->isVisible()
               && ((wt != Qt::Desktop && wt != Qt::Popup && wt != Qt::Tool) || tlw->isModal())) {
            CreateEvent(0, kEventClassQt, kEventQtRequestActivate, GetCurrentEventTime(),
                        kEventAttributeUserEvent, &request_activate_pending.event);
            PostEventToQueue(GetMainEventQueue(), request_activate_pending.event, kEventPriorityHigh);
        }
    }
}

void qt_event_request_activate(QWidget *w)
{
    if (w == request_activate_pending.widget)
        return;

    /* We put these into a timer because due to order of events being sent we need to be sure this
       comes from inside of the event loop */
    qt_event_remove_activate();
    if (!request_activate_pending.timerUPP)
        request_activate_pending.timerUPP = NewEventLoopTimerUPP(qt_event_activate_timer_callbk);
    request_activate_pending.widget = w;
    InstallEventLoopTimer(GetMainEventLoop(), 0, 0, request_activate_pending.timerUPP, 0, &request_activate_pending.timer);
}


/* menubars */
void qt_event_request_menubarupdate()
{
    // Just call this. The request has the benefit that we don't call this multiple times, but
    // we can optimize this.
    QMenuBar::macUpdateMenuBar();
}


void QApplicationPrivate::createEventDispatcher()
{
    Q_Q(QApplication);
    if (q->type() != QApplication::Tty)
        eventDispatcher = new QEventDispatcherMac(q);
    else
        eventDispatcher = new QEventDispatcherUNIX(q);
}

/* clipboard */
void qt_event_send_clipboard_changed()
{
}

/* app menu */
static QMenu *qt_mac_dock_menu = 0;
Q_GUI_EXPORT void qt_mac_set_dock_menu(QMenu *menu)
{
    qt_mac_dock_menu = menu;
    [NSApp setDockMenu:menu->macMenu()];
}

/* events that hold pointers to widgets, must be cleaned up like this */
void qt_mac_event_release(QWidget *w)
{
    if (w) {
        if (w == qt_mac_dock_menu) {
            qt_mac_dock_menu = 0;
            [NSApp setDockMenu:0];
        }
    }
}

struct QMacAppleEventTypeSpec {
    AEEventClass mac_class;
    AEEventID mac_id;
} app_apple_events[] = {
    { kCoreEventClass, kAEQuitApplication },
    { kCoreEventClass, kAEOpenDocuments },
    { kInternetEventClass, kAEGetURL },
};

static void qt_init_tablet_proximity_handler()
{
    EventTypeSpec	tabletProximityEvent = { kEventClassTablet, kEventTabletProximity };
    InstallEventHandler(GetEventMonitorTarget(), tablet_proximity_UPP,
                        1, &tabletProximityEvent, qApp, &tablet_proximity_handler);
}

static void qt_release_tablet_proximity_handler()
{
    RemoveEventHandler(tablet_proximity_handler);
}

QString QApplicationPrivate::appName() const
{
    static QString applName;
    if (applName.isEmpty()) {
        applName = QCoreApplicationPrivate::macMenuBarName();
        ProcessSerialNumber psn;
        if (applName.isEmpty() && qt_is_gui_used && GetCurrentProcess(&psn) == noErr) {
            QCFString cfstr;
            CopyProcessName(&psn, &cfstr);
            applName = cfstr;
        }
    }
    return applName;
}

void qt_release_app_proc_handler()
{
}

void qt_color_profile_changed(CFNotificationCenterRef, void *, CFStringRef, const void *,
                              CFDictionaryRef)
{
    QCoreGraphicsPaintEngine::cleanUpMacColorSpaces();
}
/* platform specific implementations */
void qt_init(QApplicationPrivate *priv, int)
{
    if (qt_is_gui_used) {
        CGDisplayRegisterReconfigurationCallback(qt_mac_display_change_callbk, 0);
        CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
        CFNotificationCenterAddObserver(center, qApp, qt_color_profile_changed,
                                        kCMDeviceUnregisteredNotification, 0,
                                        CFNotificationSuspensionBehaviorDeliverImmediately);
        CFNotificationCenterAddObserver(center, qApp, qt_color_profile_changed,
                                        kCMDefaultDeviceNotification, 0,
                                        CFNotificationSuspensionBehaviorDeliverImmediately);
        CFNotificationCenterAddObserver(center, qApp, qt_color_profile_changed,
                                        kCMDeviceProfilesNotification, 0,
                                        CFNotificationSuspensionBehaviorDeliverImmediately);
        CFNotificationCenterAddObserver(center, qApp, qt_color_profile_changed,
                                        kCMDefaultDeviceProfileNotification, 0,
                                        CFNotificationSuspensionBehaviorDeliverImmediately);
        ProcessSerialNumber psn;
        if (GetCurrentProcess(&psn) == noErr) {
            // Jambi needs to transform itself since most people aren't "used"
            // to putting things in bundles, but other people may actually not
            // want to tranform the process (running as a helper or something)
            // so don't do that for them. This means checking both LSUIElement
            // and LSBackgroundOnly. If you set them both... well, you
            // shouldn't do that.

            bool forceTransform = true;
            CFTypeRef value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                                   CFSTR("LSUIElement"));
            if (value) {
                CFTypeID valueType = CFGetTypeID(value);
                // Officially it's supposed to be a string, a boolean makes sense, so we'll check.
                // A number less so, but OK.
                if (valueType == CFStringGetTypeID())
                    forceTransform = !(QCFString::toQString(static_cast<CFStringRef>(value)).toInt());
                else if (valueType == CFBooleanGetTypeID())
                    forceTransform = !CFBooleanGetValue(static_cast<CFBooleanRef>(value));
                else if (valueType == CFNumberGetTypeID()) {
                    int valueAsInt;
                    CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &valueAsInt);
                    forceTransform = !valueAsInt;
                }
            }

            if (forceTransform) {
                value = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                             CFSTR("LSBackgroundOnly"));
                if (value) {
                    CFTypeID valueType = CFGetTypeID(value);
                    if (valueType == CFBooleanGetTypeID())
                        forceTransform = !CFBooleanGetValue(static_cast<CFBooleanRef>(value));
                    else if (valueType == CFStringGetTypeID())
                        forceTransform = !(QCFString::toQString(static_cast<CFStringRef>(value)).toInt());
                    else if (valueType == CFNumberGetTypeID()) {
                        int valueAsInt;
                        CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &valueAsInt);
                        forceTransform = !valueAsInt;
                    }
                }
            }


            if (forceTransform) {
                TransformProcessType(&psn, kProcessTransformToForegroundApplication);
            }
        }
    }

    char **argv = priv->argv;

    // Get command line params
    if (int argc = priv->argc) {
        int i, j = 1;
        QString passed_psn;
        for(i=1; i < argc; i++) {
            if (argv[i] && *argv[i] != '-') {
                argv[j++] = argv[i];
                continue;
            }
            QByteArray arg(argv[i]);
#if defined(QT_DEBUG)
            if (arg == "-nograb")
                appNoGrab = !appNoGrab;
            else
#endif // QT_DEBUG
                if (arg.left(5) == "-psn_") {
                    passed_psn = QString::fromLatin1(arg.mid(6));
                } else {
                    argv[j++] = argv[i];
                }
        }
        if (j < priv->argc) {
            priv->argv[j] = 0;
            priv->argc = j;
        }

        //special hack to change working directory (for an app bundle) when running from finder
        if (!passed_psn.isNull() && QDir::currentPath() == QLatin1String("/")) {
            QCFType<CFURLRef> bundleURL(CFBundleCopyBundleURL(CFBundleGetMainBundle()));
            QString qbundlePath = QCFString(CFURLCopyFileSystemPath(bundleURL,
                                            kCFURLPOSIXPathStyle));
            if (qbundlePath.endsWith(QLatin1String(".app")))
                QDir::setCurrent(qbundlePath.section(QLatin1Char('/'), 0, -2));
        }
   }

    QMacPasteboardMime::initialize();

    qApp->setObjectName(priv->appName());
    if (qt_is_gui_used) {
        QColormap::initialize();
        QFont::initialize();
        QCursorData::initialize();
        QCoreGraphicsPaintEngine::initialize();
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::initialize();
#endif
        QMacInputContext::initialize();
        QApplicationPrivate::inputContext = new QMacInputContext;

        if (QApplication::desktopSettingsAware())
            qt_mac_update_os_settings();
        if (!app_proc_ae_handlerUPP && !QApplication::testAttribute(Qt::AA_MacPluginApplication)) {
            app_proc_ae_handlerUPP = AEEventHandlerUPP(QApplicationPrivate::globalAppleEventProcessor);
            for(uint i = 0; i < sizeof(app_apple_events) / sizeof(QMacAppleEventTypeSpec); ++i) {
                // Install apple event handler, but avoid overwriting an already
                // existing handler (it means a 3rd party application has installed one):
                SRefCon refCon = 0;
                AEEventHandlerUPP current_handler = NULL;
                AEGetEventHandler(app_apple_events[i].mac_class, app_apple_events[i].mac_id, &current_handler, &refCon, false);
                if (!current_handler)
                    AEInstallEventHandler(app_apple_events[i].mac_class, app_apple_events[i].mac_id,
                            app_proc_ae_handlerUPP, SRefCon(qApp), false);
            }
        }

        if (QApplicationPrivate::app_style) {
            QEvent ev(QEvent::Style);
            qt_sendSpontaneousEvent(QApplicationPrivate::app_style, &ev);
        }
    }
    if (QApplication::desktopSettingsAware())
        QApplicationPrivate::qt_mac_apply_settings();

    // Cocoa application delegate
    NSApplication *cocoaApp = [QNSApplication sharedApplication];
    qt_redirectNSApplicationSendEvent();

    QMacCocoaAutoReleasePool pool;
    id oldDelegate = [cocoaApp delegate];
    QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) *newDelegate = [QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate];
    Q_ASSERT(newDelegate);
    [newDelegate setQtPrivate:priv];
    // Only do things that make sense to do once, otherwise we crash.
    if (oldDelegate != newDelegate && !QApplication::testAttribute(Qt::AA_MacPluginApplication)) {
        [newDelegate setReflectionDelegate:oldDelegate];
        [cocoaApp setDelegate:newDelegate];

        QT_MANGLE_NAMESPACE(QCocoaMenuLoader) *qtMenuLoader = [[QT_MANGLE_NAMESPACE(QCocoaMenuLoader) alloc] init];
        if ([NSBundle loadNibNamed:@"qt_menu" owner:qtMenuLoader] == false) {
            qFatal("Qt internal error: qt_menu.nib could not be loaded. The .nib file"
                   " should be placed in QtGui.framework/Versions/Current/Resources/ "
                   " or in the resources directory of your application bundle.");
        }

        [cocoaApp setMenu:[qtMenuLoader menu]];
        [newDelegate setMenuLoader:qtMenuLoader];
        [qtMenuLoader release];
    }
    // Register for Carbon tablet proximity events on the event monitor target.
    // This means that we should receive proximity events even when we aren't the active application.
    if (!tablet_proximity_handler) {
        tablet_proximity_UPP = NewEventHandlerUPP(QApplicationPrivate::tabletProximityCallback);
        qt_init_tablet_proximity_handler();
    }
   priv->native_modal_dialog_active = false;

   qt_mac_read_fontsmoothing_settings();
}

void qt_release_apple_event_handler()
{
    if(app_proc_ae_handlerUPP) {
        for(uint i = 0; i < sizeof(app_apple_events) / sizeof(QMacAppleEventTypeSpec); ++i)
            AERemoveEventHandler(app_apple_events[i].mac_class, app_apple_events[i].mac_id,
                    app_proc_ae_handlerUPP, true);
        DisposeAEEventHandlerUPP(app_proc_ae_handlerUPP);
        app_proc_ae_handlerUPP = 0;
    }
}

/*****************************************************************************
  qt_cleanup() - cleans up when the application is finished
 *****************************************************************************/

void qt_cleanup()
{
    CGDisplayRemoveReconfigurationCallback(qt_mac_display_change_callbk, 0);
    CFNotificationCenterRef center = CFNotificationCenterGetDistributedCenter();
    CFNotificationCenterRemoveObserver(center, qApp, kCMDeviceUnregisteredNotification, 0);
    CFNotificationCenterRemoveObserver(center, qApp, kCMDefaultDeviceNotification, 0);
    CFNotificationCenterRemoveObserver(center, qApp, kCMDeviceProfilesNotification, 0);
    CFNotificationCenterRemoveObserver(center, qApp, kCMDefaultDeviceProfileNotification, 0);

    qt_release_apple_event_handler();
    qt_release_tablet_proximity_handler();
    if (tablet_proximity_UPP)
        DisposeEventHandlerUPP(tablet_proximity_UPP);

    QPixmapCache::clear();
    if (qt_is_gui_used) {
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::cleanup();
#endif
        QMacInputContext::cleanup();
        QCursorData::cleanup();
        QFont::cleanup();
        QColormap::cleanup();
        if (qt_mac_safe_pdev) {
            delete qt_mac_safe_pdev;
            qt_mac_safe_pdev = 0;
        }
        extern void qt_mac_unregister_widget(); // qapplication_mac.cpp
        qt_mac_unregister_widget();
    }
}

/*****************************************************************************
  Platform specific global and internal functions
 *****************************************************************************/
void qt_updated_rootinfo()
{
}

bool qt_wstate_iconified(WId)
{
    return false;
}

/*****************************************************************************
  Platform specific QApplication members
 *****************************************************************************/
extern QWidget * mac_mouse_grabber;
extern QWidget * mac_keyboard_grabber;

#ifndef QT_NO_CURSOR

/*****************************************************************************
  QApplication cursor stack
 *****************************************************************************/

void QApplication::setOverrideCursor(const QCursor &cursor)
{
    qApp->d_func()->cursor_list.prepend(cursor);

    qt_mac_update_cursor();
}

void QApplication::restoreOverrideCursor()
{
    if (qApp->d_func()->cursor_list.isEmpty())
        return;
    qApp->d_func()->cursor_list.removeFirst();

    qt_mac_update_cursor();
}
#endif // QT_NO_CURSOR

QWidget *QApplication::topLevelAt(const QPoint &p)
{
    // Use a cache to avoid iterate through the whole list of windows for all
    // calls to to topLevelAt. We e.g. do this for each and every mouse
    // move since we need to find the widget under mouse:
    if (topLevelAt_cache && topLevelAt_cache->frameGeometry().contains(p))
        return topLevelAt_cache;

    // INVARIANT: Cache miss. Go through the list if windows instead:
    QMacCocoaAutoReleasePool pool;
    NSPoint cocoaPoint = flipPoint(p);
    NSInteger windowCount;
    NSCountWindows(&windowCount);
    if (windowCount <= 0)
        return 0;  // There's no window to find!

    QVarLengthArray<NSInteger> windowList(windowCount);
    NSWindowList(windowCount, windowList.data());
    int firstQtWindowFound = -1;
    for (int i = 0; i < windowCount; ++i) {
        NSWindow *window = [NSApp windowWithWindowNumber:windowList[i]];
        if (window) {
            QWidget *candidateWindow = [window QT_MANGLE_NAMESPACE(qt_qwidget)];
            if (candidateWindow && firstQtWindowFound == -1)
                firstQtWindowFound = i;

            if (NSPointInRect(cocoaPoint, [window frame])) {
                // Check to see if there's a hole in the window where the mask is.
                // If there is, we should just continue to see if there is a window below.
                if (candidateWindow && !candidateWindow->mask().isEmpty()) {
                    QPoint localPoint = candidateWindow->mapFromGlobal(p);
                    if (!candidateWindow->mask().contains(localPoint))
                        continue;
                    else
                        return candidateWindow;
                } else {
                    if (i == firstQtWindowFound) {
                        // The cache will only work when the window under mouse is
                        // top most (that is, not partially obscured by other windows.
                        // And we only set it if no mask is present to optimize for the common case:
                        topLevelAt_cache = candidateWindow;
                    }
                    return candidateWindow;
                }
            }
        }
    }

    topLevelAt_cache = 0;
    return 0;
}

/*****************************************************************************
  Main event loop
 *****************************************************************************/

bool QApplicationPrivate::modalState()
{
    return app_do_modal;
}


void QApplicationPrivate::enterModal_sys(QWidget *widget)
{
#ifdef DEBUG_MODAL_EVENTS
    Q_ASSERT(widget);
    qDebug("Entering modal state with %s::%s::%p (%d)", widget->metaObject()->className(), widget->objectName().toLocal8Bit().constData(),
            widget, qt_modal_stack ? (int)qt_modal_stack->count() : -1);
#endif
    if (!qt_modal_stack)
        qt_modal_stack = new QWidgetList;

    dispatchEnterLeave(0, qt_last_mouse_receiver);
    qt_last_mouse_receiver = 0;

    qt_modal_stack->insert(0, widget);
    if (!app_do_modal)
        qt_event_request_menubarupdate();
    app_do_modal = true;
    qt_button_down = 0;

    if (!qt_mac_is_macsheet(widget))
        QEventDispatcherMacPrivate::beginModalSession(widget);
}

void QApplicationPrivate::leaveModal_sys(QWidget *widget)
{
    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
#ifdef DEBUG_MODAL_EVENTS
        qDebug("Leaving modal state with %s::%s::%p (%d)", widget->metaObject()->className(), widget->objectName().toLocal8Bit().constData(),
               widget, qt_modal_stack->count());
#endif
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
            QPoint p(QCursor::pos());
            app_do_modal = false;
            QWidget* w = 0;
            if (QWidget *grabber = QWidget::mouseGrabber())
                w = grabber;
            else
                w = QApplication::widgetAt(p.x(), p.y());
            dispatchEnterLeave(w, qt_last_mouse_receiver); // send synthetic enter event
            qt_last_mouse_receiver = w;
        }
        if (!qt_mac_is_macsheet(widget))
            QEventDispatcherMacPrivate::endModalSession(widget);
    }
#ifdef DEBUG_MODAL_EVENTS
    else qDebug("Failure to remove %s::%s::%p -- %p", widget->metaObject()->className(), widget->objectName().toLocal8Bit().constData(), widget, qt_modal_stack);
#endif
    app_do_modal = (qt_modal_stack != 0);
    if (!app_do_modal)
        qt_event_request_menubarupdate();
}

QWidget *QApplicationPrivate::tryModalHelper_sys(QWidget *top)
{
    return top;
}


OSStatus QApplicationPrivate::tabletProximityCallback(EventHandlerCallRef, EventRef carbonEvent,
                                                      void *)
{
    OSType eventClass = GetEventClass(carbonEvent);
    UInt32 eventKind = GetEventKind(carbonEvent);
    if (eventClass != kEventClassTablet || eventKind != kEventTabletProximity)
        return eventNotHandledErr;

    // Get the current point of the device and its unique ID.
    ::TabletProximityRec proxRec;
    GetEventParameter(carbonEvent, kEventParamTabletProximityRec, typeTabletProximityRec, 0,
                      sizeof(proxRec), 0, &proxRec);
    qt_dispatchTabletProximityEvent(proxRec);
    return noErr;
}

OSStatus
QApplicationPrivate::globalEventProcessor(EventHandlerCallRef er, EventRef event, void *data)
{
    Q_UNUSED(er);
    Q_UNUSED(event);
    Q_UNUSED(data);
    return eventNotHandledErr;
}

void QApplicationPrivate::qt_initAfterNSAppStarted()
{
    setupAppleEvents();
    qt_mac_update_cursor();
}

void QApplicationPrivate::setupAppleEvents()
{
    // This function is called from the event dispatcher when NSApplication has
    // finished initialization, which appears to be just after [NSApplication run] has
    // started to execute. By setting up our apple events handlers this late, we override
    // the ones set up by NSApplication.

    // If Qt is used as a plugin, we let the 3rd party application handle events
    // like quit and open file events. Otherwise, if we install our own handlers, we
    // easily end up breaking functionallity the 3rd party application depend on:
    if (QApplication::testAttribute(Qt::AA_MacPluginApplication))
        return;

    QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) *newDelegate = [QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate];
    NSAppleEventManager *eventManager = [NSAppleEventManager sharedAppleEventManager];
    [eventManager setEventHandler:newDelegate andSelector:@selector(appleEventQuit:withReplyEvent:)
     forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
    [eventManager setEventHandler:newDelegate andSelector:@selector(getUrl:withReplyEvent:)
      forEventClass:kInternetEventClass andEventID:kAEGetURL];
}

// In Carbon this is your one stop for apple events.
// In Cocoa, it ISN'T. This is the catch-all Apple Event handler that exists
// for the time between instantiating the NSApplication, but before the
// NSApplication has installed it's OWN Apple Event handler. When Cocoa has
// that set up, we remove this.  So, if you are debugging problems, you likely
// want to check out QCocoaApplicationDelegate instead.
OSStatus QApplicationPrivate::globalAppleEventProcessor(const AppleEvent *ae, AppleEvent *, long handlerRefcon)
{
    QApplication *app = (QApplication *)handlerRefcon;
    bool handled_event=false;
    OSType aeID=typeWildCard, aeClass=typeWildCard;
    AEGetAttributePtr(ae, keyEventClassAttr, typeType, 0, &aeClass, sizeof(aeClass), 0);
    AEGetAttributePtr(ae, keyEventIDAttr, typeType, 0, &aeID, sizeof(aeID), 0);
    if(aeClass == kCoreEventClass) {
        switch(aeID) {
        case kAEQuitApplication: {
            extern bool qt_mac_quit_menu_item_enabled; // qmenu_mac.cpp
            if (qt_mac_quit_menu_item_enabled) {
                QCloseEvent ev;
                QApplication::sendSpontaneousEvent(app, &ev);
                if(ev.isAccepted()) {
                    handled_event = true;
                    app->quit();
                }
            } else {
                QApplication::beep();  // Sorry, you can't quit right now.
            }
            break; }
        case kAEOpenDocuments: {
            AEDescList docs;
            if(AEGetParamDesc(ae, keyDirectObject, typeAEList, &docs) == noErr) {
                long cnt = 0;
                AECountItems(&docs, &cnt);
                UInt8 *str_buffer = NULL;
                for(int i = 0; i < cnt; i++) {
                    FSRef ref;
                    if(AEGetNthPtr(&docs, i+1, typeFSRef, 0, 0, &ref, sizeof(ref), 0) != noErr)
                        continue;
                    if(!str_buffer)
                        str_buffer = (UInt8 *)malloc(1024);
                    FSRefMakePath(&ref, str_buffer, 1024);
                    QFileOpenEvent ev(QString::fromUtf8((const char *)str_buffer));
                    QApplication::sendSpontaneousEvent(app, &ev);
                }
                if(str_buffer)
                    free(str_buffer);
            }
            break; }
        default:
            break;
        }
    } else if (aeClass == kInternetEventClass) {
        switch (aeID) {
        case kAEGetURL: {
            char urlData[1024];
            Size actualSize;
            if (AEGetParamPtr(ae, keyDirectObject, typeChar, 0, urlData,
                    sizeof(urlData) - 1, &actualSize) == noErr) {
                urlData[actualSize] = 0;
                QFileOpenEvent ev(QUrl(QString::fromUtf8(urlData)));
                QApplication::sendSpontaneousEvent(app, &ev);
            }
            break;
        }
        default:
            break;
        }
    }
#ifdef DEBUG_EVENTS
    qDebug("Qt: internal: %shandled Apple event! %c%c%c%c %c%c%c%c", handled_event ? "(*)" : "",
           char(aeID >> 24), char((aeID >> 16) & 255), char((aeID >> 8) & 255),char(aeID & 255),
           char(aeClass >> 24), char((aeClass >> 16) & 255), char((aeClass >> 8) & 255),char(aeClass & 255));
#else
    if(!handled_event) //let the event go through
        return eventNotHandledErr;
    return noErr; //we eat the event
#endif
}

/*!
    \fn bool QApplication::macEventFilter(EventHandlerCallRef caller, EventRef event)

    \warning This virtual function is only used under Mac OS X, and behaves different
    depending on if Qt is based on Carbon or Cocoa.

    For the Carbon port, If you create an application that inherits QApplication and reimplement
    this function, you get direct access to all Carbon Events that Qt registers
    for from Mac OS X with this function being called with the \a caller and
    the \a event.

    For the Cocoa port, If you create an application that inherits QApplication and reimplement
    this function, you get direct access to all Cocoa Events that Qt receives
    from Mac OS X with this function being called with the \a caller being 0 and
    the \a event being an NSEvent pointer:

    NSEvent *e = reinterpret_cast<NSEvent *>(event);

    Return true if you want to stop the event from being processed.
    Return false for normal event dispatching. The default
    implementation returns false.
*/
bool QApplication::macEventFilter(EventHandlerCallRef, EventRef)
{
    return false;
}

/*!
    \internal
*/
void QApplicationPrivate::openPopup(QWidget *popup)
{
    if (!QApplicationPrivate::popupWidgets)                        // create list
        QApplicationPrivate::popupWidgets = new QWidgetList;
    QApplicationPrivate::popupWidgets->append(popup);                // add to end of list

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (QApplicationPrivate::popupWidgets->count() == 1) { // this was the first popup
        popup->setFocus(Qt::PopupFocusReason);
    }
}

/*!
    \internal
*/
void QApplicationPrivate::closePopup(QWidget *popup)
{
    Q_Q(QApplication);
    if (!QApplicationPrivate::popupWidgets)
        return;

    QApplicationPrivate::popupWidgets->removeAll(popup);
    if (popup == qt_button_down)
        qt_button_down = 0;
    if (QApplicationPrivate::popupWidgets->isEmpty()) {  // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;

        // Special case for Tool windows: since they are activated and deactived together
        // with a normal window they never become the QApplicationPrivate::active_window.
        QWidget *appFocusWidget = QApplication::focusWidget();
        if (appFocusWidget && appFocusWidget->window()->windowType() == Qt::Tool) {
            appFocusWidget->setFocus(Qt::PopupFocusReason);
        } else if (QApplicationPrivate::active_window) {
            if (QWidget *fw = QApplicationPrivate::active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    q->sendEvent(fw, &e);
                }
            }
        }
    } else {
        // popups are not focus-handled by the window system (the
        // first popup grabbed the keyboard), so we have to do that
        // manually: A popup was closed, so the previous popup gets
        // the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);
    }
}

void QApplication::beep()
{
    qt_mac_beep();
}

void QApplication::alert(QWidget *widget, int duration)
{
    if (!QApplicationPrivate::checkInstance("alert"))
        return;

    QWidgetList windowsToMark;
    if (!widget)
        windowsToMark += topLevelWidgets();
    else
        windowsToMark.append(widget->window());

    bool needNotification = false;
    for (int i = 0; i < windowsToMark.size(); ++i) {
        QWidget *window = windowsToMark.at(i);
        if (!window->isActiveWindow() && window->isVisible()) {
            needNotification = true; // yeah, we may set it multiple times, but that's OK.
            if (duration != 0) {
               QTimer *timer = new QTimer(qApp);
               timer->setSingleShot(true);
               connect(timer, SIGNAL(timeout()), qApp, SLOT(_q_alertTimeOut()));
               if (QTimer *oldTimer = qApp->d_func()->alertTimerHash.value(widget)) {
                   qApp->d_func()->alertTimerHash.remove(widget);
                   delete oldTimer;
               }
               qApp->d_func()->alertTimerHash.insert(widget, timer);
               timer->start(duration);
            }
        }
    }
    if (needNotification)
        qt_mac_send_notification();
}

void QApplicationPrivate::_q_alertTimeOut()
{
    if (QTimer *timer = qobject_cast<QTimer *>(q_func()->sender())) {
        QHash<QWidget *, QTimer *>::iterator it = alertTimerHash.begin();
        while (it != alertTimerHash.end()) {
            if (it.value() == timer) {
                alertTimerHash.erase(it);
                timer->deleteLater();
                break;
            }
            ++it;
        }
        if (alertTimerHash.isEmpty()) {
            qt_mac_cancel_notification();
        }
    }
}

void  QApplication::setCursorFlashTime(int msecs)
{
    QApplicationPrivate::cursor_flash_time = msecs;
}

int QApplication::cursorFlashTime()
{
    return QApplicationPrivate::cursor_flash_time;
}

void QApplication::setDoubleClickInterval(int ms)
{
    qt_mac_dblclick.use_qt_time_limit = true;
    QApplicationPrivate::mouse_double_click_time = ms;
}

int QApplication::doubleClickInterval()
{
    if (!qt_mac_dblclick.use_qt_time_limit) { //get it from the system
        QSettings appleSettings(QLatin1String("apple.com"));
        /* First worked as of 10.3.3 */
        double dci = appleSettings.value(QLatin1String("com/apple/mouse/doubleClickThreshold"), 0.5).toDouble();
        return int(dci * 1000);
    }
    return QApplicationPrivate::mouse_double_click_time;
}

void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::keyboardInputInterval()
{
    // FIXME: get from the system
    return QApplicationPrivate::keyboard_input_time;
}

#ifndef QT_NO_WHEELEVENT
void QApplication::setWheelScrollLines(int n)
{
    QApplicationPrivate::wheel_scroll_lines = n;
}

int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}
#endif

void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    switch (effect) {
    case Qt::UI_FadeMenu:
        QApplicationPrivate::fade_menu = enable;
        break;
    case Qt::UI_AnimateMenu:
        QApplicationPrivate::animate_menu = enable;
        break;
    case Qt::UI_FadeTooltip:
        QApplicationPrivate::fade_tooltip = enable;
        break;
    case Qt::UI_AnimateTooltip:
        QApplicationPrivate::animate_tooltip = enable;
        break;
    case Qt::UI_AnimateCombo:
        QApplicationPrivate::animate_combo = enable;
        break;
    case Qt::UI_AnimateToolBox:
        QApplicationPrivate::animate_toolbox = enable;
        break;
    case Qt::UI_General:
        QApplicationPrivate::fade_tooltip = true;
        break;
    default:
        QApplicationPrivate::animate_ui = enable;
        break;
    }

    if (enable)
        QApplicationPrivate::animate_ui = true;
}

bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    if (QColormap::instance().depth() < 16 || !QApplicationPrivate::animate_ui)
        return false;

    switch(effect) {
    case Qt::UI_AnimateMenu:
        return QApplicationPrivate::animate_menu;
    case Qt::UI_FadeMenu:
        return QApplicationPrivate::fade_menu;
    case Qt::UI_AnimateCombo:
        return QApplicationPrivate::animate_combo;
    case Qt::UI_AnimateTooltip:
        return QApplicationPrivate::animate_tooltip;
    case Qt::UI_FadeTooltip:
        return QApplicationPrivate::fade_tooltip;
    case Qt::UI_AnimateToolBox:
        return QApplicationPrivate::animate_toolbox;
    default:
        break;
    }
    return QApplicationPrivate::animate_ui;
}

/*!
    \internal
*/
bool QApplicationPrivate::qt_mac_apply_settings()
{
    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("Qt"));

    /*
      Qt settings.  This is how they are written into the datastream.
      Palette/ *             - QPalette
      font                   - QFont
      libraryPath            - QStringList
      style                  - QString
      doubleClickInterval    - int
      cursorFlashTime        - int
      wheelScrollLines       - int
      colorSpec              - QString
      defaultCodec           - QString
      globalStrut/width      - int
      globalStrut/height     - int
      GUIEffects             - QStringList
      Font Substitutions/ *  - QStringList
      Font Substitutions/... - QStringList
    */

    // read library (ie. plugin) path list
    QString libpathkey =
        QString::fromLatin1("%1.%2/libraryPath")
                    .arg(QT_VERSION >> 16)
                    .arg((QT_VERSION & 0xff00) >> 8);
    QStringList pathlist = settings.value(libpathkey).toString().split(QLatin1Char(':'));
    if (!pathlist.isEmpty()) {
        QStringList::ConstIterator it = pathlist.begin();
        while(it != pathlist.end())
            QApplication::addLibraryPath(*it++);
    }

    QString defaultcodec = settings.value(QLatin1String("defaultCodec"), QVariant(QLatin1String("none"))).toString();
    if (defaultcodec != QLatin1String("none")) {
        QTextCodec *codec = QTextCodec::codecForName(defaultcodec.toLatin1().constData());
        if (codec)
            QTextCodec::setCodecForTr(codec);
    }

    if (qt_is_gui_used) {
        QString str;
        QStringList strlist;
        int num;

        // read new palette
        int i;
        QPalette pal(QApplication::palette());
        strlist = settings.value(QLatin1String("Palette/active")).toStringList();
        if (strlist.count() == QPalette::NColorRoles) {
            for (i = 0; i < QPalette::NColorRoles; i++)
                pal.setColor(QPalette::Active, (QPalette::ColorRole) i,
                            QColor(strlist[i]));
        }
        strlist = settings.value(QLatin1String("Palette/inactive")).toStringList();
        if (strlist.count() == QPalette::NColorRoles) {
            for (i = 0; i < QPalette::NColorRoles; i++)
                pal.setColor(QPalette::Inactive, (QPalette::ColorRole) i,
                            QColor(strlist[i]));
        }
        strlist = settings.value(QLatin1String("Palette/disabled")).toStringList();
        if (strlist.count() == QPalette::NColorRoles) {
            for (i = 0; i < QPalette::NColorRoles; i++)
                pal.setColor(QPalette::Disabled, (QPalette::ColorRole) i,
                            QColor(strlist[i]));
        }

        if (pal != QApplication::palette())
            QApplication::setPalette(pal);

        // read new font
        QFont font(QApplication::font());
        str = settings.value(QLatin1String("font")).toString();
        if (!str.isEmpty()) {
            font.fromString(str);
            if (font != QApplication::font())
                QApplication::setFont(font);
        }

        // read new QStyle
        QString stylename = settings.value(QLatin1String("style")).toString();
        if (! stylename.isNull() && ! stylename.isEmpty()) {
            QStyle *style = QStyleFactory::create(stylename);
            if (style)
                QApplication::setStyle(style);
            else
                stylename = QLatin1String("default");
        } else {
            stylename = QLatin1String("default");
        }

        num = settings.value(QLatin1String("doubleClickInterval"),
                            QApplication::doubleClickInterval()).toInt();
        QApplication::setDoubleClickInterval(num);

        num = settings.value(QLatin1String("cursorFlashTime"),
                            QApplication::cursorFlashTime()).toInt();
        QApplication::setCursorFlashTime(num);

#ifndef QT_NO_WHEELEVENT
        num = settings.value(QLatin1String("wheelScrollLines"),
                            QApplication::wheelScrollLines()).toInt();
        QApplication::setWheelScrollLines(num);
#endif

        QString colorspec = settings.value(QLatin1String("colorSpec"),
                                            QVariant(QLatin1String("default"))).toString();
        if (colorspec == QLatin1String("normal"))
            QApplication::setColorSpec(QApplication::NormalColor);
        else if (colorspec == QLatin1String("custom"))
            QApplication::setColorSpec(QApplication::CustomColor);
        else if (colorspec == QLatin1String("many"))
            QApplication::setColorSpec(QApplication::ManyColor);
        else if (colorspec != QLatin1String("default"))
            colorspec = QLatin1String("default");

        int w = settings.value(QLatin1String("globalStrut/width")).toInt();
        int h = settings.value(QLatin1String("globalStrut/height")).toInt();
        QSize strut(w, h);
        if (strut.isValid())
            QApplication::setGlobalStrut(strut);

        QStringList effects = settings.value(QLatin1String("GUIEffects")).toStringList();
        if (!effects.isEmpty()) {
            if (effects.contains(QLatin1String("none")))
                QApplication::setEffectEnabled(Qt::UI_General, false);
            if (effects.contains(QLatin1String("general")))
                QApplication::setEffectEnabled(Qt::UI_General, true);
            if (effects.contains(QLatin1String("animatemenu")))
                QApplication::setEffectEnabled(Qt::UI_AnimateMenu, true);
            if (effects.contains(QLatin1String("fademenu")))
                QApplication::setEffectEnabled(Qt::UI_FadeMenu, true);
            if (effects.contains(QLatin1String("animatecombo")))
                QApplication::setEffectEnabled(Qt::UI_AnimateCombo, true);
            if (effects.contains(QLatin1String("animatetooltip")))
                QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, true);
            if (effects.contains(QLatin1String("fadetooltip")))
                QApplication::setEffectEnabled(Qt::UI_FadeTooltip, true);
            if (effects.contains(QLatin1String("animatetoolbox")))
                QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, true);
        } else {
            QApplication::setEffectEnabled(Qt::UI_General, true);
        }

        settings.beginGroup(QLatin1String("Font Substitutions"));
        QStringList fontsubs = settings.childKeys();
        if (!fontsubs.isEmpty()) {
            QStringList::Iterator it = fontsubs.begin();
            for (; it != fontsubs.end(); ++it) {
                QString fam = QString::fromLatin1((*it).toLatin1().constData());
                QStringList subs = settings.value(fam).toStringList();
                QFont::insertSubstitutions(fam, subs);
            }
        }
        settings.endGroup();
    }

    settings.endGroup();
    return true;
}

// DRSWAT

bool QApplicationPrivate::canQuit()
{
    Q_Q(QApplication);
    [[NSApp mainMenu] cancelTracking];

    bool handle_quit = true;
    if (QApplicationPrivate::modalState() && [[[[QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate]
                                                   menuLoader] quitMenuItem] isEnabled]) {
        int visible = 0;
        const QWidgetList tlws = QApplication::topLevelWidgets();
        for(int i = 0; i < tlws.size(); ++i) {
            if (tlws.at(i)->isVisible())
                ++visible;
        }
        handle_quit = (visible <= 1);
    }
    if (handle_quit) {
        QCloseEvent ev;
        QApplication::sendSpontaneousEvent(q, &ev);
        if (ev.isAccepted()) {
            return true;
        }
    }
    return false;
}

void onApplicationWindowChangedActivation(QWidget *widget, bool activated)
{
    if (!widget)
        return;

    if (activated) {
        if (QApplicationPrivate::app_style) {
            QEvent ev(QEvent::Style);
            qt_sendSpontaneousEvent(QApplicationPrivate::app_style, &ev);
        }
        qApp->setActiveWindow(widget);
    } else { // deactivated
        if (QApplicationPrivate::active_window == widget)
            qApp->setActiveWindow(0);
    }

    QMenuBar::macUpdateMenuBar();
    qt_mac_update_cursor();
}


void onApplicationChangedActivation( bool activated )
{
    QApplication    *app    = qApp;

//NSLog(@"App Changed Activation\n");

    if ( activated ) {
        if (QApplication::desktopSettingsAware())
            qt_mac_update_os_settings();

        if (qt_clipboard) { //manufacture an event so the clipboard can see if it has changed
            QEvent ev(QEvent::Clipboard);
            qt_sendSpontaneousEvent(qt_clipboard, &ev);
        }

        if (app) {
            QEvent ev(QEvent::ApplicationActivate);
            qt_sendSpontaneousEvent(app, &ev);
        }

        if (!app->activeWindow()) {
            OSWindowRef wp = [NSApp keyWindow];
            if (QWidget *tmp_w = qt_mac_find_window(wp))
                app->setActiveWindow(tmp_w);
        }
        QMenuBar::macUpdateMenuBar();
        qt_mac_update_cursor();
    } else { // de-activated
        QApplicationPrivate *priv = [[QT_MANGLE_NAMESPACE(QCocoaApplicationDelegate) sharedDelegate] qAppPrivate];
        while (priv->inPopupMode())
            app->activePopupWidget()->close();
        if (app) {
            QEvent ev(QEvent::ApplicationDeactivate);
            qt_sendSpontaneousEvent(app, &ev);
        }
        app->setActiveWindow(0);
    }
}

void QApplicationPrivate::initializeMultitouch_sys()
{ }
void QApplicationPrivate::cleanupMultitouch_sys()
{ }

QT_END_NAMESPACE
