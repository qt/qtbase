// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qguiapplication.h"

#include "private/qguiapplication_p.h"
#include "private/qabstractfileiconprovider_p.h"
#include <qpa/qplatformintegrationfactory_p.h>
#include "private/qevent_p.h"
#include "private/qeventpoint_p.h"
#include "private/qiconloader_p.h"
#include "qfont.h"
#include "qpointingdevice.h"
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformkeymapper.h>

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtCore/QVariant>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qabstracteventdispatcher_p.h>
#include <QtCore/private/qminimalflatset_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/private/qthread_p.h>
#include <QtCore/private/qlocking_p.h>
#include <QtCore/private/qflatmap_p.h>
#include <QtCore/qdir.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/private/qnumeric_p.h>
#include <QtDebug>
#if QT_CONFIG(accessibility)
#include "qaccessible.h"
#endif
#include <qpalette.h>
#include <qscreen.h>
#include "qsessionmanager.h"
#include <private/qcolortrclut_p.h>
#include <private/qscreen_p.h>

#include <QtGui/qgenericpluginfactory.h>
#include <QtGui/qstylehints.h>
#include <QtGui/private/qstylehints_p.h>
#include <QtGui/qinputmethod.h>
#include <QtGui/qpixmapcache.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatforminputcontext_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include "private/qwindow_p.h"
#include "private/qicon_p.h"
#include "private/qcursor_p.h"
#if QT_CONFIG(opengl)
#  include "private/qopenglcontext_p.h"
#endif
#include "private/qinputdevicemanager_p.h"
#include "private/qinputmethod_p.h"
#include "private/qpointingdevice_p.h"

#include <qpa/qplatformthemefactory_p.h>

#if QT_CONFIG(draganddrop)
#include <qpa/qplatformdrag.h>
#include <private/qdnd_p.h>
#endif

#ifndef QT_NO_CURSOR
#include <qpa/qplatformcursor.h>
#endif

#include <QtGui/QPixmap>

#ifndef QT_NO_CLIPBOARD
#include <QtGui/QClipboard>
#endif

#if QT_CONFIG(library)
#include <QtCore/QLibrary>
#endif

#if defined(Q_OS_MAC)
#  include "private/qcore_mac_p.h"
#elif defined(Q_OS_WIN)
#  include <QtCore/qt_windows.h>
#  include <QtCore/QLibraryInfo>
#endif // Q_OS_WIN

#ifdef Q_OS_WASM
#include <emscripten.h>
#endif

#if QT_CONFIG(vulkan)
#include <private/qvulkandefaultinstance_p.h>
#endif

#if QT_CONFIG(thread)
#include <QtCore/QThreadPool>
#endif

#include <qtgui_tracepoints_p.h>

#include <private/qtools_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPopup, "qt.gui.popup");
Q_LOGGING_CATEGORY(lcVirtualKeyboard, "qt.gui.virtualkeyboard");

using namespace Qt::StringLiterals;
using namespace QtMiscUtils;

// Helper macro for static functions to check on the existence of the application class.
#define CHECK_QAPP_INSTANCE(...) \
    if (Q_LIKELY(QCoreApplication::instance())) { \
    } else { \
        qWarning("Must construct a QGuiApplication first."); \
        return __VA_ARGS__; \
    }

Q_CORE_EXPORT void qt_call_post_routines();
Q_CONSTINIT Q_GUI_EXPORT bool qt_is_tty_app = false;

Q_CONSTINIT Qt::MouseButtons QGuiApplicationPrivate::mouse_buttons = Qt::NoButton;
Q_CONSTINIT Qt::KeyboardModifiers QGuiApplicationPrivate::modifier_buttons = Qt::NoModifier;

Q_CONSTINIT QGuiApplicationPrivate::QLastCursorPosition QGuiApplicationPrivate::lastCursorPosition;

Q_CONSTINIT QWindow *QGuiApplicationPrivate::currentMouseWindow = nullptr;

Q_CONSTINIT QString QGuiApplicationPrivate::styleOverride;

Q_CONSTINIT Qt::ApplicationState QGuiApplicationPrivate::applicationState = Qt::ApplicationInactive;

Q_CONSTINIT Qt::HighDpiScaleFactorRoundingPolicy QGuiApplicationPrivate::highDpiScaleFactorRoundingPolicy =
    Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;

Q_CONSTINIT QPointer<QWindow> QGuiApplicationPrivate::currentDragWindow;

Q_CONSTINIT QList<QGuiApplicationPrivate::TabletPointData> QGuiApplicationPrivate::tabletDevicePoints; // TODO remove

Q_CONSTINIT QPlatformIntegration *QGuiApplicationPrivate::platform_integration = nullptr;
Q_CONSTINIT QPlatformTheme *QGuiApplicationPrivate::platform_theme = nullptr;

Q_CONSTINIT QList<QObject *> QGuiApplicationPrivate::generic_plugin_list;

enum ApplicationResourceFlags
{
    ApplicationFontExplicitlySet = 0x2
};

Q_CONSTINIT static unsigned applicationResourceFlags = 0;

Q_CONSTINIT QIcon *QGuiApplicationPrivate::app_icon = nullptr;

Q_CONSTINIT QString *QGuiApplicationPrivate::platform_name = nullptr;
Q_CONSTINIT QString *QGuiApplicationPrivate::displayName = nullptr;
Q_CONSTINIT QString *QGuiApplicationPrivate::desktopFileName = nullptr;

Q_CONSTINIT QPalette *QGuiApplicationPrivate::app_pal = nullptr;        // default application palette

Q_CONSTINIT Qt::MouseButton QGuiApplicationPrivate::mousePressButton = Qt::NoButton;

Q_CONSTINIT static int mouseDoubleClickDistance = 0;
Q_CONSTINIT static int touchDoubleTapDistance = 0;

Q_CONSTINIT QWindow *QGuiApplicationPrivate::currentMousePressWindow = nullptr;

Q_CONSTINIT static Qt::LayoutDirection layout_direction = Qt::LayoutDirectionAuto;
Q_CONSTINIT static Qt::LayoutDirection effective_layout_direction = Qt::LeftToRight;
Q_CONSTINIT static bool force_reverse = false;

Q_CONSTINIT QGuiApplicationPrivate *QGuiApplicationPrivate::self = nullptr;
Q_CONSTINIT int QGuiApplicationPrivate::m_fakeMouseSourcePointId = -1;

#ifndef QT_NO_CLIPBOARD
Q_CONSTINIT QClipboard *QGuiApplicationPrivate::qt_clipboard = nullptr;
#endif

Q_CONSTINIT QList<QScreen *> QGuiApplicationPrivate::screen_list;

Q_CONSTINIT QWindowList QGuiApplicationPrivate::window_list;
Q_CONSTINIT QWindowList QGuiApplicationPrivate::popup_list;
Q_CONSTINIT const QWindow *QGuiApplicationPrivate::active_popup_on_press = nullptr;
Q_CONSTINIT QWindow *QGuiApplicationPrivate::focus_window = nullptr;

Q_CONSTINIT static QBasicMutex applicationFontMutex;
Q_CONSTINIT QFont *QGuiApplicationPrivate::app_font = nullptr;
Q_CONSTINIT QStyleHints *QGuiApplicationPrivate::styleHints = nullptr;
Q_CONSTINIT bool QGuiApplicationPrivate::obey_desktop_settings = true;
Q_CONSTINIT bool QGuiApplicationPrivate::popup_closed_on_press = false;

Q_CONSTINIT QInputDeviceManager *QGuiApplicationPrivate::m_inputDeviceManager = nullptr;

Q_CONSTINIT qreal QGuiApplicationPrivate::m_maxDevicePixelRatio = 0.0;

Q_CONSTINIT static qreal fontSmoothingGamma = 1.7;

Q_CONSTINIT bool QGuiApplicationPrivate::quitOnLastWindowClosed = true;

extern void qRegisterGuiVariant();
#if QT_CONFIG(animation)
extern void qRegisterGuiGetInterpolator();
#endif

static bool qt_detectRTLLanguage()
{
    return force_reverse ^
        (QGuiApplication::tr("QT_LAYOUT_DIRECTION",
                         "Translate this string to the string 'LTR' in left-to-right"
                         " languages or to 'RTL' in right-to-left languages (such as Hebrew"
                         " and Arabic) to get proper widget layout.") == "RTL"_L1);
}

static void initFontUnlocked()
{
    if (!QGuiApplicationPrivate::app_font) {
        if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
            if (const QFont *font = theme->font(QPlatformTheme::SystemFont))
                QGuiApplicationPrivate::app_font = new QFont(*font);
    }
    if (!QGuiApplicationPrivate::app_font)
        QGuiApplicationPrivate::app_font =
            new QFont(QGuiApplicationPrivate::platformIntegration()->fontDatabase()->defaultFont());
}

static inline void clearFontUnlocked()
{
    delete QGuiApplicationPrivate::app_font;
    QGuiApplicationPrivate::app_font = nullptr;
}

static void initThemeHints()
{
    mouseDoubleClickDistance = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::MouseDoubleClickDistance).toInt();
    touchDoubleTapDistance = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::TouchDoubleTapDistance).toInt();
}

static bool checkNeedPortalSupport()
{
#if QT_CONFIG(dbus)
    return QFileInfo::exists("/.flatpak-info"_L1) || qEnvironmentVariableIsSet("SNAP");
#else
    return false;
#endif // QT_CONFIG(dbus)
}

// Using aggregate initialization instead of ctor so we can have a POD global static
#define Q_WINDOW_GEOMETRY_SPECIFICATION_INITIALIZER { Qt::TopLeftCorner, -1, -1, -1, -1 }

// Geometry specification for top level windows following the convention of the
// -geometry command line arguments in X11 (see XParseGeometry).
struct QWindowGeometrySpecification
{
    static QWindowGeometrySpecification fromArgument(const QByteArray &a);
    void applyTo(QWindow *window) const;

    Qt::Corner corner;
    int xOffset;
    int yOffset;
    int width;
    int height;
};

// Parse a token of a X11 geometry specification "200x100+10-20".
static inline int nextGeometryToken(const QByteArray &a, int &pos, char *op)
{
    *op = 0;
    const qsizetype size = a.size();
    if (pos >= size)
        return -1;

    *op = a.at(pos);
    if (*op == '+' || *op == '-' || *op == 'x')
        pos++;
    else if (isAsciiDigit(*op))
        *op = 'x'; // If it starts with a digit, it is supposed to be a width specification.
    else
        return -1;

    const int numberPos = pos;
    for ( ; pos < size && isAsciiDigit(a.at(pos)); ++pos) ;

    bool ok;
    const int result = a.mid(numberPos, pos - numberPos).toInt(&ok);
    return ok ? result : -1;
}

QWindowGeometrySpecification QWindowGeometrySpecification::fromArgument(const QByteArray &a)
{
    QWindowGeometrySpecification result = Q_WINDOW_GEOMETRY_SPECIFICATION_INITIALIZER;
    int pos = 0;
    for (int i = 0; i < 4; ++i) {
        char op;
        const int value = nextGeometryToken(a, pos, &op);
        if (value < 0)
            break;
        switch (op) {
        case 'x':
            (result.width >= 0 ? result.height : result.width) = value;
            break;
        case '+':
        case '-':
            if (result.xOffset >= 0) {
                result.yOffset = value;
                if (op == '-')
                    result.corner = result.corner == Qt::TopRightCorner ? Qt::BottomRightCorner : Qt::BottomLeftCorner;
            } else {
                result.xOffset = value;
                if (op == '-')
                    result.corner = Qt::TopRightCorner;
            }
        }
    }
    return result;
}

void QWindowGeometrySpecification::applyTo(QWindow *window) const
{
    QRect windowGeometry = window->frameGeometry();
    QSize size = windowGeometry.size();
    if (width >= 0 || height >= 0) {
        const QSize windowMinimumSize = window->minimumSize();
        const QSize windowMaximumSize = window->maximumSize();
        if (width >= 0)
            size.setWidth(qBound(windowMinimumSize.width(), width, windowMaximumSize.width()));
        if (height >= 0)
            size.setHeight(qBound(windowMinimumSize.height(), height, windowMaximumSize.height()));
        window->resize(size);
    }
    if (xOffset >= 0 || yOffset >= 0) {
        const QRect availableGeometry = window->screen()->virtualGeometry();
        QPoint topLeft = windowGeometry.topLeft();
        if (xOffset >= 0) {
            topLeft.setX(corner == Qt::TopLeftCorner || corner == Qt::BottomLeftCorner ?
                         xOffset :
                         qMax(availableGeometry.right() - size.width() - xOffset, availableGeometry.left()));
        }
        if (yOffset >= 0) {
            topLeft.setY(corner == Qt::TopLeftCorner || corner == Qt::TopRightCorner ?
                         yOffset :
                         qMax(availableGeometry.bottom() - size.height() - yOffset, availableGeometry.top()));
        }
        window->setFramePosition(topLeft);
    }
}

static QWindowGeometrySpecification windowGeometrySpecification = Q_WINDOW_GEOMETRY_SPECIFICATION_INITIALIZER;

/*!
    \macro qGuiApp
    \relates QGuiApplication

    A global pointer referring to the unique application object.
    Only valid for use when that object is a QGuiApplication.

    \sa QCoreApplication::instance(), qApp
*/

/*!
    \class QGuiApplication
    \brief The QGuiApplication class manages the GUI application's control
    flow and main settings.

    \inmodule QtGui
    \since 5.0

    QGuiApplication contains the main event loop, where all events from the window
    system and other sources are processed and dispatched. It also handles the
    application's initialization and finalization, and provides session management.
    In addition, QGuiApplication handles most of the system-wide and application-wide
    settings.

    For any GUI application using Qt, there is precisely \b one QGuiApplication
    object no matter whether the application has 0, 1, 2 or more windows at
    any given time. For non-GUI Qt applications, use QCoreApplication instead,
    as it does not depend on the Qt GUI module. For QWidget based Qt applications,
    use QApplication instead, as it provides some functionality needed for creating
    QWidget instances.

    The QGuiApplication object is accessible through the instance() function, which
    returns a pointer equivalent to the global \l qApp pointer.

    QGuiApplication's main areas of responsibility are:
        \list
            \li  It initializes the application with the user's desktop settings,
                such as palette(), font() and styleHints(). It keeps
                track of these properties in case the user changes the desktop
                globally, for example, through some kind of control panel.

            \li  It performs event handling, meaning that it receives events
                from the underlying window system and dispatches them to the
                relevant widgets. You can send your own events to windows by
                using sendEvent() and postEvent().

            \li  It parses common command line arguments and sets its internal
                state accordingly. See the \l{QGuiApplication::QGuiApplication()}
                {constructor documentation} below for more details.

            \li  It provides localization of strings that are visible to the
                user via translate().

            \li  It provides some magical objects like the clipboard().

            \li  It knows about the application's windows. You can ask which
                window is at a certain position using topLevelAt(), get a list of
                topLevelWindows(), etc.

            \li  It manages the application's mouse cursor handling, see
                setOverrideCursor()

            \li  It provides support for sophisticated \l{Session Management}
                {session management}. This makes it possible for applications
                to terminate gracefully when the user logs out, to cancel a
                shutdown process if termination isn't possible and even to
                preserve the entire application's state for a future session.
                See isSessionRestored(), sessionId() and commitDataRequest() and
                saveStateRequest() for details.
        \endlist

    Since the QGuiApplication object does so much initialization, it \e{must} be
    created before any other objects related to the user interface are created.
    QGuiApplication also deals with common command line arguments. Hence, it is
    usually a good idea to create it \e before any interpretation or
    modification of \c argv is done in the application itself.

    \table
    \header
        \li{2,1} Groups of functions

        \row
        \li  System settings
        \li  desktopSettingsAware(),
            setDesktopSettingsAware(),
            styleHints(),
            palette(),
            setPalette(),
            font(),
            setFont().

        \row
        \li  Event handling
        \li  exec(),
            processEvents(),
            exit(),
            quit().
            sendEvent(),
            postEvent(),
            sendPostedEvents(),
            removePostedEvents(),
            notify().

        \row
        \li  Windows
        \li  allWindows(),
            topLevelWindows(),
            focusWindow(),
            clipboard(),
            topLevelAt().

        \row
        \li  Advanced cursor handling
        \li  overrideCursor(),
            setOverrideCursor(),
            restoreOverrideCursor().

        \row
        \li  Session management
        \li  isSessionRestored(),
            sessionId(),
            commitDataRequest(),
            saveStateRequest().

        \row
        \li  Miscellaneous
        \li  startingUp(),
            closingDown().
    \endtable

    \sa QCoreApplication, QAbstractEventDispatcher, QEventLoop
*/

/*!
    Initializes the window system and constructs an application object with
    \a argc command line arguments in \a argv.

    \warning The data referred to by \a argc and \a argv must stay valid for
    the entire lifetime of the QGuiApplication object. In addition, \a argc must
    be greater than zero and \a argv must contain at least one valid character
    string.

    The global \c qApp pointer refers to this application object. Only one
    application object should be created.

    This application object must be constructed before any \l{QPaintDevice}
    {paint devices} (including pixmaps, bitmaps etc.).

    \note \a argc and \a argv might be changed as Qt removes command line
    arguments that it recognizes.

    \section1 Supported Command Line Options

    All Qt programs automatically support a set of command-line options that
    allow modifying the way Qt will interact with the windowing system. Some of
    the options are also accessible via environment variables, which are the
    preferred form if the application can launch GUI sub-processes or other
    applications (environment variables will be inherited by child processes).
    When in doubt, use the environment variables.

    The options currently supported are the following:
    \list

        \li \c{-platform} \e {platformName[:options]}, specifies the
            \l{Qt Platform Abstraction} (QPA) plugin.

            Overrides the \c QT_QPA_PLATFORM environment variable.
        \li \c{-platformpluginpath} \e path, specifies the path to platform
            plugins.

            Overrides the \c QT_QPA_PLATFORM_PLUGIN_PATH environment variable.

        \li \c{-platformtheme} \e platformTheme, specifies the platform theme.

            Overrides the \c QT_QPA_PLATFORMTHEME environment variable.

        \li \c{-plugin} \e plugin, specifies additional plugins to load. The argument
            may appear multiple times.

            Concatenated with the plugins in the \c QT_QPA_GENERIC_PLUGINS environment
            variable.

        \li \c{-qmljsdebugger=}, activates the QML/JS debugger with a specified port.
            The value must be of format \c{port:1234}\e{[,block]}, where
            \e block is optional
            and will make the application wait until a debugger connects to it.
        \li \c {-qwindowgeometry} \e geometry, specifies window geometry for
            the main window using the X11-syntax. For example:
            \c {-qwindowgeometry 100x100+50+50}
        \li \c {-qwindowicon}, sets the default window icon
        \li \c {-qwindowtitle}, sets the title of the first window
        \li \c{-reverse}, sets the application's layout direction to
            Qt::RightToLeft. This option is intended to aid debugging and should
            not be used in production. The default value is automatically detected
            from the user's locale (see also QLocale::textDirection()).
        \li \c{-session} \e session, restores the application from an earlier
            \l{Session Management}{session}.
    \endlist

    The following standard command line options are available for X11:

    \list
        \li \c {-display} \e {hostname:screen_number}, switches displays on X11.

             Overrides the \c DISPLAY environment variable.
        \li \c {-geometry} \e geometry, same as \c {-qwindowgeometry}.
    \endlist

    \section1 Platform-Specific Arguments

    You can specify platform-specific arguments for the \c{-platform} option.
    Place them after the platform plugin name following a colon as a
    comma-separated list. For example,
    \c{-platform windows:dialogs=xp,fontengine=freetype}.

    The following parameters are available for \c {-platform windows}:

    \list
        \li \c {altgr}, detect the key \c {AltGr} found on some keyboards as
               Qt::GroupSwitchModifier (since Qt 5.12).
        \li \c {darkmode=[0|1|2]} controls how Qt responds to the activation
               of the \e{Dark Mode for applications} introduced in Windows 10
               1903 (since Qt 5.15).

               A value of 0 disables dark mode support.

               A value of 1 causes Qt to switch the window borders to black
               when \e{Dark Mode for applications} is activated and no High
               Contrast Theme is in use. This is intended for applications
               that implement their own theming.

               A value of 2 will in addition cause the Windows Vista style to
               be deactivated and switch to the Windows style using a
               simplified palette in dark mode. This is currently
               experimental pending the introduction of new style that
               properly adapts to dark mode.

               As of Qt 6.5, the default value is 2; to disable dark mode
               support, set the value to 0 or 1.

        \li \c {dialogs=[xp|none]}, \c xp uses XP-style native dialogs and
            \c none disables them.

        \li \c {fontengine=freetype}, uses the FreeType font engine.
        \li \c {fontengine=gdi}, uses the legacy GDI-based
               font database and defaults to using the GDI font
               engine (which is otherwise only used for some font types
               or font properties.) (Since Qt 6.8).
        \li \c {menus=[native|none]}, controls the use of native menus.

               Native menus are implemented using Win32 API and are simpler than
               QMenu-based menus in for example that they do allow for placing
               widgets on them or changing properties like fonts and do not
               provide hover signals. They are mainly intended for Qt Quick.
               By default, they will be used if the application is not an
               instance of QApplication or for Qt Quick Controls 2
               applications (since Qt 5.10).

        \li \c {nocolorfonts} Turn off DirectWrite Color fonts
               (since Qt 5.8).

        \li \c {nodirectwrite} Turn off DirectWrite fonts (since Qt 5.8). This implicitly
               also selects the GDI font engine.

        \li \c {nomousefromtouch} Ignores mouse events synthesized
               from touch events by the operating system.

        \li \c {nowmpointer} Switches from Pointer Input Messages handling
               to legacy mouse handling (since Qt 5.12).
        \li \c {reverse} Activates Right-to-left mode (experimental).
               Windows title bars will be shown accordingly in Right-to-left locales
               (since Qt 5.13).
        \li \c {tabletabsoluterange=<value>} Sets a value for mouse mode detection
               of WinTab tablets (Legacy, since Qt 5.3).
    \endlist

    The following parameter is available for \c {-platform cocoa} (on macOS):

    \list
        \li \c {fontengine=freetype}, uses the FreeType font engine.
    \endlist

    For more information about the platform-specific arguments available for
    embedded Linux platforms, see \l{Qt for Embedded Linux}.

    \sa arguments(), QGuiApplication::platformName
*/
#ifdef Q_QDOC
QGuiApplication::QGuiApplication(int &argc, char **argv)
#else
QGuiApplication::QGuiApplication(int &argc, char **argv, int)
#endif
    : QCoreApplication(*new QGuiApplicationPrivate(argc, argv))
{
    d_func()->init();

    QCoreApplicationPrivate::eventDispatcher->startingUp();
}

