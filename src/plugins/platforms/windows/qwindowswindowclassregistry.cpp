// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwindowswindowclassregistry.h"

#include <QtCore/qlibraryinfo.h>
#include <QtCore/quuid.h>

#include "qwindowscontext.h"
#include "qwindowswindowclassdescription.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQpaWindowClass, "qt.qpa.windowclass")

QWindowsWindowClassRegistry *QWindowsWindowClassRegistry::m_instance = nullptr;

QWindowsWindowClassRegistry::QWindowsWindowClassRegistry(WNDPROC defaultProcedure)
    : m_defaultProcedure(defaultProcedure)
{
    m_instance = this;
}

QWindowsWindowClassRegistry::~QWindowsWindowClassRegistry()
{
    unregisterWindowClasses();

    m_instance = nullptr;
}

QWindowsWindowClassRegistry *QWindowsWindowClassRegistry::instance()
{
    return m_instance;
}

QString QWindowsWindowClassRegistry::classNamePrefix()
{
    static QString result;
    if (result.isEmpty()) {
        QTextStream str(&result);
        str << "Qt" << QT_VERSION_MAJOR << QT_VERSION_MINOR << QT_VERSION_PATCH;
        if (QLibraryInfo::isDebugBuild())
            str << 'd';
#ifdef QT_NAMESPACE
#  define xstr(s) str(s)
#  define str(s) #s
        str << xstr(QT_NAMESPACE);
#  undef str
#  undef xstr
#endif
    }
    return result;
}

QString QWindowsWindowClassRegistry::registerWindowClass(const QWindowsWindowClassDescription &description)
{
    QString className = description.name;

    if (description.shouldAddPrefix)
        className = classNamePrefix() + className;

    // since multiple Qt versions can be used in one process
    // each one has to have window class names with a unique name
    // The first instance gets the unmodified name; if the class
    // has already been registered by another instance of Qt then
    // add a UUID. The check needs to be performed for each name
    // in case new message windows are added (QTBUG-81347).
    // Note: GetClassInfo() returns != 0 when a class exists.
    if (shouldDecorateWindowClassName(className, description.procedure))
        className += QUuid::createUuid().toString();

    if (m_registeredWindowClasses.contains(className))        // already registered in our list
        return className;

    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = description.style;
    wc.lpfnWndProc = description.procedure;
    wc.hInstance = appInstance;
    wc.hbrBackground = description.brush;
    WindowClassIcons icons;
    if (description.hasIcon) {
        // Icons loaded from the application resources without LR_SHARED are owned by
        // the caller and have to be destroyed with DestroyIcon() once the class is
        // unregistered. The fallback is a shared system icon, which must not be destroyed.
        icons.icon = static_cast<HICON>(LoadImage(appInstance, L"IDI_ICON1", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        if (icons.icon) {
            int sw = GetSystemMetrics(SM_CXSMICON);
            int sh = GetSystemMetrics(SM_CYSMICON);
            icons.iconSmall = static_cast<HICON>(LoadImage(appInstance, L"IDI_ICON1", IMAGE_ICON, sw, sh, 0));
            wc.hIcon = icons.icon;
            wc.hIconSm = icons.iconSmall;
        } else {
            wc.hIcon = static_cast<HICON>(LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
        }
    }

    wc.lpszClassName = reinterpret_cast<LPCWSTR>(className.utf16());
    ATOM atom = RegisterClassEx(&wc);
    if (!atom) {
        qCWarning(lcQpaWindowClass) << "Failed to register window class" << className
            << "(" << qt_error_string(-1) << ")";
        if (icons.icon)
            DestroyIcon(icons.icon);
        if (icons.iconSmall)
            DestroyIcon(icons.iconSmall);
        icons = {};
    }

    m_registeredWindowClasses.insert(className, icons);

    qCDebug(lcQpaWindowClass).nospace() << __FUNCTION__ << ' ' << className << ' ' << description << " atom=" << atom;

    return className;
}

QString QWindowsWindowClassRegistry::registerWindowClass(const QWindow *window)
{
    return registerWindowClass(QWindowsWindowClassDescription::fromWindow(window, m_defaultProcedure));
}

QString QWindowsWindowClassRegistry::registerWindowClass(QString name, WNDPROC procedure)
{
    return registerWindowClass(QWindowsWindowClassDescription::fromName(name, procedure));
}

bool QWindowsWindowClassRegistry::shouldDecorateWindowClassName(const QString &name, WNDPROC procedure) const
{
    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));

    WNDCLASS wc{};

    return GetClassInfo(appInstance, reinterpret_cast<LPCWSTR>(name.utf16()), &wc) != FALSE
        && wc.lpfnWndProc != procedure;
}

void QWindowsWindowClassRegistry::unregisterWindowClasses()
{
    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));

    for (auto it = m_registeredWindowClasses.cbegin(), end = m_registeredWindowClasses.cend(); it != end; ++it) {
        if (!UnregisterClass(reinterpret_cast<LPCWSTR>(it.key().utf16()), appInstance)) {
            if (QWindowsContext::verbose)
                qCWarning(lcQpaWindowClass) << "Failed to unregister window class" << it.key()
                    << "(" << qt_error_string(-1) << ")";
            continue; // the class is still in use and keeps referencing its icons
        }
        if (it->icon)
            DestroyIcon(it->icon);
        if (it->iconSmall)
            DestroyIcon(it->iconSmall);
    }
    m_registeredWindowClasses.clear();
}

QT_END_NAMESPACE
