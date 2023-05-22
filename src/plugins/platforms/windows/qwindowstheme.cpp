// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qt_windows.h>

#include "qwindowstheme.h"
#include "qwindowsmenu.h"
#include "qwindowsdialoghelpers.h"
#include "qwindowscontext.h"
#include "qwindowsintegration.h"
#if QT_CONFIG(systemtrayicon)
#  include "qwindowssystemtrayicon.h"
#endif
#include "qwindowsscreen.h"
#include <commctrl.h>
#include <objbase.h>
#ifndef Q_CC_MINGW
#  include <commoncontrols.h>
#endif
#include <shellapi.h>

#include <QtCore/qvariant.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qcache.h>
#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtGui/qcolor.h>
#include <QtGui/qpalette.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpixmapcache.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/private/qabstractfileiconengine_p.h>
#include <QtGui/private/qwindowsfontdatabase_p.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qsystemlibrary_p.h>
#include <private/qwinregistry_p.h>
#include <QtCore/private/qfunctions_win_p.h>

#include <algorithm>

#if QT_CONFIG(cpp_winrt)
#   include <QtCore/private/qt_winrtbase_p.h>

#   include <winrt/Windows.UI.ViewManagement.h>
#endif // QT_CONFIG(cpp_winrt)

#if defined(__IImageList_INTERFACE_DEFINED__) && defined(__IID_DEFINED__)
#  define USE_IIMAGELIST
#endif

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

