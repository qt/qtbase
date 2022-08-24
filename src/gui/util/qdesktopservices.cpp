// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdesktopservices.h"

#ifndef QT_NO_DESKTOPSERVICES

#include <qdebug.h>

#include <qstandardpaths.h>
#include <qhash.h>
#include <qobject.h>
#include <qcoreapplication.h>
#include <private/qguiapplication_p.h>
#include <qurl.h>
#include <qmutex.h>
#include <qpa/qplatformservices.h>
#include <qpa/qplatformintegration.h>
#include <qdir.h>

#include <QtCore/private/qlocking_p.h>

QT_BEGIN_NAMESPACE

class QOpenUrlHandlerRegistry
{
public:
    QOpenUrlHandlerRegistry() = default;

    QRecursiveMutex mutex;

    struct Handler
    {
        QObject *receiver;
        QByteArray name;
    };
    typedef QHash<QString, Handler> HandlerHash;
    HandlerHash handlers;

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    QObject context;

    void handlerDestroyed(QObject *handler);
#endif

};

Q_GLOBAL_STATIC(QOpenUrlHandlerRegistry, handlerRegistry)

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
void QOpenUrlHandlerRegistry::handlerDestroyed(QObject *handler)
{
    const auto lock = qt_scoped_lock(mutex);
    HandlerHash::Iterator it = handlers.begin();
    while (it != handlers.end()) {
        if (it->receiver == handler) {
            it = handlers.erase(it);
            qWarning("Please call QDesktopServices::unsetUrlHandler() before destroying a "
                     "registered URL handler object.\n"
                     "Support for destroying a registered URL handler object is deprecated, "
                     "and will be removed in Qt 6.6.");
        } else {
            ++it;
        }
    }
}
#endif

/*!
    \class QDesktopServices
    \brief The QDesktopServices class provides methods for accessing common desktop services.
    \since 4.2
    \ingroup desktop
    \inmodule QtGui

    Many desktop environments provide services that can be used by applications to
    perform common tasks, such as opening a web page, in a way that is both consistent
    and takes into account the user's application preferences.

    This class contains functions that provide simple interfaces to these services
    that indicate whether they succeeded or failed.

    The openUrl() function is used to open files located at arbitrary URLs in external
    applications. For URLs that correspond to resources on the local filing system
    (where the URL scheme is "file"), a suitable application will be used to open the
    file; otherwise, a web browser will be used to fetch and display the file.

    The user's desktop settings control whether certain executable file types are
    opened for browsing, or if they are executed instead. Some desktop environments
    are configured to prevent users from executing files obtained from non-local URLs,
    or to ask the user's permission before doing so.

    \section1 URL Handlers

    The behavior of the openUrl() function can be customized for individual URL
    schemes to allow applications to override the default handling behavior for
    certain types of URLs.

    The dispatch mechanism allows only one custom handler to be used for each URL
    scheme; this is set using the setUrlHandler() function. Each handler is
    implemented as a slot which accepts only a single QUrl argument.

    The existing handlers for each scheme can be removed with the
    unsetUrlHandler() function. This returns the handling behavior for the given
    scheme to the default behavior.

    This system makes it easy to implement a help system, for example. Help could be
    provided in labels and text browsers using \uicontrol{help://myapplication/mytopic}
    URLs, and by registering a handler it becomes possible to display the help text
    inside the application:

    \snippet code/src_gui_util_qdesktopservices.cpp 0
    \snippet code/src_gui_util_qdesktopservices.cpp setUrlHandler

    If inside the handler you decide that you can't open the requested
    URL, you can just call QDesktopServices::openUrl() again with the
    same argument, and it will try to open the URL using the
    appropriate mechanism for the user's desktop environment.

    Combined with platform specific settings, the schemes registered by the
    openUrl() function can also be exposed to other applications, opening up
    for application deep linking or a very basic URL-based IPC mechanism.

    \sa QSystemTrayIcon, QProcess, QStandardPaths
*/

