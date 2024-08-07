// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDITEMMODELPROXY_P_H
#define QANDROIDITEMMODELPROXY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/private/qandroidmodelindexproxy_p.h>
#include <QtCore/private/qandroidtypes_p.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QAndroidItemModelProxy : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit QAndroidItemModelProxy(QtJniTypes::JQtAbstractItemModel jInstance)
        : jInstance(jInstance)
    {
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    bool canFetchMore(const QModelIndex &parent) const override;
    bool canFetchMoreDefault(const QModelIndex &parent) const;
    QHash<int, QByteArray> roleNames() const override;
    QHash<int, QByteArray> defaultRoleNames() const;
    void fetchMore(const QModelIndex &parent) override;
    void fetchMoreDefault(const QModelIndex &parent);
    bool hasChildren(const QModelIndex &parent) const override;
    bool hasChildrenDefault(const QModelIndex &parent) const;
    QModelIndex sibling(int row, int column, const QModelIndex &parent) const override;
    QModelIndex siblingDefault(int row, int column, const QModelIndex &parent);

    Q_REQUIRED_RESULT static QAbstractItemModel *
    nativeInstance(QtJniTypes::JQtAbstractItemModel itemModel);
    Q_REQUIRED_RESULT static QAbstractItemModel *createNativeProxy(QJniObject itemModel);
    static QJniObject createProxy(QAbstractItemModel *abstractClass);

    template <typename Func, typename... Args>
    static auto invokeNativeProxyMethod(JNIEnv * /*env*/, jobject jvmObject, Func &&func,
                                        Args &&...args)
    {
        Q_ASSERT(jvmObject);
        auto model = qobject_cast<QAndroidItemModelProxy *>(nativeInstance(jvmObject));
        Q_ASSERT(model);
        return safeCall(model, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    static auto invokeNativeMethod(JNIEnv * /*env*/, jobject jvmObject, Func &&func, Args &&...args)
    {
        Q_ASSERT(jvmObject);
        auto model = nativeInstance(jvmObject);
        Q_ASSERT(model);
        return safeCall(model, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func1, typename Func2, typename... Args>
    static auto invokeNativeImpl(JNIEnv * /*env*/, jobject jvmObject, Func1 &&defaultFunc,
                                 Func2 &&func, Args &&...args)
    {
        Q_ASSERT(jvmObject);
        auto nativeModel = nativeInstance(jvmObject);
        auto nativeProxyModel = qobject_cast<QAndroidItemModelProxy *>(nativeModel);
        if (nativeProxyModel)
            return safeCall(nativeProxyModel, std::forward<Func1>(defaultFunc),
                            std::forward<Args>(args)...);
        else
            return safeCall(nativeModel, std::forward<Func2>(func), std::forward<Args>(args)...);
    }

    template <typename Object, typename Func, typename... Args>
    static auto safeCall(Object *object, Func &&func, Args &&...args)
    {
        using ReturnType = decltype(std::invoke(std::forward<Func>(func), object,
                                                std::forward<Args>(args)...));

        if constexpr (std::is_void_v<ReturnType>) {
            QMetaObject::invokeMethod(object, std::forward<Func>(func), Qt::AutoConnection,
                                      std::forward<Args>(args)...);
        } else {
            ReturnType returnValue;

            const auto connectionType = object->thread() == QThread::currentThread()
                    ? Qt::DirectConnection
                    : Qt::BlockingQueuedConnection;

            QMetaObject::invokeMethod(object, std::forward<Func>(func), connectionType,
                                      qReturnArg(returnValue), std::forward<Args>(args)...);
            return returnValue;
        }
    }

    static jint jni_columnCount(JNIEnv *env, jobject object, JQtModelIndex parent);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_columnCount)

    static jobject jni_data(JNIEnv *env, jobject object, JQtModelIndex index, jint role);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_data)

    static jobject jni_index(JNIEnv *env, jobject object, jint row, jint column,
                             JQtModelIndex parent);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_index)

    static jobject jni_parent(JNIEnv *env, jobject object, JQtModelIndex index);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_parent)

    static jint jni_rowCount(JNIEnv *env, jobject object, JQtModelIndex parent);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_rowCount)

    static jboolean jni_canFetchMore(JNIEnv *env, jobject object, JQtModelIndex parent);
    QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(jni_canFetchMore, canFetchMore)

    static void jni_fetchMore(JNIEnv *env, jobject object, JQtModelIndex parent);
    QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(jni_fetchMore, fetchMore)

    static jboolean jni_hasChildren(JNIEnv *env, jobject object, JQtModelIndex parent);
    QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(jni_hasChildren, hasChildren)

    static jboolean jni_hasIndex(JNIEnv *env, jobject object, jint row, jint column,
                                 JQtModelIndex parent);
    QT_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE_2(jni_hasIndex, hasIndex)

    static jobject jni_roleNames(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_roleNames)

    static void jni_beginInsertColumns(JNIEnv *env, jobject object, JQtModelIndex parent,
                                       jint first, jint last);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_beginInsertColumns)

    static void jni_beginInsertRows(JNIEnv *env, jobject object, JQtModelIndex parent, jint first,
                                    jint last);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_beginInsertRows)

    static jboolean jni_beginMoveColumns(JNIEnv *env, jobject object, JQtModelIndex sourceParent,
                                         jint sourceFirst, jint sourceLast,
                                         JQtModelIndex destinationParent, jint destinationChild);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_beginMoveColumns)

    static jboolean jni_beginMoveRows(JNIEnv *env, jobject object, JQtModelIndex sourceParent,
                                      jint sourceFirst, jint sourceLast,
                                      JQtModelIndex destinationParent, jint destinationChild);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_beginMoveRows)

    static void jni_beginRemoveColumns(JNIEnv *env, jobject object, JQtModelIndex parent,
                                       jint first, jint last);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_beginRemoveColumns)

    static void jni_beginRemoveRows(JNIEnv *env, jobject object, JQtModelIndex parent, jint first,
                                    jint last);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_beginRemoveRows)

    static void jni_beginResetModel(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_beginResetModel)

    static jobject jni_createIndex(JNIEnv *env, jobject object, jint row, jint column, jlong id);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_createIndex)

    static void jni_endInsertColumns(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_endInsertColumns)

    static void jni_endInsertRows(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_endInsertRows)

    static void jni_endMoveColumns(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_endMoveColumns)

    static void jni_endMoveRows(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_endMoveRows)

    static void jni_endRemoveColumns(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_endRemoveColumns)

    static void jni_endRemoveRows(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_endRemoveRows)

    static void jni_endResetModel(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_endResetModel)

    static jobject jni_sibling(JNIEnv *env, jobject object, jint row, jint column,
                                 JQtModelIndex parent);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(jni_sibling)

    static bool registerAbstractNatives(QJniEnvironment &env);
    static bool registerProxyNatives(QJniEnvironment &env);

private:
    QJniObject jInstance;
    friend class QAndroidModelIndexProxy;
};

QT_END_NAMESPACE

#endif // QANDROIDITEMMODELPROXY_P_H
