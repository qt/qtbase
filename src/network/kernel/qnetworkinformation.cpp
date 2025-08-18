// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

// #define DEBUG_LOADING

#include "qnetworkinformation.h"
#include <QtNetwork/private/qnetworkinformation_p.h>
#include <QtNetwork/qnetworkinformation.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qfactoryloader_p.h>

#include <algorithm>
#include <memory>
#include <mutex>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfo)
Q_LOGGING_CATEGORY(lcNetInfo, "qt.network.info");

struct QNetworkInformationDeleter
{
    void operator()(QNetworkInformation *information) { delete information; }
};

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, qniLoader,
                          (QNetworkInformationBackendFactory_iid,
                           QStringLiteral("/networkinformation")))

struct QStaticNetworkInformationDataHolder
{
    QMutex instanceMutex;
    std::unique_ptr<QNetworkInformation, QNetworkInformationDeleter> instanceHolder;
    QList<QNetworkInformationBackendFactory *> factories;
};
Q_GLOBAL_STATIC(QStaticNetworkInformationDataHolder, dataHolder);

static void networkInfoCleanup()
{
    if (!dataHolder.exists())
        return;
    QMutexLocker locker(&dataHolder->instanceMutex);
    QNetworkInformation *instance = dataHolder->instanceHolder.get();
    if (!instance)
        return;

    dataHolder->instanceHolder.reset();
}

using namespace Qt::Literals::StringLiterals;

class QNetworkInformationDummyBackend : public QNetworkInformationBackend {
    Q_OBJECT
public:
    QString name() const override { return u"dummy"_s; }
    QNetworkInformation::Features featuresSupported() const override
    {
        return {};
    }
};

class QNetworkInformationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QNetworkInformation)
public:
    QNetworkInformationPrivate(QNetworkInformationBackend *backend) : backend(backend) {
        qAddPostRoutine(&networkInfoCleanup);
     }

    static QNetworkInformation *create(QNetworkInformation::Features features);
    static QNetworkInformation *create(QStringView name);
    static QNetworkInformation *createDummy();
    static QNetworkInformation *instance()
    {
        if (!dataHolder())
            return nullptr;
        QMutexLocker locker(&dataHolder->instanceMutex);
        return dataHolder->instanceHolder.get();
    }
    static QStringList backendNames();
    static void addToList(QNetworkInformationBackendFactory *factory);
    static void removeFromList(QNetworkInformationBackendFactory *factory);

private:
    static bool initializeList();

    std::unique_ptr<QNetworkInformationBackend> backend;
};

bool QNetworkInformationPrivate::initializeList()
{
    if (!qniLoader())
        return false;
    if (!dataHolder())
        return false;
    Q_CONSTINIT static QBasicMutex mutex;
    QMutexLocker initLocker(&mutex);

#if QT_CONFIG(library)
    qniLoader->update();
#endif
    // Instantiates the plugins (and registers the factories)
    int index = 0;
    while (qniLoader->instance(index))
        ++index;
    initLocker.unlock();

    // Now sort the list on number of features available (then name)
    const auto featuresNameOrder = [](QNetworkInformationBackendFactory *a,
                                      QNetworkInformationBackendFactory *b) {
        if (!a || !b)
            return a && !b;
        auto aFeaturesSupported = qPopulationCount(unsigned(a->featuresSupported()));
        auto bFeaturesSupported = qPopulationCount(unsigned(b->featuresSupported()));
        return aFeaturesSupported > bFeaturesSupported
                || (aFeaturesSupported == bFeaturesSupported
                    && a->name().compare(b->name(), Qt::CaseInsensitive) < 0);
    };
    QMutexLocker instanceLocker(&dataHolder->instanceMutex);
    std::sort(dataHolder->factories.begin(), dataHolder->factories.end(), featuresNameOrder);

    return !dataHolder->factories.isEmpty();
}

void QNetworkInformationPrivate::addToList(QNetworkInformationBackendFactory *factory)
{
    // @note: factory is in the base class ctor
    if (!dataHolder())
        return;
    QMutexLocker locker(&dataHolder->instanceMutex);
    dataHolder->factories.append(factory);
}

void QNetworkInformationPrivate::removeFromList(QNetworkInformationBackendFactory *factory)
{
    // @note: factory is in the base class dtor
    if (!dataHolder.exists())
        return;
    QMutexLocker locker(&dataHolder->instanceMutex);
    dataHolder->factories.removeAll(factory);
}

