/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qstringlist.h"
#include "qvector.h"
#include "qfileinfo.h"
#include "qcorecmdlineargs_p.h"
#ifndef QT_NO_QOBJECT
#include "qmutex.h"
#include <private/qthread_p.h>
#include <private/qlocking_p.h>
#endif
#include "qtextstream.h"
#include <ctype.h>
#include <qt_windows.h>

#ifdef Q_OS_WINRT
#include <qfunctions_winrt.h>
#include <wrl.h>
#include <Windows.ApplicationModel.core.h>
#include <windows.foundation.h>
using namespace ABI::Windows::ApplicationModel;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
#endif

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT QString qAppFileName()                // get application file name
{
    /*
      GetModuleFileName() returns the length of the module name, when it has
      space to store it and 0-terminate; this return is necessarily smaller than
      the buffer size, as it doesn't include the terminator. When it lacks
      space, the function returns the full size of the buffer and fills the
      buffer, truncating the full path to make it fit. We have reports that
      GetModuleFileName sometimes doesn't set the error number to
      ERROR_INSUFFICIENT_BUFFER, as claimed by the MSDN documentation; so we
      only trust the answer when the return is actually less than the buffer
      size we pass in. (When truncating, except on XP, it does so by enough to
      still have space to 0-terminate; in either case, it fills the claimed
      space and returns the size of the space. While XP might thus give us the
      full name, without a 0 terminator, and return its actual length, we can
      never be sure that's what's happened until a later call with bigger buffer
      confirms it by returning less than its buffer size.)
    */
    // Full path may be longer than MAX_PATH - expand until we have enough space:
    QVarLengthArray<wchar_t, MAX_PATH + 1> space;
    DWORD v;
    size_t size = 1;
    do {
        size += MAX_PATH;
        space.resize(int(size));
        v = GetModuleFileName(NULL, space.data(), DWORD(space.size()));
    } while (Q_UNLIKELY(v >= size));

    return QString::fromWCharArray(space.data(), v);
}

QString QCoreApplicationPrivate::appName() const
{
    return QFileInfo(qAppFileName()).baseName();
}

QString QCoreApplicationPrivate::appVersion() const
{
    QString applicationVersion;
#ifndef QT_BOOTSTRAPPED
#  ifdef Q_OS_WINRT
    HRESULT hr;

    ComPtr<IPackageStatics> packageFactory;
    hr = RoGetActivationFactory(
        HString::MakeReference(RuntimeClass_Windows_ApplicationModel_Package).Get(),
        IID_PPV_ARGS(&packageFactory));
    RETURN_IF_FAILED("Failed to create package instance", return QString());

    ComPtr<IPackage> package;
    packageFactory->get_Current(&package);
    RETURN_IF_FAILED("Failed to get current application package", return QString());

    ComPtr<IPackageId> packageId;
    package->get_Id(&packageId);
    RETURN_IF_FAILED("Failed to get current application package ID", return QString());

    PackageVersion version;
    packageId->get_Version(&version);
    RETURN_IF_FAILED("Failed to get current application package version", return QString());

    applicationVersion = QStringLiteral("%1.%2.%3.%4")
            .arg(version.Major)
            .arg(version.Minor)
            .arg(version.Build)
            .arg(version.Revision);
#  else
    const QString appFileName = qAppFileName();
    QVarLengthArray<wchar_t> buffer(appFileName.size() + 1);
    buffer[appFileName.toWCharArray(buffer.data())] = 0;

    DWORD versionInfoSize = GetFileVersionInfoSize(buffer.data(), nullptr);
    if (versionInfoSize) {
        QVarLengthArray<BYTE> info(static_cast<int>(versionInfoSize));
        if (GetFileVersionInfo(buffer.data(), 0, versionInfoSize, info.data())) {
            UINT size;
            DWORD *fi;

            if (VerQueryValue(info.data(), __TEXT("\\"),
                              reinterpret_cast<void **>(&fi), &size) && size) {
                const VS_FIXEDFILEINFO *verInfo = reinterpret_cast<const VS_FIXEDFILEINFO *>(fi);
                applicationVersion = QStringLiteral("%1.%2.%3.%4")
                        .arg(HIWORD(verInfo->dwProductVersionMS))
                        .arg(LOWORD(verInfo->dwProductVersionMS))
                        .arg(HIWORD(verInfo->dwProductVersionLS))
                        .arg(LOWORD(verInfo->dwProductVersionLS));
            }
        }
    }
#  endif
#endif
    return applicationVersion;
}

#ifndef Q_OS_WINRT

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
Q_CORE_EXPORT HINSTANCE qWinAppInst()                // get Windows app handle
{
    return GetModuleHandle(0);
}

Q_CORE_EXPORT HINSTANCE qWinAppPrevInst()                // get Windows prev app handle
{
    return 0;
}

Q_CORE_EXPORT int qWinAppCmdShow()                        // get main window show command
{
    STARTUPINFO startupInfo;
    GetStartupInfo(&startupInfo);

    return (startupInfo.dwFlags & STARTF_USESHOWWINDOW)
        ? startupInfo.wShowWindow
        : SW_SHOWDEFAULT;
}
#endif

#ifndef QT_NO_QOBJECT

#if defined(Q_OS_WIN) && !defined(QT_NO_DEBUG_STREAM)
/*****************************************************************************
  Convenience functions for convert WM_* messages into human readable strings,
  including a nifty QDebug operator<< for simple QDebug() << msg output.
 *****************************************************************************/
QT_BEGIN_INCLUDE_NAMESPACE
#include <windowsx.h>
#include "qdebug.h"
QT_END_INCLUDE_NAMESPACE

