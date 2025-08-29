// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qt_windows.h>

#include "qwindowstheme.h"
#include "qwindowsmenu.h"
#include "qwindowsdialoghelpers.h"
#include "qwindowscontext.h"
#include "qwindowsiconengine.h"
#include "qwindowsintegration.h"
#if QT_CONFIG(systemtrayicon)
#  include "qwindowssystemtrayicon.h"
#endif
#include "qwindowsscreen.h"
#include "qwindowswindow.h"
#include <commctrl.h>
#include <objbase.h>
#include <commoncontrols.h>
#include <shellapi.h>

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qvariant.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qcache.h>
#include <QtCore/qthread.h>
#include <QtCore/qqueue.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtGui/qcolor.h>
#include <QtGui/qpalette.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpixmapcache.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qabstractfileiconengine_p.h>
#include <QtGui/private/qwindowsfontdatabase_p.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qwinregistry_p.h>
#include <QtCore/private/qfunctions_win_p.h>
#include <QtGui/private/qwindowsthemecache_p.h>

#include <algorithm>

#if QT_CONFIG(cpp_winrt)
#   include <QtCore/private/qt_winrtbase_p.h>

#   include <winrt/Windows.UI.ViewManagement.h>
#endif // QT_CONFIG(cpp_winrt)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static inline bool booleanSystemParametersInfo(UINT what, bool defaultValue)
{
    BOOL result;
    if (SystemParametersInfo(what, 0, &result, 0))
        return result != FALSE;
    return defaultValue;
}

static inline DWORD dWordSystemParametersInfo(UINT what, DWORD defaultValue)
{
    DWORD result;
    if (SystemParametersInfo(what, 0, &result, 0))
        return result;
    return defaultValue;
}

static inline QColor mixColors(const QColor &c1, const QColor &c2)
{
    return {(c1.red() + c2.red()) / 2,
            (c1.green() + c2.green()) / 2,
            (c1.blue() + c2.blue()) / 2};
}

enum AccentColorLevel {
    AccentColorDarkest,
    AccentColorDarker,
    AccentColorDark,
    AccentColorNormal,
    AccentColorLight,
    AccentColorLighter,
    AccentColorLightest
};

#if QT_CONFIG(cpp_winrt)
static constexpr QColor getSysColor(winrt::Windows::UI::Color &&color)
{
    return QColor(color.R, color.G, color.B, color.A);
}
#endif