QStringList QNetworkInformationPrivate::backendNames()
{
    if (!dataHolder())
        return {};
    if (!initializeList())
        return {};

    QMutexLocker locker(&dataHolder->instanceMutex);
    const QList copy = dataHolder->factories;
    locker.unlock();

    QStringList result;
    result.reserve(copy.size());
    for (const auto *factory : copy)
        result << factory->name();
    return result;
}

QNetworkInformation *QNetworkInformationPrivate::create(QStringView name)
{
    if (name.isEmpty())
        return nullptr;
    if (!dataHolder())
        return nullptr;
#ifdef DEBUG_LOADING
    qDebug().nospace() << "create() called with name=\"" << name
                       << "\". instanceHolder initialized? " << !!dataHolder->instanceHolder;
#endif
    if (!initializeList()) {
#ifdef DEBUG_LOADING
        qDebug("Failed to initialize list, returning.");
#endif
        return nullptr;
    }

    QMutexLocker locker(&dataHolder->instanceMutex);
    if (dataHolder->instanceHolder)
        return dataHolder->instanceHolder.get();


    const auto nameMatches = [name](QNetworkInformationBackendFactory *factory) {
        return factory->name().compare(name, Qt::CaseInsensitive) == 0;
    };
    auto it = std::find_if(dataHolder->factories.cbegin(), dataHolder->factories.cend(),
                            nameMatches);
    if (it == dataHolder->factories.cend()) {
#ifdef DEBUG_LOADING
        if (dataHolder->factories.isEmpty()) {
            qDebug("No plugins available");
        } else {
            QString listNames;
            listNames.reserve(8 * dataHolder->factories.count());
            for (const auto *factory : std::as_const(dataHolder->factories))
                listNames += factory->name() + ", "_L1;
            listNames.chop(2);
            qDebug().nospace() << "Couldn't find " << name << " in list with names: { "
                                << listNames << " }";
        }
#endif
        return nullptr;
    }
#ifdef DEBUG_LOADING
    qDebug() << "Creating instance using loader named " << (*it)->name();
#endif
    QNetworkInformationBackend *backend = (*it)->create((*it)->featuresSupported());
    if (!backend)
        return nullptr;
    dataHolder->instanceHolder.reset(new QNetworkInformation(backend));
    Q_ASSERT(name.isEmpty()
             || dataHolder->instanceHolder->backendName().compare(name, Qt::CaseInsensitive) == 0);
    return dataHolder->instanceHolder.get();
}

QNetworkInformation *QNetworkInformationPrivate::create(QNetworkInformation::Features features)
{
    if (!dataHolder())
        return nullptr;
#ifdef DEBUG_LOADING
    qDebug().nospace() << "create() called with features=\"" << features
                       << "\". instanceHolder initialized? " << !!dataHolder->instanceHolder;
#endif
    if (features == 0)
        return nullptr;

    if (!initializeList()) {
#ifdef DEBUG_LOADING
        qDebug("Failed to initialize list, returning.");
#endif
        return nullptr;
    }
    QMutexLocker locker(&dataHolder->instanceMutex);
    if (dataHolder->instanceHolder)
        return dataHolder->instanceHolder.get();

    const auto supportsRequestedFeatures = [features](QNetworkInformationBackendFactory *factory) {
        return factory && factory->featuresSupported().testFlags(features);
    };

    for (auto it = dataHolder->factories.cbegin(), end = dataHolder->factories.cend(); it != end;
         ++it) {
        it = std::find_if(it, end, supportsRequestedFeatures);
        if (it == end) {
#ifdef DEBUG_LOADING
            if (dataHolder->factories.isEmpty()) {
                qDebug("No plugins available");
            } else {
                QStringList names;
                names.reserve(dataHolder->factories.count());
                for (const auto *factory : std::as_const(dataHolder->factories))
                    names += factory->name();
                qDebug() << "None of the following backends has all the requested features:"
                         << names << features;
            }
#endif
            break;
        }
#ifdef DEBUG_LOADING
        qDebug() << "Creating instance using loader named" << (*it)->name();
#endif
        if (QNetworkInformationBackend *backend = (*it)->create(features)) {
            dataHolder->instanceHolder.reset(new QNetworkInformation(backend));
            Q_ASSERT(dataHolder->instanceHolder->supports(features));
            return dataHolder->instanceHolder.get();
        }
#ifdef DEBUG_LOADING
        else {
            qDebug() << "The factory returned a nullptr";
        }
#endif
    }
#ifdef DEBUG_LOADING
    qDebug() << "Couldn't find/create an appropriate backend.";
#endif
    return nullptr;
}

