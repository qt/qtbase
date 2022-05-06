// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDEXTRAS_H
#define QANDROIDEXTRAS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <jni.h>
#include <functional>

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qjniobject.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmap.h>

#if QT_CONFIG(future)
#include <QtCore/qfuture.h>
#endif

QT_BEGIN_NAMESPACE

class QAndroidParcel;
class QAndroidBinderPrivate;
class QAndroidBinder;

class Q_CORE_EXPORT QAndroidBinder
{
public:
    enum class CallType {
        Normal = 0,
        OneWay = 1
    };

public:
    explicit QAndroidBinder();
    QAndroidBinder(const QJniObject &binder);

    virtual ~QAndroidBinder();

    virtual bool onTransact(int code, const QAndroidParcel &data,
                            const QAndroidParcel &reply, CallType flags);
    bool transact(int code, const QAndroidParcel &data,
                  QAndroidParcel *reply = nullptr, CallType flags = CallType::Normal) const;

    QJniObject handle() const;

private:
    friend class QAndroidBinderPrivate;
    friend class QAndroidParcelPrivate;
    friend class QAndroidServicePrivate;
    QSharedPointer<QAndroidBinderPrivate> d;
};

class QAndroidParcelPrivate;

class Q_CORE_EXPORT QAndroidParcel
{
public:
    QAndroidParcel();
    explicit QAndroidParcel(const QJniObject& parcel);
    virtual ~QAndroidParcel();

    void writeData(const QByteArray &data) const;
    void writeVariant(const QVariant &value) const;
    void writeBinder(const QAndroidBinder &binder) const;
    void writeFileDescriptor(int fd) const;

    QByteArray readData() const;
    QVariant readVariant() const;
    QAndroidBinder readBinder() const;
    int readFileDescriptor() const;

    QJniObject handle() const;

private:
    friend class QAndroidParcelPrivate;
    friend class QAndroidBinder;
    QSharedPointer<QAndroidParcelPrivate> d;
};

class QAndroidActivityResultReceiverPrivate;

class Q_CORE_EXPORT QAndroidActivityResultReceiver
{
public:
    QAndroidActivityResultReceiver();
    virtual ~QAndroidActivityResultReceiver();
    virtual void handleActivityResult(int receiverRequestCode, int resultCode,
                                      const QJniObject &data) = 0;

private:
    friend class QAndroidActivityResultReceiverPrivate;
    Q_DISABLE_COPY(QAndroidActivityResultReceiver)

    QScopedPointer<QAndroidActivityResultReceiverPrivate> d;
};

class Q_CORE_EXPORT QAndroidServiceConnection
{
public:
    QAndroidServiceConnection();
    explicit QAndroidServiceConnection(const QJniObject &serviceConnection);
    virtual ~QAndroidServiceConnection();

    virtual void onServiceConnected(const QString &name,
                                    const QAndroidBinder &serviceBinder) = 0;
    virtual void onServiceDisconnected(const QString &name) = 0;

    QJniObject handle() const;
private:
    Q_DISABLE_COPY(QAndroidServiceConnection)
    QJniObject m_handle;
};

class Q_CORE_EXPORT QAndroidIntent
{
public:
    QAndroidIntent();
    virtual ~QAndroidIntent();
    explicit QAndroidIntent(const QJniObject &intent);
    explicit QAndroidIntent(const QString &action);
    explicit QAndroidIntent(const QJniObject &packageContext, const char *className);

    void putExtra(const QString &key, const QByteArray &data);
    QByteArray extraBytes(const QString &key);

    void putExtra(const QString &key, const QVariant &value);
    QVariant extraVariant(const QString &key);

    QJniObject handle() const;

private:
    QJniObject m_handle;
};

class QAndroidServicePrivate;

class Q_CORE_EXPORT QAndroidService : public QCoreApplication
{
    Q_OBJECT

public:
    QAndroidService(int &argc, char **argv
#ifndef Q_QDOC
                    , int flags = ApplicationFlags
#endif
            );
    QAndroidService(int &argc, char **argv,
                    const std::function<QAndroidBinder*(const QAndroidIntent &intent)> & binder
#ifndef Q_QDOC
                    , int flags = ApplicationFlags
#endif
            );
    virtual ~QAndroidService();

    virtual QAndroidBinder* onBind(const QAndroidIntent &intent);

private:
    friend class QAndroidServicePrivate;
    Q_DISABLE_COPY(QAndroidService)

    QScopedPointer<QAndroidServicePrivate> d;
};

class QAndroidActivityCallbackResultReceiver: public QAndroidActivityResultReceiver
{
public:
    QAndroidActivityCallbackResultReceiver();
    void handleActivityResult(int receiverRequestCode, int resultCode,
                              const QJniObject &intent) override;
    void registerCallback(int receiverRequestCode,
                          std::function<void(int, int, const QJniObject &)> callbackFunc);

    static QAndroidActivityCallbackResultReceiver *instance();
private:
    QMap<int, std::function<void(int, int, const QJniObject &data)>> callbackMap;

    static QAndroidActivityCallbackResultReceiver *s_instance;
};

namespace QtAndroidPrivate
{
    Q_CORE_EXPORT void startIntentSender(const QJniObject &intentSender,
                                         int receiverRequestCode,
                                         QAndroidActivityResultReceiver *resultReceiver = nullptr);
    Q_CORE_EXPORT void startActivity(const QJniObject &intent,
                                     int receiverRequestCode,
                                     QAndroidActivityResultReceiver *resultReceiver = nullptr);
    Q_CORE_EXPORT void startActivity(const QAndroidIntent &intent,
                                     int receiverRequestCode,
                                     QAndroidActivityResultReceiver *resultReceiver = nullptr);
    Q_CORE_EXPORT void startActivity(const QJniObject &intent,
                                     int receiverRequestCode,
                                     std::function<void(int, int, const QJniObject &data)>
                                                        callbackFunc);

    enum class BindFlag {
        None                = 0x00000000,
        AutoCreate          = 0x00000001,
        DebugUnbind         = 0x00000002,
        NotForeground       = 0x00000004,
        AboveClient         = 0x00000008,
        AllowOomManagement  = 0x00000010,
        WaivePriority       = 0x00000020,
        Important           = 0x00000040,
        AdjustWithActivity  = 0x00000080,
        ExternalService     = -2147483648 // 0x80000000

    };
    Q_DECLARE_FLAGS(BindFlags, BindFlag)

    Q_CORE_EXPORT bool bindService(const QAndroidIntent &serviceIntent,
                                   const QAndroidServiceConnection &serviceConnection,
                                   BindFlags flags = BindFlag::None);

#if QT_CONFIG(future)
    enum PermissionResult {
        Undetermined,
        Authorized,
        Denied
    };

    Q_CORE_EXPORT QFuture<QtAndroidPrivate::PermissionResult>
    requestPermission(const QString &permission);
    QFuture<QtAndroidPrivate::PermissionResult>
    requestPermissions(const QStringList &permissions);
    Q_CORE_EXPORT QFuture<QtAndroidPrivate::PermissionResult>
    checkPermission(const QString &permission);
#endif

}

QT_END_NAMESPACE

#endif // QANDROIDEXTRAS_H