static inline QColor getSysColor(int index)
{
    COLORREF cr = GetSysColor(index);
    return QColor(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

#if QT_CONFIG(cpp_winrt)
static constexpr QColor getSysColor(winrt::Windows::UI::Color &&color)
{
    return QColor(color.R, color.G, color.B, color.A);
}
#endif

// QTBUG-48823/Windows 10: SHGetFileInfo() (as called by item views on file system
// models has been observed to trigger a WM_PAINT on the mainwindow. Suppress the
// behavior by running it in a thread.

struct QShGetFileInfoParams
{
    QShGetFileInfoParams(const QString &fn, DWORD a, SHFILEINFO *i, UINT f, bool *r)
        : fileName(fn), attributes(a), flags(f), info(i), result(r)
    { }

    const QString &fileName;
    const DWORD attributes;
    const UINT flags;
    SHFILEINFO *const info;
    bool *const result;
};

class QShGetFileInfoThread : public QThread
{
public:
    explicit QShGetFileInfoThread()
        : QThread(), m_params(nullptr)
    {
        connect(this, &QThread::finished, this, &QObject::deleteLater);
    }

    void run() override
    {
        QComHelper comHelper(COINIT_MULTITHREADED);

        QMutexLocker readyLocker(&m_readyMutex);
        while (!m_cancelled.loadRelaxed()) {
            if (!m_params && !m_cancelled.loadRelaxed()
                && !m_readyCondition.wait(&m_readyMutex, QDeadlineTimer(1000ll)))
                continue;

            if (m_params) {
                const QString fileName = m_params->fileName;
                SHFILEINFO info;
                const bool result = SHGetFileInfo(reinterpret_cast<const wchar_t *>(fileName.utf16()),
                                                  m_params->attributes, &info, sizeof(SHFILEINFO),
                                                  m_params->flags);
                m_doneMutex.lock();
                if (!m_cancelled.loadRelaxed()) {
                    *m_params->result = result;
                    memcpy(m_params->info, &info, sizeof(SHFILEINFO));
                }
                m_params = nullptr;

                m_doneCondition.wakeAll();
                m_doneMutex.unlock();
            }
        }
    }

    bool runWithParams(QShGetFileInfoParams *params, qint64 timeOutMSecs)
    {
        QMutexLocker doneLocker(&m_doneMutex);

        m_readyMutex.lock();
        m_params = params;
        m_readyCondition.wakeAll();
        m_readyMutex.unlock();

        return m_doneCondition.wait(&m_doneMutex, QDeadlineTimer(timeOutMSecs));
    }

    void cancel()
    {
        QMutexLocker doneLocker(&m_doneMutex);
        m_cancelled.storeRelaxed(1);
        m_readyCondition.wakeAll();
    }

private:
    QShGetFileInfoParams *m_params;
    QAtomicInt m_cancelled;
    QWaitCondition m_readyCondition;
    QWaitCondition m_doneCondition;
    QMutex m_readyMutex;
    QMutex m_doneMutex;
};

static bool shGetFileInfoBackground(const QString &fileName, DWORD attributes,
                                    SHFILEINFO *info, UINT flags,
                                    qint64 timeOutMSecs = 5000)
{
    static QShGetFileInfoThread *getFileInfoThread = nullptr;
    if (!getFileInfoThread) {
        getFileInfoThread = new QShGetFileInfoThread;
        getFileInfoThread->start();
    }

    bool result = false;
    QShGetFileInfoParams params(fileName, attributes, info, flags, &result);
    if (!getFileInfoThread->runWithParams(&params, timeOutMSecs)) {
        // Cancel and reset getFileInfoThread. It'll
        // be reinitialized the next time we get called.
        getFileInfoThread->cancel();
        getFileInfoThread = nullptr;
        qWarning().noquote() << "SHGetFileInfo() timed out for " << fileName;
        return false;
    }
    return result;
}

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

[[maybe_unused]] [[nodiscard]] static inline QColor qt_accentColor()
{
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
}

/*
    This is used when the theme is light mode, and when the theme is dark but the
    application doesn't support dark mode. In the latter case, we need to check.
*/
static void populateLightSystemBasePalette(QPalette &result)
{
    const QColor background = getSysColor(COLOR_BTNFACE);
    const QColor textColor = getSysColor(COLOR_WINDOWTEXT);

#if QT_CONFIG(cpp_winrt)
    // respect the Windows 11 accent color
    using namespace winrt::Windows::UI::ViewManagement;
    const auto settings = UISettings();

    const QColor accent = getSysColor(settings.GetColorValue(UIColorType::Accent));
    const QColor accentDarkest = getSysColor(settings.GetColorValue(UIColorType::AccentDark3));
#else
    const QColor accent = qt_accentColor();
    const QColor accentDarkest = accent.darker(120 * 120 * 120);
#endif

    const QColor linkColor = accent;
    const QColor btnFace = background;
    const QColor btnHighlight = getSysColor(COLOR_BTNHIGHLIGHT);

    result.setColor(QPalette::Highlight, getSysColor(COLOR_HIGHLIGHT));
    result.setColor(QPalette::WindowText, getSysColor(COLOR_WINDOWTEXT));
    result.setColor(QPalette::Button, btnFace);
    result.setColor(QPalette::Light, btnHighlight);
    result.setColor(QPalette::Dark, getSysColor(COLOR_BTNSHADOW));
    result.setColor(QPalette::Mid, result.button().color().darker(150));
    result.setColor(QPalette::Text, textColor);
    result.setColor(QPalette::PlaceholderText, placeHolderColor(textColor));
    result.setColor(QPalette::BrightText, btnHighlight);
    result.setColor(QPalette::Base, getSysColor(COLOR_WINDOW));
    result.setColor(QPalette::Window, btnFace);
    result.setColor(QPalette::ButtonText, getSysColor(COLOR_BTNTEXT));
    result.setColor(QPalette::Midlight, getSysColor(COLOR_3DLIGHT));
    result.setColor(QPalette::Shadow, getSysColor(COLOR_3DDKSHADOW));
    result.setColor(QPalette::HighlightedText, getSysColor(COLOR_HIGHLIGHTTEXT));
    result.setColor(QPalette::Accent, accent);

    result.setColor(QPalette::Link, linkColor);
    result.setColor(QPalette::LinkVisited, accentDarkest);
    result.setColor(QPalette::Inactive, QPalette::Button, result.button().color());
    result.setColor(QPalette::Inactive, QPalette::Window, result.window().color());
    result.setColor(QPalette::Inactive, QPalette::Light, result.light().color());
    result.setColor(QPalette::Inactive, QPalette::Dark, result.dark().color());

    if (result.midlight() == result.button())
        result.setColor(QPalette::Midlight, result.button().color().lighter(110));
}

static void populateDarkSystemBasePalette(QPalette &result)
{
#if QT_CONFIG(cpp_winrt)
    using namespace winrt::Windows::UI::ViewManagement;
    const auto settings = UISettings();

    // We have to craft a palette from these colors. The settings.UIElementColor(UIElementType) API
    // returns the old system colors, not the dark mode colors. If the background is black (which it
    // usually), then override it with a dark gray instead so that we can go up and down the lightness.
    const QColor foreground = getSysColor(settings.GetColorValue(UIColorType::Foreground));
    const QColor background = [&settings]() -> QColor {
        auto systemBackground = getSysColor(settings.GetColorValue(UIColorType::Background));
        if (systemBackground == Qt::black)
            systemBackground = QColor(0x1E, 0x1E, 0x1E);
        return systemBackground;
    }();

    const QColor accent = getSysColor(settings.GetColorValue(UIColorType::Accent));
    const QColor accentDark = getSysColor(settings.GetColorValue(UIColorType::AccentDark1));
    const QColor accentDarker = getSysColor(settings.GetColorValue(UIColorType::AccentDark2));
    const QColor accentDarkest = getSysColor(settings.GetColorValue(UIColorType::AccentDark3));
    const QColor accentLight = getSysColor(settings.GetColorValue(UIColorType::AccentLight1));
    const QColor accentLighter = getSysColor(settings.GetColorValue(UIColorType::AccentLight2));
    const QColor accentLightest = getSysColor(settings.GetColorValue(UIColorType::AccentLight3));
#else
    const QColor foreground = Qt::white;
    const QColor background = QColor(0x1E, 0x1E, 0x1E);
    const QColor accent = qt_accentColor();
    const QColor accentDark = accent.darker(120);
    const QColor accentDarker = accentDark.darker(120);
    const QColor accentDarkest = accentDarker.darker(120);
    const QColor accentLight = accent.lighter(120);
    const QColor accentLighter = accentLight.lighter(120);
    const QColor accentLightest = accentLighter.lighter(120);
#endif
    const QColor linkColor = accent;
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
    result.setColor(QPalette::All, QPalette::LinkVisited, accentDarkest);
    result.setColor(QPalette::All, QPalette::AlternateBase, accentDarkest);
    result.setColor(QPalette::All, QPalette::ToolTipBase, buttonColor);
    result.setColor(QPalette::All, QPalette::ToolTipText, foreground.darker(120));
    result.setColor(QPalette::All, QPalette::PlaceholderText, placeHolderColor(foreground));
    result.setColor(QPalette::All, QPalette::Accent, accent);
}

static inline QPalette toolTipPalette(const QPalette &systemPalette, bool light)
{
    QPalette result(systemPalette);
    const QColor tipBgColor = light ? getSysColor(COLOR_INFOBK)
                                    : systemPalette.button().color();
    const QColor tipTextColor = light ? getSysColor(COLOR_INFOTEXT)
                                      : systemPalette.buttonText().color().darker(120);

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

QWindowsTheme::QWindowsTheme()
{
    m_instance = this;
    std::fill(m_fonts, m_fonts + NFonts, nullptr);
    std::fill(m_palettes, m_palettes + NPalettes, nullptr);
    refresh();
    refreshIconPixmapSizes();
}

QWindowsTheme::~QWindowsTheme()
{
    clearPalettes();
    clearFonts();
    m_instance = nullptr;
}

static inline QStringList iconThemeSearchPaths()
{
    const QFileInfo appDir(QCoreApplication::applicationDirPath() + "/icons"_L1);
    return appDir.isDir() ? QStringList(appDir.absoluteFilePath()) : QStringList();
}

static inline QStringList styleNames()
{
    return { QStringLiteral("WindowsVista"), QStringLiteral("Windows") };
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
    return QWindowsContext::isDarkMode() ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
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
        !QWindowsContext::isDarkMode()
        || !QWindowsIntegration::instance()->darkModeHandling().testFlag(QWindowsApplication::DarkModeStyle);
    clearPalettes();
    m_palettes[SystemPalette] = new QPalette(QWindowsTheme::systemPalette(light ? Qt::ColorScheme::Light : Qt::ColorScheme::Dark));
    m_palettes[ToolTipPalette] = new QPalette(toolTipPalette(*m_palettes[SystemPalette], light));
    m_palettes[MenuPalette] = new QPalette(menuPalette(*m_palettes[SystemPalette], light));
    m_palettes[MenuBarPalette] = menuBarPalette(*m_palettes[MenuPalette], light);
    if (!light) {
#if QT_CONFIG(cpp_winrt)
        using namespace winrt::Windows::UI::ViewManagement;
        const auto settings = UISettings();
        const QColor accent = getSysColor(settings.GetColorValue(UIColorType::Accent));
        const QColor accentLight = getSysColor(settings.GetColorValue(UIColorType::AccentLight1));
        const QColor accentDarkest = getSysColor(settings.GetColorValue(UIColorType::AccentDark3));
#else
        const QColor accent = qt_accentColor();
        const QColor accentLight = accent.lighter(120);
        const QColor accentDarkest = accent.darker(120 * 120 * 120);
#endif
        m_palettes[CheckBoxPalette] = new QPalette(*m_palettes[SystemPalette]);
        m_palettes[CheckBoxPalette]->setColor(QPalette::Active, QPalette::Base, accent);
        m_palettes[CheckBoxPalette]->setColor(QPalette::Active, QPalette::Button, accentLight);
        m_palettes[CheckBoxPalette]->setColor(QPalette::Inactive, QPalette::Base, accentDarkest);
        m_palettes[RadioButtonPalette] = new QPalette(*m_palettes[CheckBoxPalette]);
    }
}

QPalette QWindowsTheme::systemPalette(Qt::ColorScheme colorScheme)
{
    QPalette result = standardPalette();

    switch (colorScheme) {
        case Qt::ColorScheme::Light:
            populateLightSystemBasePalette(result);
            break;
        case Qt::ColorScheme::Dark:
            populateDarkSystemBasePalette(result);
            break;
        default:
            qFatal("Unknown color scheme");
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
    result.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Text, disabled);
    result.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
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

void QWindowsTheme::windowsThemeChanged(QWindow * window)
{
    refresh();
    QWindowSystemInterface::handleThemeChange(window);
}

static int fileIconSizes[FileIconSizeCount];

void QWindowsTheme::refreshIconPixmapSizes()
{
    // Standard sizes: 16, 32, 48, 256
    fileIconSizes[SmallFileIcon] = GetSystemMetrics(SM_CXSMICON); // corresponds to SHGFI_SMALLICON);
    fileIconSizes[LargeFileIcon] = GetSystemMetrics(SM_CXICON); // corresponds to SHGFI_LARGEICON
    fileIconSizes[ExtraLargeFileIcon] =
        fileIconSizes[LargeFileIcon] + fileIconSizes[LargeFileIcon] / 2;
    fileIconSizes[JumboFileIcon] = 8 * fileIconSizes[LargeFileIcon]; // empirical, has not been observed to work

#ifdef USE_IIMAGELIST
    int *availEnd = fileIconSizes + JumboFileIcon + 1;
#else
    int *availEnd = fileIconSizes + LargeFileIcon + 1;
#endif // USE_IIMAGELIST
    m_fileIconSizes = QAbstractFileIconEngine::toSizeList(fileIconSizes, availEnd);
    qCDebug(lcQpaWindow) << __FUNCTION__ << m_fileIconSizes;
}

// Defined in qpixmap_win.cpp
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);

static QPixmap loadIconFromShell32(int resourceId, QSizeF size)
{
    if (const HMODULE hmod = QSystemLibrary::load(L"shell32")) {
        auto iconHandle =
            static_cast<HICON>(LoadImage(hmod, MAKEINTRESOURCE(resourceId),
                                         IMAGE_ICON, int(size.width()), int(size.height()), 0));
        if (iconHandle) {
            QPixmap iconpixmap = qt_pixmapFromWinHICON(iconHandle);
            DestroyIcon(iconHandle);
            return iconpixmap;
        }
    }
    return QPixmap();
}

QPixmap QWindowsTheme::standardPixmap(StandardPixmap sp, const QSizeF &pixmapSize) const
{
    int resourceId = -1;
    SHSTOCKICONID stockId = SIID_INVALID;
    UINT stockFlags = 0;
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
        stockFlags = SHGSI_LINKOVERLAY;
        Q_FALLTHROUGH();
    case FileIcon:
        stockId = SIID_DOCNOASSOC;
        resourceId = 1;
        break;
    case DirLinkIcon:
        stockFlags = SHGSI_LINKOVERLAY;
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
        stockFlags = SHGSI_LINKOVERLAY;
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

    if (stockId != SIID_INVALID) {
        QPixmap pixmap;
        SHSTOCKICONINFO iconInfo;
        memset(&iconInfo, 0, sizeof(iconInfo));
        iconInfo.cbSize = sizeof(iconInfo);
        stockFlags |= (pixmapSize.width() > 16 ? SHGFI_LARGEICON : SHGFI_SMALLICON);
        if (SHGetStockIconInfo(stockId, SHGFI_ICON | stockFlags, &iconInfo) == S_OK) {
            pixmap = qt_pixmapFromWinHICON(iconInfo.hIcon);
            DestroyIcon(iconInfo.hIcon);
            return pixmap;
        }
    }

    if (resourceId != -1) {
        QPixmap pixmap = loadIconFromShell32(resourceId, pixmapSize);
        if (!pixmap.isNull()) {
            if (sp == FileLinkIcon || sp == DirLinkIcon || sp == DirLinkOpenIcon) {
                QPainter painter(&pixmap);
                QPixmap link = loadIconFromShell32(30, pixmapSize);
                painter.drawPixmap(0, 0, int(pixmapSize.width()), int(pixmapSize.height()), link);
            }
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

static QPixmap pixmapFromShellImageList(int iImageList, const SHFILEINFO &info)
{
    QPixmap result;
#ifdef USE_IIMAGELIST
    // For MinGW:
    static const IID iID_IImageList = {0x46eb5926, 0x582e, 0x4017, {0x9f, 0xdf, 0xe8, 0x99, 0x8d, 0xaa, 0x9, 0x50}};

    IImageList *imageList = nullptr;
    HRESULT hr = SHGetImageList(iImageList, iID_IImageList, reinterpret_cast<void **>(&imageList));
    if (hr != S_OK)
        return result;
    HICON hIcon;
    hr = imageList->GetIcon(info.iIcon, ILD_TRANSPARENT, &hIcon);
    if (hr == S_OK) {
        result = qt_pixmapFromWinHICON(hIcon);
        DestroyIcon(hIcon);
    }
    imageList->Release();
#else
    Q_UNUSED(iImageList);
    Q_UNUSED(info);
#endif // USE_IIMAGELIST
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
#ifdef USE_IIMAGELIST
        width > fileIconSizes[ExtraLargeFileIcon]
            ? sHIL_JUMBO
            : (width > fileIconSizes[LargeFileIcon] ? sHIL_EXTRALARGE : 0);
#else
        0;
#endif // !USE_IIMAGELIST
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

    SHFILEINFO info;
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
    const bool val = shGetFileInfoBackground(path, attributes, &info, flags);

    // Even if GetFileInfo returns a valid result, hIcon can be empty in some cases
    if (val && info.hIcon) {
        QString key;
        if (cacheableDirIcon) {
            if (useDefaultFolderIcon && defaultFolderIIcon < 0)
                defaultFolderIIcon = info.iIcon;

            //using the unique icon index provided by windows save us from duplicate keys
            key = dirIconPixmapCacheKey(info.iIcon, iconSize, requestedImageListSize);
            QPixmapCache::find(key, &pixmap);
            if (!pixmap.isNull()) {
                QMutexLocker locker(&mx);
                dirIconEntryCache.insert(filePath, FakePointer<int>::create(info.iIcon));
            }
        }

        if (pixmap.isNull()) {
            if (requestedImageListSize) {
                pixmap = pixmapFromShellImageList(requestedImageListSize, info);
                if (pixmap.isNull() && requestedImageListSize == sHIL_JUMBO)
                    pixmap = pixmapFromShellImageList(sHIL_EXTRALARGE, info);
            }
            if (pixmap.isNull())
                pixmap = qt_pixmapFromWinHICON(info.hIcon);
            if (!pixmap.isNull()) {
                if (cacheableDirIcon) {
                    QMutexLocker locker(&mx);
                    QPixmapCache::insert(key, pixmap);
                    dirIconEntryCache.insert(filePath, FakePointer<int>::create(info.iIcon));
                }
            } else {
                qWarning("QWindowsTheme::fileIconPixmap() no icon found");
            }
        }
        DestroyIcon(info.hIcon);
    }

    return pixmap;
}

QIcon QWindowsTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions) const
{
    return QIcon(new QWindowsFileIconEngine(fileInfo, iconOptions));
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

bool QWindowsTheme::queryDarkMode()
{
    if (queryHighContrast()) {
        return false;
    }
    const auto setting = QWinRegistryKey(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)")
                         .dwordValue(L"AppsUseLightTheme");
    return setting.second && setting.first == 0;
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
