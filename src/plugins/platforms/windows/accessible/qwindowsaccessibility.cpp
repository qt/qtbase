/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QtConfig>
#ifndef QT_NO_ACCESSIBILITY


#include <private/qsystemlibrary_p.h>

#include <QtCore/qlocale.h>
#include <QtCore/qmap.h>
#include <QtCore/qpair.h>
#include <QtCore/qpointer.h>
#include <QtCore/qsettings.h>
#include <QtGui/qaccessible.h>
#include <QtGui/qaccessible2.h>
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>

#include "qwindowsaccessibility.h"
#ifdef Q_CC_MINGW
# include "qwindowsmsaaaccessible.h"
#else
# include "iaccessible2.h"
#endif
#include "comutils.h"

#include <oleacc.h>

//#include <uiautomationcoreapi.h>
#ifndef UiaRootObjectId
#define UiaRootObjectId        -25
#endif

#include <winuser.h>
#if !defined(WINABLEAPI)
#  if defined(Q_OS_WINCE)
#    include <bldver.h>
#  endif
#  include <winable.h>
#endif

#include <servprov.h>
#if !defined(Q_CC_BOR) && !defined (Q_CC_GNU)
#include <comdef.h>
#endif

#include "../qtwindows_additional.h"


// This stuff is used for widgets/items with no window handle:
typedef QMap<int, QPair<QPointer<QObject>,int> > NotifyMap;
Q_GLOBAL_STATIC(NotifyMap, qAccessibleRecentSentEvents)

QT_BEGIN_NAMESPACE


/*!
    \!internal
    \class QWindowsAccessibility

    Implements QPlatformAccessibility

*/
QWindowsAccessibility::QWindowsAccessibility()
{
}

void QWindowsAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    QString soundName;
    switch (event->type()) {
    case QAccessible::PopupMenuStart:
        soundName = QLatin1String("MenuPopup");
        break;

    case QAccessible::MenuCommand:
        soundName = QLatin1String("MenuCommand");
        break;

    case QAccessible::Alert:
        {
        /*      ### FIXME
#ifndef QT_NO_MESSAGEBOX
            QMessageBox *mb = qobject_cast<QMessageBox*>(o);
            if (mb) {
                switch (mb->icon()) {
                case QMessageBox::Warning:
                    soundName = QLatin1String("SystemExclamation");
                    break;
                case QMessageBox::Critical:
                    soundName = QLatin1String("SystemHand");
                    break;
                case QMessageBox::Information:
                    soundName = QLatin1String("SystemAsterisk");
                    break;
                default:
                    break;
                }
            } else
#endif // QT_NO_MESSAGEBOX
*/
            {
                soundName = QLatin1String("SystemAsterisk");
            }

        }
        break;
    default:
        break;
    }

    if (!soundName.isEmpty()) {
#ifndef QT_NO_SETTINGS
        QSettings settings(QLatin1String("HKEY_CURRENT_USER\\AppEvents\\Schemes\\Apps\\.Default\\") + soundName,
                           QSettings::NativeFormat);
        QString file = settings.value(QLatin1String(".Current/.")).toString();
#else
        QString file;
#endif
        if (!file.isEmpty()) {
            PlaySound(reinterpret_cast<const wchar_t *>(soundName.utf16()), 0, SND_ALIAS | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
        }
    }

#if defined(Q_OS_WINCE) // ### TODO: check for NotifyWinEvent in CE 6.0
    // There is no user32.lib nor NotifyWinEvent for CE
    return;
#else
    // An event has to be associated with a window,
    // so find the first parent that is a widget and that has a WId
    QAccessibleInterface *iface = event->accessibleInterface();
    QWindow *window = iface ? QWindowsAccessibility::windowHelper(iface) : 0;
    delete iface;

    if (!window) {
        window = QGuiApplication::focusWindow();
        if (!window)
            return;
    }

    QPlatformNativeInterface *platform = QGuiApplication::platformNativeInterface();
    if (!window->handle()) // Called before show(), no native window yet.
        return;
    HWND hWnd = (HWND)platform->nativeResourceForWindow("handle", window);

    static int eventNum = 0;
    if (event->type() != QAccessible::MenuCommand && // MenuCommand is faked
        event->type() != QAccessible::ObjectDestroyed) {
        /* In some rare occasions, the server (Qt) might get a ::get_accChild call with a
           childId that references an entry in the cache where there was a dangling
           QObject-pointer. Previously we crashed on this.

           There is no point in actually notifying the AT client that the object got destroyed,
           because the AT client won't query for get_accChild if the event is ObjectDestroyed
           anyway, and we have no other way of mapping the eventId argument to the actual
           child/descendant object. (Firefox seems to simply completely ignore
           EVENT_OBJECT_DESTROY).

           We therefore guard each QObject in the cache with a QPointer, and only notify the AT
           client if the type is not ObjectDestroyed.
        */
        eventNum %= 50;              //[0..49]
        int eventId = - (eventNum - 1);
        qAccessibleRecentSentEvents()->insert(eventId, qMakePair(QPointer<QObject>(event->object()), event->child()));
        ::NotifyWinEvent(event->type(), hWnd, OBJID_CLIENT, eventId);
        ++eventNum;
    }
#endif // Q_OS_WINCE
}