QNetworkInformation *QNetworkInformationPrivate::createDummy()
{
    if (!dataHolder())
        return nullptr;

    QMutexLocker locker(&dataHolder->instanceMutex);
    if (dataHolder->instanceHolder)
        return dataHolder->instanceHolder.get();

    QNetworkInformationBackend *backend =  new QNetworkInformationDummyBackend;
    dataHolder->instanceHolder.reset(new QNetworkInformation(backend));
    return dataHolder->instanceHolder.get();
}

/*!
    \class QNetworkInformationBackend
    \internal (Semi-private)
    \brief QNetworkInformationBackend provides the interface with
    which QNetworkInformation does all of its actual work.

    Deriving from and implementing this class makes it a candidate
    for use with QNetworkInformation. The derived class must, on
    updates, call setters in the QNetworkInformationBackend which
    will update the values and emit signals if the value has changed.

    \sa QNetworkInformationBackendFactory
*/

/*!
    \internal
    Destroys base backend class.
*/
QNetworkInformationBackend::~QNetworkInformationBackend() = default;

/*!
    \fn QNetworkInformationBackend::name()

    Backend name, return the same in
    QNetworkInformationBackendFactory::name().
*/

/*!
    \fn QNetworkInformation::Features QNetworkInformationBackend::featuresSupported()

    Features supported, return the same in
    QNetworkInformationBackendFactory::featuresSupported().
*/

/*!
    \fn void QNetworkInformationBackend::reachabilityChanged(NetworkInformation::Reachability reachability)

    You should not emit this signal manually, call setReachability()
    instead which will emit this signal when the value changes.

    \sa setReachability
*/

/*!
    \fn void QNetworkInformationBackend::setReachability(QNetworkInformation::Reachability reachability)

    Call this when reachability has changed. It will automatically
    emit reachabilityChanged().

    \sa setReachability
*/

/*!
    \class QNetworkInformationBackendFactory
    \internal (Semi-private)
    \brief QNetworkInformationBackendFactory provides the interface
    for creating instances of QNetworkInformationBackend.

    Deriving from and implementing this class will let you register
    your plugin with QNetworkInformation. It must provide some basic
    information for querying information about the backend, and must
    also create the backend if requested. If some pre-conditions for
    the backend is not met it must return \nullptr.
*/

/*!
    \internal
    Adds the factory to an internal list.
*/
QNetworkInformationBackendFactory::QNetworkInformationBackendFactory()
{
    QNetworkInformationPrivate::addToList(this);
}

/*!
    \internal
    Removes the factory from an internal list.
*/
QNetworkInformationBackendFactory::~QNetworkInformationBackendFactory()
{
    QNetworkInformationPrivate::removeFromList(this);
}

/*!
    \fn QString QNetworkInformationBackendFactory::name()

    Backend name, return the same in
    QNetworkInformationBackend::name().
*/

/*!
    \fn QNetworkInformation::Features QNetworkInformationBackendFactory::featuresSupported()

    Features supported, return the same in
    QNetworkInformationBackend::featuresSupported().
    The factory should not promise support for features that wouldn't
    be available after creating the backend.
*/

/*!
    \fn QNetworkInformationBackend *QNetworkInformationBackendFactory::create()

    Create and return an instance of QNetworkInformationBackend. It
    will be deallocated by QNetworkInformation on shutdown. If some
    precondition is not met, meaning the backend would not function
    correctly, then you must return \nullptr.
*/

