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

// SHSTOCKICONINFO is only available since Vista
#if _WIN32_WINNT < 0x0601
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif

#include "qwindowstheme.h"
#include "qwindowsmenu.h"
#include "qwindowsdialoghelpers.h"
#include "qwindowscontext.h"
#include "qwindowsintegration.h"
#if QT_CONFIG(systemtrayicon)
#  include "qwindowssystemtrayicon.h"
#endif
#include "qwindowsscreen.h"
#include "qt_windows.h"
#include <commctrl.h>
#include <objbase.h>
#ifndef Q_CC_MINGW
#  include <commoncontrols.h>
#endif
#include <shellapi.h>

#include <QtCore/qvariant.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qoperatingsystemversion.h>
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
#include <QtThemeSupport/private/qabstractfileiconengine_p.h>
#include <QtFontDatabaseSupport/private/qwindowsfontdatabase_p.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qsystemlibrary_p.h>
#include <private/qwinregistry_p.h>

#include <algorithm>

#if defined(__IImageList_INTERFACE_DEFINED__) && defined(__IID_DEFINED__)
#  define USE_IIMAGELIST
#endif

QT_BEGIN_NAMESPACE

static inline QColor COLORREFToQColor(COLORREF cr)
{
    return QColor(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

static inline QTextStream& operator<<(QTextStream &str, const QColor &c)
{
    str.setIntegerBase(16);
    str.setFieldWidth(2);
    str.setPadChar(u'0');
    str << " rgb: #" << c.red()  << c.green() << c.blue();
    str.setIntegerBase(10);
    str.setFieldWidth(0);
    return str;
}

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
    return COLORREFToQColor(GetSysColor(index));
}

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
        m_init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

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

        if (m_init != S_FALSE)
            CoUninitialize();
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
    HRESULT m_init;
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

// Dark Mode constants
enum DarkModeColors : QRgb  {
    darkModeBtnHighlightRgb = 0xc0c0c0,
    darkModeBtnShadowRgb = 0x808080,
    darkModeHighlightRgb = 0x0055ff, // deviating from 0x800080
    darkModeMenuHighlightRgb = darkModeHighlightRgb
};

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

static void populateLightSystemBasePalette(QPalette &result)
{
    result.setColor(QPalette::WindowText, getSysColor(COLOR_WINDOWTEXT));
    const QColor btnFace = getSysColor(COLOR_BTNFACE);
    result.setColor(QPalette::Button, btnFace);
    const QColor btnHighlight = getSysColor(COLOR_BTNHIGHLIGHT);
    result.setColor(QPalette::Light, btnHighlight);
    result.setColor(QPalette::Dark, getSysColor(COLOR_BTNSHADOW));
    result.setColor(QPalette::Mid, result.button().color().darker(150));
    result.setColor(QPalette::Text, getSysColor(COLOR_WINDOWTEXT));
    result.setColor(QPalette::BrightText, btnHighlight);
    result.setColor(QPalette::Base, getSysColor(COLOR_WINDOW));
    result.setColor(QPalette::Window, btnFace);
    result.setColor(QPalette::ButtonText, getSysColor(COLOR_BTNTEXT));
    result.setColor(QPalette::Midlight, getSysColor(COLOR_3DLIGHT));
    result.setColor(QPalette::Shadow, getSysColor(COLOR_3DDKSHADOW));
    result.setColor(QPalette::Highlight, getSysColor(COLOR_HIGHLIGHT));
    result.setColor(QPalette::HighlightedText, getSysColor(COLOR_HIGHLIGHTTEXT));
}

static void populateDarkSystemBasePalette(QPalette &result)
{
    const QColor darkModeWindowText = Qt::white;
    result.setColor(QPalette::WindowText, darkModeWindowText);
    const QColor darkModebtnFace = Qt::black;
    result.setColor(QPalette::Button, darkModebtnFace);
    const QColor btnHighlight = QColor(darkModeBtnHighlightRgb);
    result.setColor(QPalette::Light, btnHighlight);
    result.setColor(QPalette::Dark, QColor(darkModeBtnShadowRgb));
    result.setColor(QPalette::Mid, result.button().color().darker(150));
    result.setColor(QPalette::Text, darkModeWindowText);
    result.setColor(QPalette::BrightText, btnHighlight);
    result.setColor(QPalette::Base, darkModebtnFace);
    result.setColor(QPalette::Window, darkModebtnFace);
    result.setColor(QPalette::ButtonText, darkModeWindowText);
    result.setColor(QPalette::Midlight, darkModeWindowText);
    result.setColor(QPalette::Shadow, darkModeWindowText);
    result.setColor(QPalette::Highlight, QColor(darkModeHighlightRgb));
    result.setColor(QPalette::HighlightedText, darkModeWindowText);
}

static QPalette systemPalette(bool light)
{
    QPalette result = standardPalette();
    if (light)
        populateLightSystemBasePalette(result);
    else
        populateDarkSystemBasePalette(result);

    result.setColor(QPalette::Link, Qt::blue);
    result.setColor(QPalette::LinkVisited, Qt::magenta);
    result.setColor(QPalette::Inactive, QPalette::Button, result.button().color());
    result.setColor(QPalette::Inactive, QPalette::Window, result.window().color());
    result.setColor(QPalette::Inactive, QPalette::Light, result.light().color());
    result.setColor(QPalette::Inactive, QPalette::Dark, result.dark().color());

    if (result.midlight() == result.button())
        result.setColor(QPalette::Midlight, result.button().color().lighter(110));
    if (result.window() != result.base()) {
        result.setColor(QPalette::Inactive, QPalette::Highlight, result.color(QPalette::Inactive, QPalette::Window));
        result.setColor(QPalette::Inactive, QPalette::HighlightedText, result.color(QPalette::Inactive, QPalette::Text));
    }

    const QColor disabled =
        mixColors(result.windowText().color(), result.button().color());

    result.setColorGroup(QPalette::Disabled, result.windowText(), result.button(),
                         result.light(), result.dark(), result.mid(),
                         result.text(), result.brightText(), result.base(),
                         result.window());
    result.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Text, disabled);
    result.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Highlight,
                    light ? getSysColor(COLOR_HIGHLIGHT) : QColor(darkModeHighlightRgb));
    result.setColor(QPalette::Disabled, QPalette::HighlightedText,
                    light ? getSysColor(COLOR_HIGHLIGHTTEXT) : QColor(Qt::white));
    result.setColor(QPalette::Disabled, QPalette::Base,
                    result.window().color());
    return result;
}