#if !defined(GET_X_LPARAM)
#  define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#  define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

// The values below should never change. Note that none of the usual
// WM_...FIRST & WM_...LAST values are in the list, as they normally have other
// WM_... representations

template <class IntType>
struct QWinMessageMapping {
    IntType value;
    const char *name;
};

#define FLAG_ENTRY(x) {x, #x} // for populating arrays

// Looks up a value in a list of QWinMessageMapping
template <class IntType>
static const char *findWinMessageMapping(const QWinMessageMapping<IntType> *haystack,
                                         size_t haystackSize,
                                         IntType needle)
{
    for (auto p = haystack, end = haystack + haystackSize; p < end; ++p) {
        if (p->value == needle)
            return p->name;
    }
    return nullptr;
}

// Format flags using a mapping as "Flag1 | Flag2"...
template <class IntType>
static QString flagsValue(const QWinMessageMapping<IntType> *haystack,
                          size_t haystackSize, IntType value)
{
    QString result;
    for (auto p = haystack, end = haystack + haystackSize; p < end; ++p) {
        if ((p->value & value) == p->value) {
            if (!result.isEmpty())
                result += QLatin1String(" | ");
            result += QLatin1String(p->name);
        }
    }
    return result;
}

// Looks up the WM_ message in the table inside
static const char *findWMstr(uint msg)
{
    static const QWinMessageMapping<uint> knownWM[] =
{{ 0x0000, "WM_NULL" },
 { 0x0001, "WM_CREATE" },
 { 0x0002, "WM_DESTROY" },
 { 0x0003, "WM_MOVE" },
 { 0x0005, "WM_SIZE" },
 { 0x0006, "WM_ACTIVATE" },
 { 0x0007, "WM_SETFOCUS" },
 { 0x0008, "WM_KILLFOCUS" },
 { 0x000A, "WM_ENABLE" },
 { 0x000B, "WM_SETREDRAW" },
 { 0x000C, "WM_SETTEXT" },
 { 0x000D, "WM_GETTEXT" },
 { 0x000E, "WM_GETTEXTLENGTH" },
 { 0x000F, "WM_PAINT" },
 { 0x0010, "WM_CLOSE" },
 { 0x0011, "WM_QUERYENDSESSION" },
 { 0x0013, "WM_QUERYOPEN" },
 { 0x0016, "WM_ENDSESSION" },
 { 0x0012, "WM_QUIT" },
 { 0x0014, "WM_ERASEBKGND" },
 { 0x0015, "WM_SYSCOLORCHANGE" },
 { 0x0018, "WM_SHOWWINDOW" },
 { 0x001A, "WM_WININICHANGE" },
 { 0x001B, "WM_DEVMODECHANGE" },
 { 0x001C, "WM_ACTIVATEAPP" },
 { 0x001D, "WM_FONTCHANGE" },
 { 0x001E, "WM_TIMECHANGE" },
 { 0x001F, "WM_CANCELMODE" },
 { 0x0020, "WM_SETCURSOR" },
 { 0x0021, "WM_MOUSEACTIVATE" },
 { 0x0022, "WM_CHILDACTIVATE" },
 { 0x0023, "WM_QUEUESYNC" },
 { 0x0024, "WM_GETMINMAXINFO" },
 { 0x0026, "WM_PAINTICON" },
 { 0x0027, "WM_ICONERASEBKGND" },
 { 0x0028, "WM_NEXTDLGCTL" },
 { 0x002A, "WM_SPOOLERSTATUS" },
 { 0x002B, "WM_DRAWITEM" },
 { 0x002C, "WM_MEASUREITEM" },
 { 0x002D, "WM_DELETEITEM" },
 { 0x002E, "WM_VKEYTOITEM" },
 { 0x002F, "WM_CHARTOITEM" },
 { 0x0030, "WM_SETFONT" },
 { 0x0031, "WM_GETFONT" },
 { 0x0032, "WM_SETHOTKEY" },
 { 0x0033, "WM_GETHOTKEY" },
 { 0x0037, "WM_QUERYDRAGICON" },
 { 0x0039, "WM_COMPAREITEM" },
 { 0x003D, "WM_GETOBJECT" },
 { 0x0041, "WM_COMPACTING" },
 { 0x0044, "WM_COMMNOTIFY" },
 { 0x0046, "WM_WINDOWPOSCHANGING" },
 { 0x0047, "WM_WINDOWPOSCHANGED" },
 { 0x0048, "WM_POWER" },
 { 0x004A, "WM_COPYDATA" },
 { 0x004B, "WM_CANCELJOURNAL" },
 { 0x004E, "WM_NOTIFY" },
 { 0x0050, "WM_INPUTLANGCHANGEREQUEST" },
 { 0x0051, "WM_INPUTLANGCHANGE" },
 { 0x0052, "WM_TCARD" },
 { 0x0053, "WM_HELP" },
 { 0x0054, "WM_USERCHANGED" },
 { 0x0055, "WM_NOTIFYFORMAT" },
 { 0x007B, "WM_CONTEXTMENU" },
 { 0x007C, "WM_STYLECHANGING" },
 { 0x007D, "WM_STYLECHANGED" },
 { 0x007E, "WM_DISPLAYCHANGE" },
 { 0x007F, "WM_GETICON" },
 { 0x0080, "WM_SETICON" },
 { 0x0081, "WM_NCCREATE" },
 { 0x0082, "WM_NCDESTROY" },
 { 0x0083, "WM_NCCALCSIZE" },
 { 0x0084, "WM_NCHITTEST" },
 { 0x0085, "WM_NCPAINT" },
 { 0x0086, "WM_NCACTIVATE" },
 { 0x0087, "WM_GETDLGCODE" },
 { 0x0088, "WM_SYNCPAINT" },
 { 0x00A0, "WM_NCMOUSEMOVE" },
 { 0x00A1, "WM_NCLBUTTONDOWN" },
 { 0x00A2, "WM_NCLBUTTONUP" },
 { 0x00A3, "WM_NCLBUTTONDBLCLK" },
 { 0x00A4, "WM_NCRBUTTONDOWN" },
 { 0x00A5, "WM_NCRBUTTONUP" },
 { 0x00A6, "WM_NCRBUTTONDBLCLK" },
 { 0x00A7, "WM_NCMBUTTONDOWN" },
 { 0x00A8, "WM_NCMBUTTONUP" },
 { 0x00A9, "WM_NCMBUTTONDBLCLK" },
 { 0x00AB, "WM_NCXBUTTONDOWN" },
 { 0x00AC, "WM_NCXBUTTONUP" },
 { 0x00AD, "WM_NCXBUTTONDBLCLK" },
 { 0x00FF, "WM_INPUT" },
 { 0x0100, "WM_KEYDOWN" },
 { 0x0101, "WM_KEYUP" },
 { 0x0102, "WM_CHAR" },
 { 0x0103, "WM_DEADCHAR" },
 { 0x0104, "WM_SYSKEYDOWN" },
 { 0x0105, "WM_SYSKEYUP" },
 { 0x0106, "WM_SYSCHAR" },
 { 0x0107, "WM_SYSDEADCHAR" },
 { 0x0109, "WM_UNICHAR" },
 { 0x010D, "WM_IME_STARTCOMPOSITION" },
 { 0x010E, "WM_IME_ENDCOMPOSITION" },
 { 0x010F, "WM_IME_COMPOSITION" },
 { 0x0110, "WM_INITDIALOG" },
 { 0x0111, "WM_COMMAND" },
 { 0x0112, "WM_SYSCOMMAND" },
 { 0x0113, "WM_TIMER" },
 { 0x0114, "WM_HSCROLL" },
 { 0x0115, "WM_VSCROLL" },
 { 0x0116, "WM_INITMENU" },
 { 0x0117, "WM_INITMENUPOPUP" },
 { 0x011F, "WM_MENUSELECT" },
 { 0x0120, "WM_MENUCHAR" },
 { 0x0121, "WM_ENTERIDLE" },
 { 0x0122, "WM_MENURBUTTONUP" },
 { 0x0123, "WM_MENUDRAG" },
 { 0x0124, "WM_MENUGETOBJECT" },
 { 0x0125, "WM_UNINITMENUPOPUP" },
 { 0x0126, "WM_MENUCOMMAND" },
 { 0x0127, "WM_CHANGEUISTATE" },
 { 0x0128, "WM_UPDATEUISTATE" },
 { 0x0129, "WM_QUERYUISTATE" },
 { 0x0132, "WM_CTLCOLORMSGBOX" },
 { 0x0133, "WM_CTLCOLOREDIT" },
 { 0x0134, "WM_CTLCOLORLISTBOX" },
 { 0x0135, "WM_CTLCOLORBTN" },
 { 0x0136, "WM_CTLCOLORDLG" },
 { 0x0137, "WM_CTLCOLORSCROLLBAR" },
 { 0x0138, "WM_CTLCOLORSTATIC" },
 { 0x0200, "WM_MOUSEMOVE" },
 { 0x0201, "WM_LBUTTONDOWN" },
 { 0x0202, "WM_LBUTTONUP" },
 { 0x0203, "WM_LBUTTONDBLCLK" },
 { 0x0204, "WM_RBUTTONDOWN" },
 { 0x0205, "WM_RBUTTONUP" },
 { 0x0206, "WM_RBUTTONDBLCLK" },
 { 0x0207, "WM_MBUTTONDOWN" },
 { 0x0208, "WM_MBUTTONUP" },
 { 0x0209, "WM_MBUTTONDBLCLK" },
 { 0x020A, "WM_MOUSEWHEEL" },
 { 0x020B, "WM_XBUTTONDOWN" },
 { 0x020C, "WM_XBUTTONUP" },
 { 0x020D, "WM_XBUTTONDBLCLK" },
 { 0x020E, "WM_MOUSEHWHEEL" },
 { 0x0210, "WM_PARENTNOTIFY" },
 { 0x0211, "WM_ENTERMENULOOP" },
 { 0x0212, "WM_EXITMENULOOP" },
 { 0x0213, "WM_NEXTMENU" },
 { 0x0214, "WM_SIZING" },
 { 0x0215, "WM_CAPTURECHANGED" },
 { 0x0216, "WM_MOVING" },
 { 0x0218, "WM_POWERBROADCAST" },
 { 0x0219, "WM_DEVICECHANGE" },
 { 0x0220, "WM_MDICREATE" },
 { 0x0221, "WM_MDIDESTROY" },
 { 0x0222, "WM_MDIACTIVATE" },
 { 0x0223, "WM_MDIRESTORE" },
 { 0x0224, "WM_MDINEXT" },
 { 0x0225, "WM_MDIMAXIMIZE" },
 { 0x0226, "WM_MDITILE" },
 { 0x0227, "WM_MDICASCADE" },
 { 0x0228, "WM_MDIICONARRANGE" },
 { 0x0229, "WM_MDIGETACTIVE" },
 { 0x0230, "WM_MDISETMENU" },
 { 0x0231, "WM_ENTERSIZEMOVE" },
 { 0x0232, "WM_EXITSIZEMOVE" },
 { 0x0233, "WM_DROPFILES" },
 { 0x0234, "WM_MDIREFRESHMENU" },
 { 0x0241, "WM_NCPOINTERUPDATE"},
 { 0x0242, "WM_NCPOINTERDOWN"},
 { 0x0243, "WM_NCPOINTERUP"},
 { 0x0245, "WM_POINTERUPDATE"},
 { 0x0246, "WM_POINTERDOWN"},
 { 0x0247, "WM_POINTERUP"},
 { 0x0249, "WM_POINTERENTER"},
 { 0x024A, "WM_POINTERLEAVE"},
 { 0x0248, "WM_POINTERACTIVATE"},
 { 0x024C, "WM_POINTERCAPTURECHANGED"},
 { 0x024D, "WM_TOUCHHITTESTING"},
 { 0x024E, "WM_POINTERWHEEL"},
 { 0x024F, "WM_POINTERHWHEEL"},
 { 0x0250, "DM_POINTERHITTEST"},
 { 0x0251, "WM_POINTERROUTEDTO"},
 { 0x0252, "WM_POINTERROUTEDAWAY"},
 { 0x0253, "WM_POINTERROUTEDRELEASED"},
 { 0x0281, "WM_IME_SETCONTEXT" },
 { 0x0282, "WM_IME_NOTIFY" },
 { 0x0283, "WM_IME_CONTROL" },
 { 0x0284, "WM_IME_COMPOSITIONFULL" },
 { 0x0285, "WM_IME_SELECT" },
 { 0x0286, "WM_IME_CHAR" },
 { 0x0288, "WM_IME_REQUEST" },
 { 0x0290, "WM_IME_KEYDOWN" },
 { 0x0291, "WM_IME_KEYUP" },
 { 0x02A0, "WM_NCMOUSEHOVER" },
 { 0x02A1, "WM_MOUSEHOVER" },
 { 0x02A2, "WM_NCMOUSELEAVE" },
 { 0x02A3, "WM_MOUSELEAVE" },
 { 0x02B1, "WM_WTSSESSION_CHANGE" },
 { 0x02C0, "WM_TABLET_FIRST" },
 { 0x02C1, "WM_TABLET_FIRST + 1" },
 { 0x02C2, "WM_TABLET_FIRST + 2" },
 { 0x02C3, "WM_TABLET_FIRST + 3" },
 { 0x02C4, "WM_TABLET_FIRST + 4" },
 { 0x02C5, "WM_TABLET_FIRST + 5" },
 { 0x02C6, "WM_TABLET_FIRST + 6" },
 { 0x02C7, "WM_TABLET_FIRST + 7" },
 { 0x02C8, "WM_TABLET_FIRST + 8" },
 { 0x02C9, "WM_TABLET_FIRST + 9" },
 { 0x02CA, "WM_TABLET_FIRST + 10" },
 { 0x02CB, "WM_TABLET_FIRST + 11" },
 { 0x02CC, "WM_TABLET_FIRST + 12" },
 { 0x02CD, "WM_TABLET_FIRST + 13" },
 { 0x02CE, "WM_TABLET_FIRST + 14" },
 { 0x02CF, "WM_TABLET_FIRST + 15" },
 { 0x02D0, "WM_TABLET_FIRST + 16" },
 { 0x02D1, "WM_TABLET_FIRST + 17" },
 { 0x02D2, "WM_TABLET_FIRST + 18" },
 { 0x02D3, "WM_TABLET_FIRST + 19" },
 { 0x02D4, "WM_TABLET_FIRST + 20" },
 { 0x02D5, "WM_TABLET_FIRST + 21" },
 { 0x02D6, "WM_TABLET_FIRST + 22" },
 { 0x02D7, "WM_TABLET_FIRST + 23" },
 { 0x02D8, "WM_TABLET_FIRST + 24" },
 { 0x02D9, "WM_TABLET_FIRST + 25" },
 { 0x02DA, "WM_TABLET_FIRST + 26" },
 { 0x02DB, "WM_TABLET_FIRST + 27" },
 { 0x02DC, "WM_TABLET_FIRST + 28" },
 { 0x02DD, "WM_TABLET_FIRST + 29" },
 { 0x02DE, "WM_TABLET_FIRST + 30" },
 { 0x02DF, "WM_TABLET_LAST" },
 { 0x02E0, "WM_DPICHANGED" },
 { 0x0300, "WM_CUT" },
 { 0x0301, "WM_COPY" },
 { 0x0302, "WM_PASTE" },
 { 0x0303, "WM_CLEAR" },
 { 0x0304, "WM_UNDO" },
 { 0x0305, "WM_RENDERFORMAT" },
 { 0x0306, "WM_RENDERALLFORMATS" },
 { 0x0307, "WM_DESTROYCLIPBOARD" },
 { 0x0308, "WM_DRAWCLIPBOARD" },
 { 0x0309, "WM_PAINTCLIPBOARD" },
 { 0x030A, "WM_VSCROLLCLIPBOARD" },
 { 0x030B, "WM_SIZECLIPBOARD" },
 { 0x030C, "WM_ASKCBFORMATNAME" },
 { 0x030D, "WM_CHANGECBCHAIN" },
 { 0x030E, "WM_HSCROLLCLIPBOARD" },
 { 0x030F, "WM_QUERYNEWPALETTE" },
 { 0x0310, "WM_PALETTEISCHANGING" },
 { 0x0311, "WM_PALETTECHANGED" },
 { 0x0312, "WM_HOTKEY" },
 { 0x0317, "WM_PRINT" },
 { 0x0318, "WM_PRINTCLIENT" },
 { 0x0319, "WM_APPCOMMAND" },
 { 0x031A, "WM_THEMECHANGED" },
 { 0x0358, "WM_HANDHELDFIRST" },
 { 0x0359, "WM_HANDHELDFIRST + 1" },
 { 0x035A, "WM_HANDHELDFIRST + 2" },
 { 0x035B, "WM_HANDHELDFIRST + 3" },
 { 0x035C, "WM_HANDHELDFIRST + 4" },
 { 0x035D, "WM_HANDHELDFIRST + 5" },
 { 0x035E, "WM_HANDHELDFIRST + 6" },
 { 0x035F, "WM_HANDHELDLAST" },
 { 0x0360, "WM_AFXFIRST" },
 { 0x0361, "WM_AFXFIRST + 1" },
 { 0x0362, "WM_AFXFIRST + 2" },
 { 0x0363, "WM_AFXFIRST + 3" },
 { 0x0364, "WM_AFXFIRST + 4" },
 { 0x0365, "WM_AFXFIRST + 5" },
 { 0x0366, "WM_AFXFIRST + 6" },
 { 0x0367, "WM_AFXFIRST + 7" },
 { 0x0368, "WM_AFXFIRST + 8" },
 { 0x0369, "WM_AFXFIRST + 9" },
 { 0x036A, "WM_AFXFIRST + 10" },
 { 0x036B, "WM_AFXFIRST + 11" },
 { 0x036C, "WM_AFXFIRST + 12" },
 { 0x036D, "WM_AFXFIRST + 13" },
 { 0x036E, "WM_AFXFIRST + 14" },
 { 0x036F, "WM_AFXFIRST + 15" },
 { 0x0370, "WM_AFXFIRST + 16" },
 { 0x0371, "WM_AFXFIRST + 17" },
 { 0x0372, "WM_AFXFIRST + 18" },
 { 0x0373, "WM_AFXFIRST + 19" },
 { 0x0374, "WM_AFXFIRST + 20" },
 { 0x0375, "WM_AFXFIRST + 21" },
 { 0x0376, "WM_AFXFIRST + 22" },
 { 0x0377, "WM_AFXFIRST + 23" },
 { 0x0378, "WM_AFXFIRST + 24" },
 { 0x0379, "WM_AFXFIRST + 25" },
 { 0x037A, "WM_AFXFIRST + 26" },
 { 0x037B, "WM_AFXFIRST + 27" },
 { 0x037C, "WM_AFXFIRST + 28" },
 { 0x037D, "WM_AFXFIRST + 29" },
 { 0x037E, "WM_AFXFIRST + 30" },
 { 0x037F, "WM_AFXLAST" },
 { 0x0380, "WM_PENWINFIRST" },
 { 0x0381, "WM_PENWINFIRST + 1" },
 { 0x0382, "WM_PENWINFIRST + 2" },
 { 0x0383, "WM_PENWINFIRST + 3" },
 { 0x0384, "WM_PENWINFIRST + 4" },
 { 0x0385, "WM_PENWINFIRST + 5" },
 { 0x0386, "WM_PENWINFIRST + 6" },
 { 0x0387, "WM_PENWINFIRST + 7" },
 { 0x0388, "WM_PENWINFIRST + 8" },
 { 0x0389, "WM_PENWINFIRST + 9" },
 { 0x038A, "WM_PENWINFIRST + 10" },
 { 0x038B, "WM_PENWINFIRST + 11" },
 { 0x038C, "WM_PENWINFIRST + 12" },
 { 0x038D, "WM_PENWINFIRST + 13" },
 { 0x038E, "WM_PENWINFIRST + 14" },
 { 0x038F, "WM_PENWINLAST" },
 { 0x0400, "WM_USER" },
 { 0x8000, "WM_APP" }
 };

    return findWinMessageMapping(knownWM, sizeof(knownWM) / sizeof(knownWM[0]), msg);
}

