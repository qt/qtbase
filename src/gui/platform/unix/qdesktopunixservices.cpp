// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:execute-external-code

#include "qdesktopunixservices_p.h"
#include <QtGui/private/qtguiglobal_p.h>
#include "qguiapplication.h"
#include "qwindow.h"
#include <QtGui/qpa/qplatformwindow_p.h>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

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
        QString browserVariable = qEnvironmentVariable("DEFAULT_BROWSER");
        if (browserVariable.isEmpty())
            browserVariable = qEnvironmentVariable("BROWSER");
        if (!browserVariable.isEmpty() && checkExecutable(browserVariable, browser))
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

static inline bool launch(const QString &launcher, const QUrl &url,
                          const QString &xdgActivationToken)
{

    const QString command = launcher + u' ' + QLatin1StringView(url.toEncoded());
    qCDebug(lcQpaServices, "Launching %s", qPrintable(command));
#if !QT_CONFIG(process)
    if (!xdgActivationToken.isEmpty())
        qputenv("XDG_ACTIVATION_TOKEN", xdgActivationToken.toUtf8());
    const bool ok = ::system(qPrintable(command + " &"_L1));
    if (!xdgActivationToken.isEmpty())
        qunsetenv("XDG_ACTIVATION_TOKEN");
#  else
    QStringList args = QProcess::splitCommand(command);
    bool ok = false;
    if (!args.isEmpty()) {
        QString program = args.takeFirst();
        QProcess process;
        process.setProgram(program);
        process.setArguments(args);

        if (!xdgActivationToken.isEmpty()) {
            auto env = QProcessEnvironment::systemEnvironment();
            env.insert(u"XDG_ACTIVATION_TOKEN"_s, xdgActivationToken);
            process.setEnvironment(env.toStringList());
        }
        ok = process.startDetached(nullptr);
    }
#  endif
    if (!ok)
        qCWarning(lcQpaServices, "Launch failed (%s)", qPrintable(command));

    return ok;
}

#if QT_CONFIG(dbus)
static inline bool checkNeedPortalSupport()
{
    return QFileInfo::exists("/.flatpak-info"_L1) || qEnvironmentVariableIsSet("SNAP");
}

static inline QDBusMessage xdgDesktopPortalOpenFile(const QUrl &url, const QString &parentWindow,
                                                    const QString &xdgActivationToken)
{
    // DBus signature:
    // OpenFile (IN   s      parent_window,
    //           IN   h      fd,
    //           IN   a{sv}  options,
    //           OUT  o      handle)
    // Options:
    // handle_token (s) -  A string that will be used as the last element of the @handle.
    // writable (b) - Whether to allow the chosen application to write to the file.

    const int fd = qt_safe_open(QFile::encodeName(url.toLocalFile()), O_RDONLY);
    if (fd != -1) {
        QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                                              "/org/freedesktop/portal/desktop"_L1,
                                                              "org.freedesktop.portal.OpenURI"_L1,
                                                              "OpenFile"_L1);

        QDBusUnixFileDescriptor descriptor;
        descriptor.giveFileDescriptor(fd);

        QVariantMap options = {};

        if (!xdgActivationToken.isEmpty()) {
            options.insert("activation_token"_L1, xdgActivationToken);
        }

        message << parentWindow << QVariant::fromValue(descriptor) << options;

        return QDBusConnection::sessionBus().call(message);
    }

    return QDBusMessage::createError(QDBusError::InternalError, qt_error_string());
}

static inline QDBusMessage xdgDesktopPortalOpenUrl(const QUrl &url, const QString &parentWindow,
                                                   const QString &xdgActivationToken)
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
    QVariantMap options;

    if (!xdgActivationToken.isEmpty()) {
        options.insert("activation_token"_L1, xdgActivationToken);
    }

    message << parentWindow << url.toString() << options;

    return QDBusConnection::sessionBus().call(message);
}

