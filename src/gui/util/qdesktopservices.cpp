/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

QT_BEGIN_NAMESPACE

class QOpenUrlHandlerRegistry : public QObject
{
    Q_OBJECT
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

public Q_SLOTS:
    void handlerDestroyed(QObject *handler);

};

Q_GLOBAL_STATIC(QOpenUrlHandlerRegistry, handlerRegistry)

void QOpenUrlHandlerRegistry::handlerDestroyed(QObject *handler)
{
    HandlerHash::Iterator it = handlers.begin();
    while (it != handlers.end()) {
        if (it->receiver == handler) {
            it = handlers.erase(it);
        } else {
            ++it;
        }
    }
}

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

    If inside the handler you decide that you can't open the requested
    URL, you can just call QDesktopServices::openUrl() again with the
    same argument, and it will try to open the URL using the
    appropriate mechanism for the user's desktop environment.

    Combined with platform specific settings, the schemes registered by the
    openUrl() function can also be exposed to other applications, opening up
    for application deep linking or a very basic URL-based IPC mechanism.

    \note Since Qt 5, storageLocation() and displayName() are replaced by functionality
    provided by the QStandardPaths class.

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
    \l{https://developer.apple.com/documentation/uikit/uiapplication/1622952-canopenurl}{canOpenURL(_:)}.
    For example, the following lines enable URLs with the HTTPS scheme:

    \snippet code/src_gui_util_qdesktopservices.cpp 3

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

    To use this function for receiving data from other apps on iOS you also need to
    add the custom scheme to the \c CFBundleURLSchemes list in your Info.plist file:

    \snippet code/src_gui_util_qdesktopservices.cpp 4

    For more information, see the Apple Developer Documentation for
    \l{https://developer.apple.com/documentation/uikit/core_app/allowing_apps_and_websites_to_link_to_your_content/communicating_with_other_apps_using_custom_urls?language=objc}{Communicating with Other Apps Using Custom URLs}.
    \warning It is not possible to claim support for some well known URL schemes, including http and https. This is only allowed for Universal Links.

    To claim support for http and https the above entry in the Info.plist file
    is not allowed. This is only possible when you add your domain to the
    Entitlements file:

    \snippet code/src_gui_util_qdesktopservices.cpp 7

    iOS will search for /.well-known/apple-app-site-association on your domain,
    when the application is installed. If you want to listen to
    https://your.domain.com/help?topic=ABCDEF you need to provide the following
    content there:

    \snippet code/src_gui_util_qdesktopservices.cpp 8

    For more information, see the Apple Developer Documentation for
    \l{https://developer.apple.com/documentation/safariservices/supporting_associated_domains_in_your_app}[Supporting Associated Domains}.

    If setUrlHandler() is used to set a new handler for a scheme which already
    has a handler, the existing handler is simply replaced with the new one.
    Since QDesktopServices does not take ownership of handlers, no objects are
    deleted when a handler is replaced.

    Note that the handler will always be called from within the same thread that
    calls QDesktopServices::openUrl().

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
    QObject::connect(receiver, SIGNAL(destroyed(QObject*)),
                     registry, SLOT(handlerDestroyed(QObject*)));
}

/*!
    Removes a previously set URL handler for the specified \a scheme.

    \sa setUrlHandler()
*/
void QDesktopServices::unsetUrlHandler(const QString &scheme)
{
    setUrlHandler(scheme, nullptr, nullptr);
}

#if QT_DEPRECATED_SINCE(5, 0)
/*!
    \enum QDesktopServices::StandardLocation
    \since 4.4
    \obsolete
    Use QStandardPaths::StandardLocation (see storageLocation() for porting notes)

    This enum describes the different locations that can be queried by
    QDesktopServices::storageLocation and QDesktopServices::displayName.

    \value DesktopLocation Returns the user's desktop directory.
    \value DocumentsLocation Returns the user's document.
    \value FontsLocation Returns the user's fonts.
    \value ApplicationsLocation Returns the user's applications.
    \value MusicLocation Returns the users music.
    \value MoviesLocation Returns the user's movies.
    \value PicturesLocation Returns the user's pictures.
    \value TempLocation Returns the system's temporary directory.
    \value HomeLocation Returns the user's home directory.
    \value DataLocation Returns a directory location where persistent
           application data can be stored. QCoreApplication::applicationName
           and QCoreApplication::organizationName should work on all
           platforms.
    \value CacheLocation Returns a directory location where user-specific
           non-essential (cached) data should be written.

    \sa storageLocation(), displayName()
*/

/*!
    \fn QString QDesktopServices::storageLocation(StandardLocation type)
    \obsolete
    Use QStandardPaths::writableLocation()

    \note when porting QDesktopServices::DataLocation to QStandardPaths::DataLocation,
    a different path will be returned.

    \c{QDesktopServices::DataLocation} was \c{GenericDataLocation + "/data/organization/application"},
    while QStandardPaths::DataLocation is \c{GenericDataLocation + "/organization/application"}.

    Also note that \c{application} could be empty in Qt 4, if QCoreApplication::setApplicationName()
    wasn't called, while in Qt 5 it defaults to the name of the executable.

    Therefore, if you still need to access the Qt 4 path (for example for data migration to Qt 5), replace
    \snippet code/src_gui_util_qdesktopservices.cpp 5
    with
    \snippet code/src_gui_util_qdesktopservices.cpp 6
    (assuming an organization name and an application name were set).
*/

/*!
    \fn QString QDesktopServices::displayName(StandardLocation type)
    \obsolete
    Use QStandardPaths::displayName()
*/
#endif

extern Q_CORE_EXPORT QString qt_applicationName_noFallback();

QString QDesktopServices::storageLocationImpl(QStandardPaths::StandardLocation type)
{
    if (type == QStandardPaths::AppLocalDataLocation) {
        // Preserve Qt 4 compatibility:
        // * QCoreApplication::applicationName() must default to empty
        // * Unix data location is under the "data/" subdirectory
        const QString compatAppName = qt_applicationName_noFallback();
        const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        const QString organizationName = QCoreApplication::organizationName();
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
        QString result = baseDir;
        if (!organizationName.isEmpty())
            result += QLatin1Char('/') + organizationName;
        if (!compatAppName.isEmpty())
            result += QLatin1Char('/') + compatAppName;
        return result;
#elif defined(Q_OS_UNIX)
        return baseDir + QLatin1String("/data/")
            + organizationName + QLatin1Char('/') + compatAppName;
#endif
    }
    return QStandardPaths::writableLocation(type);
}

QT_END_NAMESPACE

#include "qdesktopservices.moc"

#endif // QT_NO_DESKTOPSERVICES