static const char *activateParameter(uint p)
{
    static const QWinMessageMapping<uint> activeEnum[] = {
        {WA_ACTIVE, "Activate"}, {WA_INACTIVE, "Deactivate"},
        {WA_CLICKACTIVE, "Activate by mouseclick"}
    };

    return findWinMessageMapping(activeEnum, sizeof(activeEnum) / sizeof(activeEnum[0]), p);
}

static QString styleFlags(uint style)
{
    static const QWinMessageMapping<uint> styleFlags[] = {
        FLAG_ENTRY(WS_BORDER), FLAG_ENTRY(WS_CAPTION), FLAG_ENTRY(WS_CHILD),
        FLAG_ENTRY(WS_CLIPCHILDREN), FLAG_ENTRY(WS_CLIPSIBLINGS),
        FLAG_ENTRY(WS_DISABLED), FLAG_ENTRY(WS_DLGFRAME), FLAG_ENTRY(WS_GROUP),
        FLAG_ENTRY(WS_HSCROLL), FLAG_ENTRY(WS_OVERLAPPED),
        FLAG_ENTRY(WS_OVERLAPPEDWINDOW), FLAG_ENTRY(WS_ICONIC),
        FLAG_ENTRY(WS_MAXIMIZE), FLAG_ENTRY(WS_MAXIMIZEBOX),
        FLAG_ENTRY(WS_MINIMIZE), FLAG_ENTRY(WS_MINIMIZEBOX),
        FLAG_ENTRY(WS_OVERLAPPEDWINDOW), FLAG_ENTRY(WS_POPUP),
        FLAG_ENTRY(WS_POPUPWINDOW), FLAG_ENTRY(WS_SIZEBOX),
        FLAG_ENTRY(WS_SYSMENU), FLAG_ENTRY(WS_TABSTOP), FLAG_ENTRY(WS_THICKFRAME),
        FLAG_ENTRY(WS_TILED), FLAG_ENTRY(WS_TILEDWINDOW), FLAG_ENTRY(WS_VISIBLE),
        FLAG_ENTRY(WS_VSCROLL)
    };

    return flagsValue(styleFlags, sizeof(styleFlags) / sizeof(styleFlags[0]), style);
}