static inline QPalette toolTipPalette(const QPalette &systemPalette, bool light)
{
    QPalette result(systemPalette);
    const QColor tipBgColor = light ? getSysColor(COLOR_INFOBK) : QColor(Qt::black);
    const QColor tipTextColor = light ? getSysColor(COLOR_INFOTEXT) : QColor(Qt::white);

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
    const QColor disabled =
        mixColors(result.windowText().color(), result.button().color());
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
    QPalette result(systemPalette);
    const QColor menuColor = light ? getSysColor(COLOR_MENU) : QColor(Qt::black);
    const QColor menuTextColor = light ? getSysColor(COLOR_MENUTEXT) : QColor(Qt::white);
    const QColor disabled = light
        ? getSysColor(COLOR_GRAYTEXT) : QColor(darkModeBtnHighlightRgb);
    // we might need a special color group for the result.
    result.setColor(QPalette::Active, QPalette::Button, menuColor);
    result.setColor(QPalette::Active, QPalette::Text, menuTextColor);
    result.setColor(QPalette::Active, QPalette::WindowText, menuTextColor);
    result.setColor(QPalette::Active, QPalette::ButtonText, menuTextColor);
    result.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    result.setColor(QPalette::Disabled, QPalette::Text, disabled);
    const bool isFlat = booleanSystemParametersInfo(SPI_GETFLATMENU, false);
    const QColor highlightColor = light
        ? (getSysColor(isFlat ? COLOR_MENUHILIGHT : COLOR_HIGHLIGHT))
        : QColor(darkModeMenuHighlightRgb);
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
    if (booleanSystemParametersInfo(SPI_GETFLATMENU, false)) {
        result = new QPalette(menuPalette);
        const QColor menubar(light ? getSysColor(COLOR_MENUBAR) : QColor(Qt::black));
        result->setColor(QPalette::Active, QPalette::Button, menubar);
        result->setColor(QPalette::Disabled, QPalette::Button, menubar);
        result->setColor(QPalette::Inactive, QPalette::Button, menubar);
    }
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
    const QFileInfo appDir(QCoreApplication::applicationDirPath() + QLatin1String("/icons"));
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
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
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
        || (QWindowsIntegration::instance()->options() & QWindowsIntegration::DarkModeStyle) == 0;
    m_palettes[SystemPalette] = new QPalette(systemPalette(light));
    m_palettes[ToolTipPalette] = new QPalette(toolTipPalette(*m_palettes[SystemPalette], light));
    m_palettes[MenuPalette] = new QPalette(menuPalette(*m_palettes[SystemPalette], light));
    m_palettes[MenuBarPalette] = menuBarPalette(*m_palettes[MenuPalette], light);
    if (!light) {
        m_palettes[ButtonPalette] = new QPalette(*m_palettes[SystemPalette]);
        m_palettes[ButtonPalette]->setColor(QPalette::Button, QColor(0x666666u));
        const QColor checkBoxBlue(0x0078d7u);
        const QColor white(Qt::white);
        m_palettes[CheckBoxPalette] = new QPalette(*m_palettes[SystemPalette]);
        m_palettes[CheckBoxPalette]->setColor(QPalette::Window, checkBoxBlue);
        m_palettes[CheckBoxPalette]->setColor(QPalette::Base, checkBoxBlue);
        m_palettes[CheckBoxPalette]->setColor(QPalette::Button, checkBoxBlue);
        m_palettes[CheckBoxPalette]->setColor(QPalette::ButtonText, white);
        m_palettes[RadioButtonPalette] = new QPalette(*m_palettes[CheckBoxPalette]);

    }
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
QDebug operator<<(QDebug d, const LOGFONT &lf); // in platformsupport

QDebug operator<<(QDebug d, const NONCLIENTMETRICS &m)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "NONCLIENTMETRICS(iMenu=" << m.iMenuWidth  << 'x' << m.iMenuHeight
      << ", lfCaptionFont=" << m.lfCaptionFont << ", lfSmCaptionFont="
      << m.lfSmCaptionFont << ", lfMenuFont=" << m.lfMenuFont
      <<  ", lfMessageFont=" << m.lfMessageFont << ", lfStatusFont="
      << m.lfStatusFont << ')';
    return d;
}
#endif // QT_NO_DEBUG_STREAM