QWindow *QWindowsAccessibility::windowHelper(const QAccessibleInterface *iface)
{
    QWindow *window = iface->window();
    if (!window) {
        QAccessibleInterface *acc = iface->parent();
        while (acc && !window) {
            window = acc->window();
            QAccessibleInterface *par = acc->parent();
            delete acc;
            acc = par;
        }
    }
    return window;
}

/*!
  \internal
  helper to wrap a QAccessibleInterface inside a IAccessible*
*/
IAccessible *QWindowsAccessibility::wrap(QAccessibleInterface *acc)
{
    if (!acc)
        return 0;
#ifdef Q_CC_MINGW
    QWindowsMsaaAccessible *wacc = new QWindowsMsaaAccessible(acc);
#else
    QWindowsIA2Accessible *wacc = new QWindowsIA2Accessible(acc);
#endif
    IAccessible *iacc = 0;
    wacc->QueryInterface(IID_IAccessible, (void**)&iacc);
    return iacc;
}

/*!
  \internal
*/
QPair<QObject*, int> QWindowsAccessibility::getCachedObject(int entryId)
{
    QPair<QPointer<QObject>, int> pair = qAccessibleRecentSentEvents()->value(entryId);
    return qMakePair(pair.first.data(), pair.second);
}

/*
void QWindowsAccessibility::setRootObject(QObject *o)
{

}

void QWindowsAccessibility::initialize()
{

}

void QWindowsAccessibility::cleanup()
{

}

*/

bool QWindowsAccessibility::handleAccessibleObjectFromWindowRequest(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId)) {
        /* For UI Automation */
    } else if ((DWORD)lParam == DWORD(OBJID_CLIENT)) {
#if 1
        // Ignoring all requests while starting up
        // ### Maybe QPA takes care of this???
        if (QCoreApplication::startingUp() || QCoreApplication::closingDown())
            return false;
#endif

        typedef LRESULT (WINAPI *PtrLresultFromObject)(REFIID, WPARAM, LPUNKNOWN);
        static PtrLresultFromObject ptrLresultFromObject = 0;
        static bool oleaccChecked = false;

        if (!oleaccChecked) {
            oleaccChecked = true;
#if !defined(Q_OS_WINCE)
            ptrLresultFromObject = (PtrLresultFromObject)QSystemLibrary::resolve(QLatin1String("oleacc"), "LresultFromObject");
#endif
        }

        if (ptrLresultFromObject) {
            QWindow *window = QWindowsContext::instance()->findWindow(hwnd);
            if (window) {
                QAccessibleInterface *acc = window->accessibleRoot();
                if (acc) {
                    if (IAccessible *iface = wrap(acc)) {
                        *lResult = ptrLresultFromObject(IID_IAccessible, wParam, iface);  // ref == 2
                        if (*lResult) {
                            iface->Release(); // the client will release the object again, and then it will destroy itself
                        }
                        return true;
                    } else {
                        delete acc;
                    }
                }
            }
        }
    }
    return false;
}

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