static QString exStyleFlags(uint exStyle)
{
    static const QWinMessageMapping<uint> exStyleFlags[] = {
        FLAG_ENTRY(WS_EX_ACCEPTFILES), FLAG_ENTRY(WS_EX_APPWINDOW),
        FLAG_ENTRY(WS_EX_CLIENTEDGE), FLAG_ENTRY(WS_EX_DLGMODALFRAME),
        FLAG_ENTRY(WS_EX_LEFT), FLAG_ENTRY(WS_EX_LEFTSCROLLBAR),
        FLAG_ENTRY(WS_EX_LTRREADING), FLAG_ENTRY(WS_EX_MDICHILD),
        FLAG_ENTRY(WS_EX_NOACTIVATE), FLAG_ENTRY(WS_EX_NOPARENTNOTIFY),
        FLAG_ENTRY(WS_EX_OVERLAPPEDWINDOW), FLAG_ENTRY(WS_EX_PALETTEWINDOW),
        FLAG_ENTRY(WS_EX_RIGHT), FLAG_ENTRY(WS_EX_RIGHTSCROLLBAR),
        FLAG_ENTRY(WS_EX_RTLREADING), FLAG_ENTRY(WS_EX_STATICEDGE),
        FLAG_ENTRY(WS_EX_TOOLWINDOW), FLAG_ENTRY(WS_EX_TOPMOST),
        FLAG_ENTRY(WS_EX_TRANSPARENT), FLAG_ENTRY(WS_EX_WINDOWEDGE)
    };

    return flagsValue(exStyleFlags, sizeof(exStyleFlags) / sizeof(exStyleFlags[0]), exStyle);
}

