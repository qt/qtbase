/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
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

    auto needsReinvoke = instance->thread() && instance->thread() != QThread::currentThread();
    if (needsReinvoke) {
        QMetaObject::invokeMethod(dataHolder->instanceHolder.get(), []() { networkInfoCleanup(); });
        return;
    }
    dataHolder->instanceHolder.reset();
}

class QNetworkInformationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QNetworkInformation)
public:
    QNetworkInformationPrivate(QNetworkInformationBackend *backend) : backend(backend) {
        qAddPostRoutine(&networkInfoCleanup);
     }

    static QNetworkInformation *create(QNetworkInformation::Features features);
    static QNetworkInformation *create(QStringView name);
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
    if (!loader())
        return false;
    if (!dataHolder())
        return false;
    static QBasicMutex mutex;
    QMutexLocker initLocker(&mutex);

#if QT_CONFIG(library)
    loader->update();
#endif
    // Instantiates the plugins (and registers the factories)
    int index = 0;
    while (loader->instance(index))
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
            for (const auto *factory : qAsConst(dataHolder->factories))
                listNames += factory->name() + QStringLiteral(", ");
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
        return factory && (factory->featuresSupported() & features) == features;
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
                for (const auto *factory : qAsConst(dataHolder->factories))
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
    \fn void QNetworkInformationBackend::reachabilityChanged()

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
    \since 6.1
    \brief QNetworkInformation exposes various network information
    through native backends.

    QNetworkInformation provides a cross-platform interface to
    network-related information through plugins.

    Various plugins can have various functionality supported, and so
    you can load() plugins based on which features are needed.

    QNetworkInformation is a singleton and stays alive from the first
    successful load() until destruction of the QCoreApplication object.
    If you destroy and re-create the QCoreApplication object you must call
    load() again.

    \sa QNetworkInformation::Feature
*/

/*!
    \enum QNetworkInformation::Feature

    Lists all of the features that a plugin may currently support.
    This can be used in QNetworkInformation::load().

    \value Reachability
        If the plugin supports this feature then the \c reachability property
        will provide useful results. Otherwise it will always return
        \c{Reachability::Unknown}.
        See also QNetworkInformation::Reachability.

    \value CaptivePortal
        If the plugin supports this feature then the \c isBehindCaptivePortal
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
    \internal ctor
*/
QNetworkInformation::QNetworkInformation(QNetworkInformationBackend *backend)
    : QObject(*(new QNetworkInformationPrivate(backend)))
{
    connect(backend, &QNetworkInformationBackend::reachabilityChanged, this,
            [this]() { emit reachabilityChanged(d_func()->backend->reachability()); });
    connect(backend, &QNetworkInformationBackend::behindCaptivePortalChanged, this, [this]() {
        emit isBehindCaptivePortalChanged(d_func()->backend->behindCaptivePortal());
    });
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
    Attempts to load a backend whose name matches \a backend
    (case insensitively).

    Returns \c true if it managed to load the requested backend or
    if it was already loaded. Returns \c false otherwise

    \sa instance
*/
bool QNetworkInformation::load(QStringView backend)
{
    auto loadedBackend = QNetworkInformationPrivate::create(backend);
    return loadedBackend && loadedBackend->backendName().compare(backend, Qt::CaseInsensitive) == 0;
}

/*!
    Load a backend which supports \a features.

    Returns \c true if it managed to load the requested backend or
    if it was already loaded. Returns \c false otherwise

    \sa instance
*/
bool QNetworkInformation::load(Features features)
{
    auto loadedBackend = QNetworkInformationPrivate::create(features);
    return loadedBackend && loadedBackend->supports(features);
}

/*!
    Returns a list of the names of all currently available backends.
*/
QStringList QNetworkInformation::availableBackends()
{
    return QNetworkInformationPrivate::backendNames();
}

/*!
    Returns a pointer to the instance of the QNetworkInformation,
    if any.

    \sa load()
*/
QNetworkInformation *QNetworkInformation::instance()
{
    return QNetworkInformationPrivate::instance();
}

QT_END_NAMESPACE