static inline QColor getSysColor(int index)
{
    COLORREF cr = GetSysColor(index);
    return QColor(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

[[maybe_unused]] [[nodiscard]] static inline QColor qt_accentColor(AccentColorLevel level)
{
#if QT_CONFIG(cpp_winrt)
    using namespace winrt::Windows::UI::ViewManagement;
    const auto settings = UISettings();
    const QColor accent = getSysColor(settings.GetColorValue(UIColorType::Accent));
    const QColor accentLight = getSysColor(settings.GetColorValue(UIColorType::AccentLight1));
    const QColor accentLighter = getSysColor(settings.GetColorValue(UIColorType::AccentLight2));
    const QColor accentLightest = getSysColor(settings.GetColorValue(UIColorType::AccentLight3));
    const QColor accentDark = getSysColor(settings.GetColorValue(UIColorType::AccentDark1));
    const QColor accentDarker = getSysColor(settings.GetColorValue(UIColorType::AccentDark2));
    const QColor accentDarkest = getSysColor(settings.GetColorValue(UIColorType::AccentDark3));
#else
    const QColor accent = []()->QColor {
        // MS uses the aquatic contrast theme as an example in the URL below:
        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsyscolor#windows-1011-system-colors
        if (QWindowsTheme::queryHighContrast())
            return getSysColor(COLOR_HIGHLIGHT);
        const QWinRegistryKey registry(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\DWM)");
        if (!registry.isValid())
            return {};
        const QVariant value = registry.value(L"AccentColor");
        if (!value.isValid())
            return {};
        // The retrieved value is in the #AABBGGRR format, we need to
        // convert it to the #AARRGGBB format which Qt expects.
        const QColor abgr = QColor::fromRgba(qvariant_cast<DWORD>(value));
        if (!abgr.isValid())
            return {};
        return QColor::fromRgb(abgr.blue(), abgr.green(), abgr.red(), abgr.alpha());

    }();
    if (!accent.isValid())
        return {};

    const QColor accentLight = accent.lighter(120);
    const QColor accentLighter = accentLight.lighter(120);
    const QColor accentLightest = accentLighter.lighter(120);
    const QColor accentDark = accent.darker(120);
    const QColor accentDarker = accentDark.darker(120);
    const QColor accentDarkest = accentDarker.darker(120);
#endif
    switch (level) {
    case AccentColorDarkest:
        return accentDarkest;
    case AccentColorDarker:
        return accentDarker;
    case AccentColorDark:
        return accentDark;
    case AccentColorLight:
        return accentLight;
    case AccentColorLighter:
        return accentLighter;
    case AccentColorLightest:
        return accentLightest;
    default:
        return accent;
    }
}

// QTBUG-48823/Windows 10: SHGetFileInfo() (as called by item views on file system
// models has been observed to trigger a WM_PAINT on the mainwindow. Suppress the
// behavior by running it in a thread.
class QShGetFileInfoThread : public QThread
{
public:
    struct Task
    {
        Task(const QString &fn, DWORD a, UINT f)
            : fileName(fn), attributes(a), flags(f)
        {}
        Q_DISABLE_COPY(Task)
        ~Task()
        {
            DestroyIcon(hIcon);
            hIcon = 0;
        }
        // Request
        const QString fileName;
        const DWORD attributes;
        const UINT flags;
        // Result
        HICON hIcon = 0;
        int iIcon = -1;
        bool finished = false;
        bool resultValid() const { return hIcon != 0 && iIcon >= 0 && finished; }
    };

    QShGetFileInfoThread()
        : QThread()
    {
        start();
    }

    ~QShGetFileInfoThread()
    {
        cancel();
        wait();
    }

    QSharedPointer<Task> getNextTask()
    {
        QMutexLocker l(&m_waitForTaskMutex);
        while (!isInterruptionRequested()) {
            if (!m_taskQueue.isEmpty())
                return m_taskQueue.dequeue();
            m_waitForTaskCondition.wait(&m_waitForTaskMutex);
        }
        return nullptr;
    }

    void run() override
    {
        QComHelper comHelper(COINIT_MULTITHREADED);

        while (!isInterruptionRequested()) {
            auto task = getNextTask();
            if (task) {
                SHFILEINFO info;
                const bool result = SHGetFileInfo(reinterpret_cast<const wchar_t *>(task->fileName.utf16()),
                                                  task->attributes, &info, sizeof(SHFILEINFO),
                                                  task->flags);
                if (result) {
                    task->hIcon = info.hIcon;
                    task->iIcon = info.iIcon;
                }
                task->finished = true;
                m_doneCondition.wakeAll();
            }
        }
    }

    void runWithParams(const QSharedPointer<Task> &task,
                       std::chrono::milliseconds timeout = std::chrono::milliseconds(5000))
    {
        {
            QMutexLocker l(&m_waitForTaskMutex);
            m_taskQueue.enqueue(task);
            m_waitForTaskCondition.wakeAll();
        }

        QMutexLocker doneLocker(&m_doneMutex);
        while (!task->finished && !isInterruptionRequested()) {
            if (!m_doneCondition.wait(&m_doneMutex, QDeadlineTimer(timeout)))
                return;
        }
    }

    void cancel()
    {
        requestInterruption();
        m_doneCondition.wakeAll();
        m_waitForTaskCondition.wakeAll();
    }

private:
    QQueue<QSharedPointer<Task>> m_taskQueue;
    QWaitCondition m_doneCondition;
    QWaitCondition m_waitForTaskCondition;
    QMutex m_doneMutex;
    QMutex m_waitForTaskMutex;
};
Q_APPLICATION_STATIC(QShGetFileInfoThread, s_shGetFileInfoThread)

// from QStyle::standardPalette
static inline QPalette standardPalette()
{
    QColor backgroundColor(0xd4, 0xd0, 0xc8); // win 2000 grey
    QColor lightColor(backgroundColor.lighter());
    QColor darkColor(backgroundColor.darker());
    const QBrush darkBrush(darkColor);
    QColor midColor(Qt::gray);
    QPalette palette(Qt::black, backgroundColor, lightColor, darkColor,
                     midColor, Qt::black, Qt::white);
    palette.setBrush(QPalette::Disabled, QPalette::WindowText, darkBrush);
    palette.setBrush(QPalette::Disabled, QPalette::Text, darkBrush);
    palette.setBrush(QPalette::Disabled, QPalette::ButtonText, darkBrush);
    palette.setBrush(QPalette::Disabled, QPalette::Base, QBrush(backgroundColor));
    return palette;
}

static QColor placeHolderColor(QColor textColor)
{
    textColor.setAlpha(128);
    return textColor;
}

/*
    This is used when the theme is light mode, and when the theme is dark but the
    application doesn't support dark mode. In the latter case, we need to check.
*/
void QWindowsTheme::populateLightSystemBasePalette(QPalette &result)
{
    const bool highContrastEnabled = queryHighContrast();

    const QColor background = getSysColor(COLOR_BTNFACE);
    const QColor textColor = getSysColor(COLOR_WINDOWTEXT);

    const QColor accent = qt_accentColor(AccentColorNormal);
    const QColor accentDark = qt_accentColor(AccentColorDark);
    const QColor accentDarker = qt_accentColor(AccentColorDarker);
    const QColor accentDarkest = qt_accentColor(AccentColorDarkest);

    const QColor linkColor = highContrastEnabled ? getSysColor(COLOR_HOTLIGHT) : accentDarker;
    const QColor linkColorVisited = highContrastEnabled ? linkColor.darker(120) : accentDarkest;
    const QColor btnFace = background;
    const QColor btnHighlight = getSysColor(COLOR_BTNHIGHLIGHT);

    result.setColor(QPalette::Highlight, accent);
    result.setColor(QPalette::WindowText, getSysColor(COLOR_WINDOWTEXT));
    result.setColor(QPalette::Button, btnFace);
    result.setColor(QPalette::Light, btnHighlight);
    result.setColor(QPalette::Dark, getSysColor(COLOR_BTNSHADOW));
    result.setColor(QPalette::Mid, result.button().color().darker(150));
    result.setColor(QPalette::Text, textColor);
    result.setColor(QPalette::PlaceholderText, placeHolderColor(textColor));
    result.setColor(QPalette::BrightText, btnHighlight);
    result.setColor(QPalette::Base, getSysColor(COLOR_WINDOW));
    result.setColor(QPalette::Window, highContrastEnabled ? getSysColor(COLOR_WINDOW) : btnFace);
    result.setColor(QPalette::ButtonText, getSysColor(COLOR_BTNTEXT));
    result.setColor(QPalette::Midlight, getSysColor(COLOR_3DLIGHT));
    result.setColor(QPalette::Shadow, getSysColor(COLOR_3DDKSHADOW));
    result.setColor(QPalette::HighlightedText, getSysColor(COLOR_HIGHLIGHTTEXT));
    result.setColor(QPalette::Accent, accentDark); // default accent color for controls on Light mode is AccentDark1

    result.setColor(QPalette::Link, linkColor);
    result.setColor(QPalette::LinkVisited, linkColorVisited);
    result.setColor(QPalette::Inactive, QPalette::Button, result.button().color());
    result.setColor(QPalette::Inactive, QPalette::Window, result.window().color());
    result.setColor(QPalette::Inactive, QPalette::Light, result.light().color());
    result.setColor(QPalette::Inactive, QPalette::Dark, result.dark().color());

    if (highContrastEnabled)
        result.setColor(QPalette::Inactive, QPalette::WindowText, getSysColor(COLOR_GRAYTEXT));

    if (result.midlight() == result.button())
        result.setColor(QPalette::Midlight, result.button().color().lighter(110));
}

void QWindowsTheme::populateDarkSystemBasePalette(QPalette &result)
{
    QColor foreground, background,
           accent, accentDark, accentDarker, accentDarkest,
           accentLight, accentLighter, accentLightest;
#if QT_CONFIG(cpp_winrt)
    using namespace winrt::Windows::UI::ViewManagement;
    const auto settings = UISettings();

    // We have to craft a palette from these colors. The settings.UIElementColor(UIElementType) API
    // returns the old system colors, not the dark mode colors. If the background is black (which it
    // usually), then override it with a dark gray instead so that we can go up and down the lightness.
    if (QWindowsTheme::queryColorScheme() == Qt::ColorScheme::Dark) {
        // the system is actually running in dark mode, so UISettings will give us dark colors
        foreground = getSysColor(settings.GetColorValue(UIColorType::Foreground));
        background = [&settings]() -> QColor {
            auto systemBackground = getSysColor(settings.GetColorValue(UIColorType::Background));
            if (systemBackground == Qt::black)
                systemBackground = QColor(0x1E, 0x1E, 0x1E);
            return systemBackground;
        }();
        accent = qt_accentColor(AccentColorNormal);
        accentDark = qt_accentColor(AccentColorDark);
        accentDarker = qt_accentColor(AccentColorDarker);
        accentDarkest = qt_accentColor(AccentColorDarkest);
        accentLight = qt_accentColor(AccentColorLight);
        accentLighter = qt_accentColor(AccentColorLighter);
        accentLightest = qt_accentColor(AccentColorLightest);
    } else
#endif
    {
        // If the system is running in light mode, then we need to make up our own dark palette
        foreground = Qt::white;
        background = QColor(0x1E, 0x1E, 0x1E);
        accent = qt_accentColor(AccentColorNormal);
        accentDark = accent.darker(120);
        accentDarker = accentDark.darker(120);
        accentDarkest = accentDarker.darker(120);
        accentLight = accent.lighter(120);
        accentLighter = accentLight.lighter(120);
        accentLightest = accentLighter.lighter(120);
    }
    const QColor linkColor = accentLightest;
    const QColor buttonColor = background.lighter(200);

    result.setColor(QPalette::All, QPalette::WindowText, foreground);
    result.setColor(QPalette::All, QPalette::Text, foreground);
    result.setColor(QPalette::All, QPalette::BrightText, accentLightest);

    result.setColor(QPalette::All, QPalette::Button, buttonColor);
    result.setColor(QPalette::All, QPalette::ButtonText, foreground);
    result.setColor(QPalette::All, QPalette::Light, buttonColor.lighter(200));
    result.setColor(QPalette::All, QPalette::Midlight, buttonColor.lighter(150));
    result.setColor(QPalette::All, QPalette::Dark, buttonColor.darker(200));
    result.setColor(QPalette::All, QPalette::Mid, buttonColor.darker(150));
    result.setColor(QPalette::All, QPalette::Shadow, Qt::black);

    result.setColor(QPalette::All, QPalette::Base, background.lighter(150));
    result.setColor(QPalette::All, QPalette::Window, background);

    result.setColor(QPalette::All, QPalette::Highlight, accent);
    result.setColor(QPalette::All, QPalette::HighlightedText, accent.lightness() > 128 ? Qt::black : Qt::white);
    result.setColor(QPalette::All, QPalette::Link, linkColor);
    result.setColor(QPalette::All, QPalette::LinkVisited, accentLighter);
    result.setColor(QPalette::All, QPalette::AlternateBase, accentDarkest);
    result.setColor(QPalette::All, QPalette::ToolTipBase, buttonColor);
    result.setColor(QPalette::All, QPalette::ToolTipText, foreground.darker(120));
    result.setColor(QPalette::All, QPalette::PlaceholderText, placeHolderColor(foreground));
    result.setColor(QPalette::All, QPalette::Accent, accentLighter);
}

static inline QPalette toolTipPalette(const QPalette &systemPalette, bool light, bool highContrastEnabled)
{
    QPalette result(systemPalette);
    static const bool isWindows11 = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11;
    const QColor tipBgColor = highContrastEnabled ? (isWindows11 ? getSysColor(COLOR_WINDOW) : getSysColor(COLOR_BTNFACE)) :
                                    (light ? getSysColor(COLOR_INFOBK) : systemPalette.button().color());
    const QColor tipTextColor = highContrastEnabled ? (isWindows11 ? getSysColor(COLOR_WINDOWTEXT) : getSysColor(COLOR_BTNTEXT)) :
                                    (light ? getSysColor(COLOR_INFOTEXT) : systemPalette.buttonText().color().darker(120));

    result.setColor(QPalette::All, QPalette::Button, tipBgColor);
    result.setColor(QPalette::All, QPalette::Window, tipBgColor);
    result.setColor(QPalette::All, QPalette::Text, tipTextColor);
    result.setColor(QPalette::All, QPalette::WindowText, tipTextColor);
    result.setColor(QPalette::All, QPalette::ButtonText, tipTextColor);
    result.setColor(QPalette::All, QPalette::Button, tipBgColor);
    result.setColor(QPalette::All, QPalette::Window, tipBgColor);
    result.setColor(QPalette::All, QPalette::Text, tipTextColor);
    result.setColor(QPalette::All, QPalette::WindowText, tipTextColor);
    result.setColor(QPalette::All, QPalette::ButtonText, tipTextColor);
    result.setColor(QPalette::All, QPalette::ToolTipBase, tipBgColor);
    result.setColor(QPalette::All, QPalette::ToolTipText, tipTextColor);
    const QColor disabled = mixColors(result.windowText().color(), result.button().color());
    result.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Text, disabled);
    result.setColor(QPalette::Disabled, QPalette::ToolTipText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Base, Qt::white);
    result.setColor(QPalette::Disabled, QPalette::BrightText, Qt::white);
    result.setColor(QPalette::Disabled, QPalette::ToolTipBase, Qt::white);
    return result;
}

static inline QPalette menuPalette(const QPalette &systemPalette, bool light)
{
    if (!light)
        return systemPalette;

    QPalette result(systemPalette);
    const QColor menuColor = getSysColor(COLOR_MENU);
    const QColor menuTextColor = getSysColor(COLOR_MENUTEXT);
    const QColor disabled = getSysColor(COLOR_GRAYTEXT);
    // we might need a special color group for the result.
    result.setColor(QPalette::Active, QPalette::Button, menuColor);
    result.setColor(QPalette::Active, QPalette::Text, menuTextColor);
    result.setColor(QPalette::Active, QPalette::WindowText, menuTextColor);
    result.setColor(QPalette::Active, QPalette::ButtonText, menuTextColor);
    result.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Text, disabled);
    const bool isFlat = booleanSystemParametersInfo(SPI_GETFLATMENU, false);
    const QColor highlightColor = getSysColor(isFlat ? COLOR_MENUHILIGHT : COLOR_HIGHLIGHT);
    result.setColor(QPalette::Disabled, QPalette::Highlight, highlightColor);
    result.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Button,
                    result.color(QPalette::Active, QPalette::Button));
    result.setColor(QPalette::Inactive, QPalette::Button,
                    result.color(QPalette::Active, QPalette::Button));
    result.setColor(QPalette::Inactive, QPalette::Text,
                    result.color(QPalette::Active, QPalette::Text));
    result.setColor(QPalette::Inactive, QPalette::WindowText,
                    result.color(QPalette::Active, QPalette::WindowText));
    result.setColor(QPalette::Inactive, QPalette::ButtonText,
                    result.color(QPalette::Active, QPalette::ButtonText));
    result.setColor(QPalette::Inactive, QPalette::Highlight,
                    result.color(QPalette::Active, QPalette::Highlight));
    result.setColor(QPalette::Inactive, QPalette::HighlightedText,
                    result.color(QPalette::Active, QPalette::HighlightedText));
    result.setColor(QPalette::Inactive, QPalette::ButtonText,
                    systemPalette.color(QPalette::Inactive, QPalette::Dark));
    return result;
}