static const char *imeCommand(uint cmd)
{
     static const QWinMessageMapping<uint> commands[] = {
         FLAG_ENTRY(IMN_CHANGECANDIDATE), FLAG_ENTRY(IMN_CLOSECANDIDATE),
         FLAG_ENTRY(IMN_CLOSESTATUSWINDOW), FLAG_ENTRY(IMN_GUIDELINE),
         FLAG_ENTRY(IMN_OPENCANDIDATE), FLAG_ENTRY(IMN_OPENSTATUSWINDOW),
         FLAG_ENTRY(IMN_SETCANDIDATEPOS), FLAG_ENTRY(IMN_SETCOMPOSITIONFONT),
         FLAG_ENTRY(IMN_SETCOMPOSITIONWINDOW), FLAG_ENTRY(IMN_SETCONVERSIONMODE),
         FLAG_ENTRY(IMN_SETOPENSTATUS), FLAG_ENTRY(IMN_SETSENTENCEMODE),
         FLAG_ENTRY(IMN_SETSTATUSWINDOWPOS)
     };

     return findWinMessageMapping(commands, sizeof(commands) / sizeof(commands[0]), cmd);
}

static QString imeShowFlags(uint flags)
{
    static const QWinMessageMapping<uint> showFlags[] = {
        FLAG_ENTRY(ISC_SHOWUICOMPOSITIONWINDOW),
        FLAG_ENTRY(ISC_SHOWUICANDIDATEWINDOW),
        FLAG_ENTRY(ISC_SHOWUICANDIDATEWINDOW << 1),
        FLAG_ENTRY(ISC_SHOWUICANDIDATEWINDOW << 2),
        FLAG_ENTRY(ISC_SHOWUICANDIDATEWINDOW << 3)
    };

    return flagsValue(showFlags, sizeof(showFlags) / sizeof(showFlags[0]), flags);
}