/*!
    Opens the given \a url in the appropriate Web browser for the user's desktop
    environment, and returns \c true if successful; otherwise returns \c false.

    If the URL is a reference to a local file (i.e., the URL scheme is "file") then
    it will be opened with a suitable application instead of a Web browser.

    The following example opens a file on the Windows file system residing on a path
    that contains spaces:

    \snippet code/src_gui_util_qdesktopservices.cpp 2

    If a \c mailto URL is specified, the user's e-mail client will be used to open a
    composer window containing the options specified in the URL, similar to the way
    \c mailto links are handled by a Web browser.

    For example, the following URL contains a recipient (\c{user@foo.com}), a
    subject (\c{Test}), and a message body (\c{Just a test}):

    \snippet code/src_gui_util_qdesktopservices.cpp 1

    \warning Although many e-mail clients can send attachments and are
    Unicode-aware, the user may have configured their client without these features.
    Also, certain e-mail clients (e.g., Lotus Notes) have problems with long URLs.

    \warning A return value of \c true indicates that the application has successfully requested
    the operating system to open the URL in an external application. The external application may
    still fail to launch or fail to open the requested URL. This result will not be reported back
    to the application.

    \warning URLs passed to this function on iOS will not load unless their schemes are
    listed in the \c LSApplicationQueriesSchemes key of the application's Info.plist file.
    For more information, see the Apple Developer Documentation for
    \l {iOS: canOpenURL:}{canOpenURL:}.
    For example, the following lines enable URLs with the HTTPS scheme:

    \snippet code/src_gui_util_qdesktopservices.cpp 3

    \note For Android Nougat (SDK 24) and above, URLs with a \c file scheme
    are opened using \l {Android: FileProvider}{FileProvider} which tries to obtain
    a shareable \c content scheme URI first. For that reason, Qt for Android defines
    a file provider with the authority \c ${applicationId}.qtprovider, with \c applicationId
    being the app's package name to avoid name conflicts. For more information, also see
    \l {Android: Setting up file sharing}{Setting up file sharing}.

    \sa setUrlHandler()
*/
bool QDesktopServices::openUrl(const QUrl &url)
{
    QOpenUrlHandlerRegistry *registry = handlerRegistry();
    QMutexLocker locker(&registry->mutex);
    static bool insideOpenUrlHandler = false;

    if (!insideOpenUrlHandler) {
        QOpenUrlHandlerRegistry::HandlerHash::ConstIterator handler = registry->handlers.constFind(url.scheme());
        if (handler != registry->handlers.constEnd()) {
            insideOpenUrlHandler = true;
            bool result = QMetaObject::invokeMethod(handler->receiver, handler->name.constData(), Qt::DirectConnection, Q_ARG(QUrl, url));
            insideOpenUrlHandler = false;
            return result; // ### support bool slot return type
        }
    }
    if (!url.isValid())
        return false;

    QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    if (Q_UNLIKELY(!platformIntegration)) {
        QCoreApplication *application = QCoreApplication::instance();
        if (Q_UNLIKELY(!application))
            qWarning("QDesktopServices::openUrl: Please instantiate the QGuiApplication object "
                     "first");
        else if (Q_UNLIKELY(!qobject_cast<QGuiApplication *>(application)))
            qWarning("QDesktopServices::openUrl: Application is not a GUI application");
        return false;
    }

    QPlatformServices *platformServices = platformIntegration->services();
    if (!platformServices) {
        qWarning("The platform plugin does not support services.");
        return false;
    }
    // We only use openDocument if there is no fragment for the URL to
    // avoid it being lost when using openDocument
    if (url.isLocalFile() && !url.hasFragment())
        return platformServices->openDocument(url);
    return platformServices->openUrl(url);
}