static inline QPalette *menuBarPalette(const QPalette &menuPalette, bool light)
{
    QPalette *result = nullptr;
    if (!light || !booleanSystemParametersInfo(SPI_GETFLATMENU, false))
        return result;

    result = new QPalette(menuPalette);
    const QColor menubar(getSysColor(COLOR_MENUBAR));
    result->setColor(QPalette::Active, QPalette::Button, menubar);
    result->setColor(QPalette::Disabled, QPalette::Button, menubar);
    result->setColor(QPalette::Inactive, QPalette::Button, menubar);
    return result;
}

const char *QWindowsTheme::name = "windows";
QWindowsTheme *QWindowsTheme::m_instance = nullptr;

LRESULT QT_WIN_CALLBACK qThemeChangeObserverWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_SETTINGCHANGE:
        // Only refresh the theme if the user changes the personalize settings
        if (!((wParam == 0) && (lParam != 0) // lParam sometimes may be NULL.
            && (wcscmp(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0)))
            break;
        Q_FALLTHROUGH();
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
    case WM_DWMCOLORIZATIONCOLORCHANGED:
        qCDebug(lcQpaTheme) << "Handling theme change due to"
            << qUtf8Printable(decodeMSG(MSG{hwnd, message, wParam, lParam, 0, {0, 0}}).trimmed());
        QWindowsTheme::handleThemeChange();

        MSG msg; // Clear the message queue, we've already reacted to the change
        while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE));

        // FIXME: Despite clearing the message queue above, Windows will send
        // us redundant theme change events for our single window. We want the
        // theme change delivery to be synchronous, so we can't easily debounce
        // them by peeking into the QWSI event queue, but perhaps there are other
        // ways.

        break;
    default:
        break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