/*!
    \internal
*/
QGuiApplication::QGuiApplication(QGuiApplicationPrivate &p)
    : QCoreApplication(p)
{
}

/*!
    Destructs the application.
*/
QGuiApplication::~QGuiApplication()
{
    Q_D(QGuiApplication);

    qt_call_post_routines();

    d->eventDispatcher->closingDown();
    d->eventDispatcher = nullptr;

#ifndef QT_NO_CLIPBOARD
    delete QGuiApplicationPrivate::qt_clipboard;
    QGuiApplicationPrivate::qt_clipboard = nullptr;
#endif

#ifndef QT_NO_SESSIONMANAGER
    delete d->session_manager;
    d->session_manager = nullptr;
#endif //QT_NO_SESSIONMANAGER

    QGuiApplicationPrivate::clearPalette();
    QFontDatabase::removeAllApplicationFonts();

#ifndef QT_NO_CURSOR
    d->cursor_list.clear();
#endif

#if QT_CONFIG(qtgui_threadpool)
    // Synchronize and stop the gui thread pool threads.
    QThreadPool *guiThreadPool = nullptr;
    QT_TRY {
        guiThreadPool = QGuiApplicationPrivate::qtGuiThreadPool();
    } QT_CATCH (...) {
        // swallow the exception, since destructors shouldn't throw
    }
    if (guiThreadPool) {
        guiThreadPool->waitForDone();
        delete guiThreadPool;
    }
#endif

    delete QGuiApplicationPrivate::app_icon;
    QGuiApplicationPrivate::app_icon = nullptr;
    delete QGuiApplicationPrivate::platform_name;
    QGuiApplicationPrivate::platform_name = nullptr;
    delete QGuiApplicationPrivate::displayName;
    QGuiApplicationPrivate::displayName = nullptr;
    delete QGuiApplicationPrivate::m_inputDeviceManager;
    QGuiApplicationPrivate::m_inputDeviceManager = nullptr;
    delete QGuiApplicationPrivate::desktopFileName;
    QGuiApplicationPrivate::desktopFileName = nullptr;
    QGuiApplicationPrivate::mouse_buttons = Qt::NoButton;
    QGuiApplicationPrivate::modifier_buttons = Qt::NoModifier;
    QGuiApplicationPrivate::lastCursorPosition.reset();
    QGuiApplicationPrivate::currentMousePressWindow = QGuiApplicationPrivate::currentMouseWindow = nullptr;
    QGuiApplicationPrivate::applicationState = Qt::ApplicationInactive;
    QGuiApplicationPrivate::highDpiScaleFactorRoundingPolicy = Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
    QGuiApplicationPrivate::currentDragWindow = nullptr;
    QGuiApplicationPrivate::tabletDevicePoints.clear();
}

QGuiApplicationPrivate::QGuiApplicationPrivate(int &argc, char **argv)
    : QCoreApplicationPrivate(argc, argv),
      inputMethod(nullptr),
      lastTouchType(QEvent::TouchEnd)
{
    self = this;
    application_type = QCoreApplicationPrivate::Gui;
#ifndef QT_NO_SESSIONMANAGER
    is_session_restored = false;
    is_saving_session = false;
#endif
}

/*!
    \property QGuiApplication::applicationDisplayName
    \brief the user-visible name of this application
    \since 5.0

    This name is shown to the user, for instance in window titles.
    It can be translated, if necessary.

    If not set, the application display name defaults to the application name.

    \sa applicationName
*/
void QGuiApplication::setApplicationDisplayName(const QString &name)
{
    if (!QGuiApplicationPrivate::displayName) {
        QGuiApplicationPrivate::displayName = new QString(name);
        if (qGuiApp) {
            disconnect(qGuiApp, &QGuiApplication::applicationNameChanged,
                    qGuiApp, &QGuiApplication::applicationDisplayNameChanged);

            if (*QGuiApplicationPrivate::displayName != applicationName())
                emit qGuiApp->applicationDisplayNameChanged();
        }
    } else if (name != *QGuiApplicationPrivate::displayName) {
        *QGuiApplicationPrivate::displayName = name;
        if (qGuiApp)
            emit qGuiApp->applicationDisplayNameChanged();
    }
}

QString QGuiApplication::applicationDisplayName()
{
    return QGuiApplicationPrivate::displayName ? *QGuiApplicationPrivate::displayName : applicationName();
}

/*!
    Sets the application's badge to \a number.

    Useful for providing feedback to the user about the number
    of unread messages or similar.

    The badge will be overlaid on the application's icon in the Dock
    on \macos, the home screen icon on iOS, or the task bar on Windows
    and Linux.

    If the number is outside the range supported by the platform, the
    number will be clamped to the supported range. If the number does
    not fit within the badge, the number may be visually elided.

    Setting the number to 0 will clear the badge.

    \since 6.5
    \sa applicationName
*/
void QGuiApplication::setBadgeNumber(qint64 number)
{
    QGuiApplicationPrivate::platformIntegration()->setApplicationBadge(number);
}

/*!
    \property QGuiApplication::desktopFileName
    \brief the base name of the desktop entry for this application
    \since 5.7

    This is the file name, without the full path or the trailing ".desktop"
    extension of the desktop entry that represents this application
    according to the freedesktop desktop entry specification.

    This property gives a precise indication of what desktop entry represents
    the application and it is needed by the windowing system to retrieve
    such information without resorting to imprecise heuristics.

    The latest version of the freedesktop desktop entry specification can be obtained
    \l{http://standards.freedesktop.org/desktop-entry-spec/latest/}{here}.
*/
void QGuiApplication::setDesktopFileName(const QString &name)
{
    if (!QGuiApplicationPrivate::desktopFileName)
        QGuiApplicationPrivate::desktopFileName = new QString;
    *QGuiApplicationPrivate::desktopFileName = name;
    if (name.endsWith(QLatin1String(".desktop"))) { // ### Qt 7: remove
        const QString filePath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, name);
        if (!filePath.isEmpty()) {
            qWarning("QGuiApplication::setDesktopFileName: the specified desktop file name "
                     "ends with .desktop. For compatibility reasons, the .desktop suffix will "
                     "be removed. Please specify a desktop file name without .desktop suffix");
            (*QGuiApplicationPrivate::desktopFileName).chop(8);
        }
    }
}

QString QGuiApplication::desktopFileName()
{
    return QGuiApplicationPrivate::desktopFileName ? *QGuiApplicationPrivate::desktopFileName : QString();
}

/*!
    Returns the most recently shown modal window. If no modal windows are
    visible, this function returns zero.

    A modal window is a window which has its
    \l{QWindow::modality}{modality} property set to Qt::WindowModal
    or Qt::ApplicationModal. A modal window must be closed before the user can
    continue with other parts of the program.

    Modal window are organized in a stack. This function returns the modal
    window at the top of the stack.

    \sa Qt::WindowModality, QWindow::setModality()
*/
QWindow *QGuiApplication::modalWindow()
{
    CHECK_QAPP_INSTANCE(nullptr)
    if (QGuiApplicationPrivate::self->modalWindowList.isEmpty())
        return nullptr;
    return QGuiApplicationPrivate::self->modalWindowList.constFirst();
}

static void updateBlockedStatusRecursion(QWindow *window, bool shouldBeBlocked)
{
    QWindowPrivate *p = qt_window_private(window);
    if (p->blockedByModalWindow != shouldBeBlocked) {
        p->blockedByModalWindow = shouldBeBlocked;
        QEvent e(shouldBeBlocked ? QEvent::WindowBlocked : QEvent::WindowUnblocked);
        QGuiApplication::sendEvent(window, &e);
        for (QObject *c : window->children()) {
            if (c->isWindowType())
                updateBlockedStatusRecursion(static_cast<QWindow *>(c), shouldBeBlocked);
        }
    }
}

void QGuiApplicationPrivate::updateBlockedStatus(QWindow *window)
{
    bool shouldBeBlocked = false;
    const bool popupType = (window->type() == Qt::ToolTip) || (window->type() == Qt::Popup);
    if (!popupType && !self->modalWindowList.isEmpty())
        shouldBeBlocked = self->isWindowBlocked(window);
    updateBlockedStatusRecursion(window, shouldBeBlocked);
}

// Return whether the window needs to be notified about window blocked events.
// As opposed to QGuiApplication::topLevelWindows(), embedded windows are
// included in this list (QTBUG-18099).
static inline bool needsWindowBlockedEvent(const QWindow *w)
{
    return w->isTopLevel() && w->type() != Qt::Desktop;
}

void QGuiApplicationPrivate::showModalWindow(QWindow *modal)
{
    self->modalWindowList.prepend(modal);

    // Send leave for currently entered window if it should be blocked
    if (currentMouseWindow && !QWindowPrivate::get(currentMouseWindow)->isPopup()) {
        bool shouldBeBlocked = self->isWindowBlocked(currentMouseWindow);
        if (shouldBeBlocked) {
            // Remove the new window from modalWindowList temporarily so leave can go through
            self->modalWindowList.removeFirst();
            QEvent e(QEvent::Leave);
            QGuiApplication::sendEvent(currentMouseWindow, &e);
            currentMouseWindow = nullptr;
            self->modalWindowList.prepend(modal);
        }
    }

    for (QWindow *window : std::as_const(QGuiApplicationPrivate::window_list)) {
        if (needsWindowBlockedEvent(window) && !window->d_func()->blockedByModalWindow)
            updateBlockedStatus(window);
    }

    updateBlockedStatus(modal);
}

void QGuiApplicationPrivate::hideModalWindow(QWindow *window)
{
    self->modalWindowList.removeAll(window);

    for (QWindow *window : std::as_const(QGuiApplicationPrivate::window_list)) {
        if (needsWindowBlockedEvent(window) && window->d_func()->blockedByModalWindow)
            updateBlockedStatus(window);
    }
}

Qt::WindowModality QGuiApplicationPrivate::defaultModality() const
{
    return Qt::NonModal;
}

bool QGuiApplicationPrivate::windowNeverBlocked(QWindow *window) const
{
    Q_UNUSED(window);
    return false;
}

/*
    Returns \c true if \a window is blocked by a modal window. If \a
    blockingWindow is non-zero, *blockingWindow will be set to the blocking
    window (or to zero if \a window is not blocked).
*/
bool QGuiApplicationPrivate::isWindowBlocked(QWindow *window, QWindow **blockingWindow) const
{
    Q_ASSERT_X(window, Q_FUNC_INFO, "The window must not be null");

    QWindow *unused = nullptr;
    if (!blockingWindow)
        blockingWindow = &unused;
    *blockingWindow = nullptr;

    if (modalWindowList.isEmpty() || windowNeverBlocked(window))
        return false;

    for (int i = 0; i < modalWindowList.size(); ++i) {
        QWindow *modalWindow = modalWindowList.at(i);

        // A window is not blocked by another modal window if the two are
        // the same, or if the window is a child of the modal window.
        if (window == modalWindow || modalWindow->isAncestorOf(window, QWindow::IncludeTransients))
            return false;

        switch (modalWindow->modality() == Qt::NonModal ? defaultModality()
                                                        : modalWindow->modality()) {
        case Qt::ApplicationModal:
            *blockingWindow = modalWindow;
            return true;
        case Qt::WindowModal: {
            // Find the nearest ancestor of window which is also an ancestor of modal window to
            // determine if the modal window blocks the window.
            auto *current = window;
            do {
                if (current->isAncestorOf(modalWindow, QWindow::IncludeTransients)) {
                    *blockingWindow = modalWindow;
                    return true;
                }
                current = current->parent(QWindow::IncludeTransients);
            } while (current);
            break;
        }
        default:
            Q_ASSERT_X(false, "QGuiApplication", "internal error, a modal widget cannot be modeless");
            break;
        }
    }
    return false;
}

QWindow *QGuiApplicationPrivate::activePopupWindow()
{
    // might be the same as focusWindow() if that's a popup
    return QGuiApplicationPrivate::popup_list.isEmpty() ?
            nullptr : QGuiApplicationPrivate::popup_list.constLast();
}

void QGuiApplicationPrivate::activatePopup(QWindow *popup)
{
    if (!popup->isVisible())
        return;
    popup_list.removeOne(popup); // ensure that there's only one entry, and it's the last
    qCDebug(lcPopup) << "appending popup" << popup << "to existing" << popup_list;
    popup_list.append(popup);
}

bool QGuiApplicationPrivate::closePopup(QWindow *popup)
{
    const auto removed = QGuiApplicationPrivate::popup_list.removeAll(popup);
    qCDebug(lcPopup) << "removed?" << removed << "popup" << popup << "; remaining" << popup_list;
    return removed; // >= 1 if something was removed
}

/*!
    Returns \c true if there are no more open popups.
*/
bool QGuiApplicationPrivate::closeAllPopups()
{
    // Close all popups: In case some popup refuses to close,
    // we give up after 1024 attempts (to avoid an infinite loop).
    int maxiter = 1024;
    QWindow *popup;
    while ((popup = activePopupWindow()) && maxiter--)
        popup->close(); // this will call QApplicationPrivate::closePopup
    return QGuiApplicationPrivate::popup_list.isEmpty();
}

/*!
    Returns the QWindow that receives events tied to focus,
    such as key events.

    \sa QWindow::requestActivate()
*/
QWindow *QGuiApplication::focusWindow()
{
    return QGuiApplicationPrivate::focus_window;
}

/*!
    \fn QGuiApplication::focusObjectChanged(QObject *focusObject)

    This signal is emitted when final receiver of events tied to focus is changed.
    \a focusObject is the new receiver.

    \sa focusObject()
*/

/*!
    \fn QGuiApplication::focusWindowChanged(QWindow *focusWindow)

    This signal is emitted when the focused window changes.
    \a focusWindow is the new focused window.

    \sa focusWindow()
*/

/*!
    Returns the QObject in currently active window that will be final receiver of events
    tied to focus, such as key events.
 */
QObject *QGuiApplication::focusObject()
{
    if (focusWindow())
        return focusWindow()->focusObject();
    return nullptr;
}

/*!
    \fn QGuiApplication::allWindows()

    Returns a list of all the windows in the application.

    The list is empty if there are no windows.

    \sa topLevelWindows()
 */
QWindowList QGuiApplication::allWindows()
{
    return QGuiApplicationPrivate::window_list;
}

/*!
    \fn QGuiApplication::topLevelWindows()

    Returns a list of the top-level windows in the application.

    \sa allWindows()
 */
QWindowList QGuiApplication::topLevelWindows()
{
    const QWindowList &list = QGuiApplicationPrivate::window_list;
    QWindowList topLevelWindows;
    for (int i = 0; i < list.size(); ++i) {
        QWindow *window = list.at(i);
        if (!window->isTopLevel())
            continue;

        // Desktop windows are special, as each individual desktop window
        // will report that it's a top level window, but we don't want to
        // include them in the application wide list of top level windows.
        if (window->type() == Qt::Desktop)
            continue;

        // Windows embedded in native windows do not have QWindow parents,
        // but they are not true top level windows, so do not include them.
        if (window->handle() && window->handle()->isEmbedded())
            continue;

        topLevelWindows.prepend(window);
    }

    return topLevelWindows;
}

QScreen *QGuiApplication::primaryScreen()
{
    if (QGuiApplicationPrivate::screen_list.isEmpty())
        return nullptr;
    return QGuiApplicationPrivate::screen_list.at(0);
}

/*!
    Returns a list of all the screens associated with the
    windowing system the application is connected to.
*/
QList<QScreen *> QGuiApplication::screens()
{
    return QGuiApplicationPrivate::screen_list;
}

/*!
    Returns the screen at \a point, or \nullptr if outside of any screen.

    The \a point is in relation to the virtualGeometry() of each set of virtual
    siblings. If the point maps to more than one set of virtual siblings the first
    match is returned.  If you wish to search only the virtual desktop siblings
    of a known screen (for example siblings of the screen of your application
    window \c QWidget::windowHandle()->screen()), use QScreen::virtualSiblingAt().

    \since 5.10
*/
QScreen *QGuiApplication::screenAt(const QPoint &point)
{
    QVarLengthArray<const QScreen *, 8> visitedScreens;
    for (const QScreen *screen : QGuiApplication::screens()) {
        if (visitedScreens.contains(screen))
            continue;

        // The virtual siblings include the screen itself, so iterate directly
        for (QScreen *sibling : screen->virtualSiblings()) {
            if (sibling->geometry().contains(point))
                return sibling;

            visitedScreens.append(sibling);
        }
    }

    return nullptr;
}

/*!
    \fn void QGuiApplication::screenAdded(QScreen *screen)

    This signal is emitted whenever a new screen \a screen has been added to the system.

    \sa screens(), primaryScreen, screenRemoved()
*/

/*!
    \fn void QGuiApplication::screenRemoved(QScreen *screen)

    This signal is emitted whenever a \a screen is removed from the system. It
    provides an opportunity to manage the windows on the screen before Qt falls back
    to moving them to the primary screen.

    \sa screens(), screenAdded(), QObject::destroyed(), QWindow::setScreen()

    \since 5.4
*/


/*!
    \property QGuiApplication::primaryScreen

    \brief the primary (or default) screen of the application.

    This will be the screen where QWindows are initially shown, unless otherwise specified.

    The primaryScreenChanged signal was introduced in Qt 5.6.

    \sa screens()
*/

/*!
    Returns the highest screen device pixel ratio found on
    the system. This is the ratio between physical pixels and
    device-independent pixels.

    Use this function only when you don't know which window you are targeting.
    If you do know the target window, use QWindow::devicePixelRatio() instead.

    \sa QWindow::devicePixelRatio()
*/
qreal QGuiApplication::devicePixelRatio() const
{
    if (!qFuzzyIsNull(QGuiApplicationPrivate::m_maxDevicePixelRatio))
        return QGuiApplicationPrivate::m_maxDevicePixelRatio;

    QGuiApplicationPrivate::m_maxDevicePixelRatio = 1.0; // make sure we never return 0.
    for (QScreen *screen : std::as_const(QGuiApplicationPrivate::screen_list))
        QGuiApplicationPrivate::m_maxDevicePixelRatio = qMax(QGuiApplicationPrivate::m_maxDevicePixelRatio, screen->devicePixelRatio());

    return QGuiApplicationPrivate::m_maxDevicePixelRatio;
}

void QGuiApplicationPrivate::resetCachedDevicePixelRatio()
{
    m_maxDevicePixelRatio = 0.0;
}

/*!
    Returns the top level window at the given position \a pos, if any.
*/
QWindow *QGuiApplication::topLevelAt(const QPoint &pos)
{
    if (QScreen *windowScreen = screenAt(pos)) {
        const QPoint devicePosition = QHighDpi::toNativePixels(pos, windowScreen);
        return windowScreen->handle()->topLevelAt(devicePosition);
    }
    return nullptr;
}

