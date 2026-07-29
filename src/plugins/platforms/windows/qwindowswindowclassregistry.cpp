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

    if (m_registeredWindowClassNames.contains(className))        // already registered in our list
        return className;

    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = description.style;
    wc.lpfnWndProc = description.procedure;
    wc.hInstance = appInstance;
    wc.hbrBackground = description.brush;
    if (description.hasIcon) {
        wc.hIcon = static_cast<HICON>(LoadImage(appInstance, L"IDI_ICON1", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        if (wc.hIcon) {
            int sw = GetSystemMetrics(SM_CXSMICON);
            int sh = GetSystemMetrics(SM_CYSMICON);
            wc.hIconSm = static_cast<HICON>(LoadImage(appInstance, L"IDI_ICON1", IMAGE_ICON, sw, sh, 0));
        }
        else {
            wc.hIcon = static_cast<HICON>(LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
        }
    }

    wc.lpszClassName = reinterpret_cast<LPCWSTR>(className.utf16());
    ATOM atom = RegisterClassEx(&wc);
    if (!atom)
        qCWarning(lcQpaWindowClass) << "Failed to register window class" << className
            << "(" << qt_error_string(-1) << ")";

    m_registeredWindowClassNames.insert(className);

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

    for (const QString &name : std::as_const(m_registeredWindowClassNames)) {
        if (!UnregisterClass(reinterpret_cast<LPCWSTR>(name.utf16()), appInstance) && QWindowsContext::verbose)
            qCWarning(lcQpaWindowClass) << "Failed to unregister window class" << name
                << "(" << qt_error_string(-1) << ")";
    }
    m_registeredWindowClassNames.clear();
}

QT_END_NAMESPACE