QWindowsTheme::QWindowsTheme()
{
    m_instance = this;
    s_colorScheme = Qt::ColorScheme::Unknown; // Used inside QWindowsTheme::effectiveColorScheme();
    s_colorScheme = QWindowsTheme::effectiveColorScheme();
    std::fill(m_fonts, m_fonts + NFonts, nullptr);
    std::fill(m_palettes, m_palettes + NPalettes, nullptr);
    refresh();
    refreshIconPixmapSizes();

    auto className = QWindowsContext::instance()->registerWindowClass(
        QWindowsContext::classNamePrefix() + QLatin1String("ThemeChangeObserverWindow"),
        qThemeChangeObserverWndProc);
    // HWND_MESSAGE windows do not get the required theme events,
    // so we use a real top-level window that we never show.
    m_themeChangeObserver = CreateWindowEx(0, reinterpret_cast<LPCWSTR>(className.utf16()),
        nullptr, WS_TILED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    Q_ASSERT(m_themeChangeObserver);
}

QWindowsTheme::~QWindowsTheme()
{
    clearPalettes();
    clearFonts();
    m_instance = nullptr;
}

void QWindowsTheme::destroyThemeChangeWindow()
{
    qCDebug(lcQpaTheme) << "Destroying theme change window";
    DestroyWindow(m_themeChangeObserver);
    m_themeChangeObserver = nullptr;
}

static inline QStringList iconThemeSearchPaths()
{
    const QFileInfo appDir(QCoreApplication::applicationDirPath() + "/icons"_L1);
    return appDir.isDir() ? QStringList(appDir.absoluteFilePath()) : QStringList();
}

static inline QStringList styleNames()
{
    QStringList styles = { QStringLiteral("WindowsVista"), QStringLiteral("Windows") };
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11)
        styles.prepend(QStringLiteral("Windows11"));
    return styles;
}

static inline int uiEffects()
{
    int result = QPlatformTheme::HoverEffect;
    if (booleanSystemParametersInfo(SPI_GETUIEFFECTS, false))
        result |= QPlatformTheme::GeneralUiEffect;
    if (booleanSystemParametersInfo(SPI_GETMENUANIMATION, false))
        result |= QPlatformTheme::AnimateMenuUiEffect;
    if (booleanSystemParametersInfo(SPI_GETMENUFADE, false))
        result |= QPlatformTheme::FadeMenuUiEffect;
    if (booleanSystemParametersInfo(SPI_GETCOMBOBOXANIMATION, false))
        result |= QPlatformTheme::AnimateComboUiEffect;
    if (booleanSystemParametersInfo(SPI_GETTOOLTIPANIMATION, false))
        result |= QPlatformTheme::AnimateTooltipUiEffect;
    return result;
}

QVariant QWindowsTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case UseFullScreenForPopupMenu:
        return QVariant(true);
    case DialogButtonBoxLayout:
        return QVariant(QPlatformDialogHelper::WinLayout);
    case IconThemeSearchPaths:
        return QVariant(iconThemeSearchPaths());
    case StyleNames:
        return QVariant(styleNames());
    case TextCursorWidth:
        return QVariant(int(dWordSystemParametersInfo(SPI_GETCARETWIDTH, 1u)));
    case DropShadow:
        return QVariant(booleanSystemParametersInfo(SPI_GETDROPSHADOW, false));
    case MaximumScrollBarDragDistance:
        return QVariant(qRound(qreal(QWindowsContext::instance()->defaultDPI()) * 1.375));
    case KeyboardScheme:
        return QVariant(int(WindowsKeyboardScheme));
    case UiEffects:
        return QVariant(uiEffects());
    case IconPixmapSizes:
        return QVariant::fromValue(m_fileIconSizes);
    case DialogSnapToDefaultButton:
        return QVariant(booleanSystemParametersInfo(SPI_GETSNAPTODEFBUTTON, false));
    case ContextMenuOnMouseRelease:
        return QVariant(true);
    case WheelScrollLines: {
        int result = 3;
        const DWORD scrollLines = dWordSystemParametersInfo(SPI_GETWHEELSCROLLLINES, DWORD(result));
        if (scrollLines != DWORD(-1)) // Special value meaning "scroll one screen", unimplemented in Qt.
            result = int(scrollLines);
        return QVariant(result);
    }
    case MouseDoubleClickDistance:
        return GetSystemMetrics(SM_CXDOUBLECLK);
    case MenuBarFocusOnAltPressRelease:
        return true;
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

Qt::ColorScheme QWindowsTheme::colorScheme() const
{
    return QWindowsTheme::effectiveColorScheme();
}