/*!
    \class QNetworkInformation
    \inmodule QtNetwork
    \since 6.1
    \brief QNetworkInformation exposes various network information
    through native backends.

    QNetworkInformation provides a cross-platform interface to
    network-related information through plugins.

    Various plugins can have various functionality supported, and so
    you can load plugins based on which features are needed.

    In most cases, the recommended approach is to load the
    platform-specific backend by calling loadDefaultBackend(). This will
    automatically select the most appropriate backend available on the
    current platform and is suitable for the majority of applications.

    \snippet code/src_network_kernel_qnetworkinformation.cpp file

    For more advanced uses cases, developers may prefer to load a backend
    based on specific capabilities or preferences. loadBackendByFeatures()
    allows selecting a backend that supports a specific set of features,
    such as reporting the transport mediom or signal strength. Alternatively,
    loadBackendByName() allows loading a plugin by its name, which can include
    platform-specific or custom backend implementations.

    QNetworkInformation is a singleton and stays alive from the first
    successful load until destruction of the QCoreApplication object.
    If you destroy and re-create the QCoreApplication object you must load it
    again to reinitialize the plugin.

    \note Because the class is a singleton while also relying on
    QCoreApplication, QNetworkInformation should always first be loaded
    in the same thread as the QCoreApplication object. This is because the
    object will also be destroyed in this thread, and various backend-specific
    components may rely on being destroyed in the same thread as it is created.

    One possible use case for QNetworkInformation is to monitor network
    connectivity status. reachability() provides an indication of whether the
    system is considered online, based on information reported by the
    underlying operating system or plugin. However, this information may not
    always be accurate. For example, on Windows, the online check may rely on
    connectivity to a Microsoft-owned server; if that server is unreachable
    (e.g., due to firewall rules), the system may incorrectly report that it is
    offline. As such, reachability() should not be used as a definitive
    pre-check before attempting a network connection, but rather as a general
    signal of connectivity state.

    To use reachability() effectively, the application must also understand
    what kind of destination it is trying to reach. For example, if the target
    is a local IP address, then Reachability::Local or Reachability::Site may
    be sufficient. If the destination is on the public internet, then
    Reachability::Online is required. Without this context, interpretring the
    reported reachability may lead to incorrect assumptions about actual
    network access.

    \warning Only Linux and Windows provide support for the finer-grained
    Reachability::Site and Reachability::Local options. On Android and Apple
    platforms reachability() is limited to reporting only Online, Offline or
    Unknown. Therefore, any logic that relies on detecting Local or Site level
    connectivity must include appropriate platform checks or fallbacks.

    \snippet code/src_network_kernel_qnetworkinformation_reachability.cpp 0
    \dots
    \snippet code/src_network_kernel_qnetworkinformation_reachability.cpp 1
    \dots

    \sa QNetworkInformation::Feature
*/

/*!
    \enum QNetworkInformation::Feature

    Lists all of the features that a plugin may currently support.
    This can be used in QNetworkInformation::loadBackendByFeatures().

    \value Reachability
        If the plugin supports this feature then the \c reachability property
        will provide useful results. Otherwise it will always return
        \c{Reachability::Unknown}.
        See also QNetworkInformation::Reachability.

    \value CaptivePortal
        If the plugin supports this feature then the \c isBehindCaptivePortal
        property will provide useful results. Otherwise it will always return
        \c{false}.

    \value TransportMedium
        If the plugin supports this feature then the \c transportMedium
        property will provide useful results. Otherwise it will always return
        \c{TransportMedium::Unknown}.
        See also QNetworkInformation::TransportMedium.

    \value Metered
        If the plugin supports this feature then the \c isMetered
        property will provide useful results. Otherwise it will always return
        \c{false}.
*/

/*!
    \enum QNetworkInformation::Reachability

    \value Unknown
        If this value is returned then we may be connected but the OS
        has still not confirmed full connectivity, or this feature
        is not supported.
    \value Disconnected
        Indicates that the system may have no connectivity at all.
    \value Local
        Indicates that the system is connected to a network, but it
        might only be able to access devices on the local network.
    \value Site
        Indicates that the system is connected to a network, but it
        might only be able to access devices on the local subnet or an
        intranet.
    \value Online
        Indicates that the system is connected to a network and
        able to access the Internet.

    \sa QNetworkInformation::reachability
*/

/*!
    \enum QNetworkInformation::TransportMedium
    \since 6.3

    Lists the currently recognized media with which one can connect to the
    internet.

    \value Unknown
        Returned if either the OS reports no active medium, the active medium is
        not recognized by Qt, or the TransportMedium feature is not supported.
    \value Ethernet
        Indicates that the currently active connection is using ethernet.
        Note: This value may also be returned when Windows is connected to a
        Bluetooth personal area network.
    \value Cellular
        Indicates that the currently active connection is using a cellular
        network.
    \value WiFi
        Indicates that the currently active connection is using Wi-Fi.
    \value Bluetooth
        Indicates that the currently active connection is connected using
        Bluetooth.

    \sa QNetworkInformation::transportMedium
*/

