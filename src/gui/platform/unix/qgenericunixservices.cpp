// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgenericunixservices_p.h"
#include <QtGui/private/qtguiglobal_p.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#if QT_CONFIG(process)
# include <QtCore/QProcess>
#endif
#if QT_CONFIG(settings)
#include <QtCore/QSettings>
#endif
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>

#if QT_CONFIG(dbus)
// These QtCore includes are needed for xdg-desktop-portal support
#include <QtCore/private/qcore_unix_p.h>

#include <QtCore/QFileInfo>
#include <QtCore/QUrlQuery>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusPendingCall>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusPendingReply>
#include <QtDBus/QDBusUnixFileDescriptor>

#include <fcntl.h>

#endif // QT_CONFIG(dbus)

#include <stdlib.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(multiprocess)

enum { debug = 0 };

static inline QByteArray detectDesktopEnvironment()
{
    const QByteArray xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    if (!xdgCurrentDesktop.isEmpty())
        return xdgCurrentDesktop.toUpper(); // KDE, GNOME, UNITY, LXDE, MATE, XFCE...

    // Classic fallbacks
    if (!qEnvironmentVariableIsEmpty("KDE_FULL_SESSION"))
        return QByteArrayLiteral("KDE");
    if (!qEnvironmentVariableIsEmpty("GNOME_DESKTOP_SESSION_ID"))
        return QByteArrayLiteral("GNOME");

    // Fallback to checking $DESKTOP_SESSION (unreliable)
    QByteArray desktopSession = qgetenv("DESKTOP_SESSION");

    // This can be a path in /usr/share/xsessions
    int slash = desktopSession.lastIndexOf('/');
    if (slash != -1) {
#if QT_CONFIG(settings)
        QSettings desktopFile(QFile::decodeName(desktopSession + ".desktop"), QSettings::IniFormat);
        desktopFile.beginGroup(QStringLiteral("Desktop Entry"));
        QByteArray desktopName = desktopFile.value(QStringLiteral("DesktopNames")).toByteArray();
        if (!desktopName.isEmpty())
            return desktopName;
#endif

        // try decoding just the basename
        desktopSession = desktopSession.mid(slash + 1);
    }

    if (desktopSession == "gnome")
        return QByteArrayLiteral("GNOME");
    else if (desktopSession == "xfce")
        return QByteArrayLiteral("XFCE");
    else if (desktopSession == "kde")
        return QByteArrayLiteral("KDE");

    return QByteArrayLiteral("UNKNOWN");
}

static inline bool checkExecutable(const QString &candidate, QString *result)
{
    *result = QStandardPaths::findExecutable(candidate);
    return !result->isEmpty();
}

static inline bool detectWebBrowser(const QByteArray &desktop,
                                    bool checkBrowserVariable,
                                    QString *browser)
{
    const char *browsers[] = {"google-chrome", "firefox", "mozilla", "opera"};

    browser->clear();
    if (checkExecutable(QStringLiteral("xdg-open"), browser))
        return true;

    if (checkBrowserVariable) {
        QByteArray browserVariable = qgetenv("DEFAULT_BROWSER");
        if (browserVariable.isEmpty())
            browserVariable = qgetenv("BROWSER");
        if (!browserVariable.isEmpty() && checkExecutable(QString::fromLocal8Bit(browserVariable), browser))
            return true;
    }

    if (desktop == QByteArray("KDE")) {
        if (checkExecutable(QStringLiteral("kde-open5"), browser))
            return true;
        // Konqueror launcher
        if (checkExecutable(QStringLiteral("kfmclient"), browser)) {
            browser->append(" exec"_L1);
            return true;
        }
    } else if (desktop == QByteArray("GNOME")) {
        if (checkExecutable(QStringLiteral("gnome-open"), browser))
            return true;
    }

    for (size_t i = 0; i < sizeof(browsers)/sizeof(char *); ++i)
        if (checkExecutable(QLatin1StringView(browsers[i]), browser))
            return true;
    return false;
}

static inline bool launch(const QString &launcher, const QUrl &url)
{
    const QString command = launcher + u' ' + QLatin1StringView(url.toEncoded());
    if (debug)
        qDebug("Launching %s", qPrintable(command));
#if !QT_CONFIG(process)
    const bool ok = ::system(qPrintable(command + " &"_L1));
#else
    QStringList args = QProcess::splitCommand(command);
    bool ok = false;
    if (!args.isEmpty()) {
        QString program = args.takeFirst();
        ok = QProcess::startDetached(program, args);
    }
#endif
    if (!ok)
        qWarning("Launch failed (%s)", qPrintable(command));
    return ok;
}

#if QT_CONFIG(dbus)
static inline bool checkNeedPortalSupport()
{
    return !QStandardPaths::locate(QStandardPaths::RuntimeLocation, "flatpak-info"_L1).isEmpty() || qEnvironmentVariableIsSet("SNAP");
}