Qt::ColorScheme QWindowsTheme::effectiveColorScheme()
{
    auto integration = QWindowsIntegration::instance();
    if (queryHighContrast())
        return Qt::ColorScheme::Unknown;
    if (s_colorSchemeOverride != Qt::ColorScheme::Unknown)
        return s_colorSchemeOverride;
    if (s_colorScheme != Qt::ColorScheme::Unknown)
        return s_colorScheme;
    if (!integration->darkModeHandling().testFlag(QWindowsApplication::DarkModeStyle))
        return Qt::ColorScheme::Light;
    return queryColorScheme();
}

void QWindowsTheme::requestColorScheme(Qt::ColorScheme scheme)
{
    s_colorSchemeOverride = scheme;
    handleThemeChange();
}

Qt::ContrastPreference QWindowsTheme::contrastPreference() const
{
    return queryHighContrast() ? Qt::ContrastPreference::HighContrast
                               : Qt::ContrastPreference::NoPreference;
}

void QWindowsTheme::handleThemeChange()
{
    QWindowsThemeCache::clearAllThemeCaches();

    const auto oldColorScheme = s_colorScheme;
    s_colorScheme = Qt::ColorScheme::Unknown; // make effectiveColorScheme() query registry
    s_colorScheme = effectiveColorScheme();
    if (s_colorScheme != oldColorScheme) {
        // Only propagate color scheme changes if the scheme actually changed
        auto integration = QWindowsIntegration::instance();
        integration->updateApplicationBadge();

        for (QWindowsWindow *w : std::as_const(QWindowsContext::instance()->windows()))
            w->setDarkBorder(s_colorScheme == Qt::ColorScheme::Dark);
    }

    // But always reset palette and fonts, and signal the theme
    // change, as other parts of the theme could have changed,
    // such as the accent color.
    QWindowsTheme::instance()->refresh();
    QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
}

void QWindowsTheme::clearPalettes()
{
    qDeleteAll(m_palettes, m_palettes + NPalettes);
    std::fill(m_palettes, m_palettes + NPalettes, nullptr);
}

void QWindowsTheme::refreshPalettes()
{
    if (!QGuiApplication::desktopSettingsAware())
        return;
    const bool light =
        effectiveColorScheme() != Qt::ColorScheme::Dark
        || !QWindowsIntegration::instance()->darkModeHandling().testFlag(QWindowsApplication::DarkModeStyle);
    clearPalettes();
    m_palettes[SystemPalette] = new QPalette(QWindowsTheme::systemPalette(s_colorScheme));
    m_palettes[ToolTipPalette] = new QPalette(toolTipPalette(*m_palettes[SystemPalette], light, queryHighContrast()));
    m_palettes[MenuPalette] = new QPalette(menuPalette(*m_palettes[SystemPalette], light));
    m_palettes[MenuBarPalette] = menuBarPalette(*m_palettes[MenuPalette], light);
    if (!light) {
        m_palettes[CheckBoxPalette] = new QPalette(*m_palettes[SystemPalette]);
        m_palettes[CheckBoxPalette]->setColor(QPalette::Active, QPalette::Base, qt_accentColor(AccentColorNormal));
        m_palettes[CheckBoxPalette]->setColor(QPalette::Active, QPalette::Button, qt_accentColor(AccentColorLighter));
        m_palettes[CheckBoxPalette]->setColor(QPalette::Inactive, QPalette::Base, qt_accentColor(AccentColorDarkest));
        m_palettes[RadioButtonPalette] = new QPalette(*m_palettes[CheckBoxPalette]);
    }
}

QPalette QWindowsTheme::systemPalette(Qt::ColorScheme colorScheme)
{
    QPalette result = standardPalette();

    switch (colorScheme) {
    case Qt::ColorScheme::Unknown:
        // when a high-contrast theme is active or when we fail to read, assume light
        Q_FALLTHROUGH();
    case Qt::ColorScheme::Light:
        populateLightSystemBasePalette(result);
        break;
    case Qt::ColorScheme::Dark:
        populateDarkSystemBasePalette(result);
        break;
    }

    if (result.window() != result.base()) {
        result.setColor(QPalette::Inactive, QPalette::Highlight,
                        result.color(QPalette::Inactive, QPalette::Window));
        result.setColor(QPalette::Inactive, QPalette::HighlightedText,
                        result.color(QPalette::Inactive, QPalette::Text));
        result.setColor(QPalette::Inactive, QPalette::Accent,
                        result.color(QPalette::Inactive, QPalette::Window));
    }

    const QColor disabled = mixColors(result.windowText().color(), result.button().color());

    result.setColorGroup(QPalette::Disabled, result.windowText(), result.button(),
                         result.light(), result.dark(), result.mid(),
                         result.text(), result.brightText(), result.base(),
                         result.window());

    const bool highContrastEnabled = queryHighContrast();
    const QColor disabledTextColor = highContrastEnabled ? getSysColor(COLOR_GRAYTEXT) : disabled;
    result.setColor(QPalette::Disabled, QPalette::WindowText, disabledTextColor);
    result.setColor(QPalette::Disabled, QPalette::Text, disabledTextColor);
    result.setColor(QPalette::Disabled, QPalette::ButtonText, disabledTextColor);
    if (highContrastEnabled)
        result.setColor(QPalette::Disabled, QPalette::Button, result.button().color().darker(150));

    result.setColor(QPalette::Disabled, QPalette::Highlight, result.color(QPalette::Highlight));
    result.setColor(QPalette::Disabled, QPalette::HighlightedText, result.color(QPalette::HighlightedText));
    result.setColor(QPalette::Disabled, QPalette::Accent, disabled);
    result.setColor(QPalette::Disabled, QPalette::Base, result.window().color());
    return result;
}

void QWindowsTheme::clearFonts()
{
    qDeleteAll(m_fonts, m_fonts + NFonts);
    std::fill(m_fonts, m_fonts + NFonts, nullptr);
}

void QWindowsTheme::refresh()
{
    refreshPalettes();
    refreshFonts();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const NONCLIENTMETRICS &m)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "NONCLIENTMETRICS(iMenu=" << m.iMenuWidth  << 'x' << m.iMenuHeight
      << ", lfCaptionFont=";
    QWindowsFontDatabase::debugFormat(d, m.lfCaptionFont);
    d << ", lfSmCaptionFont=";
    QWindowsFontDatabase::debugFormat(d, m.lfSmCaptionFont);
    d << ", lfMenuFont=";
    QWindowsFontDatabase::debugFormat(d, m.lfMenuFont);
    d <<  ", lfMessageFont=";
    QWindowsFontDatabase::debugFormat(d, m.lfMessageFont);
    d <<", lfStatusFont=";
    QWindowsFontDatabase::debugFormat(d, m.lfStatusFont);
    d << ')';
    return d;
}
#endif // QT_NO_DEBUG_STREAM