/*!
    \internal ctor
*/
QNetworkInformation::QNetworkInformation(QNetworkInformationBackend *backend)
    : QObject(*(new QNetworkInformationPrivate(backend)))
{
    connect(backend, &QNetworkInformationBackend::reachabilityChanged, this,
            &QNetworkInformation::reachabilityChanged);
    connect(backend, &QNetworkInformationBackend::behindCaptivePortalChanged, this,
            &QNetworkInformation::isBehindCaptivePortalChanged);
    connect(backend, &QNetworkInformationBackend::transportMediumChanged, this,
            &QNetworkInformation::transportMediumChanged);
    connect(backend, &QNetworkInformationBackend::isMeteredChanged, this,
           &QNetworkInformation::isMeteredChanged);

    QThread *main = nullptr;

    if (QCoreApplication::instance())
        main = QCoreApplication::instance()->thread();

    if (main && thread() != main)
        moveToThread(main);
}

/*!
    \internal dtor
*/
QNetworkInformation::~QNetworkInformation() = default;

/*!
    \property QNetworkInformation::reachability
    \brief The current state of the system's network connectivity.

    Indicates the level of connectivity that can be expected. Do note
    that this is only based on what the plugin/operating system
    reports. In certain scenarios this is known to be wrong. For
    example, on Windows the 'Online' check, by default, is performed
    by Windows connecting to a Microsoft-owned server. If this server
    is for any reason blocked then it will assume it does not have
    Online reachability. Because of this you should not use this as a
    pre-check before attempting to make a connection.
*/

QNetworkInformation::Reachability QNetworkInformation::reachability() const
{
    return d_func()->backend->reachability();
}

/*!
    \property QNetworkInformation::isBehindCaptivePortal
    \brief Lets you know if the user's device is behind a captive portal.
    \since 6.2

    This property indicates if the user's device is currently known to be
    behind a captive portal. This functionality relies on the operating system's
    detection of captive portals and is not supported on systems that don't
    report this. On systems where this is not supported this will always return
    \c{false}.
*/
bool QNetworkInformation::isBehindCaptivePortal() const
{
    return d_func()->backend->behindCaptivePortal();
}

/*!
    \property QNetworkInformation::transportMedium
    \brief The currently active transport medium for the application
    \since 6.3

    This property returns the currently active transport medium for the
    application, on operating systems where such information is available.

    When the current transport medium changes a signal is emitted, this can,
    for instance, occur when a user leaves the range of a WiFi network, unplugs
    their ethernet cable or enables Airplane mode.
*/
QNetworkInformation::TransportMedium QNetworkInformation::transportMedium() const
{
    return d_func()->backend->transportMedium();
}

/*!
    \property QNetworkInformation::isMetered
    \brief Check if the current connection is metered
    \since 6.3

    This property returns whether the current connection is (known to be)
    metered or not. You can use this as a guiding factor to decide whether your
    application should perform certain network requests or uploads.
    For instance, you may not want to upload logs or diagnostics while this
    property is \c true.

    \snippet code/src_network_kernel_qnetworkinformation_metering.cpp 0
    \dots
    \snippet code/src_network_kernel_qnetworkinformation_metering.cpp 1
    \dots
    \snippet code/src_network_kernel_qnetworkinformation_metering.cpp 2
    \dots
    \snippet code/src_network_kernel_qnetworkinformation_metering.cpp 3
*/
bool QNetworkInformation::isMetered() const
{
    return d_func()->backend->isMetered();
}

/*!
    Returns the name of the currently loaded backend.
*/
QString QNetworkInformation::backendName() const
{
    return d_func()->backend->name();
}

/*!
    Returns \c true if the currently loaded backend supports
    \a features.
*/
bool QNetworkInformation::supports(Features features) const
{
    return (d_func()->backend->featuresSupported() & features) == features;
}

/*!
    \since 6.3

    Returns all the supported features of the current backend.
*/
QNetworkInformation::Features QNetworkInformation::supportedFeatures() const
{
    return d_func()->backend->featuresSupported();
}