static inline QDBusMessage xdgDesktopPortalSendEmail(const QUrl &url, const QString &parentWindow,
                                                     const QString &xdgActivationToken)
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

    if (!xdgActivationToken.isEmpty()) {
        options.insert("activation_token"_L1, xdgActivationToken);
    }

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop"_L1,
                                                          "/org/freedesktop/portal/desktop"_L1,
                                                          "org.freedesktop.portal.Email"_L1,
                                                          "ComposeEmail"_L1);

    message << parentWindow << options;

    return QDBusConnection::sessionBus().call(message);
}

namespace {
struct XDGDesktopColor
{
    double r = 0;
    double g = 0;
    double b = 0;

    QColor toQColor() const
    {
        constexpr auto rgbMax = 255;
        return { static_cast<int>(r * rgbMax), static_cast<int>(g * rgbMax),
                 static_cast<int>(b * rgbMax) };
    }
};

const QDBusArgument &operator>>(const QDBusArgument &argument, XDGDesktopColor &myStruct)
{
    argument.beginStructure();
    argument >> myStruct.r >> myStruct.g >> myStruct.b;
    argument.endStructure();
    return argument;
}

class XdgDesktopPortalColorPicker : public QPlatformServiceColorPicker
{
    Q_OBJECT
public:
    XdgDesktopPortalColorPicker(const QString &parentWindowId, QWindow *parent)
        : QPlatformServiceColorPicker(parent), m_parentWindowId(parentWindowId)
    {
    }

    void pickColor() override
    {
        // DBus signature:
        // PickColor (IN   s      parent_window,
        //            IN   a{sv}  options
        //            OUT  o      handle)
        // Options:
        // handle_token (s) -  A string that will be used as the last element of the @handle.

        QDBusMessage message = QDBusMessage::createMethodCall(
                "org.freedesktop.portal.Desktop"_L1, "/org/freedesktop/portal/desktop"_L1,
                "org.freedesktop.portal.Screenshot"_L1, "PickColor"_L1);
        message << m_parentWindowId << QVariantMap();

        QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
        auto watcher = new QDBusPendingCallWatcher(pendingCall, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this,
                [this](QDBusPendingCallWatcher *watcher) {
                    watcher->deleteLater();
                    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
                    if (reply.isError()) {
                        qWarning("DBus call to pick color failed: %s",
                                 qPrintable(reply.error().message()));
                        Q_EMIT colorPicked({});
                    } else {
                        QDBusConnection::sessionBus().connect(
                                "org.freedesktop.portal.Desktop"_L1, reply.value().path(),
                                "org.freedesktop.portal.Request"_L1, "Response"_L1, this,
                                // clang-format off
                                SLOT(gotColorResponse(uint,QVariantMap))
                                // clang-format on
                        );
                    }
                });
    }

private Q_SLOTS:
    void gotColorResponse(uint result, const QVariantMap &map)
    {
        if (result != 0)
            return;
        if (map.contains(u"color"_s)) {
            XDGDesktopColor color{};
            map.value(u"color"_s).value<QDBusArgument>() >> color;
            Q_EMIT colorPicked(color.toQColor());
        } else {
            Q_EMIT colorPicked({});
        }
        deleteLater();
    }

private:
    const QString m_parentWindowId;
};
} // namespace

#endif // QT_CONFIG(dbus)

QDesktopUnixServices::QDesktopUnixServices()
{
    if (detectDesktopEnvironment() == QByteArrayLiteral("UNKNOWN"))
        return;

#if QT_CONFIG(dbus)
    if (qEnvironmentVariableIntValue("QT_NO_XDG_DESKTOP_PORTAL") > 0) {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(
            "org.freedesktop.portal.Desktop"_L1, "/org/freedesktop/portal/desktop"_L1,
            "org.freedesktop.DBus.Properties"_L1, "Get"_L1);
    message << "org.freedesktop.portal.Screenshot"_L1
            << "version"_L1;

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    auto watcher = new QDBusPendingCallWatcher(pendingCall);
    m_watcher = watcher;
            QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher,
                     [this](QDBusPendingCallWatcher *watcher) {
                         watcher->deleteLater();
                         QDBusPendingReply<QVariant> reply = *watcher;
                         if (!reply.isError() && reply.value().toUInt() >= 2)
                             m_hasScreenshotPortalWithColorPicking = true;
                     });

#endif
}