void QWindowsTheme::refreshFonts()
{
    clearFonts();
    if (!QGuiApplication::desktopSettingsAware())
        return;

    const int dpi = 96;
    NONCLIENTMETRICS ncm;
    QWindowsContext::nonClientMetrics(&ncm, dpi);
    qCDebug(lcQpaWindow) << __FUNCTION__ << ncm;

    const QFont menuFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfMenuFont, dpi);
    const QFont messageBoxFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfMessageFont, dpi);
    const QFont statusFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfStatusFont, dpi);
    const QFont titleFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfCaptionFont, dpi);
    QFont fixedFont(QStringLiteral("Courier New"), messageBoxFont.pointSize());
    fixedFont.setStyleHint(QFont::TypeWriter);

    LOGFONT lfIconTitleFont;
    SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(lfIconTitleFont), &lfIconTitleFont, 0, dpi);
    const QFont iconTitleFont = QWindowsFontDatabase::LOGFONT_to_QFont(lfIconTitleFont, dpi);

    m_fonts[SystemFont] = new QFont(QWindowsFontDatabase::systemDefaultFont());
    m_fonts[MenuFont] = new QFont(menuFont);
    m_fonts[MenuBarFont] = new QFont(menuFont);
    m_fonts[MessageBoxFont] = new QFont(messageBoxFont);
    m_fonts[TipLabelFont] = new QFont(statusFont);
    m_fonts[StatusBarFont] = new QFont(statusFont);
    m_fonts[MdiSubWindowTitleFont] = new QFont(titleFont);
    m_fonts[DockWidgetTitleFont] = new QFont(titleFont);
    m_fonts[ItemViewFont] = new QFont(iconTitleFont);
    m_fonts[FixedFont] = new QFont(fixedFont);
}

enum FileIconSize {
    // Standard icons obtainable via shGetFileInfo(), SHGFI_SMALLICON, SHGFI_LARGEICON
    SmallFileIcon, LargeFileIcon,
    // Larger icons obtainable via SHGetImageList()
    ExtraLargeFileIcon,
    JumboFileIcon, // Vista onwards
    FileIconSizeCount
};

bool QWindowsTheme::usePlatformNativeDialog(DialogType type) const
{
    return QWindowsDialogs::useHelper(type);
}

QPlatformDialogHelper *QWindowsTheme::createPlatformDialogHelper(DialogType type) const
{
    return QWindowsDialogs::createHelper(type);
}

#if QT_CONFIG(systemtrayicon)
QPlatformSystemTrayIcon *QWindowsTheme::createPlatformSystemTrayIcon() const
{
    return new QWindowsSystemTrayIcon;
}
#endif

static int fileIconSizes[FileIconSizeCount];

void QWindowsTheme::refreshIconPixmapSizes()
{
    // Standard sizes: 16, 32, 48, 256
    fileIconSizes[SmallFileIcon] = GetSystemMetrics(SM_CXSMICON); // corresponds to SHGFI_SMALLICON);
    fileIconSizes[LargeFileIcon] = GetSystemMetrics(SM_CXICON); // corresponds to SHGFI_LARGEICON
    fileIconSizes[ExtraLargeFileIcon] =
        fileIconSizes[LargeFileIcon] + fileIconSizes[LargeFileIcon] / 2;
    fileIconSizes[JumboFileIcon] = 8 * fileIconSizes[LargeFileIcon]; // empirical, has not been observed to work

    int *availEnd = fileIconSizes + JumboFileIcon + 1;
    m_fileIconSizes = QAbstractFileIconEngine::toSizeList(fileIconSizes, availEnd);
    qCDebug(lcQpaWindow) << __FUNCTION__ << m_fileIconSizes;
}

// Defined in qpixmap_win.cpp
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);

static QPixmap loadIconFromShell32(int resourceId, QSizeF size)
{
    HMODULE shell32 = ::GetModuleHandleW(L"shell32.dll");
    Q_ASSERT(shell32);
    auto iconHandle =
        static_cast<HICON>(LoadImage(shell32, MAKEINTRESOURCE(resourceId),
                                     IMAGE_ICON, int(size.width()), int(size.height()), 0));
    if (iconHandle) {
        QPixmap iconpixmap = qt_pixmapFromWinHICON(iconHandle);
        DestroyIcon(iconHandle);
        return iconpixmap;
    }
    return QPixmap();
}