/*!
    Sets the handler for the given \a scheme to be the handler \a method provided by
    the \a receiver object.

    This function provides a way to customize the behavior of openUrl(). If openUrl()
    is called with a URL with the specified \a scheme then the given \a method on the
    \a receiver object is called instead of QDesktopServices launching an external
    application.

    The provided method must be implemented as a slot that only accepts a single QUrl
    argument.

    \snippet code/src_gui_util_qdesktopservices.cpp 0

    If setUrlHandler() is used to set a new handler for a scheme which already
    has a handler, the existing handler is simply replaced with the new one.
    Since QDesktopServices does not take ownership of handlers, no objects are
    deleted when a handler is replaced.

    Note that the handler will always be called from within the same thread that
    calls QDesktopServices::openUrl().

    You must call unsetUrlHandler() before destroying the handler object, so
    the destruction of the handler object does not overlap with concurrent
    invocations of openUrl() using it.

    \section1 iOS

    To use this function for receiving data from other apps on iOS you also need to
    add the custom scheme to the \c CFBundleURLSchemes list in your Info.plist file:

    \snippet code/src_gui_util_qdesktopservices.cpp 4

    For more information, see the Apple Developer Documentation for
    \l {iOS: Defining a Custom URL Scheme for Your App}{Defining a Custom URL Scheme for Your App}.
    \warning It is not possible to claim support for some well known URL schemes, including http and
    https. This is only allowed for Universal Links.

    To claim support for http and https the above entry in the Info.plist file
    is not allowed. This is only possible when you add your domain to the
    Entitlements file:

    \snippet code/src_gui_util_qdesktopservices.cpp 7

    iOS will search for /.well-known/apple-app-site-association on your domain,
    when the application is installed. If you want to listen to
    \c{https://your.domain.com/help?topic=ABCDEF} you need to provide the following
    content there:

    \snippet code/src_gui_util_qdesktopservices.cpp 8

    For more information, see the Apple Developer Documentation for
    \l {iOS: Supporting Associated Domains}{Supporting Associated Domains}.

    \section1 Android

    To use this function for receiving data from other apps on Android, you
    need to add one or more intent filter to the \c activity in your app manifest:

    \snippet code/src_gui_util_qdesktopservices.cpp 9

    For more information, see the Android Developer Documentation for
    \l {Android: Create Deep Links to App Content}{Create Deep Links to App Content}.

    To immediately open the corresponding content in your Android app, without
    requiring the user to select the app, you need to verify your link. To
    enable the verification, add an additional parameter to your intent filter:

    \snippet code/src_gui_util_qdesktopservices.cpp 10

    Android will look for \c{https://your.domain.com/.well-known/assetlinks.json},
    when the application is installed. If you want to listen to
    \c{https://your.domain.com:1337/help}, you need to provide the following
    content there:

    \snippet code/src_gui_util_qdesktopservices.cpp 11

    For more information, see the Android Developer Documentation for
    \l {Android: Verify Android App Links}{Verify Android App Links}.

    \sa openUrl(), unsetUrlHandler()
*/
void QDesktopServices::setUrlHandler(const QString &scheme, QObject *receiver, const char *method)
{
    QOpenUrlHandlerRegistry *registry = handlerRegistry();
    QMutexLocker locker(&registry->mutex);
    if (!receiver) {
        registry->handlers.remove(scheme.toLower());
        return;
    }
    QOpenUrlHandlerRegistry::Handler h;
    h.receiver = receiver;
    h.name = method;
    registry->handlers.insert(scheme.toLower(), h);
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    QObject::connect(receiver, &QObject::destroyed, &registry->context,
                     [registry](QObject *obj) { registry->handlerDestroyed(obj); },
                     Qt::DirectConnection);
#endif
}

/*!
    Removes a previously set URL handler for the specified \a scheme.

    Call this function before the handler object that was registered for \a scheme
    is destroyed, to prevent concurrent openUrl() calls from continuing to call
    the destroyed handler object.

    \sa setUrlHandler()
*/
void QDesktopServices::unsetUrlHandler(const QString &scheme)
{
    setUrlHandler(scheme, nullptr, nullptr);
}

QT_END_NAMESPACE

#endif // QT_NO_DESKTOPSERVICES