/*!
    \since 6.3

    Attempts to load the platform-default backend.

    \note Starting with 6.7 this tries to load any backend that supports
    \l{QNetworkInformation::Feature::Reachability}{Reachability} if the
    platform-default backend is not available or fails to load.
    If this also fails it will fall back to a backend that only returns
    the default values for all properties.

    This platform-to-plugin mapping is as follows:

    \table
    \header
        \li Platform
        \li Plugin-name
    \row
        \li Windows
        \li networklistmanager
    \row
        \li Apple (macOS/iOS)
        \li applenetworkinformation
    \row
        \li Android
        \li android
    \row
        \li Linux
        \li networkmanager
    \endtable

    This function is provided for convenience where the logic earlier
    is good enough. If you require a specific plugin then you should call
    loadBackendByName() or loadBackendByFeatures() directly instead.

    Determines a suitable backend to load and returns \c true if this backend
    is already loaded or on successful loading of it. Returns \c false if any
    other backend has already been loaded, or if loading of the selected
    backend fails.

    \sa instance(), loadBackendByName(), loadBackendByFeatures()
*/
bool QNetworkInformation::loadDefaultBackend()
{
    int index = -1;
#ifdef Q_OS_WIN
    index = QNetworkInformationBackend::PluginNamesWindowsIndex;
#elif defined(Q_OS_DARWIN)
    index = QNetworkInformationBackend::PluginNamesAppleIndex;
#elif defined(Q_OS_ANDROID)
    index = QNetworkInformationBackend::PluginNamesAndroidIndex;
#elif defined(Q_OS_LINUX)
    index = QNetworkInformationBackend::PluginNamesLinuxIndex;
#endif
    if (index != -1 && loadBackendByName(QNetworkInformationBackend::PluginNames[index]))
        return true;
    // We assume reachability is the most commonly wanted feature, and try to
    // load the backend that advertises the most features including that:
    if (loadBackendByFeatures(Feature::Reachability))
        return true;

    // Fall back to the dummy backend
    return loadBackendByName(u"dummy");
}

/*!
    \since 6.4

    Attempts to load a backend whose name matches \a backend
    (case insensitively).

    Returns \c true if it managed to load the requested backend or
    if it was already loaded. Returns \c false otherwise.

    \sa instance
*/
bool QNetworkInformation::loadBackendByName(QStringView backend)
{
    if (backend == u"dummy")
        return QNetworkInformationPrivate::createDummy() != nullptr;

    auto loadedBackend = QNetworkInformationPrivate::create(backend);
    return loadedBackend && loadedBackend->backendName().compare(backend, Qt::CaseInsensitive) == 0;
}

#if QT_DEPRECATED_SINCE(6,4)
/*!
    \deprecated [6.4] Use loadBackendByName() instead.

    \sa loadBackendByName(), loadDefaultBackend(), loadBackendByFeatures()
*/
bool QNetworkInformation::load(QStringView backend)
{
    return loadBackendByName(backend);
}
#endif // QT_DEPRECATED_SINCE(6,4)

/*!
    \since 6.4
    Load a backend which supports \a features.

    Returns \c true if it managed to load the requested backend or
    if it was already loaded. Returns \c false otherwise.

    \sa instance
*/
bool QNetworkInformation::loadBackendByFeatures(Features features)
{
    auto loadedBackend = QNetworkInformationPrivate::create(features);
    return loadedBackend && loadedBackend->supports(features);
}

#if QT_DEPRECATED_SINCE(6,4)
/*!
    \deprecated [6.4] Use loadBackendByFeatures() instead.

    \sa loadBackendByName(), loadDefaultBackend(), loadBackendByFeatures()
*/
bool QNetworkInformation::load(Features features)
{
    return loadBackendByFeatures(features);
}
#endif // QT_DEPRECATED_SINCE(6,4)

/*!
    Returns a list of the names of all currently available backends.
*/
QStringList QNetworkInformation::availableBackends()
{
    return QNetworkInformationPrivate::backendNames();
}

/*!
    Returns a pointer to the instance of the QNetworkInformation,
    if any. If this method is called before a backend is loaded,
    it returns a null pointer.

    \sa loadBackendByName(), loadDefaultBackend(), loadBackendByFeatures()
*/
QNetworkInformation *QNetworkInformation::instance()
{
    return QNetworkInformationPrivate::instance();
}

QT_END_NAMESPACE

#include "moc_qnetworkinformation.cpp"
#include "moc_qnetworkinformation_p.cpp"
#include "qnetworkinformation.moc"
