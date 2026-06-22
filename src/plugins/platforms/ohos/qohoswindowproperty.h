// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSWINDOWPROPERTY_H
#define QOHOSWINDOWPROPERTY_H

#include <qohosplugincore.h>
#include <qohosutils.h>
#include <QtCore/qpointer.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtGui/qwindow.h>
#include <memory>
#include <utility>

QT_BEGIN_NAMESPACE

template<typename T>
class QOhosPropertyDescriptor
{
public:
    constexpr QOhosPropertyDescriptor();
};

template<typename T>
constexpr QOhosPropertyDescriptor<T>::QOhosPropertyDescriptor()
{
    qMetaTypeId<T>();
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
void setQOhosPropertyOnQObject(QObject *qObject, T propertyValue);

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
QOhosOptional<T> tryGetQOhosPropertyFromQObject(QObject *qObject);

class QOhosPropertiesStore
{
public:
    explicit QOhosPropertiesStore(QObject *qObject);

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    QOhosOptional<T> tryGetProperty() const;

    void notifyPropertyWrite(const QByteArray &propertyName);

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    std::shared_ptr<void> addPropertyWriteCallback(QOhosConsumer<T> propertyWriteCallback);

private:
    template<typename T>
    class PropertyWriteConsumersStore
    {
    public:
        void notify(T value);
        std::shared_ptr<void> addConsumer(QOhosConsumer<T> consumer);

    private:
        std::vector<std::weak_ptr<QOhosConsumer<T>>> m_consumers;
    };

    QObject *objectOrFail() const;

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    void notifyPropertyWriteInternal();

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    std::shared_ptr<PropertyWriteConsumersStore<T>> getOrCreatePropertyWriteConsumersStore();

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    std::shared_ptr<PropertyWriteConsumersStore<T>> getPropertyWriteConsumersStoreOrNull();

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    void registerPropertyNotifierIfMissing();

    QPointer<QObject> m_object;
    std::map<const void *, std::shared_ptr<void>> m_propertyWriteConsumersStoresMap;
    std::map<QByteArray, std::function<void()>> m_propertyNameToNotifierMap;
};

class QOhosPropertiesProvider
{
public:
    explicit QOhosPropertiesProvider(QOhosPropertiesStore &store);

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    QOhosOptional<T> tryGetProperty() const;

    template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
    std::shared_ptr<void> addPropertyWriteCallback(QOhosConsumer<T> propertyWriteCallback);

private:
    QOhosPropertiesStore *m_store;
};

namespace qohoswindowproperty_h_detail {

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
const char *qObjectOhosPropertyName();

template<typename T>
QOhosOptional<T> tryMapFromQVariant(QVariant variant);

}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
void setQOhosPropertyOnQObject(QObject *qObject, T propertyValue)
{
    qObject->setProperty(
        qohoswindowproperty_h_detail::qObjectOhosPropertyName<T, propertyPtr>(),
        QVariant::fromValue(propertyValue));
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
QOhosOptional<T> tryGetQOhosPropertyFromQObject(QObject *qObject)
{
    const char *propertyName = qohoswindowproperty_h_detail::qObjectOhosPropertyName<T, propertyPtr>();
    const auto propertyTypeId = qMetaTypeId<T>();
    auto value = qObject->property(propertyName);

    if (!value.isValid())
        return makeEmptyQOhosOptional();

    auto optValue = qohoswindowproperty_h_detail::tryMapFromQVariant<T>(value);
    if (!optValue.has_value()) {
        qOhosPrintfError(
            "Property \"%s\" type mismatch, expected: %d, got: %d",
            propertyName, propertyTypeId, value.userType());
    }

    return optValue;
}

inline QOhosPropertiesStore::QOhosPropertiesStore(QObject *qObject)
    : m_object(qObject)
{
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
QOhosOptional<T> QOhosPropertiesStore::tryGetProperty() const
{
    return tryGetQOhosPropertyFromQObject<T, propertyPtr>(objectOrFail());
}

inline void QOhosPropertiesStore::notifyPropertyWrite(const QByteArray &propertyName)
{
    auto notifierIt = m_propertyNameToNotifierMap.find(propertyName);
    if (notifierIt != m_propertyNameToNotifierMap.end())
        notifierIt->second();
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
std::shared_ptr<void> QOhosPropertiesStore::addPropertyWriteCallback(QOhosConsumer<T> propertyWriteCallback)
{
    registerPropertyNotifierIfMissing<T, propertyPtr>();
    return getOrCreatePropertyWriteConsumersStore<T, propertyPtr>()->addConsumer(std::move(propertyWriteCallback));
}

template<typename T>
void QOhosPropertiesStore::PropertyWriteConsumersStore<T>::notify(T value)
{
    auto weakConsumersPendingNotify = std::exchange(m_consumers, {});
    std::vector<std::weak_ptr<QOhosConsumer<T>>> weakConsumersToAdd;
    for (auto &weakConsumerPendingNotify: weakConsumersPendingNotify) {
        auto sharedConsumer = weakConsumerPendingNotify.lock();
        if (sharedConsumer) {
            weakConsumersToAdd.push_back(weakConsumerPendingNotify);
            (*sharedConsumer)(value);
        }
    }
    m_consumers.insert(m_consumers.begin(), weakConsumersToAdd.begin(), weakConsumersToAdd.end());
}

template<typename T>
std::shared_ptr<void> QOhosPropertiesStore::PropertyWriteConsumersStore<T>::addConsumer(QOhosConsumer<T> consumer)
{
    auto sharedConsumer = QtOhos::moveToSharedPtr(std::move(consumer));
    m_consumers.emplace_back(sharedConsumer);
    return sharedConsumer;
}

inline QObject *QOhosPropertiesStore::objectOrFail() const
{
    QObject *object = m_object;
    if (object == nullptr)
        qOhosReportFatalErrorAndAbort("%s: m_object was null", Q_FUNC_INFO);
    return object;
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
void QOhosPropertiesStore::notifyPropertyWriteInternal()
{
    auto propertyWriteConsumersStore = getPropertyWriteConsumersStoreOrNull<T, propertyPtr>();
    if (!propertyWriteConsumersStore)
        return;

    auto propertyValue = tryGetProperty<T, propertyPtr>();
    if (propertyValue.has_value())
        propertyWriteConsumersStore->notify(propertyValue.value());
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
std::shared_ptr<QOhosPropertiesStore::PropertyWriteConsumersStore<T>> QOhosPropertiesStore::getOrCreatePropertyWriteConsumersStore()
{
    auto propertyWriteConsumersStore = getPropertyWriteConsumersStoreOrNull<T, propertyPtr>();
    if (!propertyWriteConsumersStore) {
        propertyWriteConsumersStore = std::make_shared<PropertyWriteConsumersStore<T>>();
        m_propertyWriteConsumersStoresMap.insert({propertyPtr, propertyWriteConsumersStore});
    }

    return propertyWriteConsumersStore;
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
std::shared_ptr<QOhosPropertiesStore::PropertyWriteConsumersStore<T>> QOhosPropertiesStore::getPropertyWriteConsumersStoreOrNull()
{
    auto propertyWriteConsumersStoresMapIt = m_propertyWriteConsumersStoresMap.find(propertyPtr);
    if (propertyWriteConsumersStoresMapIt == m_propertyWriteConsumersStoresMap.end())
        return nullptr;
    return std::static_pointer_cast<PropertyWriteConsumersStore<T>>(propertyWriteConsumersStoresMapIt->second);
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
void QOhosPropertiesStore::registerPropertyNotifierIfMissing()
{
    auto propertyName = QByteArray(qohoswindowproperty_h_detail::qObjectOhosPropertyName<T, propertyPtr>());
    auto propertyNotifierIt = m_propertyNameToNotifierMap.find(propertyName);
    if (propertyNotifierIt == m_propertyNameToNotifierMap.end()) {
        m_propertyNameToNotifierMap.emplace(
            propertyName,
            [this]() {
                this->notifyPropertyWriteInternal<T, propertyPtr>();
            });
    }
}

inline QOhosPropertiesProvider::QOhosPropertiesProvider(QOhosPropertiesStore &store)
    : m_store(&store)
{
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
QOhosOptional<T> QOhosPropertiesProvider::tryGetProperty() const
{
    return m_store->tryGetProperty<T, propertyPtr>();
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
std::shared_ptr<void> QOhosPropertiesProvider::addPropertyWriteCallback(QOhosConsumer<T> propertyWriteCallback)
{
    return m_store->addPropertyWriteCallback<T, propertyPtr>(std::move(propertyWriteCallback));
}

namespace qohoswindowproperty_h_detail {

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
std::string makeQObjectOhosPropertyName()
{
    return QtOhos::printfToString(
        "_q_platform_ohos_%d_%p", qMetaTypeId<T>(), static_cast<const void *>(propertyPtr));
}

template<typename T, const QOhosPropertyDescriptor<T> *propertyPtr>
const char *qObjectOhosPropertyName()
{
    static const std::string propertyName = makeQObjectOhosPropertyName<T, propertyPtr>();
    return propertyName.c_str();
}

template<typename T>
QOhosOptional<T> tryMapFromQVariant(QVariant variant)
{
    return qMetaTypeId<T>() == variant.userType()
        ? makeQOhosOptional(variant.value<T>())
        : makeEmptyQOhosOptional();
}

template<>
inline QOhosOptional<QWindow *> tryMapFromQVariant<QWindow *>(QVariant variant)
{
    if (!variant.canConvert<QWindow *>())
        return makeEmptyQOhosOptional();
    return !variant.isNull()
        ? makeQOhosOptional(reinterpret_cast<QWindow *>(variant.value<QObject*>()))
        : makeQOhosOptional<QWindow *>(nullptr);
}

}

QT_END_NAMESPACE

#endif
