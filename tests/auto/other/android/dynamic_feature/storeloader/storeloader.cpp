// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "storeloader.h"

#include <QtCore/private/qobject_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qhash.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qlogging.h>
#include <QtCore/qmutex.h>
#include <QtCore/quuid.h>

#include <jni.h>

Q_DECLARE_JNI_CLASS(StoreLoader,
                    "org/qtproject/example/android_dynamic_feature/StoreLoader");

class StoreLoaderHandlerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(StoreLoaderHandler)
public:
    void setState(StoreLoader::State state)
    {
        if (m_state == state)
            return;
        Q_Q(StoreLoaderHandler);
        m_state = state;
        q->stateChanged(m_state);
    }

    const QString &callId() const & noexcept { return m_callId; }

private:
    StoreLoader::State m_state = StoreLoader::State::Unknown;
    QString m_callId = QUuid::createUuid().toString();
};

namespace QtPrivate {
QString asString(const jstring &s)
{
    return QJniObject(s).toString();
}
} // namespace QtPrivate

namespace {

class StoreLoaderImpl
{
public:
    StoreLoaderImpl();
    bool registerNatives() const;

    void addHandler(StoreLoaderHandler *handler);
    StoreLoaderHandler *findHandler(const jstring &callId);
    void removeHandler(const jstring &callId);

    QtJniTypes::StoreLoader loader = nullptr;

private:
    QHash<QString, QPointer<StoreLoaderHandler>> m_handlers;
    QMutex m_lock;
};

Q_GLOBAL_STATIC(StoreLoaderImpl, loaderInstance)

void stateChangedNative(JNIEnv *, jobject, jstring callId, int state)
{
    qDebug("State changed %s.", qPrintable(callId));

    if (auto *handler = loaderInstance->findHandler(callId))
        emit handler->stateChanged(static_cast<StoreLoader::State>(state));
}

void errorOccurredNative(JNIEnv *, jobject, jstring callId, int errorCode, jstring errorMessage)
{
    qDebug("Error occurred %s %d %s.", qPrintable(callId), errorCode, qPrintable(errorMessage));
    auto *handler = loaderInstance->findHandler(callId);
    if (!handler)
        return;

    emit handler->errorOccured(errorCode, QJniObject(errorMessage).toString());
}

void userConfirmationRequestedNative(JNIEnv *, jobject, jstring callId, int errorCode,
                                     jstring errorMessage)
{
    qDebug("User confirmation requested %s %d %s.", qPrintable(callId), errorCode,
        qPrintable(errorMessage));
    auto *handler = loaderInstance->findHandler(callId);
    if (!handler)
        return;

    emit handler->confirmationRequest(errorCode, QJniObject(errorMessage).toString());
}

void downloadProgressChangedNative(JNIEnv *, jobject, jstring callId, jlong bytes, jlong total)
{
    qDebug() << "Download progress changed" << bytes << "/" << total;
    auto *handler = loaderInstance->findHandler(callId);
    if (!handler)
        return;

    emit handler->downloadProgress(bytes, total);
}

void finishedNative(JNIEnv *, jobject, jstring callId)
{
    auto *handler = loaderInstance->findHandler(callId);
    if (!handler)
        return;

    emit handler->finished();
}

} // namespace

Q_DECLARE_JNI_NATIVE_METHOD(stateChangedNative)
Q_DECLARE_JNI_NATIVE_METHOD(errorOccurredNative)
Q_DECLARE_JNI_NATIVE_METHOD(userConfirmationRequestedNative)
Q_DECLARE_JNI_NATIVE_METHOD(downloadProgressChangedNative)
Q_DECLARE_JNI_NATIVE_METHOD(finishedNative)

StoreLoaderImpl::StoreLoaderImpl()
{
    loader = QJniObject::construct<QtJniTypes::StoreLoader, QtJniTypes::Context>(
            QNativeInterface::QAndroidApplication::context());
}

bool StoreLoaderImpl::registerNatives() const
{
    static bool result = [] {
        return QtJniTypes::StoreLoader::registerNativeMethods({
                Q_JNI_NATIVE_METHOD(stateChangedNative),
                Q_JNI_NATIVE_METHOD(errorOccurredNative),
                Q_JNI_NATIVE_METHOD(userConfirmationRequestedNative),
                Q_JNI_NATIVE_METHOD(downloadProgressChangedNative),
                Q_JNI_NATIVE_METHOD(finishedNative),
        });
    }();

    if (!result)
        qCritical("Unable to register native methods.");

    return result;
}

void StoreLoaderImpl::addHandler(StoreLoaderHandler *handler)
{
    Q_ASSERT(handler);

    QMutexLocker lock(&m_lock);
    const auto &callId = handler->callId();
    Q_ASSERT_X(m_handlers.constFind(callId) != m_handlers.constEnd(), "StoreLoaderImpl::addHandler",
               qPrintable(QString("Handler with callId %1 already exists.").arg(callId)));

    m_handlers[callId] = QPointer(handler);
}


StoreLoaderHandler *StoreLoaderImpl::findHandler(const jstring &callId)
{
    QMutexLocker lock(&m_lock);
    const auto it = m_handlers.constFind(QJniObject(callId).toString());
    if (it == m_handlers.constEnd()) {
        qCritical("The handler for the call %s was not found.", qPrintable(callId));
        return nullptr;
    }

    if (it.value().isNull()) {
        qCritical("The handler for the call %s expired.", qPrintable(callId));
        m_handlers.erase(it);
    }

    return it.value().get();
}

void StoreLoaderImpl::removeHandler(const jstring &callId)
{
    QMutexLocker lock(&m_lock);
    m_handlers.remove(QJniObject(callId).toString());
}

std::unique_ptr<StoreLoaderHandler>
StoreLoader::loadModule(const QString &moduleName)
{
    if (moduleName.isEmpty())
        return {};

    if (!loaderInstance->registerNatives())
        return {};

    if (!loaderInstance->loader.isValid()) {
        qCritical("StoreLoader not constructed");
        return {};
    }

    auto handlerPtr = std::make_unique<StoreLoaderHandler>(
            nullptr, StoreLoaderHandler::PrivateConstructor{});
    loaderInstance->addHandler(handlerPtr.get());

    const auto &callId = handlerPtr->callId();
    qDebug("Loading module %s, callId: %s.", qPrintable(moduleName), qPrintable(callId));

    loaderInstance->loader.callMethod<void>("installModuleFromStore", moduleName, callId);
    return handlerPtr;
}

StoreLoaderHandler::StoreLoaderHandler(QObject *parent, PrivateConstructor)
    : QObject(*new StoreLoaderHandlerPrivate(), parent)
{
}

StoreLoaderHandler::~StoreLoaderHandler() = default;

const QString &StoreLoaderHandler::callId() const & noexcept
{
    Q_D(const StoreLoaderHandler);
    return d->callId();
}

void StoreLoaderHandler::cancel()
{
    Q_D(StoreLoaderHandler);
    loaderInstance->loader.callMethod<void>("cancelInstall", d->callId());
}

#include "moc_storeloader.cpp"
