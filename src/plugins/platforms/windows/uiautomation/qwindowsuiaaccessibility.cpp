// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiaaccessibility.h"
#include "qwindowsuiautomation.h"
#include "qwindowsuiamainprovider.h"
#include "qwindowsuiautils.h"

#include <QtGui/qaccessible.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtCore/qt_windows.h>
#include <qpa/qplatformintegration.h>

#include <QtCore/private/qwinregistry_p.h>

QT_BEGIN_NAMESPACE

using namespace QWindowsUiAutomation;
using namespace Qt::Literals::StringLiterals;

bool QWindowsUiaAccessibility::m_accessibleActive = false;

QWindowsUiaAccessibility::QWindowsUiaAccessibility()
{
}

QWindowsUiaAccessibility::~QWindowsUiaAccessibility()
{
}

// Handles UI Automation window messages.
bool QWindowsUiaAccessibility::handleWmGetObject(HWND hwnd, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    // Start handling accessibility internally
    QGuiApplicationPrivate::platformIntegration()->accessibility()->setActive(true);
    m_accessibleActive = true;

    // Ignoring all requests while starting up / shutting down
    if (QCoreApplication::startingUp() || QCoreApplication::closingDown())
        return false;

    if (QWindow *window = QWindowsContext::instance()->findWindow(hwnd)) {
        if (QAccessibleInterface *accessible = window->accessibleRoot()) {
            auto provider = QWindowsUiaMainProvider::providerForAccessible(accessible);
            *lResult = UiaReturnRawElementProvider(hwnd, wParam, lParam, provider.Get());
            return true;
        }
    }
    return false;
}

// Retrieve sound name by checking the icon property of a message box
// should it be the event object.
static QString alertSound(const QObject *object)
{
    if (object->inherits("QMessageBox")) {
        enum MessageBoxIcon { // Keep in sync with QMessageBox::Icon
            Information = 1,
            Warning = 2,
            Critical = 3
        };
        switch (object->property("icon").toInt()) {
        case Information:
            return QStringLiteral("SystemAsterisk");
        case Warning:
            return QStringLiteral("SystemExclamation");
        case Critical:
            return QStringLiteral("SystemHand");
        }
        return QString();
    }
    return QStringLiteral("SystemAsterisk");
}

static QString soundFileName(const QString &soundName)
{
    const QString key = "AppEvents\\Schemes\\Apps\\.Default\\"_L1
        + soundName + "\\.Current"_L1;
    return QWinRegistryKey(HKEY_CURRENT_USER, key).stringValue(L"");
}

static void playSystemSound(const QString &soundName)
{
    if (!soundName.isEmpty() && !soundFileName(soundName).isEmpty()) {
        PlaySound(reinterpret_cast<const wchar_t *>(soundName.utf16()), nullptr,
                  SND_ALIAS | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT);
    }
}

// Handles accessibility update notifications.
void QWindowsUiaAccessibility::notifyAccessibilityUpdate(QAccessibleEvent *event)
{
    if (!event)
        return;

    // Always handle system sound events
    switch (event->type()) {
        case QAccessible::PopupMenuStart:
            playSystemSound(QStringLiteral("MenuPopup"));
            break;
        case QAccessible::MenuCommand:
            playSystemSound(QStringLiteral("MenuCommand"));
            break;
        case QAccessible::Alert:
            playSystemSound(alertSound(event->object()));
            break;
        default:
            break;
    }

    // Ignore events sent before the first UI Automation
    // request or while QAccessible is being activated.
    if (!m_accessibleActive)
        return;

    QAccessibleInterface *accessible = event->accessibleInterface();
    if (!isActive() || !accessible || !accessible->isValid())
        return;

    // No need to do anything when nobody is listening.
    if (!UiaClientsAreListening())
        return;

    switch (event->type()) {
    case QAccessible::Announcement:
        QWindowsUiaMainProvider::raiseNotification(static_cast<QAccessibleAnnouncementEvent *>(event));
        break;
    case QAccessible::Focus:
        QWindowsUiaMainProvider::notifyFocusChange(event);
        break;
    case QAccessible::StateChanged:
        QWindowsUiaMainProvider::notifyStateChange(static_cast<QAccessibleStateChangeEvent *>(event));
        break;
    case QAccessible::ValueChanged:
        QWindowsUiaMainProvider::notifyValueChange(static_cast<QAccessibleValueChangeEvent *>(event));
        break;
    case QAccessible::NameChanged:
        QWindowsUiaMainProvider::notifyNameChange(event);
        break;
    case QAccessible::SelectionAdd:
        QWindowsUiaMainProvider::notifySelectionChange(event);
        break;
    case QAccessible::TextAttributeChanged:
    case QAccessible::TextColumnChanged:
    case QAccessible::TextInserted:
    case QAccessible::TextRemoved:
    case QAccessible::TextUpdated:
    case QAccessible::TextSelectionChanged:
    case QAccessible::TextCaretMoved:
        QWindowsUiaMainProvider::notifyTextChange(event);
        break;
    default:
        break;
    }
}

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)