QPixmap QWindowsTheme::standardPixmap(StandardPixmap sp, const QSizeF &pixmapSize) const
{
    int resourceId = -1;
    SHSTOCKICONID stockId = SIID_INVALID;
    LPCTSTR iconName = nullptr;
    switch (sp) {
    case DriveCDIcon:
        stockId = SIID_DRIVECD;
        resourceId = 12;
        break;
    case DriveDVDIcon:
        stockId = SIID_DRIVEDVD;
        resourceId = 12;
        break;
    case DriveNetIcon:
        stockId = SIID_DRIVENET;
        resourceId = 10;
        break;
    case DriveHDIcon:
        stockId = SIID_DRIVEFIXED;
        resourceId = 9;
        break;
    case DriveFDIcon:
        stockId = SIID_DRIVE35;
        resourceId = 7;
        break;
    case FileLinkIcon:
        Q_FALLTHROUGH();
    case FileIcon:
        stockId = SIID_DOCNOASSOC;
        resourceId = 1;
        break;
    case DirLinkIcon:
        Q_FALLTHROUGH();
    case DirClosedIcon:
    case DirIcon:
        stockId = SIID_FOLDER;
        resourceId = 4;
        break;
    case DesktopIcon:
        resourceId = 35;
        break;
    case ComputerIcon:
        resourceId = 16;
        break;
    case DirLinkOpenIcon:
        Q_FALLTHROUGH();
    case DirOpenIcon:
        stockId = SIID_FOLDEROPEN;
        resourceId = 5;
        break;
    case FileDialogNewFolder:
        stockId = SIID_FOLDER;
        resourceId = 319;
        break;
    case DirHomeIcon:
        resourceId = 235;
        break;
    case TrashIcon:
        stockId = SIID_RECYCLER;
        resourceId = 191;
        break;
    case MessageBoxInformation:
        stockId = SIID_INFO;
        iconName = IDI_INFORMATION;
        break;
    case MessageBoxWarning:
        stockId = SIID_WARNING;
        iconName = IDI_WARNING;
        break;
    case MessageBoxCritical:
        stockId = SIID_ERROR;
        iconName = IDI_ERROR;
        break;
    case MessageBoxQuestion:
        stockId = SIID_HELP;
        iconName = IDI_QUESTION;
        break;
    case VistaShield:
        stockId = SIID_SHIELD;
        break;
    default:
        break;
    }

    // Even with SHGSI_LINKOVERLAY flag set, loaded Icon with SHDefExtractIcon doesn't have
    // any overlay, so we avoid SHGSI_LINKOVERLAY flag and draw it manually (QTBUG-131843)
    const auto drawLinkOverlayIconIfNeeded = [](StandardPixmap sp, QPixmap &pixmap, QSizeF pixmapSize) {
        if (sp == FileLinkIcon || sp == DirLinkIcon || sp == DirLinkOpenIcon) {
            QPainter painter(&pixmap);
            const QSizeF linkSize = pixmapSize / (pixmapSize.height() >= 48 ? 3 : 2);
            static constexpr auto LinkOverlayIconId = 16769;
            const QPixmap link = loadIconFromShell32(LinkOverlayIconId, linkSize.toSize());
            const int yPos = pixmap.height() - link.size().height();
            painter.drawPixmap(0, yPos, int(linkSize.width()), int(linkSize.height()), link);
        }
    };

    if (stockId != SIID_INVALID) {
        SHSTOCKICONINFO iconInfo;
        memset(&iconInfo, 0, sizeof(iconInfo));
        iconInfo.cbSize = sizeof(iconInfo);
        constexpr UINT stockFlags = SHGSI_ICONLOCATION;
        if (SHGetStockIconInfo(stockId, stockFlags, &iconInfo) == S_OK) {
            const auto iconSize = pixmapSize.width();
            HICON icon;
            if (SHDefExtractIcon(iconInfo.szPath, iconInfo.iIcon, 0, &icon, nullptr, iconSize) == S_OK) {
                QPixmap pixmap = qt_pixmapFromWinHICON(icon);
                DestroyIcon(icon);
                if (!pixmap.isNull()) {
                    drawLinkOverlayIconIfNeeded(sp, pixmap, pixmap.size());
                    return pixmap;
                }
            }
        }
    }

    if (resourceId != -1) {
        QPixmap pixmap = loadIconFromShell32(resourceId, pixmapSize);
        if (!pixmap.isNull()) {
            drawLinkOverlayIconIfNeeded(sp, pixmap, pixmapSize);
            return pixmap;
        }
    }

    if (iconName) {
        HICON iconHandle = LoadIcon(nullptr, iconName);
        QPixmap pixmap = qt_pixmapFromWinHICON(iconHandle);
        DestroyIcon(iconHandle);
        if (!pixmap.isNull())
            return pixmap;
    }

    return QPlatformTheme::standardPixmap(sp, pixmapSize);
}

enum { // Shell image list ids
    sHIL_EXTRALARGE = 0x2, // 48x48 or user-defined
    sHIL_JUMBO = 0x4 // 256x256 (Vista or later)
};

static QString dirIconPixmapCacheKey(int iIcon, int iconSize, int imageListSize)
{
    QString key = "qt_dir_"_L1 + QString::number(iIcon);
    if (iconSize == SHGFI_LARGEICON)
        key += u'l';
    switch (imageListSize) {
    case sHIL_EXTRALARGE:
        key += u'e';
        break;
    case sHIL_JUMBO:
        key += u'j';
        break;
    }
    return key;
}

template <typename T>
class FakePointer
{
public:

    static_assert(sizeof(T) <= sizeof(void *), "FakePointers can only go that far.");

    static FakePointer *create(T thing)
    {
        return reinterpret_cast<FakePointer *>(qintptr(thing));
    }

    T operator * () const
    {
        return T(qintptr(this));
    }

    void operator delete (void *) {}
};

// Shell image list helper functions.

static QPixmap pixmapFromShellImageList(int iImageList, int iIcon)
{
    QPixmap result;
    // For MinGW:
    static const IID iID_IImageList = {0x46eb5926, 0x582e, 0x4017, {0x9f, 0xdf, 0xe8, 0x99, 0x8d, 0xaa, 0x9, 0x50}};

    IImageList *imageList = nullptr;
    HRESULT hr = SHGetImageList(iImageList, iID_IImageList, reinterpret_cast<void **>(&imageList));
    if (hr != S_OK)
        return result;
    HICON hIcon;
    hr = imageList->GetIcon(iIcon, ILD_TRANSPARENT, &hIcon);
    if (hr == S_OK) {
        result = qt_pixmapFromWinHICON(hIcon);
        DestroyIcon(hIcon);
    }
    imageList->Release();
    return result;
}

class QWindowsFileIconEngine : public QAbstractFileIconEngine
{
public:
    explicit QWindowsFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts) :
        QAbstractFileIconEngine(info, opts) {}

    QList<QSize> availableSizes(QIcon::Mode = QIcon::Normal, QIcon::State = QIcon::Off) override
    { return QWindowsTheme::instance()->availableFileIconSizes(); }

protected:
    QString cacheKey() const override;
    QPixmap filePixmap(const QSize &size, QIcon::Mode mode, QIcon::State) override;
};

QString QWindowsFileIconEngine::cacheKey() const
{
    // Cache directories unless custom or drives, which have custom icons depending on type
    if ((options() & QPlatformTheme::DontUseCustomDirectoryIcons) && fileInfo().isDir() && !fileInfo().isRoot())
        return QStringLiteral("qt_/directory/");
    if (!fileInfo().isFile())
        return QString();
    // Return "" for .exe, .lnk and .ico extensions.
    // It is faster to just look at the file extensions;
    // avoiding slow QFileInfo::isExecutable() (QTBUG-13182)
    QString suffix = fileInfo().suffix();
    if (!suffix.compare(u"exe", Qt::CaseInsensitive)
        || !suffix.compare(u"lnk", Qt::CaseInsensitive)
        || !suffix.compare(u"ico", Qt::CaseInsensitive)) {
        return QString();
    }
    return "qt_."_L1
        + (suffix.isEmpty() ? fileInfo().fileName() : std::move(suffix).toUpper()); // handle "Makefile"                                    ;)
}

