// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qandroiditemmodelproxy_p.h>
#include <QtCore/private/qandroidmodelindexproxy_p.h>
#include <QtCore/private/qandroidtypeconverter_p.h>

#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

using namespace QtJniTypes;

std::map<const QAbstractItemModel *, QRecursiveMutex> QAndroidItemModelProxy::s_mutexes =
        std::map<const QAbstractItemModel *, QRecursiveMutex>{};

jint QAndroidItemModelProxy::columnCount(const QModelIndex &parent) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    auto parentIndex = QAndroidModelIndexProxy::jInstance(parent);
    return jInstance.callMethod<jint>("columnCount", parentIndex);
}

bool QAndroidItemModelProxy::canFetchMore(const QModelIndex &parent) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    auto parentIndex = QAndroidModelIndexProxy::jInstance(parent);
    return jInstance.callMethod<jboolean>("canFetchMore", parentIndex);
}

bool QAndroidItemModelProxy::canFetchMoreDefault(const QModelIndex &parent) const
{
    return QAbstractItemModel::canFetchMore(parent);
}

QVariant QAndroidItemModelProxy::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    auto jIndex = QAndroidModelIndexProxy::jInstance(index);
    QJniObject jData = jInstance.callMethod<jobject>("data", jIndex, role);
    return QAndroidTypeConverter::toQVariant(jData);
}

QModelIndex QAndroidItemModelProxy::index(int row, int column, const QModelIndex &parent) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    JQtModelIndex jIndex = jInstance.callMethod<JQtModelIndex>(
            "index", row, column, QAndroidModelIndexProxy::jInstance(parent));
    return QAndroidModelIndexProxy::qInstance(jIndex);
}

QModelIndex QAndroidItemModelProxy::parent(const QModelIndex &index) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);

    auto jIndex = QAndroidModelIndexProxy::jInstance(index);
    return QAndroidModelIndexProxy::qInstance(
            jInstance.callMethod<JQtModelIndex>("parent", jIndex));
}
int QAndroidItemModelProxy::rowCount(const QModelIndex &parent) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);

    auto parentIndex = QAndroidModelIndexProxy::jInstance(parent);
    return jInstance.callMethod<int>("rowCount", parentIndex);
}

QHash<int, QByteArray> QAndroidItemModelProxy::roleNames() const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);

    QHash<int, QByteArray> roleNames;
    HashMap hashMap = jInstance.callMethod<HashMap>("roleNames");
    Set set = hashMap.callMethod<Set>("keySet");
    const QJniArray keyArray = set.callMethod<jobject[]>("toArray");

    for (const auto &key : keyArray) {
        const QJniObject roleName = hashMap.callMethod<jobject>("get", key);
        const int intKey = QJniObject(key).callMethod<jint>("intValue");
        const QByteArray roleByteArray = String(roleName).toString().toLatin1();
        roleNames.insert(intKey, roleByteArray);
    }
    return roleNames;
}

QHash<int, QByteArray> QAndroidItemModelProxy::defaultRoleNames() const
{
    return QAbstractItemModel::roleNames();
}

void QAndroidItemModelProxy::fetchMore(const QModelIndex &parent)
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    auto parentIndex = QAndroidModelIndexProxy::jInstance(parent);
    jInstance.callMethod<void>("fetchMore", parentIndex);
}

void QAndroidItemModelProxy::fetchMoreDefault(const QModelIndex &parent)
{
    QAbstractItemModel::fetchMore(parent);
}

bool QAndroidItemModelProxy::hasChildren(const QModelIndex &parent) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    auto parentIndex = QAndroidModelIndexProxy::jInstance(parent);
    return jInstance.callMethod<jboolean>("hasChildren", parentIndex);
}

bool QAndroidItemModelProxy::hasChildrenDefault(const QModelIndex &parent) const
{
    return QAbstractItemModel::hasChildren(parent);
}

QModelIndex QAndroidItemModelProxy::sibling(int row, int column, const QModelIndex &parent) const
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    return QAndroidModelIndexProxy::qInstance(jInstance.callMethod<JQtModelIndex>(
            "sibling", row, column, QAndroidModelIndexProxy::jInstance(parent)));
}

QModelIndex QAndroidItemModelProxy::siblingDefault(int row, int column, const QModelIndex &parent)
{
    return QAbstractItemModel::sibling(row, column, parent);
}