static const char *wmSizeParam(uint p)
{
    static const QWinMessageMapping<uint> sizeParams[] = {
        FLAG_ENTRY(SIZE_MAXHIDE), FLAG_ENTRY(SIZE_MAXIMIZED),
        FLAG_ENTRY(SIZE_MAXSHOW), FLAG_ENTRY(SIZE_MINIMIZED),
        FLAG_ENTRY(SIZE_RESTORED)
    };

    return findWinMessageMapping(sizeParams, sizeof(sizeParams) / sizeof(sizeParams[0]), p);
}

static QString virtualKeys(uint vk)
{
    static const QWinMessageMapping<uint> keys[] = {
        FLAG_ENTRY(MK_CONTROL), FLAG_ENTRY(MK_LBUTTON), FLAG_ENTRY(MK_MBUTTON),
        FLAG_ENTRY(MK_RBUTTON), FLAG_ENTRY(MK_SHIFT), FLAG_ENTRY(MK_XBUTTON1),
        FLAG_ENTRY(MK_XBUTTON2)
    };

    return flagsValue(keys, sizeof(keys) / sizeof(keys[0]), vk);
}

static QString winPosFlags(uint f)
{
     static const QWinMessageMapping<uint> winPosValues[] = {
         FLAG_ENTRY(SWP_DRAWFRAME), FLAG_ENTRY(SWP_FRAMECHANGED),
         FLAG_ENTRY(SWP_HIDEWINDOW), FLAG_ENTRY(SWP_NOACTIVATE),
         FLAG_ENTRY(SWP_NOCOPYBITS), FLAG_ENTRY(SWP_NOMOVE),
         FLAG_ENTRY(SWP_NOOWNERZORDER), FLAG_ENTRY(SWP_NOREDRAW),
         FLAG_ENTRY(SWP_NOREPOSITION), FLAG_ENTRY(SWP_NOSENDCHANGING),
         FLAG_ENTRY(SWP_NOSIZE), FLAG_ENTRY(SWP_NOZORDER),
         FLAG_ENTRY(SWP_SHOWWINDOW)
     };

     return flagsValue(winPosValues, sizeof(winPosValues) / sizeof(winPosValues[0]), f);
}