/*!
    \property QGuiApplication::platformName
    \brief The name of the underlying platform plugin.

    The QPA platform plugins are located in \c {qtbase\src\plugins\platforms}.
    At the time of writing, the following platform plugin names are supported:

    \list
        \li \c android
        \li \c cocoa is a platform plugin for \macos.
        \li \c directfb
        \li \c eglfs is a platform plugin for running Qt5 applications on top of
            EGL and  OpenGL ES 2.0 without an actual windowing system (like X11
            or Wayland). For more information, see \l{EGLFS}.
        \li \c ios (also used for tvOS)
        \li \c linuxfb writes directly to the framebuffer. For more information,
            see \l{LinuxFB}.
        \li \c minimal is provided as an examples for developers who want to
            write their own platform plugins. However, you can use the plugin to
            run GUI applications in environments without a GUI, such as servers.
        \li \c minimalegl is an example plugin.
        \li \c offscreen
        \li \c qnx
        \li \c windows
        \li \c wayland is a platform plugin for the Wayland display server protocol,
            used on some Linux desktops and embedded systems.
        \li \c xcb is a plugin for the X11 window system, used on some desktop Linux platforms.
    \endlist

    \note Calling this function without a QGuiApplication will return the default
    platform name, if available. The default platform name is not affected by the
    \c{-platform} command line option, or the \c QT_QPA_PLATFORM environment variable.

    For more information about the platform plugins for embedded Linux devices,
    see \l{Qt for Embedded Linux}.
*/

QString QGuiApplication::platformName()
{
    if (!QGuiApplication::instance()) {
#ifdef QT_QPA_DEFAULT_PLATFORM_NAME
        return QStringLiteral(QT_QPA_DEFAULT_PLATFORM_NAME);
#else
        return QString();
#endif
    } else {
        return QGuiApplicationPrivate::platform_name ?
            *QGuiApplicationPrivate::platform_name : QString();
    }
}

Q_STATIC_LOGGING_CATEGORY(lcQpaPluginLoading, "qt.qpa.plugin");
Q_STATIC_LOGGING_CATEGORY(lcQpaTheme, "qt.qpa.theme");
Q_STATIC_LOGGING_CATEGORY(lcPtrDispatch, "qt.pointer.dispatch");

static void init_platform(const QString &pluginNamesWithArguments, const QString &platformPluginPath, const QString &platformThemeName, int &argc, char **argv)
{
    qCDebug(lcQpaPluginLoading) << "init_platform called with"
        << "pluginNamesWithArguments" << pluginNamesWithArguments
        << "platformPluginPath" << platformPluginPath
        << "platformThemeName" << platformThemeName;

    QStringList plugins = pluginNamesWithArguments.split(u';', Qt::SkipEmptyParts);
    QStringList platformArguments;
    QStringList availablePlugins = QPlatformIntegrationFactory::keys(platformPluginPath);
    for (const auto &pluginArgument : std::as_const(plugins)) {
        // Split into platform name and arguments
        QStringList arguments = pluginArgument.split(u':', Qt::SkipEmptyParts);
        if (arguments.isEmpty())
            continue;
        const QString name = arguments.takeFirst().toLower();
        QString argumentsKey = name;
        if (name.isEmpty())
            continue;
        argumentsKey[0] = argumentsKey.at(0).toUpper();
        arguments.append(QLibraryInfo::platformPluginArguments(argumentsKey));

        qCDebug(lcQpaPluginLoading) << "Attempting to load Qt platform plugin" << name << "with arguments" << arguments;

        // Create the platform integration.
        QGuiApplicationPrivate::platform_integration = QPlatformIntegrationFactory::create(name, arguments, argc, argv, platformPluginPath);
        if (Q_UNLIKELY(!QGuiApplicationPrivate::platform_integration)) {
            if (availablePlugins.contains(name)) {
                if (name == QStringLiteral("xcb") && QVersionNumber::compare(QLibraryInfo::version(), QVersionNumber(6, 5, 0)) >= 0) {
                    qCWarning(lcQpaPluginLoading).nospace().noquote()
                            << "From 6.5.0, xcb-cursor0 or libxcb-cursor0 is needed to load the Qt xcb platform plugin.";
                }
                qCInfo(lcQpaPluginLoading).nospace().noquote()
                        << "Could not load the Qt platform plugin \"" << name << "\" in \""
                        << QDir::toNativeSeparators(platformPluginPath) << "\" even though it was found.";
            } else {
                qCWarning(lcQpaPluginLoading).nospace().noquote()
                        << "Could not find the Qt platform plugin \"" << name << "\" in \""
                        << QDir::toNativeSeparators(platformPluginPath) << "\"";
            }
        } else {
            qCDebug(lcQpaPluginLoading) << "Successfully loaded Qt platform plugin" << name;
            QGuiApplicationPrivate::platform_name = new QString(name);
            platformArguments = arguments;
            break;
        }
    }

    if (Q_UNLIKELY(!QGuiApplicationPrivate::platform_integration)) {
        QString fatalMessage = QStringLiteral("This application failed to start because no Qt platform plugin could be initialized. "
                                              "Reinstalling the application may fix this problem.\n");

        if (!availablePlugins.isEmpty())
            fatalMessage += "\nAvailable platform plugins are: %1.\n"_L1.arg(availablePlugins.join(", "_L1));

#if defined(Q_OS_WIN)
        // Windows: Display message box unless it is a console application
        // or debug build showing an assert box.
        if (!QLibraryInfo::isDebugBuild() && !GetConsoleWindow())
            MessageBox(0, (LPCTSTR)fatalMessage.utf16(), (LPCTSTR)(QCoreApplication::applicationName().utf16()), MB_OK | MB_ICONERROR);
#endif // Q_OS_WIN
        qFatal("%s", qPrintable(fatalMessage));

        return;
    }

    // Create the platform theme:

    // 1) Try the platform name from the environment if present
    QStringList themeNames;
    if (!platformThemeName.isEmpty()) {
        qCDebug(lcQpaTheme) << "Adding" << platformThemeName << "from environment";
        themeNames.append(platformThemeName);
    }

    // 2) Special case - check whether it's a flatpak or snap app to use xdg-desktop-portal platform theme for portals support
    if (checkNeedPortalSupport()) {
        qCDebug(lcQpaTheme) << "Adding xdgdesktopportal to list of theme names";
        themeNames.append(QStringLiteral("xdgdesktopportal"));
    }

    // 3) Ask the platform integration for a list of theme names
    const auto platformIntegrationThemeNames = QGuiApplicationPrivate::platform_integration->themeNames();
    qCDebug(lcQpaTheme) << "Adding platform integration's theme names to list of theme names:" << platformIntegrationThemeNames;
    themeNames.append(platformIntegrationThemeNames);

    // 4) Look for a theme plugin.
    for (const QString &themeName : std::as_const(themeNames)) {
        qCDebug(lcQpaTheme) << "Attempting to create platform theme" << themeName << "via QPlatformThemeFactory::create";
        QGuiApplicationPrivate::platform_theme = QPlatformThemeFactory::create(themeName, platformPluginPath);
        if (QGuiApplicationPrivate::platform_theme) {
            qCDebug(lcQpaTheme) << "Successfully created platform theme" << themeName << "via QPlatformThemeFactory::create";
            break;
        }
        qCDebug(lcQpaTheme) << "Attempting to create platform theme" << themeName << "via createPlatformTheme";
        QGuiApplicationPrivate::platform_theme = QGuiApplicationPrivate::platform_integration->createPlatformTheme(themeName);
        if (QGuiApplicationPrivate::platform_theme) {
            qCDebug(lcQpaTheme) << "Successfully created platform theme" << themeName << "via createPlatformTheme";
            break;
        }
    }

    // 5) Fall back on the built-in "null" platform theme.
    if (!QGuiApplicationPrivate::platform_theme) {
        qCDebug(lcQpaTheme) << "Failed to create platform theme; using \"null\" platform theme";
        QGuiApplicationPrivate::platform_theme = new QPlatformTheme;
    }

    // Set arguments as dynamic properties on the native interface as
    // boolean 'foo' or strings: 'foo=bar'
    if (!platformArguments.isEmpty()) {
        if (QObject *nativeInterface = QGuiApplicationPrivate::platform_integration->nativeInterface()) {
            for (const QString &argument : std::as_const(platformArguments)) {
                const qsizetype equalsPos = argument.indexOf(u'=');
                const QByteArray name =
                    equalsPos != -1 ? argument.left(equalsPos).toUtf8() : argument.toUtf8();
                QVariant value =
                    equalsPos != -1 ? QVariant(argument.mid(equalsPos + 1)) : QVariant(true);
                nativeInterface->setProperty(name.constData(), std::move(value));
            }
        }
    }

    const auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    fontSmoothingGamma = platformIntegration->styleHint(QPlatformIntegration::FontSmoothingGamma).toReal();
    QCoreApplication::setAttribute(Qt::AA_DontShowShortcutsInContextMenus,
        !QGuiApplication::styleHints()->showShortcutsInContextMenus());

    if (const auto *platformTheme = QGuiApplicationPrivate::platformTheme()) {
        QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus,
            !platformTheme->themeHint(QPlatformTheme::ShowIconsInMenus).toBool());
    }
}

static void init_plugins(const QList<QByteArray> &pluginList)
{
    for (int i = 0; i < pluginList.size(); ++i) {
        QByteArray pluginSpec = pluginList.at(i);
        qsizetype colonPos = pluginSpec.indexOf(':');
        QObject *plugin;
        if (colonPos < 0)
            plugin = QGenericPluginFactory::create(QLatin1StringView(pluginSpec), QString());
        else
            plugin = QGenericPluginFactory::create(QLatin1StringView(pluginSpec.mid(0, colonPos)),
                                                   QLatin1StringView(pluginSpec.mid(colonPos+1)));
        if (plugin)
            QGuiApplicationPrivate::generic_plugin_list.append(plugin);
        else
            qWarning("No such plugin for spec \"%s\"", pluginSpec.constData());
    }
}

#if QT_CONFIG(commandlineparser)
void QGuiApplicationPrivate::addQtOptions(QList<QCommandLineOption> *options)
{
    QCoreApplicationPrivate::addQtOptions(options);

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    const QByteArray sessionType = qgetenv("XDG_SESSION_TYPE");
    const bool x11 = sessionType == "x11";
    // Technically the x11 aliases are only available if platformName is "xcb", but we can't know that here.
#else
    const bool x11 = false;
#endif

    options->append(QCommandLineOption(QStringLiteral("platform"),
                    QGuiApplication::tr("QPA plugin. See QGuiApplication documentation for available options for each plugin."), QStringLiteral("platformName[:options]")));
    options->append(QCommandLineOption(QStringLiteral("platformpluginpath"),
                    QGuiApplication::tr("Path to the platform plugins."), QStringLiteral("path")));
    options->append(QCommandLineOption(QStringLiteral("platformtheme"),
                    QGuiApplication::tr("Platform theme."), QStringLiteral("theme")));
    options->append(QCommandLineOption(QStringLiteral("plugin"),
                    QGuiApplication::tr("Additional plugins to load, can be specified multiple times."), QStringLiteral("plugin")));
    options->append(QCommandLineOption(QStringLiteral("qwindowgeometry"),
                    QGuiApplication::tr("Window geometry for the main window, using the X11-syntax, like 100x100+50+50."), QStringLiteral("geometry")));
    options->append(QCommandLineOption(QStringLiteral("qwindowicon"),
                    QGuiApplication::tr("Default window icon."), QStringLiteral("icon")));
    options->append(QCommandLineOption(QStringLiteral("qwindowtitle"),
                    QGuiApplication::tr("Title of the first window."), QStringLiteral("title")));
    options->append(QCommandLineOption(QStringLiteral("reverse"),
                    QGuiApplication::tr("Sets the application's layout direction to Qt::RightToLeft (debugging helper).")));
    options->append(QCommandLineOption(QStringLiteral("session"),
                    QGuiApplication::tr("Restores the application from an earlier session."), QStringLiteral("session")));

    if (x11) {
         options->append(QCommandLineOption(QStringLiteral("display"),
                         QGuiApplication::tr("Display name, overrides $DISPLAY."), QStringLiteral("display")));
         options->append(QCommandLineOption(QStringLiteral("name"),
                         QGuiApplication::tr("Instance name according to ICCCM 4.1.2.5."), QStringLiteral("name")));
         options->append(QCommandLineOption(QStringLiteral("nograb"),
                         QGuiApplication::tr("Disable mouse grabbing (useful in debuggers).")));
         options->append(QCommandLineOption(QStringLiteral("dograb"),
                         QGuiApplication::tr("Force mouse grabbing (even when running in a debugger).")));
         options->append(QCommandLineOption(QStringLiteral("visual"),
                         QGuiApplication::tr("ID of the X11 Visual to use."), QStringLiteral("id")));
         // Not using the "QStringList names" solution for those aliases, because it makes the first column too wide
         options->append(QCommandLineOption(QStringLiteral("geometry"),
                         QGuiApplication::tr("Alias for --qwindowgeometry."), QStringLiteral("geometry")));
         options->append(QCommandLineOption(QStringLiteral("icon"),
                         QGuiApplication::tr("Alias for --qwindowicon."), QStringLiteral("icon")));
         options->append(QCommandLineOption(QStringLiteral("title"),
                         QGuiApplication::tr("Alias for --qwindowtitle."), QStringLiteral("title")));
    }
}
#endif // QT_CONFIG(commandlineparser)

void QGuiApplicationPrivate::createPlatformIntegration()
{
    QHighDpiScaling::initHighDpiScaling();

    // Load the platform integration
    QString platformPluginPath = qEnvironmentVariable("QT_QPA_PLATFORM_PLUGIN_PATH");


    QByteArray platformName;
#ifdef QT_QPA_DEFAULT_PLATFORM_NAME
    platformName = QT_QPA_DEFAULT_PLATFORM_NAME;
#endif
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    QList<QByteArray> platformArguments = platformName.split(':');
    QByteArray platformPluginBase = platformArguments.first();

    const bool hasWaylandDisplay = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");
    const bool isWaylandSessionType = qgetenv("XDG_SESSION_TYPE") == "wayland";

    QVector<QByteArray> preferredPlatformOrder;
    const bool defaultIsXcb = platformPluginBase == "xcb";
    const QByteArray xcbPlatformName = defaultIsXcb ? platformName : "xcb";
    if (qEnvironmentVariableIsSet("DISPLAY")) {
        preferredPlatformOrder << xcbPlatformName;
        if (defaultIsXcb)
            platformName.clear();
    }

    const bool defaultIsWayland = !defaultIsXcb && platformPluginBase.startsWith("wayland");
    const QByteArray waylandPlatformName = defaultIsWayland ? platformName : "wayland";
    if (hasWaylandDisplay || isWaylandSessionType) {
        preferredPlatformOrder.prepend(waylandPlatformName);

        if (defaultIsWayland)
            platformName.clear();
    }

    if (!platformName.isEmpty())
        preferredPlatformOrder.append(platformName);

    platformName = preferredPlatformOrder.join(';');
#endif

    bool platformExplicitlySelected = false;
    QByteArray platformNameEnv = qgetenv("QT_QPA_PLATFORM");
    if (!platformNameEnv.isEmpty()) {
        platformName = platformNameEnv;
        platformExplicitlySelected = true;
    }

    QString platformThemeName = QString::fromLocal8Bit(qgetenv("QT_QPA_PLATFORMTHEME"));

    // Get command line params

    QString icon;

    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (!argv[i])
            continue;
        if (*argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        const bool xcbIsDefault = platformName.startsWith("xcb");
        const char *arg = argv[i];
        if (arg[1] == '-') // startsWith("--")
            ++arg;
        if (strcmp(arg, "-platformpluginpath") == 0) {
            if (++i < argc)
                platformPluginPath = QFile::decodeName(argv[i]);
        } else if (strcmp(arg, "-platform") == 0) {
            if (++i < argc) {
                platformExplicitlySelected = true;
                platformName = argv[i];
            }
        } else if (strcmp(arg, "-platformtheme") == 0) {
            if (++i < argc)
                platformThemeName = QString::fromLocal8Bit(argv[i]);
        } else if (strcmp(arg, "-qwindowgeometry") == 0 || (xcbIsDefault && strcmp(arg, "-geometry") == 0)) {
            if (++i < argc)
                windowGeometrySpecification = QWindowGeometrySpecification::fromArgument(argv[i]);
        } else if (strcmp(arg, "-qwindowtitle") == 0 || (xcbIsDefault && strcmp(arg, "-title") == 0)) {
            if (++i < argc)
                firstWindowTitle = QString::fromLocal8Bit(argv[i]);
        } else if (strcmp(arg, "-qwindowicon") == 0 || (xcbIsDefault && strcmp(arg, "-icon") == 0)) {
            if (++i < argc) {
                icon = QFile::decodeName(argv[i]);
            }
        } else {
            argv[j++] = argv[i];
        }
    }

    if (j < argc) {
        argv[j] = nullptr;
        argc = j;
    }

    Q_UNUSED(platformExplicitlySelected);

    init_platform(QLatin1StringView(platformName), platformPluginPath, platformThemeName, argc, argv);
    QStyleHintsPrivate::get(QGuiApplication::styleHints())->update(platformTheme());

    if (!icon.isEmpty())
        forcedWindowIcon = QDir::isAbsolutePath(icon) ? QIcon(icon) : QIcon::fromTheme(icon);
}

/*!
    Called from QCoreApplication::init()

    Responsible for creating an event dispatcher when QCoreApplication
    decides that it needs one (because a custom one has not been set).
*/
void QGuiApplicationPrivate::createEventDispatcher()
{
    Q_ASSERT(!eventDispatcher);

    if (platform_integration == nullptr)
        createPlatformIntegration();

    // The platform integration should not result in creating an event dispatcher
    Q_ASSERT_X(!threadData.loadRelaxed()->eventDispatcher, "QGuiApplication",
        "Creating the platform integration resulted in creating an event dispatcher");

    // Nor should it mess with the QCoreApplication's event dispatcher
    Q_ASSERT(!eventDispatcher);

    eventDispatcher = platform_integration->createEventDispatcher();
}

void QGuiApplicationPrivate::eventDispatcherReady()
{
    if (platform_integration == nullptr)
        createPlatformIntegration();

    platform_integration->initialize();
}