static inline QDBusMessage xdgDesktopPortalOpenFile(const QUrl &url)
{
    // DBus signature:
    // OpenFile (IN   s      parent_window,
    //           IN   h      fd,
    //           IN   a{sv}  options,
    //           OUT  o      handle)
    // Options:
    // handle_token (s) -  A string that will be used as the last element of the @handle.
    // writable (b) - Whether to allow the chosen application to write to the file.

#ifdef O_PATH
    const int fd = qt_safe_open(QFile::encodeName(url.toLocalFile()), O_PATH);
    if (fd != -1) {
        QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                                              "/org/freedesktop/portal/desktop"_L1,
                                                              "org.freedesktop.portal.OpenURI"_L1,
                                                              "OpenFile"_L1);

        QDBusUnixFileDescriptor descriptor;
        descriptor.giveFileDescriptor(fd);

        const QVariantMap options = {{"writable"_L1, true}};

        // FIXME parent_window_id
        message << QString() << QVariant::fromValue(descriptor) << options;

        return QDBusConnection::sessionBus().call(message);
    }
#else
    Q_UNUSED(url);
#endif

    return QDBusMessage::createError(QDBusError::InternalError, qt_error_string());
}

static inline QDBusMessage xdgDesktopPortalOpenUrl(const QUrl &url)
{
    // DBus signature:
    // OpenURI (IN   s      parent_window,
    //          IN   s      uri,
    //          IN   a{sv}  options,
    //          OUT  o      handle)
    // Options:
    // handle_token (s) -  A string that will be used as the last element of the @handle.
    // writable (b) - Whether to allow the chosen application to write to the file.
    //                This key only takes effect the uri points to a local file that is exported in the document portal,
    //                and the chosen application is sandboxed itself.

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                                          "/org/freedesktop/portal/desktop"_L1,
                                                          "org.freedesktop.portal.OpenURI"_L1,
                                                          "OpenURI"_L1);
    // FIXME parent_window_id and handle writable option
    message << QString() << url.toString() << QVariantMap();

    return QDBusConnection::sessionBus().call(message);
}

static inline QDBusMessage xdgDesktopPortalSendEmail(const QUrl &url)
{
    // DBus signature:
    // ComposeEmail (IN   s      parent_window,
    //               IN   a{sv}  options,
    //               OUT  o      handle)
    // Options:
    // address (s) - The email address to send to.
    // subject (s) - The subject for the email.
    // body (s) - The body for the email.
    // attachment_fds (ah) - File descriptors for files to attach.

    QUrlQuery urlQuery(url);
    QVariantMap options;
    options.insert("address"_L1, url.path());
    options.insert("subject"_L1, urlQuery.queryItemValue("subject"_L1));
    options.insert("body"_L1, urlQuery.queryItemValue("body"_L1));

    // O_PATH seems to be present since Linux 2.6.39, which is not case of RHEL 6
#ifdef O_PATH
    QList<QDBusUnixFileDescriptor> attachments;
    const QStringList attachmentUris = urlQuery.allQueryItemValues("attachment"_L1);

    for (const QString &attachmentUri : attachmentUris) {
        const int fd = qt_safe_open(QFile::encodeName(attachmentUri), O_PATH);
        if (fd != -1) {
            QDBusUnixFileDescriptor descriptor(fd);
            attachments << descriptor;
            qt_safe_close(fd);
        }
    }

    options.insert("attachment_fds"_L1, QVariant::fromValue(attachments));
#endif

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                                          "/org/freedesktop/portal/desktop"_L1,
                                                          "org.freedesktop.portal.Email"_L1,
                                                          "ComposeEmail"_L1);

    // FIXME parent_window_id
    message << QString() << options;

    return QDBusConnection::sessionBus().call(message);
}
#endif // QT_CONFIG(dbus)

QByteArray QGenericUnixServices::desktopEnvironment() const
{
    static const QByteArray result = detectDesktopEnvironment();
    return result;
}

bool QGenericUnixServices::openUrl(const QUrl &url)
{
    if (url.scheme() == "mailto"_L1) {
#if QT_CONFIG(dbus)
        if (checkNeedPortalSupport()) {
            QDBusError error = xdgDesktopPortalSendEmail(url);
            if (!error.isValid())
                return true;

            // service not running, fall back
        }
#endif
        return openDocument(url);
    }

#if QT_CONFIG(dbus)
    if (checkNeedPortalSupport()) {
        QDBusError error = xdgDesktopPortalOpenUrl(url);
        if (!error.isValid())
            return true;
    }
#endif

    if (m_webBrowser.isEmpty() && !detectWebBrowser(desktopEnvironment(), true, &m_webBrowser)) {
        qWarning("Unable to detect a web browser to launch '%s'", qPrintable(url.toString()));
        return false;
    }
    return launch(m_webBrowser, url);
}

bool QGenericUnixServices::openDocument(const QUrl &url)
{
#if QT_CONFIG(dbus)
    if (checkNeedPortalSupport()) {
        QDBusError error = xdgDesktopPortalOpenFile(url);
        if (!error.isValid())
            return true;
    }
#endif

    if (m_documentLauncher.isEmpty() && !detectWebBrowser(desktopEnvironment(), false, &m_documentLauncher)) {
        qWarning("Unable to detect a launcher for '%s'", qPrintable(url.toString()));
        return false;
    }
    return launch(m_documentLauncher, url);
}

#else
QByteArray QGenericUnixServices::desktopEnvironment() const
{
    return QByteArrayLiteral("UNKNOWN");
}

bool QGenericUnixServices::openUrl(const QUrl &url)
{
    Q_UNUSED(url);
    qWarning("openUrl() not supported on this platform");
    return false;
}

bool QGenericUnixServices::openDocument(const QUrl &url)
{
    Q_UNUSED(url);
    qWarning("openDocument() not supported on this platform");
    return false;
}

#endif // QT_NO_MULTIPROCESS

QT_END_NAMESPACE