void QWindowsTheme::refreshFonts()
{
    clearFonts();
    if (!QGuiApplication::desktopSettingsAware())
        return;
    NONCLIENTMETRICS ncm;
    auto screenManager = QWindowsContext::instance()->screenManager();
    QWindowsContext::nonClientMetricsForScreen(&ncm, screenManager.screens().value(0));
    qCDebug(lcQpaWindows) << __FUNCTION__ << ncm;
    const QFont menuFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfMenuFont);
    const QFont messageBoxFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfMessageFont);
    const QFont statusFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfStatusFont);
    const QFont titleFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfCaptionFont);
    QFont fixedFont(QStringLiteral("Courier New"), messageBoxFont.pointSize());
    fixedFont.setStyleHint(QFont::TypeWriter);

    LOGFONT lfIconTitleFont;
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lfIconTitleFont), &lfIconTitleFont, 0);
    const QFont iconTitleFont = QWindowsFontDatabase::LOGFONT_to_QFont(lfIconTitleFont);

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
    qCDebug(lcQpaWindows) << __FUNCTION__ << m_fileIconSizes;
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
    QString key = QLatin1String("qt_dir_") + QString::number(iIcon);
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

    Q_STATIC_ASSERT_X(sizeof(T) <= sizeof(void *), "FakePointers can only go that far.");

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
    Q_UNUSED(iImageList)
    Q_UNUSED(info)
#endif // USE_IIMAGELIST
    return result;
}

class QWindowsFileIconEngine : public QAbstractFileIconEngine
{
public:
    explicit QWindowsFileIconEngine(const QFileInfo &info, QPlatformTheme::IconOptions opts) :
        QAbstractFileIconEngine(info, opts) {}

    QList<QSize> availableSizes(QIcon::Mode = QIcon::Normal, QIcon::State = QIcon::Off) const override
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
    return QLatin1String("qt_.")
        + (suffix.isEmpty() ? fileInfo().fileName() : std::move(suffix).toUpper()); // handle "Makefile"                                    ;)
}

QPixmap QWindowsFileIconEngine::filePixmap(const QSize &size, QIcon::Mode, QIcon::State)
{
    /* We don't use the variable, but by storing it statically, we
     * ensure CoInitialize is only called once. */
    static HRESULT comInit = CoInitialize(nullptr);
    Q_UNUSED(comInit);

    static QCache<QString, FakePointer<int> > dirIconEntryCache(1000);
    static QMutex mx;
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
    if (QOperatingSystemVersion::current()
        < QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 17763)
        || queryHighContrast()) {
        return false;
    }
    const auto setting = QWinRegistryKey(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)")
                         .dwordValue(L"AppsUseLightTheme");
    return setting.second && setting.first == 0;
}

bool QWindowsTheme::queryHighContrast()
{
    return booleanSystemParametersInfo(SPI_GETHIGHCONTRAST, false);
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