void Q_TRACE_INSTRUMENT(qtgui) QGuiApplicationPrivate::init()
{
    Q_TRACE_SCOPE(QGuiApplicationPrivate_init);

#if defined(Q_OS_MACOS)
    QMacAutoReleasePool pool;
#endif

    QCoreApplicationPrivate::init();

    QCoreApplicationPrivate::is_app_running = false; // Starting up.

    bool loadTestability = false;
    QList<QByteArray> pluginList;
    // Get command line params
#ifndef QT_NO_SESSIONMANAGER
    QString session_id;
    QString session_key;
# if defined(Q_OS_WIN)
    wchar_t guidstr[40];
    GUID guid;
    CoCreateGuid(&guid);
    StringFromGUID2(guid, guidstr, 40);
    session_id = QString::fromWCharArray(guidstr);
    CoCreateGuid(&guid);
    StringFromGUID2(guid, guidstr, 40);
    session_key = QString::fromWCharArray(guidstr);
# endif
#endif
    QString s;
    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (!argv[i])
            continue;
        if (*argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        const char *arg = argv[i];
        if (arg[1] == '-') // startsWith("--")
            ++arg;
        if (strcmp(arg, "-plugin") == 0) {
            if (++i < argc)
                pluginList << argv[i];
        } else if (strcmp(arg, "-reverse") == 0) {
            force_reverse = true;
#ifdef Q_OS_MAC
        } else if (strncmp(arg, "-psn_", 5) == 0) {
            // eat "-psn_xxxx" on Mac, which is passed when starting an app from Finder.
            // special hack to change working directory (for an app bundle) when running from finder
            if (QDir::currentPath() == "/"_L1) {
                QCFType<CFURLRef> bundleURL(CFBundleCopyBundleURL(CFBundleGetMainBundle()));
                QString qbundlePath = QCFString(CFURLCopyFileSystemPath(bundleURL,
                                                                        kCFURLPOSIXPathStyle));
                if (qbundlePath.endsWith(".app"_L1))
                    QDir::setCurrent(qbundlePath.section(u'/', 0, -2));
            }
#endif
#ifndef QT_NO_SESSIONMANAGER
        } else if (strcmp(arg, "-session") == 0 && i < argc - 1) {
            ++i;
            if (argv[i] && *argv[i]) {
                session_id = QString::fromLatin1(argv[i]);
                qsizetype p = session_id.indexOf(u'_');
                if (p >= 0) {
                    session_key = session_id.mid(p +1);
                    session_id = session_id.left(p);
                }
                is_session_restored = true;
            }
#endif
        } else if (strcmp(arg, "-testability") == 0) {
            loadTestability = true;
        } else if (strncmp(arg, "-style=", 7) == 0) {
            s = QString::fromLocal8Bit(arg + 7);
        } else if (strcmp(arg, "-style") == 0 && i < argc - 1) {
            s = QString::fromLocal8Bit(argv[++i]);
        } else {
            argv[j++] = argv[i];
        }

        if (!s.isEmpty())
            styleOverride = s;
    }

    if (j < argc) {
        argv[j] = nullptr;
        argc = j;
    }

    // Load environment exported generic plugins
    QByteArray envPlugins = qgetenv("QT_QPA_GENERIC_PLUGINS");
    if (!envPlugins.isEmpty())
        pluginList += envPlugins.split(',');

    if (platform_integration == nullptr)
        createPlatformIntegration();

    updatePalette();
    QFont::initialize();
    initThemeHints();

#ifndef QT_NO_CURSOR
    QCursorData::initialize();
#endif

    // trigger registering of QVariant's GUI types
    qRegisterGuiVariant();

#if QT_CONFIG(animation)
    // trigger registering of animation interpolators
    qRegisterGuiGetInterpolator();
#endif

    QWindowSystemInterfacePrivate::eventTime.start();

    is_app_running = true;
    init_plugins(pluginList);
    QWindowSystemInterface::flushWindowSystemEvents();

    Q_Q(QGuiApplication);
#ifndef QT_NO_SESSIONMANAGER
    // connect to the session manager
    session_manager = new QSessionManager(q, session_id, session_key);
#endif

#if QT_CONFIG(library)
    if (qEnvironmentVariableIntValue("QT_LOAD_TESTABILITY") > 0)
        loadTestability = true;

    if (loadTestability) {
        QLibrary testLib(QStringLiteral("qttestability"));
        if (Q_UNLIKELY(!testLib.load())) {
            qCritical() << "Library qttestability load failed:" << testLib.errorString();
        } else {
            typedef void (*TasInitialize)(void);
            TasInitialize initFunction = (TasInitialize)testLib.resolve("qt_testability_init");
            if (Q_UNLIKELY(!initFunction)) {
                qCritical("Library qttestability resolve failed!");
            } else {
                initFunction();
            }
        }
    }
#else
    Q_UNUSED(loadTestability);
#endif // QT_CONFIG(library)

    // trigger changed signal and event delivery
    QGuiApplication::setLayoutDirection(layout_direction);

    if (!QGuiApplicationPrivate::displayName)
        QObject::connect(q, &QGuiApplication::applicationNameChanged,
                         q, &QGuiApplication::applicationDisplayNameChanged);
}

extern void qt_cleanupFontDatabase();

QGuiApplicationPrivate::~QGuiApplicationPrivate()
{
    is_app_closing = true;
    is_app_running = false;

    for (int i = 0; i < generic_plugin_list.size(); ++i)
        delete generic_plugin_list.at(i);
    generic_plugin_list.clear();

    clearFontUnlocked();

    QFont::cleanup();

#ifndef QT_NO_CURSOR
    QCursorData::cleanup();
#endif

    layout_direction = Qt::LayoutDirectionAuto;

    cleanupThreadData();

    delete QGuiApplicationPrivate::styleHints;
    QGuiApplicationPrivate::styleHints = nullptr;
    delete inputMethod;

    qt_cleanupFontDatabase();

    QPixmapCache::clear();

#ifndef QT_NO_OPENGL
    if (ownGlobalShareContext) {
        delete qt_gl_global_share_context();
        qt_gl_set_global_share_context(nullptr);
    }
#endif

#if QT_CONFIG(vulkan)
    QVulkanDefaultInstance::cleanup();
#endif

    platform_integration->destroy();

    delete platform_theme;
    platform_theme = nullptr;
    delete platform_integration;
    platform_integration = nullptr;

    window_list.clear();
    popup_list.clear();
    screen_list.clear();

    self = nullptr;
}

#if 0
#ifndef QT_NO_CURSOR
QCursor *overrideCursor();
void setOverrideCursor(const QCursor &);
void changeOverrideCursor(const QCursor &);
void restoreOverrideCursor();
#endif

static QFont font();
static QFont font(const QWidget*);
static QFont font(const char *className);
static void setFont(const QFont &, const char *className = nullptr);
static QFontMetrics fontMetrics();

#ifndef QT_NO_CLIPBOARD
static QClipboard *clipboard();
#endif
#endif

/*!
    Returns the current state of the modifier keys on the keyboard. The current
    state is updated synchronously as the event queue is emptied of events that
    will spontaneously change the keyboard state (QEvent::KeyPress and
    QEvent::KeyRelease events).

    It should be noted this may not reflect the actual keys held on the input
    device at the time of calling but rather the modifiers as last reported in
    one of the above events. If no keys are being held Qt::NoModifier is
    returned.

    \sa mouseButtons(), queryKeyboardModifiers()
*/
Qt::KeyboardModifiers QGuiApplication::keyboardModifiers()
{
    return QGuiApplicationPrivate::modifier_buttons;
}

/*!
    \fn Qt::KeyboardModifiers QGuiApplication::queryKeyboardModifiers()

    Queries and returns the state of the modifier keys on the keyboard.
    Unlike keyboardModifiers, this method returns the actual keys held
    on the input device at the time of calling the method.

    It does not rely on the keypress events having been received by this
    process, which makes it possible to check the modifiers while moving
    a window, for instance. Note that in most cases, you should use
    keyboardModifiers(), which is faster and more accurate since it contains
    the state of the modifiers as they were when the currently processed
    event was received.

    \sa keyboardModifiers()
*/
Qt::KeyboardModifiers QGuiApplication::queryKeyboardModifiers()
{
    CHECK_QAPP_INSTANCE(Qt::KeyboardModifiers{})
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    return pi->keyMapper()->queryKeyboardModifiers();
}

/*!
    Returns the current state of the buttons on the mouse. The current state is
    updated synchronously as the event queue is emptied of events that will
    spontaneously change the mouse state (QEvent::MouseButtonPress and
    QEvent::MouseButtonRelease events).

    It should be noted this may not reflect the actual buttons held on the
    input device at the time of calling but rather the mouse buttons as last
    reported in one of the above events. If no mouse buttons are being held
    Qt::NoButton is returned.

    \sa keyboardModifiers()
*/
Qt::MouseButtons QGuiApplication::mouseButtons()
{
    return QGuiApplicationPrivate::mouse_buttons;
}

/*!
    \internal
    Returns the platform's native interface, for platform specific
    functionality.
*/
QPlatformNativeInterface *QGuiApplication::platformNativeInterface()
{
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    return pi ? pi->nativeInterface() : nullptr;
}

/*!
    \internal
    Returns a function pointer from the platformplugin matching \a function
*/
QFunctionPointer QGuiApplication::platformFunction(const QByteArray &function)
{
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    if (!pi) {
        qWarning("QGuiApplication::platformFunction(): Must construct a QGuiApplication before accessing a platform function");
        return nullptr;
    }

    return pi->nativeInterface() ? pi->nativeInterface()->platformFunction(function) : nullptr;
}

/*!
    Enters the main event loop and waits until exit() is called, and then
    returns the value that was set to exit() (which is 0 if exit() is called
    via quit()).

    It is necessary to call this function to start event handling. The main
    event loop receives events from the window system and dispatches these to
    the application widgets.

    Generally, no user interaction can take place before calling exec().

    To make your application perform idle processing, e.g., executing a
    special function whenever there are no pending events, use a QChronoTimer
    with 0ns timeout. More advanced idle processing schemes can be achieved
    using processEvents().

    We recommend that you connect clean-up code to the
    \l{QCoreApplication::}{aboutToQuit()} signal, instead of putting it in your
    application's \c{main()} function. This is because, on some platforms, the
    QApplication::exec() call may not return.

    \sa quitOnLastWindowClosed, quit(), exit(), processEvents(),
        QCoreApplication::exec()
*/
int QGuiApplication::exec()
{
#if QT_CONFIG(accessibility)
    QAccessible::setRootObject(qApp);
#endif
    return QCoreApplication::exec();
}

void QGuiApplicationPrivate::captureGlobalModifierState(QEvent *e)
{
    if (e->spontaneous()) {
        // Capture the current mouse and keyboard states. Doing so here is
        // required in order to support Qt Test synthesized events. Real mouse
        // and keyboard state updates from the platform plugin are managed by
        // QGuiApplicationPrivate::process(Mouse|Wheel|Key|Touch|Tablet)Event();
        // ### FIXME: Qt Test should not call qapp->notify(), but rather route
        // the events through the proper QPA interface. This is required to
        // properly generate all other events such as enter/leave etc.
        switch (e->type()) {
        case QEvent::MouseButtonPress: {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);
            QGuiApplicationPrivate::modifier_buttons = me->modifiers();
            QGuiApplicationPrivate::mouse_buttons |= me->button();
            break;
        }
        case QEvent::MouseButtonDblClick: {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);
            QGuiApplicationPrivate::modifier_buttons = me->modifiers();
            QGuiApplicationPrivate::mouse_buttons |= me->button();
            break;
        }
        case QEvent::MouseButtonRelease: {
            QMouseEvent *me = static_cast<QMouseEvent *>(e);
            QGuiApplicationPrivate::modifier_buttons = me->modifiers();
            QGuiApplicationPrivate::mouse_buttons &= ~me->button();
            break;
        }
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::MouseMove:
#if QT_CONFIG(wheelevent)
        case QEvent::Wheel:
#endif
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
#if QT_CONFIG(tabletevent)
        case QEvent::TabletMove:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
#endif
        {
            QInputEvent *ie = static_cast<QInputEvent *>(e);
            QGuiApplicationPrivate::modifier_buttons = ie->modifiers();
            break;
        }
        default:
            break;
        }
    }
}

/*! \reimp
*/
bool QGuiApplication::notify(QObject *object, QEvent *event)
{
    Q_D(QGuiApplication);
    if (object->isWindowType()) {
        if (QGuiApplicationPrivate::sendQWindowEventToQPlatformWindow(static_cast<QWindow *>(object), event))
            return true; // Platform plugin ate the event
    }

    switch (event->type()) {
    case QEvent::ApplicationDeactivate:
    case QEvent::OrientationChange:
        // Close all popups (triggers when switching applications
        // by pressing ALT-TAB on Windows, which is not received as a key event.
        // triggers when the screen rotates.)
        // This is also necessary on Wayland, and platforms where
        // QWindow::setMouseGrabEnabled(true) doesn't work.
        d->closeAllPopups();
        break;
    default:
        break;
    }

    QGuiApplicationPrivate::captureGlobalModifierState(event);

    return QCoreApplication::notify(object, event);
}

/*! \reimp
*/
bool QGuiApplication::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        // if the layout direction was set explicitly, then don't override it here
        if (layout_direction == Qt::LayoutDirectionAuto)
            setLayoutDirection(layout_direction);
        for (auto *topLevelWindow : QGuiApplication::topLevelWindows()) {
            if (topLevelWindow->flags() != Qt::Desktop)
                postEvent(topLevelWindow, new QEvent(QEvent::LanguageChange));
        }
        break;
    case QEvent::ApplicationFontChange:
    case QEvent::ApplicationPaletteChange:
        for (auto *topLevelWindow : QGuiApplication::topLevelWindows()) {
            if (topLevelWindow->flags() != Qt::Desktop)
                postEvent(topLevelWindow, new QEvent(e->type()));
        }
        break;
    case QEvent::ThemeChange:
        for (auto *w : QGuiApplication::allWindows())
            forwardEvent(w, e);
        break;
    case QEvent::Quit:
        // Close open windows. This is done in order to deliver de-expose
        // events while the event loop is still running.
        for (QWindow *topLevelWindow : QGuiApplication::topLevelWindows()) {
            // Already closed windows will not have a platform window, skip those
            if (!topLevelWindow->handle())
                continue;
            if (!topLevelWindow->close()) {
                e->ignore();
                return true;
            }
        }
        break;
    default:
        break;
    }
    return QCoreApplication::event(e);
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
/*!
    \internal
*/
bool QGuiApplication::compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents)
{
    QT_IGNORE_DEPRECATIONS(
    return QCoreApplication::compressEvent(event, receiver, postedEvents);
    )
}
#endif

bool QGuiApplicationPrivate::sendQWindowEventToQPlatformWindow(QWindow *window, QEvent *event)
{
    if (!window)
        return false;
    QPlatformWindow *platformWindow = window->handle();
    if (!platformWindow)
        return false;
    // spontaneous events come from the platform integration already, we don't need to send the events back
    if (event->spontaneous())
        return false;
    // let the platform window do any handling it needs to as well
    return platformWindow->windowEvent(event);
}

bool QGuiApplicationPrivate::processNativeEvent(QWindow *window, const QByteArray &eventType, void *message, qintptr *result)
{
    return window->nativeEvent(eventType, message, result);
}

bool QGuiApplicationPrivate::isUsingVirtualKeyboard()
{
    static const bool usingVirtualKeyboard = getenv("QT_IM_MODULE") == QByteArray("qtvirtualkeyboard");
    return usingVirtualKeyboard;
}

// If a virtual keyboard exists, forward mouse event
bool QGuiApplicationPrivate::maybeForwardEventToVirtualKeyboard(QEvent *e)
{
    if (!isUsingVirtualKeyboard()) {
        qCDebug(lcVirtualKeyboard) << "Virtual keyboard not supported.";
        return false;
    }

    static QPointer<QWindow> virtualKeyboard;
    const QEvent::Type type = e->type();
    Q_ASSERT(type == QEvent::MouseButtonPress || type == QEvent::MouseButtonRelease);
    const auto me = static_cast<QMouseEvent *>(e);
    const QPointF posF = me->globalPosition();
    const QPoint pos = posF.toPoint();

    // Is there a visible virtual keyboard at event position?
    if (!virtualKeyboard) {
        if (QWindow *win = QGuiApplication::topLevelAt(pos);
            win->inherits("QtVirtualKeyboard::InputView")) {
            virtualKeyboard = win;
        } else {
            qCDebug(lcVirtualKeyboard) << "Virtual keyboard supported, but inactive.";
            return false;
        }
    }

    Q_ASSERT(virtualKeyboard);
    const bool virtualKeyboardUnderMouse = virtualKeyboard->isVisible()
                                           && virtualKeyboard->geometry().contains(pos);

    if (!virtualKeyboardUnderMouse) {
        qCDebug(lcVirtualKeyboard) << type << "at" << pos << "is outside geometry"
                                   << virtualKeyboard->geometry() << "of" << virtualKeyboard.data();
        return false;
    }

    QMouseEvent vkbEvent(type, virtualKeyboard->mapFromGlobal(pos), pos,
                         me->button(), me->buttons(), me->modifiers(),
                         me->pointingDevice());

    QGuiApplication::sendEvent(virtualKeyboard, &vkbEvent);
    qCDebug(lcVirtualKeyboard) << "Forwarded" << type << "to" << virtualKeyboard.data()
                               << "at" << pos;

    return true;
}