bool QAndroidItemModelProxy::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_ASSERT(jInstance.isValid());
    const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(this);
    auto jIndex = QAndroidModelIndexProxy::jInstance(index);
    auto jValue = QAndroidTypeConverter::toJavaObject(value, QJniEnvironment::getJniEnv());
    return jInstance.callMethod<jboolean>("setData", jIndex, jValue, role);
}

bool QAndroidItemModelProxy::setDataDefault(const QModelIndex &index, const QVariant &value,
                                            int role)
{
    return QAbstractItemModel::setData(index, value, role);
}

Q_REQUIRED_RESULT QAbstractItemModel *
QAndroidItemModelProxy::nativeInstance(JQtAbstractItemModel itemModel)
{
    jlong nativeReference = itemModel.callMethod<jlong>("nativeReference");
    return reinterpret_cast<QAbstractItemModel *>(nativeReference);
}

Q_REQUIRED_RESULT QAbstractItemModel *
QAndroidItemModelProxy::createNativeProxy(QJniObject itemModel)
{
    QAbstractItemModel *nativeProxy = nativeInstance(itemModel);
    if (!nativeProxy) {
        Q_ASSERT(QCoreApplication::instance());

        nativeProxy = new QAndroidItemModelProxy(itemModel);
        QThread *qtMainThread = QCoreApplication::instance()->thread();
        if (nativeProxy->thread() != qtMainThread)
            nativeProxy->moveToThread(qtMainThread);

        itemModel.callMethod<void>("setNativeReference", reinterpret_cast<jlong>(nativeProxy));
        connect(nativeProxy, &QAndroidItemModelProxy::destroyed, nativeProxy, [](QObject *obj) {
            auto proxy = qobject_cast<QAndroidItemModelProxy *>(obj);
            if (proxy) {
                const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(proxy);
                proxy->jInstance.callMethod<void>("detachFromNative");
            }
        });

        connect(nativeProxy, &QAndroidItemModelProxy::dataChanged, nativeProxy,
                [nativeProxy](const QModelIndex &topLeft, const QModelIndex &bottomRight,
                              const QList<int> &roles) {
                    auto proxy = qobject_cast<QAndroidItemModelProxy *>(nativeProxy);
                    if (proxy) {
                        QJniObject jInstance = proxy->jInstance;
                        const QMutexLocker<QRecursiveMutex> lock = getMutexLocker(proxy);
                        jInstance.callMethod<void>("handleDataChanged",
                                                    QAndroidModelIndexProxy::jInstance(topLeft),
                                                    QAndroidModelIndexProxy::jInstance(bottomRight),
                                                    QJniArray<jint>(roles));
                    }
                });
    }
    return nativeProxy;
}

QJniObject QAndroidItemModelProxy::createProxy(QAbstractItemModel *itemModel)
{
    return JQtAndroidItemModelProxy(reinterpret_cast<jlong>(itemModel));
}

int QAndroidItemModelProxy::jni_columnCount(JNIEnv *env, jobject object, JQtModelIndex parent)
{
    const QModelIndex nativeParent = QAndroidModelIndexProxy::qInstance(parent);
    return invokeNativeMethod(env, object, &QAbstractItemModel::columnCount, nativeParent);
}

jobject QAndroidItemModelProxy::jni_data(JNIEnv *env, jobject object, JQtModelIndex index,
                                         jint role)
{
    const QModelIndex nativeIndex = QAndroidModelIndexProxy::qInstance(index);
    const QVariant data =
            invokeNativeMethod(env, object, &QAbstractItemModel::data, nativeIndex, role);
    return QAndroidTypeConverter::toJavaObject(data, env);
}

jobject QAndroidItemModelProxy::jni_index(JNIEnv *env, jobject object, jint row, jint column,
                                          JQtModelIndex parent)
{
    auto nativeParent = QAndroidModelIndexProxy::qInstance(parent);
    const QModelIndex modelIndex =
            invokeNativeMethod(env, object, &QAbstractItemModel::index, row, column, nativeParent);
    return env->NewLocalRef(QAndroidModelIndexProxy::jInstance(modelIndex).object());
}

jobject QAndroidItemModelProxy::jni_parent(JNIEnv *env, jobject object, JQtModelIndex index)
{
    const QModelIndex nativeIndex = QAndroidModelIndexProxy::qInstance(index);
    QModelIndex (QAbstractItemModel::*parentOverloadPtr)(const QModelIndex &) const =
            &QAbstractItemModel::parent;
    const QModelIndex parent = invokeNativeMethod(env, object, parentOverloadPtr, nativeIndex);
    return env->NewLocalRef(QAndroidModelIndexProxy::jInstance(parent).object());
}