static const char *winPosInsertAfter(quintptr h)
{
    static const QWinMessageMapping<quintptr> insertAfterValues[] = {
        {quintptr(HWND_BOTTOM),    "HWND_BOTTOM"},
        {quintptr(HWND_NOTOPMOST), "HWND_NOTOPMOST"},
        {quintptr(HWND_TOP),       "HWND_TOP"},
        {quintptr(HWND_TOPMOST),   "HWND_TOPMOST"}
    };
    return findWinMessageMapping(insertAfterValues, sizeof(insertAfterValues) / sizeof(insertAfterValues[0]), h);
}

static const char *sessionMgrLogOffOption(uint p)
{
#ifndef ENDSESSION_CLOSEAPP
#define ENDSESSION_CLOSEAPP 0x00000001
#endif
#ifndef ENDSESSION_CRITICAL
#define ENDSESSION_CRITICAL 0x40000000
#endif
    static const QWinMessageMapping<uint> values[] = {
        {ENDSESSION_CLOSEAPP, "Close application"},
        {ENDSESSION_CRITICAL, "Force application end"},
        {ENDSESSION_LOGOFF,   "User logoff"}
    };

    return findWinMessageMapping(values, sizeof(values) / sizeof(values[0]), p);
}

// Returns a "human readable" string representation of the MSG and the
// information it points to
QString decodeMSG(const MSG& msg)
{
    const WPARAM wParam = msg.wParam;
    const LPARAM lParam = msg.lParam;

    QString message;
    // Custom WM_'s
    if (msg.message > WM_APP)
        message= QString::fromLatin1("WM_APP + %1").arg(msg.message - WM_APP);
    else if (msg.message > WM_USER)
        message = QString::fromLatin1("WM_USER + %1").arg(msg.message - WM_USER);
    else if (const char *wmmsgC = findWMstr(msg.message))
        message = QString::fromLatin1(wmmsgC);
    else
        message = QString::fromLatin1("WM_(0x%1)").arg(msg.message, 0, 16); // Unknown WM_, so use number

    // Yes, we want to give the WM_ names 20 chars of space before showing the
    // decoded message, since some of the common messages are quite long, and
    // we don't want the decoded information to vary in output position
    if (message.size() < 20)
        message.prepend(QString(20 - message.size(), QLatin1Char(' ')));
    message += QLatin1String(": ");

    const QString hwndS = QString::asprintf("(%p)", reinterpret_cast<void *>(msg.hwnd));
    const QString wParamS = QString::asprintf("(%p)", reinterpret_cast<void *>(wParam));
    const QString lParamS = QString::asprintf("(%p)", reinterpret_cast<void *>(lParam));

    QString parameters;
    switch (msg.message) {
        case WM_ACTIVATE:
            if (const char *a = activateParameter(uint(wParam)))
                parameters += QLatin1String(a);
            parameters += QLatin1String(" Hwnd ") + hwndS;
            break;
        case WM_CAPTURECHANGED:
            parameters = QLatin1String("Hwnd gaining capture ") + hwndS;
            break;
        case WM_CREATE:
            {
                auto lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
                QString className;
                if (lpcs->lpszClass != nullptr) {
                    className = HIWORD(lpcs->lpszClass) == 0
                        ? QString::number(LOWORD(lpcs->lpszClass), 16) // Atom
                        : QString::fromWCharArray(lpcs->lpszClass);
                }

                const QString windowName = lpcs->lpszName
                    ? QString::fromWCharArray(lpcs->lpszName) : QString();

                parameters = QString::asprintf("x,y(%4d,%4d) w,h(%4d,%4d) className(%s) windowName(%s) parent(0x%p) style(%s) exStyle(%s)",
                                               lpcs->x, lpcs->y, lpcs->cx, lpcs->cy,
                                               className.toLatin1().constData(),
                                               windowName.toLatin1().constData(),
                                               reinterpret_cast<void *>(lpcs->hwndParent),
                                               styleFlags(uint(lpcs->style)).toLatin1().constData(),
                                               exStyleFlags(lpcs->dwExStyle).toLatin1().constData());
            }
            break;
        case WM_DESTROY:
            parameters = QLatin1String("Destroy hwnd ") + hwndS;
            break;
        case 0x02E0u: { // WM_DPICHANGED
            auto rect = reinterpret_cast<const RECT *>(lParam);
            QTextStream(&parameters) << "DPI: " << HIWORD(wParam) << ','
                << LOWORD(wParam) << ' ' << (rect->right - rect->left) << 'x'
                << (rect->bottom - rect->top) << Qt::forcesign << rect->left << rect->top;
            }
            break;
        case WM_IME_NOTIFY:
            {
                parameters = QLatin1String("Command(");
                if (const char *c = imeCommand(uint(wParam)))
                    parameters += QLatin1String(c);
                parameters += QLatin1String(" : ") + lParamS;
            }
            break;
        case WM_IME_SETCONTEXT:
            parameters = QLatin1String("Input context(")
                         + QLatin1String(wParam == TRUE ? "Active" : "Inactive")
                         + QLatin1String(") Show flags(")
                         + imeShowFlags(DWORD(lParam)) + QLatin1Char(')');
            break;
        case WM_KILLFOCUS:
            parameters = QLatin1String("Hwnd gaining keyboard focus ") + wParamS;
            break;
        case WM_CHAR:
        case WM_IME_CHAR:
        case WM_KEYDOWN:
        case WM_KEYUP:
            {
                const int nVirtKey  = int(wParam);
                const long lKeyData = long(lParam);
                int repCount     = (lKeyData & 0xffff);        // Bit 0-15
                int scanCode     = (lKeyData & 0xf0000) >> 16; // Bit 16-23
                bool contextCode = !!(lKeyData & 0x20000000);  // Bit 29
                bool prevState   = !!(lKeyData & 0x40000000);  // Bit 30
                bool transState  = !!(lKeyData & 0x80000000);  // Bit 31
                parameters = QString::asprintf("Virtual-key(0x%x) Scancode(%d) Rep(%d) Contextcode(%d), Prev state(%d), Trans state(%d)",
                                               nVirtKey, scanCode, repCount,
                                               contextCode, prevState, transState);
            }
            break;
        case WM_INPUTLANGCHANGE:
            parameters = QStringLiteral("Keyboard layout changed");
            break;
        case WM_NCACTIVATE:
            parameters = (msg.wParam? QLatin1String("Active Titlebar") : QLatin1String("Inactive Titlebar"));
            break;
        case WM_MOUSEACTIVATE:
            {
                const char *mouseMsg = findWMstr(HIWORD(lParam));
                parameters = QString::asprintf("TLW(0x%p) HittestCode(0x%x) MouseMsg(%s)",
                                               reinterpret_cast<void *>(wParam),
                                               LOWORD(lParam), mouseMsg ? mouseMsg : "");
            }
            break;
        case WM_MOUSELEAVE:
            break; // wParam & lParam not used
        case WM_MOUSEHOVER:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
            parameters = QString::asprintf("x,y(%4d,%4d) Virtual Keys(",
                                           GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))
                         + virtualKeys(uint(wParam)) + QLatin1Char(')');
            break;
        case WM_MOVE:
            parameters = QString::asprintf("x,y(%4d,%4d)", LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_ERASEBKGND:
        case WM_PAINT:
            parameters = QLatin1String("hdc") + wParamS;
            break;
        case WM_QUERYNEWPALETTE:
            break; // lParam & wParam are unused
        case WM_SETCURSOR:
            parameters = QString::asprintf("HitTestCode(0x%x) MouseMsg(", LOWORD(lParam));
            if (const char *mouseMsg = findWMstr(HIWORD(lParam)))
                parameters += QLatin1String(mouseMsg);
            parameters += QLatin1Char(')');
            break;
        case WM_SETFOCUS:
            parameters = QLatin1String("Lost Focus ") + wParamS;
            break;
        case WM_SETTEXT:
            parameters = QLatin1String("Set Text (")
                         + QString::fromWCharArray(reinterpret_cast<const wchar_t *>(lParam))
                         + QLatin1Char(')');
            break;
        case WM_SIZE:
            parameters = QString::asprintf("w,h(%4d,%4d) showmode(",
                                           LOWORD(lParam), HIWORD(lParam));
            if (const char *showMode = wmSizeParam(uint(wParam)))
                parameters += QLatin1String(showMode);
            parameters += QLatin1Char(')');
            break;
        case WM_WINDOWPOSCHANGED:
            {
                auto winPos = reinterpret_cast<LPWINDOWPOS>(lParam);
                if (!winPos)
                    break;
                const auto insertAfter = quintptr(winPos->hwndInsertAfter);
                parameters = QString::asprintf("x,y(%4d,%4d) w,h(%4d,%4d) flags(%s) hwndAfter(",
                                               winPos->x, winPos->y, winPos->cx, winPos->cy,
                                               winPosFlags(winPos->flags).toLatin1().constData());
                if (const char *h = winPosInsertAfter(insertAfter))
                    parameters += QLatin1String(h);
                else
                    parameters += QString::number(insertAfter, 16);
                parameters += QLatin1Char(')');
            }
            break;
        case WM_QUERYENDSESSION:
            parameters = QLatin1String("End session: ");
            if (const char *logoffOption = sessionMgrLogOffOption(uint(wParam)))
                parameters += QLatin1String(logoffOption);
            break;
        default:
            parameters = QLatin1String("wParam") + wParamS + QLatin1String(" lParam") + lParamS;
            break;
    }

    return message + QLatin1String("hwnd") + hwndS + QLatin1Char(' ') + parameters;
}