void Q_TRACE_INSTRUMENT(qtgui) QGuiApplicationPrivate::processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e)
{
    Q_TRACE_PARAM_REPLACE(QWindowSystemInterfacePrivate::WindowSystemEvent *, int);
    Q_TRACE_SCOPE(QGuiApplicationPrivate_processWindowSystemEvent, e->type);

    switch(e->type) {
    case QWindowSystemInterfacePrivate::Mouse:
        QGuiApplicationPrivate::processMouseEvent(static_cast<QWindowSystemInterfacePrivate::MouseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Wheel:
        QGuiApplicationPrivate::processWheelEvent(static_cast<QWindowSystemInterfacePrivate::WheelEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Key:
        QGuiApplicationPrivate::processKeyEvent(static_cast<QWindowSystemInterfacePrivate::KeyEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Touch:
        QGuiApplicationPrivate::processTouchEvent(static_cast<QWindowSystemInterfacePrivate::TouchEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::GeometryChange:
        QGuiApplicationPrivate::processGeometryChangeEvent(static_cast<QWindowSystemInterfacePrivate::GeometryChangeEvent*>(e));
        break;
    case QWindowSystemInterfacePrivate::Enter:
        QGuiApplicationPrivate::processEnterEvent(static_cast<QWindowSystemInterfacePrivate::EnterEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Leave:
        QGuiApplicationPrivate::processLeaveEvent(static_cast<QWindowSystemInterfacePrivate::LeaveEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::FocusWindow:
        QGuiApplicationPrivate::processFocusWindowEvent(static_cast<QWindowSystemInterfacePrivate::FocusWindowEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::WindowStateChanged:
        QGuiApplicationPrivate::processWindowStateChangedEvent(static_cast<QWindowSystemInterfacePrivate::WindowStateChangedEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::WindowScreenChanged:
        QGuiApplicationPrivate::processWindowScreenChangedEvent(static_cast<QWindowSystemInterfacePrivate::WindowScreenChangedEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::WindowDevicePixelRatioChanged:
        QGuiApplicationPrivate::processWindowDevicePixelRatioChangedEvent(static_cast<QWindowSystemInterfacePrivate::WindowDevicePixelRatioChangedEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::SafeAreaMarginsChanged:
        QGuiApplicationPrivate::processSafeAreaMarginsChangedEvent(static_cast<QWindowSystemInterfacePrivate::SafeAreaMarginsChangedEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ApplicationStateChanged: {
        QWindowSystemInterfacePrivate::ApplicationStateChangedEvent * changeEvent = static_cast<QWindowSystemInterfacePrivate::ApplicationStateChangedEvent *>(e);
        QGuiApplicationPrivate::setApplicationState(changeEvent->newState, changeEvent->forcePropagate); }
        break;
    case QWindowSystemInterfacePrivate::ApplicationTermination:
        QGuiApplicationPrivate::processApplicationTermination(e);
        break;
    case QWindowSystemInterfacePrivate::FlushEvents: {
        QWindowSystemInterfacePrivate::FlushEventsEvent *flushEventsEvent = static_cast<QWindowSystemInterfacePrivate::FlushEventsEvent *>(e);
        QWindowSystemInterface::deferredFlushWindowSystemEvents(flushEventsEvent->flags); }
        break;
    case QWindowSystemInterfacePrivate::Close:
        QGuiApplicationPrivate::processCloseEvent(
                static_cast<QWindowSystemInterfacePrivate::CloseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenOrientation:
        QGuiApplicationPrivate::processScreenOrientationChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenOrientationEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenGeometry:
        QGuiApplicationPrivate::processScreenGeometryChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenGeometryEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInch:
        QGuiApplicationPrivate::processScreenLogicalDotsPerInchChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenRefreshRate:
        QGuiApplicationPrivate::processScreenRefreshRateChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ThemeChange:
        QGuiApplicationPrivate::processThemeChanged(
                    static_cast<QWindowSystemInterfacePrivate::ThemeChangeEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Expose:
        QGuiApplicationPrivate::processExposeEvent(static_cast<QWindowSystemInterfacePrivate::ExposeEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Paint:
        QGuiApplicationPrivate::processPaintEvent(static_cast<QWindowSystemInterfacePrivate::PaintEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Tablet:
        QGuiApplicationPrivate::processTabletEvent(
                    static_cast<QWindowSystemInterfacePrivate::TabletEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::TabletEnterProximity:
        QGuiApplicationPrivate::processTabletEnterProximityEvent(
                    static_cast<QWindowSystemInterfacePrivate::TabletEnterProximityEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::TabletLeaveProximity:
        QGuiApplicationPrivate::processTabletLeaveProximityEvent(
                    static_cast<QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *>(e));
        break;
#ifndef QT_NO_GESTURES
    case QWindowSystemInterfacePrivate::Gesture:
        QGuiApplicationPrivate::processGestureEvent(
                    static_cast<QWindowSystemInterfacePrivate::GestureEvent *>(e));
        break;
#endif
    case QWindowSystemInterfacePrivate::PlatformPanel:
        QGuiApplicationPrivate::processPlatformPanelEvent(
                    static_cast<QWindowSystemInterfacePrivate::PlatformPanelEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::FileOpen:
        QGuiApplicationPrivate::processFileOpenEvent(
                    static_cast<QWindowSystemInterfacePrivate::FileOpenEvent *>(e));
        break;
#ifndef QT_NO_CONTEXTMENU
        case QWindowSystemInterfacePrivate::ContextMenu:
        QGuiApplicationPrivate::processContextMenuEvent(
                    static_cast<QWindowSystemInterfacePrivate::ContextMenuEvent *>(e));
        break;
#endif
    case QWindowSystemInterfacePrivate::EnterWhatsThisMode:
        QGuiApplication::postEvent(QGuiApplication::instance(), new QEvent(QEvent::EnterWhatsThisMode));
        break;
    default:
        qWarning() << "Unknown user input event type:" << e->type;
        break;
    }
}

/*! \internal

    History is silent on why Qt splits mouse events that change position and
    button state at the same time. We believe that this was done to emulate mouse
    behavior on touch screens. If mouse tracking is enabled, we will get move
    events before the button is pressed. A touch panel does not generally give
    move events when not pressed, so without event splitting code path we would
    only see a press in a new location without any intervening moves. This could
    confuse code that is written for a real mouse. The same is true for mouse
    release events that change position, see tst_QWidget::touchEventSynthesizedMouseEvent()
    and tst_QWindow::generatedMouseMove() auto tests.
*/
void QGuiApplicationPrivate::processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e)
{
    QEvent::Type type = QEvent::None;
    Qt::MouseButton button = Qt::NoButton;
    QWindow *window = e->window.data();
    const QPointingDevice *device = static_cast<const QPointingDevice *>(e->device);
    Q_ASSERT(device);
    QPointingDevicePrivate *devPriv = QPointingDevicePrivate::get(const_cast<QPointingDevice*>(device));
    bool positionChanged = QGuiApplicationPrivate::lastCursorPosition != e->globalPos;
    bool mouseMove = false;
    bool mousePress = false;
    const QPointF lastGlobalPosition = QGuiApplicationPrivate::lastCursorPosition;
    QPointF globalPoint = e->globalPos;

    if (qIsNaN(e->globalPos.x()) || qIsNaN(e->globalPos.y())) {
        qWarning("QGuiApplicationPrivate::processMouseEvent: Got NaN in mouse position");
        return;
    }

    type = e->buttonType;
    button = e->button;

    if (type == QEvent::NonClientAreaMouseMove || type == QEvent::MouseMove)
        mouseMove = true;
    else if (type == QEvent::NonClientAreaMouseButtonPress || type == QEvent::MouseButtonPress)
        mousePress = true;

    if (!mouseMove && positionChanged) {
        QWindowSystemInterfacePrivate::MouseEvent moveEvent(window, e->timestamp,
            e->localPos, e->globalPos, e->buttons ^ button, e->modifiers, Qt::NoButton,
            e->nonClientArea ? QEvent::NonClientAreaMouseMove : QEvent::MouseMove,
            e->source, e->nonClientArea, device, e->eventPointId);
        if (e->synthetic())
            moveEvent.flags |= QWindowSystemInterfacePrivate::WindowSystemEvent::Synthetic;
        processMouseEvent(&moveEvent); // mouse move excluding state change
        processMouseEvent(e); // the original mouse event
        return;
    }
    if (type == QEvent::MouseMove && !positionChanged) {
        // On Windows, and possibly other platforms, a touchpad can send a mouse move
        // that does not change position, between a press and a release. This may
        // confuse applications, so we always filter out these mouse events for
        // consistent behavior among platforms.
        return;
    }

    modifier_buttons = e->modifiers;
    QPointF localPoint = e->localPos;
    bool doubleClick = false;
    auto persistentEPD = devPriv->pointById(0);

    if (e->synthetic(); auto *originalDeviceEPD = devPriv->queryPointById(e->eventPointId))
        QMutableEventPoint::update(originalDeviceEPD->eventPoint, persistentEPD->eventPoint);

    if (mouseMove) {
        QGuiApplicationPrivate::lastCursorPosition = globalPoint;
        const auto doubleClickDistance = (e->device && e->device->type() == QInputDevice::DeviceType::Mouse ?
                    mouseDoubleClickDistance : touchDoubleTapDistance);
        const auto pressPos = persistentEPD->eventPoint.globalPressPosition();
        if (qAbs(globalPoint.x() - pressPos.x()) > doubleClickDistance ||
            qAbs(globalPoint.y() - pressPos.y()) > doubleClickDistance)
            mousePressButton = Qt::NoButton;
    } else {
        static unsigned long lastPressTimestamp = 0;
        static QPointer<QWindow> lastPressWindow = nullptr;
        mouse_buttons = e->buttons;
        if (mousePress) {
            ulong doubleClickInterval = static_cast<ulong>(QGuiApplication::styleHints()->mouseDoubleClickInterval());
            const auto timestampDelta = e->timestamp - lastPressTimestamp;
            doubleClick = timestampDelta > 0 && timestampDelta < doubleClickInterval
                          && button == mousePressButton && lastPressWindow == e->window;
            mousePressButton = button;
            lastPressTimestamp = e ->timestamp;
            lastPressWindow = e->window;
        }
    }

    if (e->nullWindow()) {
        window = QGuiApplication::topLevelAt(globalPoint.toPoint());
        if (window) {
            // Moves and the release following a press must go to the same
            // window, even if the cursor has moved on over another window.
            if (e->buttons != Qt::NoButton) {
                if (!currentMousePressWindow)
                    currentMousePressWindow = window;
                else
                    window = currentMousePressWindow;
            } else if (currentMousePressWindow) {
                window = currentMousePressWindow;
                currentMousePressWindow = nullptr;
            }
            localPoint = window->mapFromGlobal(globalPoint);
        }
    }

    if (!window)
        return;

#ifndef QT_NO_CURSOR
    if (!e->synthetic()) {
        if (const QScreen *screen = window->screen())
            if (QPlatformCursor *cursor = screen->handle()->cursor()) {
                const QPointF nativeLocalPoint = QHighDpi::toNativePixels(localPoint, screen);
                const QPointF nativeGlobalPoint = QHighDpi::toNativePixels(globalPoint, screen);
                QMouseEvent ev(type, nativeLocalPoint, nativeLocalPoint, nativeGlobalPoint,
                               button, e->buttons, e->modifiers, e->source, device);
                // avoid incorrect velocity calculation: ev is in the native coordinate system,
                // but we need to consistently use the logical coordinate system for velocity
                // whenever QEventPoint::setTimestamp() is called
                ev.QInputEvent::setTimestamp(e->timestamp);
                cursor->pointerEvent(ev);
            }
    }
#endif

    const auto *activePopup = activePopupWindow();
    if (type == QEvent::MouseButtonPress)
        active_popup_on_press = activePopup;
    if (window->d_func()->blockedByModalWindow && !activePopup) {
        // a modal window is blocking this window, don't allow mouse events through
        return;
    }

    QMouseEvent ev(type, localPoint, localPoint, globalPoint, button, e->buttons, e->modifiers, e->source, device);
    Q_ASSERT(devPriv->pointById(0) == persistentEPD); // we don't expect reallocation in QPlatformCursor::pointerEvenmt()
    // restore globalLastPosition to avoid invalidating the velocity calculations,
    // because the QPlatformCursor mouse event above was in native coordinates
    QMutableEventPoint::setGlobalLastPosition(persistentEPD->eventPoint, lastGlobalPosition);
    persistentEPD = nullptr; // incoming and synth events can cause reallocation during delivery, so don't use this again
    // ev now contains a detached copy of the QEventPoint from QPointingDevicePrivate::activePoints
    ev.setTimestamp(e->timestamp);

    if (activePopup && activePopup != window && (!popup_closed_on_press || type == QEvent::MouseButtonRelease)) {
        // If the popup handles the event, we're done.
        auto *handlingPopup = window->d_func()->forwardToPopup(&ev, active_popup_on_press);
        if (handlingPopup) {
            if (type == QEvent::MouseButtonPress)
                active_popup_on_press = handlingPopup;
            return;
        }
    }

    if (doubleClick && (ev.type() == QEvent::MouseButtonPress)) {
        // QtBUG-25831, used to suppress delivery in qwidgetwindow.cpp
        QMutableSinglePointEvent::setDoubleClick(&ev, true);
    }

    QGuiApplication::sendSpontaneousEvent(window, &ev);
    e->eventAccepted = ev.isAccepted();
    if (!e->synthetic() && !ev.isAccepted()
        && !e->nonClientArea
        && qApp->testAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents)) {
        QList<QWindowSystemInterface::TouchPoint> points;
        QWindowSystemInterface::TouchPoint point;
        point.id = 1;
        point.area = QHighDpi::toNativePixels(QRectF(globalPoint.x() - 2, globalPoint.y() - 2, 4, 4), window);

        // only translate left button related events to
        // avoid strange touch event sequences when several
        // buttons are pressed
        if (type == QEvent::MouseButtonPress && button == Qt::LeftButton) {
            point.state = QEventPoint::State::Pressed;
        } else if (type == QEvent::MouseButtonRelease && button == Qt::LeftButton) {
            point.state = QEventPoint::State::Released;
        } else if (type == QEvent::MouseMove && (e->buttons & Qt::LeftButton)) {
            point.state = QEventPoint::State::Updated;
        } else {
            return;
        }

        points << point;

        QEvent::Type type;
        const QList<QEventPoint> &touchPoints =
                QWindowSystemInterfacePrivate::fromNativeTouchPoints(points, window, &type);

        QWindowSystemInterfacePrivate::TouchEvent fake(window, e->timestamp, type, device, touchPoints, e->modifiers);
        fake.flags |= QWindowSystemInterfacePrivate::WindowSystemEvent::Synthetic;
        processTouchEvent(&fake);
    }
    if (doubleClick) {
        mousePressButton = Qt::NoButton;
        if (!e->window.isNull() || e->nullWindow()) { // QTBUG-36364, check if window closed in response to press
            const QEvent::Type doubleClickType = e->nonClientArea ? QEvent::NonClientAreaMouseButtonDblClick : QEvent::MouseButtonDblClick;
            QMouseEvent dblClickEvent(doubleClickType, localPoint, localPoint, globalPoint,
                                      button, e->buttons, e->modifiers, e->source, device);
            dblClickEvent.setTimestamp(e->timestamp);
            QGuiApplication::sendSpontaneousEvent(window, &dblClickEvent);
        }
    }
    if (type == QEvent::MouseButtonRelease && e->buttons == Qt::NoButton) {
        popup_closed_on_press = false;
        if (auto *persistentEPD = devPriv->queryPointById(0)) {
            ev.setExclusiveGrabber(persistentEPD->eventPoint, nullptr);
            ev.clearPassiveGrabbers(persistentEPD->eventPoint);
        }
    }
}

void QGuiApplicationPrivate::processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e)
{
#if QT_CONFIG(wheelevent)
    QWindow *window = e->window.data();
    QPointF globalPoint = e->globalPos;
    QPointF localPoint = e->localPos;

    if (e->nullWindow()) {
        window = QGuiApplication::topLevelAt(globalPoint.toPoint());
        if (window)
            localPoint = window->mapFromGlobal(globalPoint);
    }

    if (!window)
        return;

    QGuiApplicationPrivate::lastCursorPosition = globalPoint;
    modifier_buttons = e->modifiers;

    if (window->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow wheel events through
        return;
    }

    const QPointingDevice *device = static_cast<const QPointingDevice *>(e->device);
    QWheelEvent ev(localPoint, globalPoint, e->pixelDelta, e->angleDelta,
                   mouse_buttons, e->modifiers, e->phase, e->inverted, e->source, device);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(window, &ev);
    e->eventAccepted = ev.isAccepted();
#else
     Q_UNUSED(e);
#endif // QT_CONFIG(wheelevent)
}

void QGuiApplicationPrivate::processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e)
{
    QWindow *window = e->window.data();
    modifier_buttons = e->modifiers;
    if (e->nullWindow()
#ifdef Q_OS_ANDROID
           || e->key == Qt::Key_Back || e->key == Qt::Key_Menu
#endif
            ) {
        window = QGuiApplication::focusWindow();
    }

    if (!window) {
        e->eventAccepted = false;
        return;
    }

#if defined(Q_OS_ANDROID)
    static bool backKeyPressAccepted = false;
    static bool menuKeyPressAccepted = false;
#endif

#if !defined(Q_OS_MACOS)
    // FIXME: Include OS X in this code path by passing the key event through
    // QPlatformInputContext::filterEvent().
    if (e->keyType == QEvent::KeyPress) {
        if (QWindowSystemInterface::handleShortcutEvent(window, e->timestamp, e->key, e->modifiers,
            e->nativeScanCode, e->nativeVirtualKey, e->nativeModifiers, e->unicode, e->repeat, e->repeatCount)) {
#if defined(Q_OS_ANDROID)
            backKeyPressAccepted = e->key == Qt::Key_Back;
            menuKeyPressAccepted = e->key == Qt::Key_Menu;
#endif
            return;
        }
    }
#endif

    QKeyEvent ev(e->keyType, e->key, e->modifiers,
                 e->nativeScanCode, e->nativeVirtualKey, e->nativeModifiers,
                 e->unicode, e->repeat, e->repeatCount);
    ev.setTimestamp(e->timestamp);

    const auto *activePopup = activePopupWindow();
    if (activePopup && activePopup != window) {
        // If the popup handles the event, we're done.
        if (window->d_func()->forwardToPopup(&ev, active_popup_on_press))
            return;
    }

    // only deliver key events when we have a window, and no modal window is blocking this window

    if (!window->d_func()->blockedByModalWindow)
        QGuiApplication::sendSpontaneousEvent(window, &ev);
#ifdef Q_OS_ANDROID
    else
        ev.setAccepted(false);

    if (e->keyType == QEvent::KeyPress) {
        backKeyPressAccepted = e->key == Qt::Key_Back && ev.isAccepted();
        menuKeyPressAccepted = e->key == Qt::Key_Menu && ev.isAccepted();
    } else if (e->keyType == QEvent::KeyRelease) {
        if (e->key == Qt::Key_Back && !backKeyPressAccepted && !ev.isAccepted()) {
            if (window)
                QWindowSystemInterface::handleCloseEvent(window);
        } else if (e->key == Qt::Key_Menu && !menuKeyPressAccepted && !ev.isAccepted()) {
            platform_theme->showPlatformMenuBar();
        }
    }
#endif
    e->eventAccepted = ev.isAccepted();
}

void QGuiApplicationPrivate::processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e)
{
    if (!e->enter)
        return;
    if (e->enter.data()->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow enter events through
        return;
    }

    currentMouseWindow = e->enter;

    // TODO later: EnterEvent must report _which_ mouse entered the window; for now we assume primaryPointingDevice()
    QEnterEvent event(e->localPos, e->localPos, e->globalPos);

    // Since we don't always track mouse moves that occur outside a window, any residual velocity
    // stored in the persistent QEventPoint may be inaccurate (especially in fast-moving autotests).
    // Reset the Kalman filter so that the velocity of the first mouse event after entering the window
    // will be based on a zero residual velocity (but the result can still be non-zero if the mouse
    // moves to a different position from where this enter event occurred; tests often do that).
    const QPointingDevicePrivate *devPriv = QPointingDevicePrivate::get(event.pointingDevice());
    auto epd = devPriv->queryPointById(event.points().first().id());
    Q_ASSERT(epd);
    QMutableEventPoint::setVelocity(epd->eventPoint, {});

    QCoreApplication::sendSpontaneousEvent(e->enter.data(), &event);
}

void QGuiApplicationPrivate::processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e)
{
    if (!e->leave)
        return;
    if (e->leave.data()->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow leave events through
        return;
    }

    currentMouseWindow = nullptr;

    QEvent event(QEvent::Leave);
    QCoreApplication::sendSpontaneousEvent(e->leave.data(), &event);
}

void QGuiApplicationPrivate::processFocusWindowEvent(QWindowSystemInterfacePrivate::FocusWindowEvent *e)
{
    QWindow *previous = QGuiApplicationPrivate::focus_window;
    QWindow *newFocus = e->focused.data();

    if (previous == newFocus)
        return;

    bool activatedPopup = false;
    if (newFocus) {
        if (QPlatformWindow *platformWindow = newFocus->handle())
            if (platformWindow->isAlertState())
                platformWindow->setAlertState(false);
        activatedPopup = (newFocus->flags() & Qt::WindowType_Mask) == Qt::Popup;
        if (activatedPopup)
            activatePopup(newFocus);
    }

    QObject *previousFocusObject = previous ? previous->focusObject() : nullptr;

    if (previous) {
        QFocusEvent focusAboutToChange(QEvent::FocusAboutToChange);
        QCoreApplication::sendSpontaneousEvent(previous, &focusAboutToChange);
    }

    QGuiApplicationPrivate::focus_window = newFocus;
    if (!qApp)
        return;

    if (previous) {
        Qt::FocusReason r = e->reason;
        if ((r == Qt::OtherFocusReason || r == Qt::ActiveWindowFocusReason) && activatedPopup)
            r = Qt::PopupFocusReason;
        QFocusEvent focusOut(QEvent::FocusOut, r);
        QCoreApplication::sendSpontaneousEvent(previous, &focusOut);
        QObject::disconnect(previous, SIGNAL(focusObjectChanged(QObject*)),
                            qApp, SLOT(_q_updateFocusObject(QObject*)));
    } else if (!platformIntegration()->hasCapability(QPlatformIntegration::ApplicationState)) {
        setApplicationState(Qt::ApplicationActive);
    }

    if (QGuiApplicationPrivate::focus_window) {
        Qt::FocusReason r = e->reason;
        if ((r == Qt::OtherFocusReason || r == Qt::ActiveWindowFocusReason) &&
                previous && (previous->flags() & Qt::Popup) == Qt::Popup)
            r = Qt::PopupFocusReason;
        QFocusEvent focusIn(QEvent::FocusIn, r);
        QCoreApplication::sendSpontaneousEvent(QGuiApplicationPrivate::focus_window, &focusIn);
        QObject::connect(QGuiApplicationPrivate::focus_window, SIGNAL(focusObjectChanged(QObject*)),
                         qApp, SLOT(_q_updateFocusObject(QObject*)));
    } else if (!platformIntegration()->hasCapability(QPlatformIntegration::ApplicationState)) {
        setApplicationState(Qt::ApplicationInactive);
    }

    if (self) {
        self->notifyActiveWindowChange(previous);

        if (previousFocusObject != qApp->focusObject() ||
            // We are getting an activation change but there is no new focusObject, and we also
            // don't have a previousFocusObject in the previously active window anymore. This can
            // happen when window gets destroyed (see QWidgetWindow::focusObject returning nullptr
            // when already in the QWidget destructor), so update the focusObject to avoid dangling
            // pointers. See also QWidget::clearFocus(), which tries to cover for this as well.
            (previous && previousFocusObject == nullptr && qApp->focusObject() == nullptr)) {
            self->_q_updateFocusObject(qApp->focusObject());
        }
    }

    emit qApp->focusWindowChanged(newFocus);
    if (previous)
        emit previous->activeChanged();
    if (newFocus)
        emit newFocus->activeChanged();
}

void QGuiApplicationPrivate::processWindowStateChangedEvent(QWindowSystemInterfacePrivate::WindowStateChangedEvent *wse)
{
    if (QWindow *window = wse->window.data()) {
        QWindowPrivate *windowPrivate = qt_window_private(window);
        const auto originalEffectiveState = QWindowPrivate::effectiveState(windowPrivate->windowState);

        windowPrivate->windowState = wse->newState;
        const auto newEffectiveState = QWindowPrivate::effectiveState(windowPrivate->windowState);
        if (newEffectiveState != originalEffectiveState)
            emit window->windowStateChanged(newEffectiveState);

        windowPrivate->updateVisibility();

        QWindowStateChangeEvent e(wse->oldState);
        QGuiApplication::sendSpontaneousEvent(window, &e);
    }
}

void QGuiApplicationPrivate::processWindowScreenChangedEvent(QWindowSystemInterfacePrivate::WindowScreenChangedEvent *wse)
{
    QWindow *window = wse->window.data();
    if (!window)
        return;

    if (window->screen() == wse->screen.data())
        return;

    if (QWindow *topLevelWindow = window->d_func()->topLevelWindow(QWindow::ExcludeTransients)) {
        if (QScreen *screen = wse->screen.data())
            topLevelWindow->d_func()->setTopLevelScreen(screen, false /* recreate */);
        else // Fall back to default behavior, and try to find some appropriate screen
            topLevelWindow->setScreen(nullptr);
    }
}

void QGuiApplicationPrivate::processWindowDevicePixelRatioChangedEvent(QWindowSystemInterfacePrivate::WindowDevicePixelRatioChangedEvent *wde)
{
    if (wde->window.isNull())
        return;
    QWindowPrivate::get(wde->window)->updateDevicePixelRatio();
}

void QGuiApplicationPrivate::processSafeAreaMarginsChangedEvent(QWindowSystemInterfacePrivate::SafeAreaMarginsChangedEvent *wse)
{
    if (wse->window.isNull())
        return;

    emit wse->window->safeAreaMarginsChanged(wse->window->safeAreaMargins());

    QEvent event(QEvent::SafeAreaMarginsChange);
    QGuiApplication::sendSpontaneousEvent(wse->window, &event);
}

void QGuiApplicationPrivate::processThemeChanged(QWindowSystemInterfacePrivate::ThemeChangeEvent *)
{
    if (self)
        self->handleThemeChanged();

    QIconPrivate::clearIconCache();

    QEvent themeChangeEvent(QEvent::ThemeChange);
    QGuiApplication::sendSpontaneousEvent(qGuiApp, &themeChangeEvent);
}

void QGuiApplicationPrivate::handleThemeChanged()
{
    QStyleHintsPrivate::get(QGuiApplication::styleHints())->update(platformTheme());
    updatePalette();

    QIconLoader::instance()->updateSystemTheme();
    QAbstractFileIconProviderPrivate::clearIconTypeCache();

    if (!(applicationResourceFlags & ApplicationFontExplicitlySet)) {
        const auto locker = qt_scoped_lock(applicationFontMutex);
        clearFontUnlocked();
        initFontUnlocked();
    }
    initThemeHints();
}

void QGuiApplicationPrivate::processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e)
{
    if (e->window.isNull())
       return;

    QWindow *window = e->window.data();
    if (!window)
        return;

    const QRect lastReportedGeometry = window->d_func()->geometry;
    const QRect requestedGeometry = e->requestedGeometry;
    const QRect actualGeometry = e->newGeometry;

    // We send size and move events only if the geometry has changed from
    // what was last reported, or if the user tried to set a new geometry,
    // but the window manager responded by keeping the old geometry. In the
    // latter case we send move/resize events with the same geometry as the
    // last reported geometry, to indicate that the window wasn't moved or
    // resized. Note that this logic does not apply to the property changes
    // of the window, as we don't treat them as part of this request/response
    // protocol of QWindow/QPA.
    const bool isResize = actualGeometry.size() != lastReportedGeometry.size()
        || requestedGeometry.size() != actualGeometry.size();
    const bool isMove = actualGeometry.topLeft() != lastReportedGeometry.topLeft()
        || requestedGeometry.topLeft() != actualGeometry.topLeft();

    window->d_func()->geometry = actualGeometry;

    if (isResize || window->d_func()->resizeEventPending) {
        QResizeEvent e(actualGeometry.size(), lastReportedGeometry.size());
        QGuiApplication::sendSpontaneousEvent(window, &e);

        window->d_func()->resizeEventPending = false;

        if (actualGeometry.width() != lastReportedGeometry.width())
            emit window->widthChanged(actualGeometry.width());
        if (actualGeometry.height() != lastReportedGeometry.height())
            emit window->heightChanged(actualGeometry.height());
    }

    if (isMove) {
        //### frame geometry
        QMoveEvent e(actualGeometry.topLeft(), lastReportedGeometry.topLeft());
        QGuiApplication::sendSpontaneousEvent(window, &e);

        if (actualGeometry.x() != lastReportedGeometry.x())
            emit window->xChanged(actualGeometry.x());
        if (actualGeometry.y() != lastReportedGeometry.y())
            emit window->yChanged(actualGeometry.y());
    }
}

void QGuiApplicationPrivate::processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e)
{
    if (e->window.isNull())
        return;
    if (e->window.data()->d_func()->blockedByModalWindow && !e->window.data()->d_func()->inClose) {
        // a modal window is blocking this window, don't allow close events through, unless they
        // originate from a call to QWindow::close.
        e->eventAccepted = false;
        return;
    }

    QCloseEvent event;
    QGuiApplication::sendSpontaneousEvent(e->window.data(), &event);

    e->eventAccepted = event.isAccepted();
}

void QGuiApplicationPrivate::processFileOpenEvent(QWindowSystemInterfacePrivate::FileOpenEvent *e)
{
    if (e->url.isEmpty())
        return;

    QFileOpenEvent event(e->url);
    QGuiApplication::sendSpontaneousEvent(qApp, &event);
}

QGuiApplicationPrivate::TabletPointData &QGuiApplicationPrivate::tabletDevicePoint(qint64 deviceId)
{
    for (int i = 0; i < tabletDevicePoints.size(); ++i) {
        TabletPointData &pointData = tabletDevicePoints[i];
        if (pointData.deviceId == deviceId)
            return pointData;
    }

    tabletDevicePoints.append(TabletPointData(deviceId));
    return tabletDevicePoints.last();
}

void QGuiApplicationPrivate::processTabletEvent(QWindowSystemInterfacePrivate::TabletEvent *e)
{
#if QT_CONFIG(tabletevent)
    const auto device = static_cast<const QPointingDevice *>(e->device);
    TabletPointData &pointData = tabletDevicePoint(device->uniqueId().numericId());

    QEvent::Type type = QEvent::TabletMove;
    if (e->buttons != pointData.state)
        type = (e->buttons > pointData.state) ? QEvent::TabletPress : QEvent::TabletRelease;

    QWindow *window = e->window.data();
    modifier_buttons = e->modifiers;

    bool localValid = true;
    // If window is null, pick one based on the global position and make sure all
    // subsequent events up to the release are delivered to that same window.
    // If window is given, just send to that.
    if (type == QEvent::TabletPress) {
        if (e->nullWindow()) {
            window = QGuiApplication::topLevelAt(e->global.toPoint());
            localValid = false;
        }
        if (!window)
            return;
        active_popup_on_press = activePopupWindow();
        pointData.target = window;
    } else {
        if (e->nullWindow()) {
            window = pointData.target;
            localValid = false;
        }
        if (type == QEvent::TabletRelease)
            pointData.target = nullptr;
        if (!window)
            return;
    }
    QPointF local = e->local;
    if (!localValid) {
        QPointF delta = e->global - e->global.toPoint();
        local = window->mapFromGlobal(e->global.toPoint()) + delta;
    }

    // TODO stop deducing the button state change here: rather require it from the platform plugin, as with mouse events
    Qt::MouseButtons stateChange = e->buttons ^ pointData.state;
    Qt::MouseButton button = Qt::NoButton;
    for (int check = Qt::LeftButton; check <= int(Qt::MaxMouseButton); check = check << 1) {
        if (check & stateChange) {
            button = Qt::MouseButton(check);
            break;
        }
    }

    const auto *activePopup = activePopupWindow();
    if (window->d_func()->blockedByModalWindow && !activePopup) {
        // a modal window is blocking this window, don't allow events through
        return;
    }

    QTabletEvent tabletEvent(type, device, local, e->global,
                             e->pressure, e->xTilt, e->yTilt,
                             e->tangentialPressure, e->rotation, e->z,
                             e->modifiers, button, e->buttons);
    tabletEvent.setAccepted(false);
    tabletEvent.setTimestamp(e->timestamp);

    if (activePopup && activePopup != window) {
        // If the popup handles the event, we're done.
        if (window->d_func()->forwardToPopup(&tabletEvent, active_popup_on_press))
            return;
    }

    QGuiApplication::sendSpontaneousEvent(window, &tabletEvent);
    pointData.state = e->buttons;
    if (!tabletEvent.isAccepted()
        && !QWindowSystemInterfacePrivate::TabletEvent::platformSynthesizesMouse
        && qApp->testAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents)) {

        const QEvent::Type mouseType = [&]() {
            switch (type) {
            case QEvent::TabletPress:   return QEvent::MouseButtonPress;
            case QEvent::TabletMove:    return QEvent::MouseMove;
            case QEvent::TabletRelease: return QEvent::MouseButtonRelease;
            default: Q_UNREACHABLE();
            }
        }();
        QWindowSystemInterfacePrivate::MouseEvent mouseEvent(window, e->timestamp, e->local,
            e->global, e->buttons, e->modifiers, button, mouseType, Qt::MouseEventNotSynthesized, false, device);
        mouseEvent.flags |= QWindowSystemInterfacePrivate::WindowSystemEvent::Synthetic;
        qCDebug(lcPtrDispatch) << "synthesizing mouse from tablet event" << mouseType
                               << e->local << button << e->buttons << e->modifiers;
        processMouseEvent(&mouseEvent);
    }
#else
    Q_UNUSED(e);
#endif
}

void QGuiApplicationPrivate::processTabletEnterProximityEvent(QWindowSystemInterfacePrivate::TabletEnterProximityEvent *e)
{
#if QT_CONFIG(tabletevent)
    const QPointingDevice *dev = static_cast<const QPointingDevice *>(e->device);
    QTabletEvent ev(QEvent::TabletEnterProximity, dev, QPointF(), QPointF(),
                    0, 0, 0, 0, 0, 0, e->modifiers, Qt::NoButton,
                    tabletDevicePoint(dev->uniqueId().numericId()).state);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(qGuiApp, &ev);
#else
    Q_UNUSED(e);
#endif
}

void QGuiApplicationPrivate::processTabletLeaveProximityEvent(QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *e)
{
#if QT_CONFIG(tabletevent)
    const QPointingDevice *dev = static_cast<const QPointingDevice *>(e->device);
    QTabletEvent ev(QEvent::TabletLeaveProximity, dev, QPointF(), QPointF(),
                    0, 0, 0, 0, 0, 0, e->modifiers, Qt::NoButton,
                    tabletDevicePoint(dev->uniqueId().numericId()).state);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(qGuiApp, &ev);
#else
    Q_UNUSED(e);
#endif
}

#ifndef QT_NO_GESTURES
void QGuiApplicationPrivate::processGestureEvent(QWindowSystemInterfacePrivate::GestureEvent *e)
{
    if (e->window.isNull())
        return;

    const QPointingDevice *device = static_cast<const QPointingDevice *>(e->device);
    QNativeGestureEvent ev(e->type, device, e->fingerCount, e->pos, e->pos, e->globalPos, (e->intValue ? e->intValue : e->realValue),
                           e->delta, e->sequenceId);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(e->window, &ev);
}
#endif // QT_NO_GESTURES

void QGuiApplicationPrivate::processPlatformPanelEvent(QWindowSystemInterfacePrivate::PlatformPanelEvent *e)
{
    if (!e->window)
        return;

    if (e->window->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow events through
        return;
    }

    QEvent ev(QEvent::PlatformPanel);
    QGuiApplication::sendSpontaneousEvent(e->window.data(), &ev);
}

#ifndef QT_NO_CONTEXTMENU
void QGuiApplicationPrivate::processContextMenuEvent(QWindowSystemInterfacePrivate::ContextMenuEvent *e)
{
    // Widgets do not care about mouse triggered context menu events. Also, do not forward event
    // to a window blocked by a modal window.
    if (!e->window || e->mouseTriggered || e->window->d_func()->blockedByModalWindow)
        return;

    QContextMenuEvent ev(QContextMenuEvent::Keyboard, e->pos, e->globalPos, e->modifiers);
    QGuiApplication::sendSpontaneousEvent(e->window.data(), &ev);
    e->eventAccepted = ev.isAccepted();
}
#endif

void QGuiApplicationPrivate::processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e)
{
    if (!QInputDevicePrivate::isRegistered(e->device))
        return;

    modifier_buttons = e->modifiers;
    QPointingDevice *device = const_cast<QPointingDevice *>(static_cast<const QPointingDevice *>(e->device));
    QPointingDevicePrivate *devPriv = QPointingDevicePrivate::get(device);

    if (e->touchType == QEvent::TouchCancel) {
        // The touch sequence has been canceled (e.g. by the compositor).
        // Send the TouchCancel to all windows with active touches and clean up.
        QTouchEvent touchEvent(QEvent::TouchCancel, device, e->modifiers);
        touchEvent.setTimestamp(e->timestamp);
        constexpr qsizetype Prealloc = decltype(devPriv->activePoints)::mapped_container_type::PreallocatedSize;
        QMinimalVarLengthFlatSet<QWindow *, Prealloc> windowsNeedingCancel;

        for (auto &epd : devPriv->activePoints.values()) {
            if (QWindow *w = QMutableEventPoint::window(epd.eventPoint))
                windowsNeedingCancel.insert(w);
        }

        for (QWindow *w : windowsNeedingCancel)
            QGuiApplication::sendSpontaneousEvent(w, &touchEvent);

        if (!self->synthesizedMousePoints.isEmpty() && !e->synthetic()) {
            for (QHash<QWindow *, SynthesizedMouseData>::const_iterator synthIt = self->synthesizedMousePoints.constBegin(),
                 synthItEnd = self->synthesizedMousePoints.constEnd(); synthIt != synthItEnd; ++synthIt) {
                if (!synthIt->window)
                    continue;
                QWindowSystemInterfacePrivate::MouseEvent fake(synthIt->window.data(),
                                                               e->timestamp,
                                                               synthIt->pos,
                                                               synthIt->screenPos,
                                                               Qt::NoButton,
                                                               e->modifiers,
                                                               Qt::LeftButton,
                                                               QEvent::MouseButtonRelease,
                                                               Qt::MouseEventNotSynthesized,
                                                               false,
                                                               device);
                fake.flags |= QWindowSystemInterfacePrivate::WindowSystemEvent::Synthetic;
                processMouseEvent(&fake);
            }
            self->synthesizedMousePoints.clear();
        }
        self->lastTouchType = e->touchType;
        return;
    }

    // Prevent sending ill-formed event sequences: Cancel can only be followed by a Begin.
    if (self->lastTouchType == QEvent::TouchCancel && e->touchType != QEvent::TouchBegin)
        return;

    self->lastTouchType = e->touchType;

    QPointer<QWindow> window = e->window;  // the platform hopefully tells us which window received the event
    QVarLengthArray<QMutableTouchEvent, 2> touchEvents;

    // For each temporary QEventPoint from the QPA TouchEvent:
    // - update the persistent QEventPoint in QPointingDevicePrivate::activePoints with current values
    // - determine which window to deliver it to
    // - add it to the QTouchEvent instance for that window (QMutableTouchEvent::target() will be QWindow*, for now)
    for (auto &tempPt : e->points) {
        // update state
        auto epd = devPriv->pointById(tempPt.id());
        auto &ep = epd->eventPoint;
        epd->eventPoint.setAccepted(false);
        switch (tempPt.state()) {
        case QEventPoint::State::Pressed:
            // On touchpads, send all touch points to the same window.
            if (!window && e->device && e->device->type() == QInputDevice::DeviceType::TouchPad)
                window = devPriv->firstActiveWindow();
            // If the QPA event didn't tell us which window, find the one under the touchpoint position.
            if (!window)
                window = QGuiApplication::topLevelAt(tempPt.globalPosition().toPoint());
            QMutableEventPoint::setWindow(ep, window);
            active_popup_on_press = activePopupWindow();
            break;

        case QEventPoint::State::Released:
            if (Q_UNLIKELY(!window.isNull() && window != QMutableEventPoint::window(ep)))
                qCDebug(lcPtrDispatch) << "delivering touch release to same window"
                                       << QMutableEventPoint::window(ep) << "not" << window.data();
            window = QMutableEventPoint::window(ep);
            break;

        default: // update or stationary
            if (Q_UNLIKELY(!window.isNull() && window != QMutableEventPoint::window(ep)))
                qCDebug(lcPtrDispatch) << "delivering touch update to same window"
                                       << QMutableEventPoint::window(ep) << "not" << window.data();
            window = QMutableEventPoint::window(ep);
            break;
        }
        // If we somehow still don't have a window, we can't deliver this touchpoint.  (should never happen)
        if (Q_UNLIKELY(!window)) {
            qCDebug(lcPtrDispatch) << "skipping" << &tempPt << ": no target window";
            continue;
        }
        QMutableEventPoint::update(tempPt, ep);

        Q_ASSERT(window.data() != nullptr);

        // make the *scene* position the same as the *global* position
        QMutableEventPoint::setScenePosition(ep, tempPt.globalPosition());

        // store the scene position as local position, for now
        QMutableEventPoint::setPosition(ep, window->mapFromGlobal(tempPt.globalPosition()));

        // setTimeStamp has side effects, so we do it last
        QMutableEventPoint::setTimestamp(ep, e->timestamp);

        // add the touchpoint to the event that will be delivered to the window
        bool added = false;
        for (QMutableTouchEvent &ev : touchEvents) {
            if (ev.target() == window.data()) {
                ev.addPoint(ep);
                added = true;
                break;
            }
        }
        if (!added) {
            QMutableTouchEvent mte(e->touchType, device, e->modifiers, {ep});
            mte.setTimestamp(e->timestamp);
            mte.setTarget(window.data());
            touchEvents.append(mte);
        }
    }

    if (touchEvents.isEmpty())
        return;

    for (QMutableTouchEvent &touchEvent : touchEvents) {
        QWindow *window = static_cast<QWindow *>(touchEvent.target());

        QEvent::Type eventType;
        switch (touchEvent.touchPointStates()) {
        case QEventPoint::State::Pressed:
            eventType = QEvent::TouchBegin;
            break;
        case QEventPoint::State::Released:
            eventType = QEvent::TouchEnd;
            break;
        default:
            eventType = QEvent::TouchUpdate;
            break;
        }

        const auto *activePopup = activePopupWindow();
        if (window->d_func()->blockedByModalWindow && !activePopup) {
            // a modal window is blocking this window, don't allow touch events through

            // QTBUG-37371 temporary fix; TODO: revisit when we have a forwarding solution
            if (touchEvent.type() == QEvent::TouchEnd) {
                // but don't leave dangling state: e.g.
                // QQuickWindowPrivate::itemForTouchPointId needs to be cleared.
                QTouchEvent touchEvent(QEvent::TouchCancel, device, e->modifiers);
                touchEvent.setTimestamp(e->timestamp);
                QGuiApplication::sendSpontaneousEvent(window, &touchEvent);
            }
            continue;
        }

        if (activePopup && activePopup != window) {
            // If the popup handles the event, we're done.
            if (window->d_func()->forwardToPopup(&touchEvent, active_popup_on_press))
                return;
        }

        // Note: after the call to sendSpontaneousEvent, touchEvent.position() will have
        // changed to reflect the local position inside the last (random) widget it tried
        // to deliver the touch event to, and will therefore be invalid afterwards.
        QGuiApplication::sendSpontaneousEvent(window, &touchEvent);

        if (!e->synthetic() && !touchEvent.isAccepted() && qApp->testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents)) {
            // exclude devices which generate their own mouse events
            if (!(touchEvent.device()->capabilities().testFlag(QInputDevice::Capability::MouseEmulation))) {

                QEvent::Type mouseEventType = QEvent::MouseMove;
                Qt::MouseButton button = Qt::NoButton;
                Qt::MouseButtons buttons = Qt::LeftButton;
                if (eventType == QEvent::TouchBegin || m_fakeMouseSourcePointId < 0) {
                    m_fakeMouseSourcePointId = touchEvent.point(0).id();
                    qCDebug(lcPtrDispatch) << "synthesizing mouse events from touchpoint" << m_fakeMouseSourcePointId;
                }
                if (m_fakeMouseSourcePointId >= 0) {
                    const auto *touchPoint = touchEvent.pointById(m_fakeMouseSourcePointId);
                    if (touchPoint) {
                        switch (touchPoint->state()) {
                        case QEventPoint::State::Pressed:
                            mouseEventType = QEvent::MouseButtonPress;
                            button = Qt::LeftButton;
                            break;
                        case QEventPoint::State::Released:
                            mouseEventType = QEvent::MouseButtonRelease;
                            button = Qt::LeftButton;
                            buttons = Qt::NoButton;
                            Q_ASSERT(m_fakeMouseSourcePointId == touchPoint->id());
                            m_fakeMouseSourcePointId = -1;
                            break;
                        default:
                            break;
                        }
                        if (touchPoint->state() != QEventPoint::State::Released) {
                            self->synthesizedMousePoints.insert(window, SynthesizedMouseData(
                                                                    touchPoint->position(), touchPoint->globalPosition(), window));
                        }
                        // All touch events that are not accepted by the application will be translated to
                        // left mouse button events instead (see AA_SynthesizeMouseForUnhandledTouchEvents docs).
                        // Sending a QPA event (rather than simply sending a QMouseEvent) takes care of
                        // side-effects such as double-click synthesis.
                        QWindowSystemInterfacePrivate::MouseEvent fake(window, e->timestamp,
                                                                       window->mapFromGlobal(touchPoint->globalPosition().toPoint()),
                                                                       touchPoint->globalPosition(),
                                                                       buttons,
                                                                       e->modifiers,
                                                                       button,
                                                                       mouseEventType,
                                                                       Qt::MouseEventSynthesizedByQt,
                                                                       false,
                                                                       device,
                                                                       touchPoint->id());
                        fake.flags |= QWindowSystemInterfacePrivate::WindowSystemEvent::Synthetic;
                        processMouseEvent(&fake);
                    }
                }
                if (eventType == QEvent::TouchEnd)
                    self->synthesizedMousePoints.clear();
            }
        }
    }

    // Remove released points from QPointingDevicePrivate::activePoints only after the event is
    // delivered.  Widgets and Qt Quick are allowed to access them at any time before this.
    for (const QEventPoint &touchPoint : e->points) {
        if (touchPoint.state() == QEventPoint::State::Released)
            devPriv->removePointById(touchPoint.id());
    }
}

void QGuiApplicationPrivate::processScreenOrientationChange(QWindowSystemInterfacePrivate::ScreenOrientationEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    QScreen *s = e->screen.data();
    s->d_func()->orientation = e->orientation;

    emit s->orientationChanged(s->orientation());

    QScreenOrientationChangeEvent event(s, s->orientation());
    QCoreApplication::sendEvent(QCoreApplication::instance(), &event);
}

void QGuiApplicationPrivate::processScreenGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    {
        QScreen *s = e->screen.data();
        QScreenPrivate::UpdateEmitter updateEmitter(s);

        // Note: The incoming geometries have already been scaled by QHighDpi
        // in the QWSI layer, so we don't need to call updateGeometry() here.
        s->d_func()->geometry = e->geometry;
        s->d_func()->availableGeometry = e->availableGeometry;

        s->d_func()->updatePrimaryOrientation();
    }

    resetCachedDevicePixelRatio();
}

void QGuiApplicationPrivate::processScreenLogicalDotsPerInchChange(QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    QHighDpiScaling::updateHighDpiScaling();

    if (!e->screen)
        return;

    {
        QScreen *s = e->screen.data();
        QScreenPrivate::UpdateEmitter updateEmitter(s);
        s->d_func()->logicalDpi = QDpi(e->dpiX, e->dpiY);
        s->d_func()->updateGeometry();
    }

    for (QWindow *window : QGuiApplication::allWindows())
        if (window->screen() == e->screen)
            QWindowPrivate::get(window)->updateDevicePixelRatio();

    resetCachedDevicePixelRatio();
}

void QGuiApplicationPrivate::processScreenRefreshRateChange(QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    QScreen *s = e->screen.data();
    qreal rate = e->rate;
    // safeguard ourselves against buggy platform behavior...
    if (rate < 1.0)
        rate = 60.0;
    if (!qFuzzyCompare(s->d_func()->refreshRate, rate)) {
        s->d_func()->refreshRate = rate;
        emit s->refreshRateChanged(s->refreshRate());
    }
}

void QGuiApplicationPrivate::processExposeEvent(QWindowSystemInterfacePrivate::ExposeEvent *e)
{
    if (!e->window)
        return;

    QWindow *window = e->window.data();
    if (!window)
        return;
    QWindowPrivate *p = qt_window_private(window);

    if (!p->receivedExpose) {
        if (p->resizeEventPending) {
            // as a convenience for plugins, send a resize event before the first expose event if they haven't done so
            // window->geometry() should have a valid size as soon as a handle exists.
            QResizeEvent e(window->geometry().size(), p->geometry.size());
            QGuiApplication::sendSpontaneousEvent(window, &e);

            p->resizeEventPending = false;
        }

        // FIXME: It would logically make sense to set this _after_ we've sent the
        // expose event to the window, to mark that it now has received an expose.
        // But some parts of Qt (mis)use this private member to check whether the
        // window has been mapped yet, which they do in code that is triggered
        // by the very same expose event we send below. To keep the code working
        // we need to set the variable up front, until the code has been fixed.
        p->receivedExpose = true;
    }

    // If the platform does not send paint events we need to synthesize them from expose events
    const bool shouldSynthesizePaintEvents = !platformIntegration()->hasCapability(QPlatformIntegration::PaintEvents);

    const bool wasExposed = p->exposed;
    p->exposed = e->isExposed && window->screen();

    // We expect that the platform plugins send DevicePixelRatioChange events.
    // As a fail-safe make a final check here to make sure the cached DPR value is
    // always up to date before sending the expose event.
    if (e->isExposed && !e->region.isEmpty()) {
        const bool dprWasChanged = QWindowPrivate::get(window)->updateDevicePixelRatio();
        if (dprWasChanged)
            qWarning() << "The cached device pixel ratio value was stale on window expose. "
                       << "Please file a QTBUG which explains how to reproduce.";
    }

    // We treat expose events for an already exposed window as paint events
    if (wasExposed && p->exposed && shouldSynthesizePaintEvents) {
        QPaintEvent paintEvent(e->region);
        QCoreApplication::sendSpontaneousEvent(window, &paintEvent);
        if (paintEvent.isAccepted())
            return; // No need to send expose

        // The paint event was not accepted, so we fall through and send an expose
        // event instead, to maintain compatibility for clients that haven't adopted
        // paint events yet.
    }

    QExposeEvent exposeEvent(e->region);
    QCoreApplication::sendSpontaneousEvent(window, &exposeEvent);
    e->eventAccepted = exposeEvent.isAccepted();

    // If the window was just exposed we also need to send a paint event,
    // so that clients that implement paint events will draw something.
    // Note that we we can not skip this based on the expose event being
    // accepted, as clients may implement exposeEvent to track the state
    // change, but without drawing anything.
    if (!wasExposed && p->exposed && shouldSynthesizePaintEvents) {
        QPaintEvent paintEvent(e->region);
        QCoreApplication::sendSpontaneousEvent(window, &paintEvent);
    }
}

void QGuiApplicationPrivate::processPaintEvent(QWindowSystemInterfacePrivate::PaintEvent *e)
{
    Q_ASSERT_X(platformIntegration()->hasCapability(QPlatformIntegration::PaintEvents), "QGuiApplication",
        "The platform sent paint events without claiming support for it in QPlatformIntegration::capabilities()");

    if (!e->window)
        return;

    QPaintEvent paintEvent(e->region);
    QCoreApplication::sendSpontaneousEvent(e->window, &paintEvent);

    // We report back the accepted state to the platform, so that it can
    // decide when the best time to send the fallback expose event is.
    e->eventAccepted = paintEvent.isAccepted();
}

#if QT_CONFIG(draganddrop)

/*! \internal

  This function updates an internal state to keep the source compatibility.
  ### Qt 6 - Won't need after QTBUG-73829
*/
static void updateMouseAndModifierButtonState(Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    QGuiApplicationPrivate::mouse_buttons = buttons;
    QGuiApplicationPrivate::modifier_buttons = modifiers;
}

QPlatformDragQtResponse QGuiApplicationPrivate::processDrag(QWindow *w, const QMimeData *dropData,
                                                            const QPoint &p, Qt::DropActions supportedActions,
                                                            Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    updateMouseAndModifierButtonState(buttons, modifiers);

    static Qt::DropAction lastAcceptedDropAction = Qt::IgnoreAction;
    QPlatformDrag *platformDrag = platformIntegration()->drag();
    if (!platformDrag || (w && w->d_func()->blockedByModalWindow)) {
        lastAcceptedDropAction = Qt::IgnoreAction;
        return QPlatformDragQtResponse(false, lastAcceptedDropAction, QRect());
    }

    if (!dropData) {
        currentDragWindow = nullptr;
        QDragLeaveEvent e;
        QGuiApplication::sendEvent(w, &e);
        lastAcceptedDropAction = Qt::IgnoreAction;
        return QPlatformDragQtResponse(false, lastAcceptedDropAction, QRect());
    }
    QDragMoveEvent me(p, supportedActions, dropData, buttons, modifiers);

    if (w != currentDragWindow) {
        lastAcceptedDropAction = Qt::IgnoreAction;
        if (currentDragWindow) {
            QDragLeaveEvent e;
            QGuiApplication::sendEvent(currentDragWindow, &e);
        }
        currentDragWindow = w;
        QDragEnterEvent e(p, supportedActions, dropData, buttons, modifiers);
        QGuiApplication::sendEvent(w, &e);
        if (e.isAccepted() && e.dropAction() != Qt::IgnoreAction)
            lastAcceptedDropAction = e.dropAction();
    }

    // Handling 'DragEnter' should suffice for the application.
    if (lastAcceptedDropAction != Qt::IgnoreAction
        && (supportedActions & lastAcceptedDropAction)) {
        me.setDropAction(lastAcceptedDropAction);
        me.accept();
    }
    QGuiApplication::sendEvent(w, &me);
    lastAcceptedDropAction = me.isAccepted() ?
                             me.dropAction() :  Qt::IgnoreAction;
    return QPlatformDragQtResponse(me.isAccepted(), lastAcceptedDropAction, me.answerRect());
}

QPlatformDropQtResponse QGuiApplicationPrivate::processDrop(QWindow *w, const QMimeData *dropData,
                                                            const QPoint &p, Qt::DropActions supportedActions,
                                                            Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    updateMouseAndModifierButtonState(buttons, modifiers);

    currentDragWindow = nullptr;

    QDropEvent de(p, supportedActions, dropData, buttons, modifiers);
    QGuiApplication::sendEvent(w, &de);

    Qt::DropAction acceptedAction = de.isAccepted() ? de.dropAction() : Qt::IgnoreAction;
    QPlatformDropQtResponse response(de.isAccepted(),acceptedAction);
    return response;
}

#endif // QT_CONFIG(draganddrop)

#ifndef QT_NO_CLIPBOARD
/*!
    Returns the object for interacting with the clipboard.
*/
QClipboard * QGuiApplication::clipboard()
{
    if (QGuiApplicationPrivate::qt_clipboard == nullptr) {
        if (!qApp) {
            qWarning("QGuiApplication: Must construct a QGuiApplication before accessing a QClipboard");
            return nullptr;
        }
        QGuiApplicationPrivate::qt_clipboard = new QClipboard(nullptr);
    }
    return QGuiApplicationPrivate::qt_clipboard;
}
#endif

/*!
    \since 5.4
    \fn void QGuiApplication::paletteChanged(const QPalette &palette)
    \deprecated [6.0] Handle QEvent::ApplicationPaletteChange instead.

    This signal is emitted when the \a palette of the application changes. Use
    QEvent::ApplicationPaletteChanged instead.

    \sa palette()
*/

/*!
    Returns the current application palette.

    Roles that have not been explicitly set will reflect the system's platform theme.

    \sa setPalette()
*/

QPalette QGuiApplication::palette()
{
    if (!QGuiApplicationPrivate::app_pal)
        QGuiApplicationPrivate::updatePalette();

    return *QGuiApplicationPrivate::app_pal;
}

void QGuiApplicationPrivate::updatePalette()
{
    if (app_pal) {
        if (setPalette(*app_pal) && qGuiApp)
            qGuiApp->d_func()->handlePaletteChanged();
    } else {
        setPalette(QPalette());
    }
}

QEvent::Type QGuiApplicationPrivate::contextMenuEventType()
{
    switch (QGuiApplication::styleHints()->contextMenuTrigger()) {
    case Qt::ContextMenuTrigger::Press: return QEvent::MouseButtonPress;
    case Qt::ContextMenuTrigger::Release: return QEvent::MouseButtonRelease;
    }
    return QEvent::None;
}

void QGuiApplicationPrivate::clearPalette()
{
    delete app_pal;
    app_pal = nullptr;
}

/*!
    Changes the application palette to \a pal.

    The color roles from this palette are combined with the system's platform
    theme to form the application's final palette.

    \sa palette()
*/
void QGuiApplication::setPalette(const QPalette &pal)
{
    if (QGuiApplicationPrivate::setPalette(pal) && qGuiApp)
        qGuiApp->d_func()->handlePaletteChanged();
}

bool QGuiApplicationPrivate::setPalette(const QPalette &palette)
{
    // Resolve the palette against the theme palette, filling in
    // any missing roles, while keeping the original resolve mask.
    QPalette basePalette = qGuiApp ? qGuiApp->d_func()->basePalette() : Qt::gray;
    basePalette.setResolveMask(0); // The base palette only contributes missing colors roles
    QPalette resolvedPalette = palette.resolve(basePalette);

    if (app_pal && resolvedPalette == *app_pal && resolvedPalette.resolveMask() == app_pal->resolveMask())
        return false;

    if (!app_pal)
        app_pal = new QPalette(resolvedPalette);
    else
        *app_pal = resolvedPalette;

    QCoreApplication::setAttribute(Qt::AA_SetPalette, app_pal->resolveMask() != 0);

    return true;
}

/*
    Returns the base palette used to fill in missing roles in
    the current application palette.

    Normally this is the theme palette, but QApplication
    overrides this for compatibility reasons.
*/
QPalette QGuiApplicationPrivate::basePalette() const
{
    const auto pf = platformTheme();
    return pf && pf->palette() ? *pf->palette() : Qt::gray;
}

void QGuiApplicationPrivate::handlePaletteChanged(const char *className)
{
#if QT_DEPRECATED_SINCE(6, 0)
    if (!className) {
        Q_ASSERT(app_pal);
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        emit qGuiApp->paletteChanged(*QGuiApplicationPrivate::app_pal);
QT_WARNING_POP
    }
#else
    Q_UNUSED(className);
#endif // QT_DEPRECATED_SINCE(6, 0)

    if (is_app_running && !is_app_closing) {
        QEvent event(QEvent::ApplicationPaletteChange);
        QGuiApplication::sendEvent(qGuiApp, &event);
    }
}

void QGuiApplicationPrivate::applyWindowGeometrySpecificationTo(QWindow *window)
{
    windowGeometrySpecification.applyTo(window);
}

/*!
    \since 5.11
    \fn void QGuiApplication::fontChanged(const QFont &font)
    \deprecated [6.0] Handle QEvent::ApplicationFontChange instead.

    This signal is emitted when the \a font of the application changes. Use
    QEvent::ApplicationFontChanged instead.

    \sa font()
*/

/*!
    Returns the default application font.

    \sa setFont()
*/
QFont QGuiApplication::font()
{
    const auto locker = qt_scoped_lock(applicationFontMutex);
    if (!QGuiApplicationPrivate::self && !QGuiApplicationPrivate::app_font) {
        qWarning("QGuiApplication::font(): no QGuiApplication instance and no application font set.");
        return QFont();  // in effect: QFont((QFontPrivate*)nullptr), so no recursion
    }
    initFontUnlocked();
    return *QGuiApplicationPrivate::app_font;
}

/*!
    Changes the default application font to \a font.

    \sa font()
*/
void QGuiApplication::setFont(const QFont &font)
{
    auto locker = qt_unique_lock(applicationFontMutex);
    const bool emitChange = !QGuiApplicationPrivate::app_font
                            || (*QGuiApplicationPrivate::app_font != font);
    if (!QGuiApplicationPrivate::app_font)
        QGuiApplicationPrivate::app_font = new QFont(font);
    else
        *QGuiApplicationPrivate::app_font = font;
    applicationResourceFlags |= ApplicationFontExplicitlySet;

    if (emitChange && qGuiApp) {
        auto font = *QGuiApplicationPrivate::app_font;
        locker.unlock();
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        emit qGuiApp->fontChanged(font);
QT_WARNING_POP
#else
        Q_UNUSED(font);
#endif // QT_DEPRECATED_SINCE(6, 0)
        QEvent event(QEvent::ApplicationFontChange);
        QGuiApplication::sendEvent(qGuiApp, &event);
    }
}

/*!
    \fn bool QGuiApplication::isRightToLeft()

    Returns \c true if the application's layout direction is
    Qt::RightToLeft; otherwise returns \c false.

    \sa layoutDirection(), isLeftToRight()
*/

/*!
    \fn bool QGuiApplication::isLeftToRight()

    Returns \c true if the application's layout direction is
    Qt::LeftToRight; otherwise returns \c false.

    \sa layoutDirection(), isRightToLeft()
*/

void QGuiApplicationPrivate::notifyLayoutDirectionChange()
{
    const QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = 0; i < list.size(); ++i) {
        QEvent ev(QEvent::ApplicationLayoutDirectionChange);
        QCoreApplication::sendEvent(list.at(i), &ev);
    }
}

void QGuiApplicationPrivate::notifyActiveWindowChange(QWindow *prev)
{
    if (prev) {
        QEvent de(QEvent::WindowDeactivate);
        QCoreApplication::sendEvent(prev, &de);
    }
    if (self->focus_window) {
        QEvent ae(QEvent::WindowActivate);
        QCoreApplication::sendEvent(focus_window, &ae);
    }
}

/*!
    \property QGuiApplication::windowIcon
    \brief the default window icon

    \sa QWindow::setIcon(), {Setting the Application Icon}
*/
QIcon QGuiApplication::windowIcon()
{
    return QGuiApplicationPrivate::app_icon ? *QGuiApplicationPrivate::app_icon : QIcon();
}

void QGuiApplication::setWindowIcon(const QIcon &icon)
{
    if (!QGuiApplicationPrivate::app_icon)
        QGuiApplicationPrivate::app_icon = new QIcon();
    *QGuiApplicationPrivate::app_icon = icon;
    if (QGuiApplicationPrivate::platform_integration
            && QGuiApplicationPrivate::platform_integration->hasCapability(QPlatformIntegration::ApplicationIcon))
        QGuiApplicationPrivate::platform_integration->setApplicationIcon(icon);
    if (QGuiApplicationPrivate::is_app_running && !QGuiApplicationPrivate::is_app_closing)
        QGuiApplicationPrivate::self->notifyWindowIconChanged();
}

void QGuiApplicationPrivate::notifyWindowIconChanged()
{
    QEvent ev(QEvent::ApplicationWindowIconChange);
    const QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = 0; i < list.size(); ++i)
        QCoreApplication::sendEvent(list.at(i), &ev);
}



/*!
    \property QGuiApplication::quitOnLastWindowClosed

    \brief whether the application implicitly quits when the last window is
    closed.

    The default is \c true.

    If this property is \c true, the application will attempt to
    quit when the last visible \l{Primary and Secondary Windows}{primary window}
    (i.e. top level window with no transient parent) is closed.

    Note that attempting a quit may not necessarily result in the
    application quitting, for example if there still are active
    QEventLoopLocker instances, or the QEvent::Quit event is ignored.

    \sa quit(), QWindow::close()
 */

void QGuiApplication::setQuitOnLastWindowClosed(bool quit)
{
    QGuiApplicationPrivate::quitOnLastWindowClosed = quit;
}

bool QGuiApplication::quitOnLastWindowClosed()
{
    return QGuiApplicationPrivate::quitOnLastWindowClosed;
}

void QGuiApplicationPrivate::maybeLastWindowClosed()
{
    if (!lastWindowClosed())
        return;

    if (in_exec)
        emit q_func()->lastWindowClosed();

    if (quitOnLastWindowClosed && canQuitAutomatically())
        quitAutomatically();
}

/*!
    \fn void QGuiApplication::lastWindowClosed()

    This signal is emitted from exec() when the last visible
    \l{Primary and Secondary Windows}{primary window} (i.e.
    top level window with no transient parent) is closed.

    By default, QGuiApplication quits after this signal is emitted. This feature
    can be turned off by setting \l quitOnLastWindowClosed to \c false.

    \sa QWindow::close(), QWindow::isTopLevel(), QWindow::transientParent()
*/

bool QGuiApplicationPrivate::lastWindowClosed() const
{
    for (auto *window : QGuiApplication::topLevelWindows()) {
        auto *windowPrivate = qt_window_private(window);
        if (!windowPrivate->participatesInLastWindowClosed())
            continue;

        if (windowPrivate->treatAsVisible())
            return false;
     }

     return true;
}

bool QGuiApplicationPrivate::canQuitAutomatically()
{
    // The automatic quit functionality is triggered by
    // both QEventLoopLocker and maybeLastWindowClosed.
    // Although the former is a QCoreApplication feature
    // we don't want to quit the application when there
    // are open windows, regardless of whether the app
    // also quits automatically on maybeLastWindowClosed.
    if (!lastWindowClosed())
        return false;

    return QCoreApplicationPrivate::canQuitAutomatically();
}

void QGuiApplicationPrivate::quit()
{
    if (auto *platformIntegration = QGuiApplicationPrivate::platformIntegration())
        platformIntegration->quit();
    else
        QCoreApplicationPrivate::quit();
}

void QGuiApplicationPrivate::processApplicationTermination(QWindowSystemInterfacePrivate::WindowSystemEvent *windowSystemEvent)
{
    QEvent event(QEvent::Quit);
    QGuiApplication::sendSpontaneousEvent(QGuiApplication::instance(), &event);
    windowSystemEvent->eventAccepted = event.isAccepted();
}

/*!
    \since 5.2
    \fn Qt::ApplicationState QGuiApplication::applicationState()


    Returns the current state of the application.

    You can react to application state changes to perform actions such as
    stopping/resuming CPU-intensive tasks, freeing/loading resources or
    saving/restoring application data.
 */

Qt::ApplicationState QGuiApplication::applicationState()
{
    return QGuiApplicationPrivate::applicationState;
}

/*!
    \since 5.14

    Sets the high-DPI scale factor rounding policy for the application. The
    \a policy decides how non-integer scale factors (such as Windows 150%) are
    handled.

    The two principal options are whether fractional scale factors should
    be rounded to an integer or not. Keeping the scale factor as-is will
    make the user interface size match the OS setting exactly, but may cause
    painting errors, for example with the Windows style.

    If rounding is wanted, then which type of rounding should be decided
    next. Mathematically correct rounding is supported but may not give
    the best visual results: Consider if you want to render 1.5x as 1x
    ("small UI") or as 2x ("large UI"). See the Qt::HighDpiScaleFactorRoundingPolicy
    enum for a complete list of all options.

    This function must be called before creating the application object.
    The QGuiApplication::highDpiScaleFactorRoundingPolicy()
    accessor will reflect the environment, if set.

    The default value is Qt::HighDpiScaleFactorRoundingPolicy::PassThrough.
*/
void QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy policy)
{
    if (qApp)
        qWarning("setHighDpiScaleFactorRoundingPolicy must be called before creating the QGuiApplication instance");
    QGuiApplicationPrivate::highDpiScaleFactorRoundingPolicy = policy;
}

/*!
  \since 5.14

  Returns the high-DPI scale factor rounding policy.
*/
Qt::HighDpiScaleFactorRoundingPolicy QGuiApplication::highDpiScaleFactorRoundingPolicy()
{
    return QGuiApplicationPrivate::highDpiScaleFactorRoundingPolicy;
}

/*!
    \since 5.2
    \fn void QGuiApplication::applicationStateChanged(Qt::ApplicationState state)

    This signal is emitted when the \a state of the application changes.

    \sa applicationState()
*/

void QGuiApplicationPrivate::setApplicationState(Qt::ApplicationState state, bool forcePropagate)
{
    if ((applicationState == state) && !forcePropagate)
        return;

    applicationState = state;

    switch (state) {
    case Qt::ApplicationActive: {
        QEvent appActivate(QEvent::ApplicationActivate);
        QCoreApplication::sendSpontaneousEvent(qApp, &appActivate);
        break; }
    case Qt::ApplicationInactive: {
        QEvent appDeactivate(QEvent::ApplicationDeactivate);
        QCoreApplication::sendSpontaneousEvent(qApp, &appDeactivate);
        break; }
    default:
        break;
    }

    QApplicationStateChangeEvent event(applicationState);
    QCoreApplication::sendSpontaneousEvent(qApp, &event);

    emit qApp->applicationStateChanged(applicationState);
}

/*!
    \since 4.2
    \fn void QGuiApplication::commitDataRequest(QSessionManager &manager)

    This signal deals with \l{Session Management}{session management}. It is
    emitted when the QSessionManager wants the application to commit all its
    data.

    Usually this means saving all open files, after getting permission from
    the user. Furthermore you may want to provide a means by which the user
    can cancel the shutdown.

    You should not exit the application within this signal. Instead,
    the session manager may or may not do this afterwards, depending on the
    context.

    \warning Within this signal, no user interaction is possible, \e
    unless you ask the \a manager for explicit permission. See
    QSessionManager::allowsInteraction() and
    QSessionManager::allowsErrorInteraction() for details and example
    usage.

    \note You should use Qt::DirectConnection when connecting to this signal.

    \sa isSessionRestored(), sessionId(), saveStateRequest(), {Session Management}
*/

/*!
    \since 4.2
    \fn void QGuiApplication::saveStateRequest(QSessionManager &manager)

    This signal deals with \l{Session Management}{session management}. It is
    invoked when the \l{QSessionManager}{session manager} wants the application
    to preserve its state for a future session.

    For example, a text editor would create a temporary file that includes the
    current contents of its edit buffers, the location of the cursor and other
    aspects of the current editing session.

    You should never exit the application within this signal. Instead, the
    session manager may or may not do this afterwards, depending on the
    context. Furthermore, most session managers will very likely request a saved
    state immediately after the application has been started. This permits the
    session manager to learn about the application's restart policy.

    \warning Within this signal, no user interaction is possible, \e
    unless you ask the \a manager for explicit permission. See
    QSessionManager::allowsInteraction() and
    QSessionManager::allowsErrorInteraction() for details.

    \note You should use Qt::DirectConnection when connecting to this signal.

    \sa isSessionRestored(), sessionId(), commitDataRequest(), {Session Management}
*/

/*!
    \fn bool QGuiApplication::isSessionRestored() const

    Returns \c true if the application has been restored from an earlier
    \l{Session Management}{session}; otherwise returns \c false.

    \sa sessionId(), commitDataRequest(), saveStateRequest()
*/

/*!
    \since 5.0
    \fn bool QGuiApplication::isSavingSession() const

    Returns \c true if the application is currently saving the
    \l{Session Management}{session}; otherwise returns \c false.

    This is \c true when commitDataRequest() and saveStateRequest() are emitted,
    but also when the windows are closed afterwards by session management.

    \sa sessionId(), commitDataRequest(), saveStateRequest()
*/

/*!
    \fn QString QGuiApplication::sessionId() const

    Returns the current \l{Session Management}{session's} identifier.

    If the application has been restored from an earlier session, this
    identifier is the same as it was in that previous session. The session
    identifier is guaranteed to be unique both for different applications
    and for different instances of the same application.

    \sa isSessionRestored(), sessionKey(), commitDataRequest(), saveStateRequest()
*/

/*!
    \fn QString QGuiApplication::sessionKey() const

    Returns the session key in the current \l{Session Management}{session}.

    If the application has been restored from an earlier session, this key is
    the same as it was when the previous session ended.

    The session key changes every time the session is saved. If the shutdown process
    is cancelled, another session key will be used when shutting down again.

    \sa isSessionRestored(), sessionId(), commitDataRequest(), saveStateRequest()
*/
#ifndef QT_NO_SESSIONMANAGER
bool QGuiApplication::isSessionRestored() const
{
    Q_D(const QGuiApplication);
    return d->is_session_restored;
}

QString QGuiApplication::sessionId() const
{
    Q_D(const QGuiApplication);
    return d->session_manager->sessionId();
}

QString QGuiApplication::sessionKey() const
{
    Q_D(const QGuiApplication);
    return d->session_manager->sessionKey();
}

bool QGuiApplication::isSavingSession() const
{
    Q_D(const QGuiApplication);
    return d->is_saving_session;
}

void QGuiApplicationPrivate::commitData()
{
    Q_Q(QGuiApplication);
    is_saving_session = true;
    emit q->commitDataRequest(*session_manager);
    is_saving_session = false;
}


void QGuiApplicationPrivate::saveState()
{
    Q_Q(QGuiApplication);
    is_saving_session = true;
    emit q->saveStateRequest(*session_manager);
    is_saving_session = false;
}
#endif //QT_NO_SESSIONMANAGER

/*!
    \since 5.2

    Function that can be used to sync Qt state with the Window Systems state.

    This function will first empty Qts events by calling QCoreApplication::processEvents(),
    then the platform plugin will sync up with the windowsystem, and finally Qts events
    will be delived by another call to QCoreApplication::processEvents();

    This function is timeconsuming and its use is discouraged.
*/
void QGuiApplication::sync()
{
    QCoreApplication::processEvents();
    if (QGuiApplicationPrivate::platform_integration
            && QGuiApplicationPrivate::platform_integration->hasCapability(QPlatformIntegration::SyncState)) {
        QGuiApplicationPrivate::platform_integration->sync();
        QCoreApplication::processEvents();
        QWindowSystemInterface::flushWindowSystemEvents();
    }
}

/*!
    \property QGuiApplication::layoutDirection
    \brief the default layout direction for this application

    On system start-up, or when the direction is explicitly set to
    Qt::LayoutDirectionAuto, the default layout direction depends on the
    application's language.

    The notifier signal was introduced in Qt 5.4.

    \sa QWidget::layoutDirection, isLeftToRight(), isRightToLeft()
 */

void QGuiApplication::setLayoutDirection(Qt::LayoutDirection direction)
{
    layout_direction = direction;
    if (direction == Qt::LayoutDirectionAuto)
        direction = qt_detectRTLLanguage() ? Qt::RightToLeft : Qt::LeftToRight;

    // no change to the explicitly set or auto-detected layout direction
    if (direction == effective_layout_direction)
        return;

    effective_layout_direction = direction;
    if (qGuiApp) {
        emit qGuiApp->layoutDirectionChanged(direction);
        QGuiApplicationPrivate::self->notifyLayoutDirectionChange();
    }
}

Qt::LayoutDirection QGuiApplication::layoutDirection()
{
    /*
        effective_layout_direction defaults to Qt::LeftToRight, and is updated with what is
        auto-detected by a call to setLayoutDirection(Qt::LayoutDirectionAuto). This happens in
        QGuiApplicationPrivate::init and when the language changes (or before if the application
        calls the static function, but then no translators are installed so the auto-detection
        always yields Qt::LeftToRight).
        So we can be certain that it's always the right value.
    */
    return effective_layout_direction;
}

/*!
    \fn QCursor *QGuiApplication::overrideCursor()

    Returns the active application override cursor.

    This function returns \nullptr if no application cursor has been defined (i.e. the
    internal cursor stack is empty).

    \sa setOverrideCursor(), restoreOverrideCursor()
*/
#ifndef QT_NO_CURSOR
QCursor *QGuiApplication::overrideCursor()
{
    CHECK_QAPP_INSTANCE(nullptr)
    return qGuiApp->d_func()->cursor_list.isEmpty() ? nullptr : &qGuiApp->d_func()->cursor_list.first();
}

/*!
    Changes the currently active application override cursor to \a cursor.

    This function has no effect if setOverrideCursor() was not called.

    \sa setOverrideCursor(), overrideCursor(), restoreOverrideCursor(),
    QWidget::setCursor()
 */
void QGuiApplication::changeOverrideCursor(const QCursor &cursor)
{
    CHECK_QAPP_INSTANCE()
    if (qGuiApp->d_func()->cursor_list.isEmpty())
        return;
    qGuiApp->d_func()->cursor_list.removeFirst();
    setOverrideCursor(cursor);
}
#endif


#ifndef QT_NO_CURSOR
static inline void applyCursor(QWindow *w, QCursor c)
{
    if (const QScreen *screen = w->screen())
        if (QPlatformCursor *cursor = screen->handle()->cursor())
            cursor->changeCursor(&c, w);
}

static inline void unsetCursor(QWindow *w)
{
    if (const QScreen *screen = w->screen())
        if (QPlatformCursor *cursor = screen->handle()->cursor())
            cursor->changeCursor(nullptr, w);
}

static inline void applyCursor(const QList<QWindow *> &l, const QCursor &c)
{
    for (int i = 0; i < l.size(); ++i) {
        QWindow *w = l.at(i);
        if (w->handle() && w->type() != Qt::Desktop)
            applyCursor(w, c);
    }
}

static inline void applyOverrideCursor(const QList<QScreen *> &screens, const QCursor &c)
{
    for (QScreen *screen : screens) {
        if (QPlatformCursor *cursor = screen->handle()->cursor())
            cursor->setOverrideCursor(c);
    }
}

static inline void clearOverrideCursor(const QList<QScreen *> &screens)
{
    for (QScreen *screen : screens) {
        if (QPlatformCursor *cursor = screen->handle()->cursor())
            cursor->clearOverrideCursor();
    }
}

static inline void applyWindowCursor(const QList<QWindow *> &l)
{
    for (int i = 0; i < l.size(); ++i) {
        QWindow *w = l.at(i);
        if (w->handle() && w->type() != Qt::Desktop) {
            if (qt_window_private(w)->hasCursor) {
                applyCursor(w, w->cursor());
            } else {
                unsetCursor(w);
            }
        }
    }
}

/*!
    \fn void QGuiApplication::setOverrideCursor(const QCursor &cursor)

    Sets the application override cursor to \a cursor.

    Application override cursors are intended for showing the user that the
    application is in a special state, for example during an operation that
    might take some time.

    This cursor will be displayed in all the application's widgets until
    restoreOverrideCursor() or another setOverrideCursor() is called.

    Application cursors are stored on an internal stack. setOverrideCursor()
    pushes the cursor onto the stack, and restoreOverrideCursor() pops the
    active cursor off the stack. changeOverrideCursor() changes the currently
    active application override cursor.

    Every setOverrideCursor() must eventually be followed by a corresponding
    restoreOverrideCursor(), otherwise the stack will never be emptied.

    Example:
    \snippet code/src_gui_kernel_qguiapplication_x11.cpp 0

    \sa overrideCursor(), restoreOverrideCursor(), changeOverrideCursor(),
    QWidget::setCursor()
*/
void QGuiApplication::setOverrideCursor(const QCursor &cursor)
{
    CHECK_QAPP_INSTANCE()
    qGuiApp->d_func()->cursor_list.prepend(cursor);
    if (QPlatformCursor::capabilities().testFlag(QPlatformCursor::OverrideCursor))
        applyOverrideCursor(QGuiApplicationPrivate::screen_list, cursor);
    else
        applyCursor(QGuiApplicationPrivate::window_list, cursor);
}

/*!
    \fn void QGuiApplication::restoreOverrideCursor()

    Undoes the last setOverrideCursor().

    If setOverrideCursor() has been called twice, calling
    restoreOverrideCursor() will activate the first cursor set. Calling this
    function a second time restores the original widgets' cursors.

    \sa setOverrideCursor(), overrideCursor()
*/
void QGuiApplication::restoreOverrideCursor()
{
    CHECK_QAPP_INSTANCE()
    if (qGuiApp->d_func()->cursor_list.isEmpty())
        return;
    qGuiApp->d_func()->cursor_list.removeFirst();
    if (qGuiApp->d_func()->cursor_list.size() > 0) {
        QCursor c(qGuiApp->d_func()->cursor_list.value(0));
        if (QPlatformCursor::capabilities().testFlag(QPlatformCursor::OverrideCursor))
            applyOverrideCursor(QGuiApplicationPrivate::screen_list, c);
        else
            applyCursor(QGuiApplicationPrivate::window_list, c);
    } else {
        if (QPlatformCursor::capabilities().testFlag(QPlatformCursor::OverrideCursor))
            clearOverrideCursor(QGuiApplicationPrivate::screen_list);
        applyWindowCursor(QGuiApplicationPrivate::window_list);
    }
}
#endif// QT_NO_CURSOR

/*!
  Returns the application's style hints.

  The style hints encapsulate a set of platform dependent properties
  such as double click intervals, full width selection and others.

  The hints can be used to integrate tighter with the underlying platform.

  \sa QStyleHints
  */
QStyleHints *QGuiApplication::styleHints()
{
    if (!QGuiApplicationPrivate::styleHints)
        QGuiApplicationPrivate::styleHints = new QStyleHints();
    return QGuiApplicationPrivate::styleHints;
}

/*!
    Sets whether Qt should use the system's standard colors, fonts, etc., to
    \a on. By default, this is \c true.

    This function must be called before creating the QGuiApplication object, like
    this:

    \snippet code/src_gui_kernel_qguiapplication.cpp 0

    \sa desktopSettingsAware()
*/
void QGuiApplication::setDesktopSettingsAware(bool on)
{
    QGuiApplicationPrivate::obey_desktop_settings = on;
}

/*!
    Returns \c true if Qt is set to use the system's standard colors, fonts, etc.;
    otherwise returns \c false. The default is \c true.

    \sa setDesktopSettingsAware()
*/
bool QGuiApplication::desktopSettingsAware()
{
    return QGuiApplicationPrivate::obey_desktop_settings;
}

/*!
  returns the input method.

  The input method returns properties about the state and position of
  the virtual keyboard. It also provides information about the position of the
  current focused input element.

  \sa QInputMethod
  */
QInputMethod *QGuiApplication::inputMethod()
{
    CHECK_QAPP_INSTANCE(nullptr)
    if (!qGuiApp->d_func()->inputMethod)
        qGuiApp->d_func()->inputMethod = new QInputMethod();
    return qGuiApp->d_func()->inputMethod;
}

/*!
    \fn void QGuiApplication::fontDatabaseChanged()

    This signal is emitted when the available fonts have changed.

    This can happen when application fonts are added or removed, or when the
    system fonts change.

    \sa QFontDatabase::addApplicationFont(),
    QFontDatabase::addApplicationFontFromData(),
    QFontDatabase::removeAllApplicationFonts(),
    QFontDatabase::removeApplicationFont()
*/

QPixmap QGuiApplicationPrivate::getPixmapCursor(Qt::CursorShape cshape)
{
    Q_UNUSED(cshape);
    return QPixmap();
}

QPoint QGuiApplicationPrivate::QLastCursorPosition::toPoint() const noexcept
{
    // Guard against the default initialization of qInf() (avoid UB or SIGFPE in conversion).
    if (Q_UNLIKELY(qIsInf(thePoint.x())))
        return QPoint(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    return thePoint.toPoint();
}

#if QT_CONFIG(draganddrop)
void QGuiApplicationPrivate::notifyDragStarted(const QDrag *drag)
{
    Q_UNUSED(drag);

}
#endif

const QColorTrcLut *QGuiApplicationPrivate::colorProfileForA8Text()
{
#ifdef Q_OS_WIN
    if (!m_a8ColorProfile)
        m_a8ColorProfile = QColorTrcLut::fromGamma(2.31f); // This is a hard-coded thing for Windows text rendering
    return m_a8ColorProfile.get();
#else
    return colorProfileForA32Text();
#endif
}

const QColorTrcLut *QGuiApplicationPrivate::colorProfileForA32Text()
{
    if (!m_a32ColorProfile)
        m_a32ColorProfile = QColorTrcLut::fromGamma(float(fontSmoothingGamma));
    return m_a32ColorProfile.get();
}

void QGuiApplicationPrivate::_q_updateFocusObject(QObject *object)
{
    Q_Q(QGuiApplication);

    QPlatformInputContext *inputContext = platformIntegration()->inputContext();
    const bool enabled = inputContext && QInputMethodPrivate::objectAcceptsInputMethod(object);

    QPlatformInputContextPrivate::setInputMethodAccepted(enabled);
    if (inputContext)
        inputContext->setFocusObject(object);
    emit q->focusObjectChanged(object);
}

enum MouseMasks {
    MouseCapsMask = 0xFF,
    MouseSourceMaskDst = 0xFF00,
    MouseSourceMaskSrc = MouseCapsMask,
    MouseSourceShift = 8,
    MouseFlagsCapsMask = 0xFF0000,
    MouseFlagsShift = 16
};

QInputDeviceManager *QGuiApplicationPrivate::inputDeviceManager()
{
    Q_ASSERT(QGuiApplication::instance());

    if (!m_inputDeviceManager)
        m_inputDeviceManager = new QInputDeviceManager(QGuiApplication::instance());

    return m_inputDeviceManager;
}

/*!
    Returns the QThreadPool instance for Qt Gui.
    \internal
*/
QThreadPool *QGuiApplicationPrivate::qtGuiThreadPool()
{
#if QT_CONFIG(qtgui_threadpool)
    Q_CONSTINIT static QPointer<QThreadPool> guiInstance;
    Q_CONSTINIT static QBasicMutex theMutex;
    const static bool runtime_disable = qEnvironmentVariableIsSet("QT_NO_GUI_THREADPOOL");
    if (runtime_disable)
        return nullptr;
    const QMutexLocker locker(&theMutex);
    if (guiInstance.isNull() && !QCoreApplication::closingDown()) {
        guiInstance = new QThreadPool();
        // Limit max thread to avoid too many parallel threads.
        // We are not optimized for much more than 4 or 8 threads.
        if (guiInstance && guiInstance->maxThreadCount() > 4)
            guiInstance->setMaxThreadCount(qBound(4, guiInstance->maxThreadCount() / 2, 8));
    }
    return guiInstance;
#else
    return nullptr;
#endif
}

/*!
    \fn template <typename QNativeInterface> QNativeInterface *QGuiApplication::nativeInterface() const

    Returns a native interface of the given type for the application.

    This function provides access to platform specific functionality
    of QGuiApplication, as defined in the QNativeInterface namespace:

    \annotatedlist native-interfaces-qguiapplication

    If the requested interface is not available a \nullptr is returned.
 */

void *QGuiApplication::resolveInterface(const char *name, int revision) const
{
    using namespace QNativeInterface;
    using namespace QNativeInterface::Private;

    auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    Q_UNUSED(platformIntegration);

#if defined(Q_OS_WIN)
    QT_NATIVE_INTERFACE_RETURN_IF(QWindowsApplication, platformIntegration);
#endif
#if QT_CONFIG(xcb)
    QT_NATIVE_INTERFACE_RETURN_IF(QX11Application, platformNativeInterface());
#endif
#if QT_CONFIG(wayland)
    QT_NATIVE_INTERFACE_RETURN_IF(QWaylandApplication, platformNativeInterface());
#endif
#if defined(Q_OS_VISIONOS)
    QT_NATIVE_INTERFACE_RETURN_IF(QVisionOSApplication, platformIntegration);
#endif

    return QCoreApplication::resolveInterface(name, revision);
}

QT_END_NAMESPACE

#include "moc_qguiapplication.cpp"