jint QAndroidItemModelProxy::jni_rowCount(JNIEnv *env, jobject object, JQtModelIndex parent)
{
    return invokeNativeMethod(env, object, &QAbstractItemModel::rowCount,
                              QAndroidModelIndexProxy::qInstance(parent));
}

jobject QAndroidItemModelProxy::jni_roleNames(JNIEnv *env, jobject object)
{
    auto roleNames = invokeNativeImpl(env, object, &QAndroidItemModelProxy::defaultRoleNames,
                                      &QAbstractItemModel::roleNames);
    HashMap jRoleNames{};
    for (auto [role, roleName] : roleNames.asKeyValueRange()) {
        const Integer jRole(role);
        jRoleNames.callMethod<jobject>("put", jRole.object(), QString::fromUtf8(roleName));
    }
    return env->NewLocalRef(jRoleNames.object());
}

jobject QAndroidItemModelProxy::jni_createIndex(JNIEnv *env, jobject object, jint row, jint column,
                                                jlong id)
{
    QModelIndex (QAndroidItemModelProxy::*createIndexPtr)(int, int, quintptr) const =
            &QAndroidItemModelProxy::createIndex;
    const QModelIndex index =
            invokeNativeProxyMethod(env, object, createIndexPtr, row, column, quintptr(id));
    return env->NewLocalRef(QAndroidModelIndexProxy::jInstance(index).object());
}

jboolean QAndroidItemModelProxy::jni_canFetchMore(JNIEnv *env, jobject object, JQtModelIndex parent)
{
    return invokeNativeImpl(env, object, &QAndroidItemModelProxy::canFetchMoreDefault,
                            &QAbstractItemModel::canFetchMore,
                            QAndroidModelIndexProxy::qInstance(parent));
}

void QAndroidItemModelProxy::jni_fetchMore(JNIEnv *env, jobject object, JQtModelIndex parent)
{
    return invokeNativeImpl(env, object, &QAndroidItemModelProxy::fetchMoreDefault,
                            &QAbstractItemModel::fetchMore,
                            QAndroidModelIndexProxy::qInstance(parent));
}

jboolean QAndroidItemModelProxy::jni_hasChildren(JNIEnv *env, jobject object, JQtModelIndex parent)
{
    return invokeNativeImpl(env, object, &QAndroidItemModelProxy::hasChildrenDefault,
                            &QAbstractItemModel::hasChildren,
                            QAndroidModelIndexProxy::qInstance(parent));
}

jboolean QAndroidItemModelProxy::jni_hasIndex(JNIEnv *env, jobject object, jint row, jint column,
                                              JQtModelIndex parent)
{
    return invokeNativeMethod(env, object, &QAbstractItemModel::hasIndex, row, column,
                              QAndroidModelIndexProxy::qInstance(parent));
}

void QAndroidItemModelProxy::jni_beginInsertColumns(JNIEnv *env, jobject object,
                                                    JQtModelIndex parent, jint first, jint last)
{

    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::beginInsertColumns,
                            QAndroidModelIndexProxy::qInstance(parent), first, last);
}

void QAndroidItemModelProxy::jni_beginInsertRows(JNIEnv *env, jobject object, JQtModelIndex parent,
                                                 jint first, jint last)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::beginInsertRows,
                            QAndroidModelIndexProxy::qInstance(parent), first, last);
}

jboolean QAndroidItemModelProxy::jni_beginMoveColumns(JNIEnv *env, jobject object,
                                                      JQtModelIndex sourceParent, jint sourceFirst,
                                                      jint sourceLast,
                                                      JQtModelIndex destinationParent,
                                                      jint destinationChild)
{
    return invokeNativeProxyMethod(
            env, object, &QAndroidItemModelProxy::beginMoveColumns,
            QAndroidModelIndexProxy::qInstance(sourceParent), sourceFirst, sourceLast,
            QAndroidModelIndexProxy::qInstance(destinationParent), destinationChild);
}

jboolean QAndroidItemModelProxy::jni_beginMoveRows(JNIEnv *env, jobject object,
                                                   JQtModelIndex sourceParent, jint sourceFirst,
                                                   jint sourceLast, JQtModelIndex destinationParent,
                                                   jint destinationChild)
{
    return invokeNativeProxyMethod(
            env, object, &QAndroidItemModelProxy::beginMoveRows,
            QAndroidModelIndexProxy::qInstance(sourceParent), sourceFirst, sourceLast,
            QAndroidModelIndexProxy::qInstance(destinationParent), destinationChild);
}