QPixmap QWindowsFileIconEngine::filePixmap(const QSize &size, QIcon::Mode, QIcon::State)
{
    QComHelper comHelper;

    static QCache<QString, FakePointer<int> > dirIconEntryCache(1000);
    Q_CONSTINIT static QMutex mx;
    static int defaultFolderIIcon = -1;
    const bool useDefaultFolderIcon = options() & QPlatformTheme::DontUseCustomDirectoryIcons;

    QPixmap pixmap;
    const QString filePath = QDir::toNativeSeparators(fileInfo().filePath());
    const int width = int(size.width());
    const int iconSize = width > fileIconSizes[SmallFileIcon] ? SHGFI_LARGEICON : SHGFI_SMALLICON;
    const int requestedImageListSize =
        width > fileIconSizes[ExtraLargeFileIcon]
            ? sHIL_JUMBO
            : (width > fileIconSizes[LargeFileIcon] ? sHIL_EXTRALARGE : 0);
    bool cacheableDirIcon = fileInfo().isDir() && !fileInfo().isRoot();
    if (cacheableDirIcon) {
        QMutexLocker locker(&mx);
        int iIcon = (useDefaultFolderIcon && defaultFolderIIcon >= 0) ? defaultFolderIIcon
                                                                      : **dirIconEntryCache.object(filePath);
        if (iIcon) {
            QPixmapCache::find(dirIconPixmapCacheKey(iIcon, iconSize, requestedImageListSize),
                               &pixmap);
            if (pixmap.isNull()) // Let's keep both caches in sync
                dirIconEntryCache.remove(filePath);
            else
                return pixmap;
        }
    }

    unsigned int flags = SHGFI_ICON | iconSize | SHGFI_SYSICONINDEX | SHGFI_ADDOVERLAYS | SHGFI_OVERLAYINDEX;
    DWORD attributes = 0;
    QString path = filePath;
    if (cacheableDirIcon && useDefaultFolderIcon) {
        flags |= SHGFI_USEFILEATTRIBUTES;
        attributes |= FILE_ATTRIBUTE_DIRECTORY;
        path = QStringLiteral("dummy");
    } else if (!fileInfo().exists()) {
        flags |= SHGFI_USEFILEATTRIBUTES;
        attributes |= FILE_ATTRIBUTE_NORMAL;
    }
    auto task = QSharedPointer<QShGetFileInfoThread::Task>(
            new QShGetFileInfoThread::Task(path, attributes, flags));
    s_shGetFileInfoThread()->runWithParams(task);
    // Even if GetFileInfo returns a valid result, hIcon can be empty in some cases
    if (task->resultValid()) {
        QString key;
        if (cacheableDirIcon) {
            if (useDefaultFolderIcon && defaultFolderIIcon < 0)
                defaultFolderIIcon = task->iIcon;

            //using the unique icon index provided by windows save us from duplicate keys
            key = dirIconPixmapCacheKey(task->iIcon, iconSize, requestedImageListSize);
            QPixmapCache::find(key, &pixmap);
            if (!pixmap.isNull()) {
                QMutexLocker locker(&mx);
                dirIconEntryCache.insert(filePath, FakePointer<int>::create(task->iIcon));
            }
        }

        if (pixmap.isNull()) {
            if (requestedImageListSize) {
                pixmap = pixmapFromShellImageList(requestedImageListSize, task->iIcon);
                if (pixmap.isNull() && requestedImageListSize == sHIL_JUMBO)
                    pixmap = pixmapFromShellImageList(sHIL_EXTRALARGE, task->iIcon);
            }
            if (pixmap.isNull())
                pixmap = qt_pixmapFromWinHICON(task->hIcon);
            if (!pixmap.isNull()) {
                if (cacheableDirIcon) {
                    QMutexLocker locker(&mx);
                    QPixmapCache::insert(key, pixmap);
                    dirIconEntryCache.insert(filePath, FakePointer<int>::create(task->iIcon));
                }
            } else {
                qWarning("QWindowsTheme::fileIconPixmap() no icon found");
            }
        }
    }

    return pixmap;
}

QIcon QWindowsTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions) const
{
    return QIcon(new QWindowsFileIconEngine(fileInfo, iconOptions));
}

QIconEngine *QWindowsTheme::createIconEngine(const QString &iconName) const
{
    return new QWindowsIconEngine(iconName);
}

static inline bool doUseNativeMenus()
{
    const unsigned options = QWindowsIntegration::instance()->options();
    if ((options & QWindowsIntegration::NoNativeMenus) != 0)
        return false;
    if ((options & QWindowsIntegration::AlwaysUseNativeMenus) != 0)
        return true;
    // "Auto" mode: For non-widget or Quick Controls 2 applications
    if (!QCoreApplication::instance()->inherits("QApplication"))
        return true;
    const QWindowList &topLevels = QGuiApplication::topLevelWindows();
    for (const QWindow *t : topLevels) {
        if (t->inherits("QQuickApplicationWindow"))
            return true;
    }
    return false;
}

bool QWindowsTheme::useNativeMenus()
{
    static const bool result = doUseNativeMenus();
    return result;
}

Qt::ColorScheme QWindowsTheme::queryColorScheme()
{
    if (queryHighContrast())
        return Qt::ColorScheme::Unknown;

    QWinRegistryKey personalizeKey{
        HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)"
    };
    const bool useDarkTheme = personalizeKey.value<DWORD>(L"AppsUseLightTheme") == 0;
    return useDarkTheme ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

bool QWindowsTheme::queryHighContrast()
{
    HIGHCONTRAST hcf = {};
    hcf.cbSize = static_cast<UINT>(sizeof(HIGHCONTRAST));
    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, hcf.cbSize, &hcf, FALSE))
        return hcf.dwFlags & HCF_HIGHCONTRASTON;
    return false;
}

QPlatformMenuItem *QWindowsTheme::createPlatformMenuItem() const
{
    qCDebug(lcQpaMenus) << __FUNCTION__;
    return QWindowsTheme::useNativeMenus() ? new QWindowsMenuItem : nullptr;
}

QPlatformMenu *QWindowsTheme::createPlatformMenu() const
{
    qCDebug(lcQpaMenus) << __FUNCTION__;
    // We create a popup menu here, since it will likely be used as context
    // menu. Submenus should be created the factory functions of
    // QPlatformMenu/Bar. Note though that Quick Controls 1 will use this
    // function for submenus as well, but this has been found to work.
    return QWindowsTheme::useNativeMenus() ? new QWindowsPopupMenu : nullptr;
}

QPlatformMenuBar *QWindowsTheme::createPlatformMenuBar() const
{
    qCDebug(lcQpaMenus) << __FUNCTION__;
    return QWindowsTheme::useNativeMenus() ? new QWindowsMenuBar : nullptr;
}

void QWindowsTheme::showPlatformMenuBar()
{
    qCDebug(lcQpaMenus) << __FUNCTION__;
}

QT_END_NAMESPACE