QDesktopUnixServices::~QDesktopUnixServices()
{
#if QT_CONFIG(dbus)
    delete m_watcher;
#endif
}

QPlatformServiceColorPicker *QDesktopUnixServices::colorPicker(QWindow *parent)
{
#if QT_CONFIG(dbus)
    // Make double sure that we are in a wayland environment. In particular check
    // WAYLAND_DISPLAY so also XWayland apps benefit from portal-based color picking.
    // Outside wayland we'll rather rely on other means than the XDG desktop portal.
    if (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
        || QGuiApplication::platformName().startsWith("wayland"_L1)) {
        return new XdgDesktopPortalColorPicker(portalWindowIdentifier(parent), parent);
    }
    return nullptr;
#else
    Q_UNUSED(parent);
    return nullptr;
#endif
}

QByteArray QDesktopUnixServices::desktopEnvironment() const
{
    static const QByteArray result = detectDesktopEnvironment();
    return result;
}

template<typename F>
void runWithXdgActivationToken(F &&functionToCall)
{
#if QT_CONFIG(wayland)
    QWindow *window = qGuiApp->focusWindow();

    if (!window) {
        functionToCall({});
        return;
    }

    auto waylandApp = dynamic_cast<QNativeInterface::QWaylandApplication *>(
            qGuiApp->platformNativeInterface());
    auto waylandWindow =
            dynamic_cast<QNativeInterface::Private::QWaylandWindow *>(window->handle());

    if (!waylandWindow || !waylandApp) {
        functionToCall({});
        return;
    }

    QObject::connect(waylandWindow,
                     &QNativeInterface::Private::QWaylandWindow::xdgActivationTokenCreated,
                     waylandWindow, functionToCall, Qt::SingleShotConnection);
    waylandWindow->requestXdgActivationToken(waylandApp->lastInputSerial());
#else
    functionToCall({});
#endif
}

bool QDesktopUnixServices::openUrl(const QUrl &url)
{
    auto openUrlInternal = [this](const QUrl &url, const QString &xdgActivationToken) {
        if (url.scheme() == "mailto"_L1) {
#  if QT_CONFIG(dbus)
            if (checkNeedPortalSupport()) {
                const QString parentWindow = QGuiApplication::focusWindow()
                        ? portalWindowIdentifier(QGuiApplication::focusWindow())
                        : QString();
                QDBusError error = xdgDesktopPortalSendEmail(url, parentWindow, xdgActivationToken);
                if (!error.isValid())
                    return true;

                // service not running, fall back
            }
#  endif
            return openDocument(url);
        }

#  if QT_CONFIG(dbus)
        if (checkNeedPortalSupport()) {
            const QString parentWindow = QGuiApplication::focusWindow()
                    ? portalWindowIdentifier(QGuiApplication::focusWindow())
                    : QString();
            QDBusError error = xdgDesktopPortalOpenUrl(url, parentWindow, xdgActivationToken);
            if (!error.isValid())
                return true;
        }
#  endif

        if (m_webBrowser.isEmpty()
            && !detectWebBrowser(desktopEnvironment(), true, &m_webBrowser)) {
            qCWarning(lcQpaServices, "Unable to detect a web browser to launch '%s'", qPrintable(url.toString()));
            return false;
        }
        return launch(m_webBrowser, url, xdgActivationToken);
    };

    if (QGuiApplication::platformName().startsWith("wayland"_L1)) {
        runWithXdgActivationToken(
                [openUrlInternal, url](const QString &token) { openUrlInternal(url, token); });

        return true;

    } else {
        return openUrlInternal(url, QString());
    }
}