void QAndroidItemModelProxy::jni_beginRemoveColumns(JNIEnv *env, jobject object,
                                                    JQtModelIndex parent, jint first, jint last)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::beginRemoveColumns,
                            QAndroidModelIndexProxy::qInstance(parent), first, last);
}

void QAndroidItemModelProxy::jni_beginRemoveRows(JNIEnv *env, jobject object, JQtModelIndex parent,
                                                 jint first, jint last)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::beginRemoveRows,
                            QAndroidModelIndexProxy::qInstance(parent), first, last);
}

void QAndroidItemModelProxy::jni_beginResetModel(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::beginResetModel);
}

void QAndroidItemModelProxy::jni_endInsertColumns(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::endInsertColumns);
}

void QAndroidItemModelProxy::jni_endInsertRows(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::endInsertRows);
}

void QAndroidItemModelProxy::jni_endMoveColumns(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::endMoveColumns);
}

void QAndroidItemModelProxy::jni_endMoveRows(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::endMoveRows);
}

void QAndroidItemModelProxy::jni_endRemoveColumns(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::endRemoveColumns);
}

void QAndroidItemModelProxy::jni_endRemoveRows(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::endRemoveRows);
}

void QAndroidItemModelProxy::jni_endResetModel(JNIEnv *env, jobject object)
{
    invokeNativeProxyMethod(env, object, &QAndroidItemModelProxy::endResetModel);
}

jobject QAndroidItemModelProxy::jni_sibling(JNIEnv *env, jobject object, jint row, jint column,
                                            JQtModelIndex parent)
{
    const QModelIndex index = invokeNativeImpl(env, object, &QAndroidItemModelProxy::siblingDefault,
                                               &QAbstractItemModel::sibling, row, column,
                                               QAndroidModelIndexProxy::qInstance(parent));
    return env->NewLocalRef(QAndroidModelIndexProxy::jInstance(index).object());
}

jboolean QAndroidItemModelProxy::jni_setData(JNIEnv *env, jobject object, JQtModelIndex index,
                                             jobject value, jint role)
{
    const QModelIndex nativeIndex = QAndroidModelIndexProxy::qInstance(index);
    const QVariant qValue = QAndroidTypeConverter::toQVariant(QJniObject(value));
    return invokeNativeImpl(env, object, &QAndroidItemModelProxy::setDataDefault,
                            &QAbstractItemModel::setData, nativeIndex, qValue, role);
}

void QAndroidItemModelProxy::jni_dataChanged(JNIEnv *env, jobject object, JQtModelIndex topLeft,
                                             JQtModelIndex bottomRight, QJniArray<jint> roles)
{
    const QModelIndex nativeTopLeft = QAndroidModelIndexProxy::qInstance(topLeft);
    const QModelIndex nativeBottomRight = QAndroidModelIndexProxy::qInstance(bottomRight);
    const QList<int> nativeRoles = roles.toContainer();
    invokeNativeImpl(env, object, &QAndroidItemModelProxy::dataChanged,
                     &QAbstractItemModel::dataChanged, nativeTopLeft, nativeBottomRight,
                     nativeRoles);
}

bool QAndroidItemModelProxy::registerAbstractNatives(QJniEnvironment &env)
{
    return env.registerNativeMethods(
            Traits<JQtAbstractItemModel>::className(),
            { Q_JNI_NATIVE_SCOPED_METHOD(jni_roleNames, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_canFetchMore, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_createIndex, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_fetchMore, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_hasChildren, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_hasIndex, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_beginInsertColumns, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_beginInsertRows, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_beginMoveColumns, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_beginMoveRows, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_beginRemoveColumns, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_beginRemoveRows, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_beginResetModel, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_endInsertColumns, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_endInsertRows, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_endMoveColumns, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_endMoveRows, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_endRemoveColumns, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_endRemoveRows, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_endResetModel, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_sibling, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_setData, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_dataChanged, QAndroidItemModelProxy) });
}

bool QAndroidItemModelProxy::registerProxyNatives(QJniEnvironment &env)
{
    return env.registerNativeMethods(
            Traits<JQtAndroidItemModelProxy>::className(),
            { Q_JNI_NATIVE_SCOPED_METHOD(jni_columnCount, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_data, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_index, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_parent, QAndroidItemModelProxy),
              Q_JNI_NATIVE_SCOPED_METHOD(jni_rowCount, QAndroidItemModelProxy) });
}

QT_END_NAMESPACE