QDebug operator<<(QDebug dbg, const MSG &msg)
{
    QDebugStateSaver saver(dbg);
    dbg.noquote();
    dbg.nospace();
    dbg << decodeMSG(msg);
    return dbg;
}
#endif

#endif // QT_NO_QOBJECT

#endif // !defined(Q_OS_WINRT)

#ifndef QT_NO_QOBJECT
void QCoreApplicationPrivate::removePostedTimerEvent(QObject *object, int timerId)
{
    QThreadData *data = object->d_func()->threadData.loadRelaxed();

    const auto locker = qt_scoped_lock(data->postEventList.mutex);
    if (data->postEventList.size() == 0)
        return;
    for (int i = 0; i < data->postEventList.size(); ++i) {
        const QPostEvent &pe = data->postEventList.at(i);
        if (pe.receiver == object
                && pe.event
                && (pe.event->type() == QEvent::Timer || pe.event->type() == QEvent::ZeroTimerEvent)
                && static_cast<QTimerEvent *>(pe.event)->timerId() == timerId) {
            --pe.receiver->d_func()->postedEvents;
            pe.event->posted = false;
            delete pe.event;
            const_cast<QPostEvent &>(pe).event = 0;
            return;
        }
    }
}
#endif // QT_NO_QOBJECT

QT_END_NAMESPACE