bool QDesktopUnixServices::openDocument(const QUrl &url)
{
    auto openDocumentInternal = [this](const QUrl &url, const QString &xdgActivationToken) {

#  if QT_CONFIG(dbus)
        if (checkNeedPortalSupport()) {
            const QString parentWindow = QGuiApplication::focusWindow()
                    ? portalWindowIdentifier(QGuiApplication::focusWindow())
                    : QString();
            QDBusError error = xdgDesktopPortalOpenFile(url, parentWindow, xdgActivationToken);
            if (!error.isValid())
                return true;
        }
#  endif

        if (m_documentLauncher.isEmpty()
            && !detectWebBrowser(desktopEnvironment(), false, &m_documentLauncher)) {
            qCWarning(lcQpaServices, "Unable to detect a launcher for '%s'", qPrintable(url.toString()));
            return false;
        }
        return launch(m_documentLauncher, url, xdgActivationToken);
    };

    if (QGuiApplication::platformName().startsWith("wayland"_L1)) {
        runWithXdgActivationToken([openDocumentInternal, url](const QString &token) {
            openDocumentInternal(url, token);
        });

        return true;
    } else {
        return openDocumentInternal(url, QString());
    }
}

#else
QDesktopUnixServices::QDesktopUnixServices() = default;
QDesktopUnixServices::~QDesktopUnixServices() = default;

QByteArray QDesktopUnixServices::desktopEnvironment() const
{
    return QByteArrayLiteral("UNKNOWN");
}

bool QDesktopUnixServices::openUrl(const QUrl &url)
{
    Q_UNUSED(url);
    qWarning("openUrl() not supported on this platform");
    return false;
}

bool QDesktopUnixServices::openDocument(const QUrl &url)
{
    Q_UNUSED(url);
    qWarning("openDocument() not supported on this platform");
    return false;
}

QPlatformServiceColorPicker *QDesktopUnixServices::colorPicker(QWindow *parent)
{
    Q_UNUSED(parent);
    return nullptr;
}

#endif // QT_CONFIG(multiprocess)

QString QDesktopUnixServices::portalWindowIdentifier(QWindow *window)
{
    Q_UNUSED(window);
    return QString();
}


void QDesktopUnixServices::registerDBusMenuForWindow(QWindow *window, const QString &service, const QString &path)
{
    Q_UNUSED(window);
    Q_UNUSED(service);
    Q_UNUSED(path);
}

void QDesktopUnixServices::unregisterDBusMenuForWindow(QWindow *window)
{
    Q_UNUSED(window);
}


bool QDesktopUnixServices::hasCapability(Capability capability) const
{
    switch (capability) {
    case Capability::ColorPicking:
        return m_hasScreenshotPortalWithColorPicking;
    }
    return false;
}

void QDesktopUnixServices::setApplicationBadge(qint64 number)
{
#if QT_CONFIG(dbus)
    if (qGuiApp->desktopFileName().isEmpty()) {
        qCWarning(lcQpaServices, "Cannot set badge number - QGuiApplication::desktopFileName() is empty");
        return;
    }


    const QString launcherUrl = QStringLiteral("application://") + qGuiApp->desktopFileName() + QStringLiteral(".desktop");
    const qint64 count = qBound(0, number, 9999);
    QVariantMap dbusUnityProperties;

    if (count > 0) {
        dbusUnityProperties[QStringLiteral("count")] = count;
        dbusUnityProperties[QStringLiteral("count-visible")] = true;
    } else {
        dbusUnityProperties[QStringLiteral("count-visible")] = false;
    }

    auto signal = QDBusMessage::createSignal(QStringLiteral("/com/canonical/unity/launcherentry/")
        + qGuiApp->applicationName(), QStringLiteral("com.canonical.Unity.LauncherEntry"), QStringLiteral("Update"));

    signal.setArguments({launcherUrl, dbusUnityProperties});

    QDBusConnection::sessionBus().send(signal);
#else
    Q_UNUSED(number)
#endif
}

QT_END_NAMESPACE

#include "qdesktopunixservices.moc"
