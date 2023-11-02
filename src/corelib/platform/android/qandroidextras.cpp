// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidextras_p.h"

#include <QtCore/qbuffer.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmutex.h>
#include <QtCore/qtimer.h>
#include <QtCore/qset.h>

#if QT_CONFIG(future)
#include <QtCore/qpromise.h>
#endif

QT_BEGIN_NAMESPACE

class QAndroidParcelPrivate
{
public:
    QAndroidParcelPrivate();
    explicit QAndroidParcelPrivate(const QJniObject& parcel);

    void writeData(const QByteArray &data) const;
    void writeBinder(const QAndroidBinder &binder) const;
    void writeFileDescriptor(int fd) const;

    QByteArray readData() const;
    int readFileDescriptor() const;
    QAndroidBinder readBinder() const;

private:
    friend class QAndroidBinder;
    friend class QAndroidParcel;
    QJniObject handle;
};

struct FileDescriptor
{
    explicit FileDescriptor(int fd = -1)
        : handle("java/io/FileDescriptor")
    {
        QJniEnvironment().checkAndClearExceptions();
        handle.setField("descriptor", fd);
    }

    QJniObject handle;
};

QAndroidParcelPrivate::QAndroidParcelPrivate()
    : handle(QJniObject::callStaticObjectMethod("android/os/Parcel","obtain",
                                                "()Landroid/os/Parcel;").object())
{}

QAndroidParcelPrivate::QAndroidParcelPrivate(const QJniObject &parcel)
    : handle(parcel)
{}

void QAndroidParcelPrivate::writeData(const QByteArray &data) const
{
    if (data.isEmpty())
        return;

    QJniEnvironment().checkAndClearExceptions();
    QJniEnvironment env;
    jbyteArray array = env->NewByteArray(data.size());
    env->SetByteArrayRegion(array, 0, data.length(),
                            reinterpret_cast<const jbyte*>(data.constData()));
    handle.callMethod<void>("writeByteArray", "([B)V", array);
    env->DeleteLocalRef(array);
}

void QAndroidParcelPrivate::writeBinder(const QAndroidBinder &binder) const
{
    QJniEnvironment().checkAndClearExceptions();
    handle.callMethod<void>("writeStrongBinder", "(Landroid/os/IBinder;)V",
                            binder.handle().object());
}

void QAndroidParcelPrivate::writeFileDescriptor(int fd) const
{
    QJniEnvironment().checkAndClearExceptions();
    handle.callMethod<void>("writeFileDescriptor", "(Ljava/io/FileDescriptor;)V",
                            FileDescriptor(fd).handle.object());
}

QByteArray QAndroidParcelPrivate::readData() const
{
    QJniEnvironment().checkAndClearExceptions();
    auto array = handle.callObjectMethod("createByteArray", "()[B");
    QJniEnvironment env;
    auto sz = env->GetArrayLength(jbyteArray(array.object()));
    QByteArray res(sz, Qt::Initialization::Uninitialized);
    env->GetByteArrayRegion(jbyteArray(array.object()), 0, sz,
                            reinterpret_cast<jbyte *>(res.data()));
    return res;
}

int QAndroidParcelPrivate::readFileDescriptor() const
{
    QJniEnvironment().checkAndClearExceptions();
    auto parcelFD = handle.callObjectMethod("readFileDescriptor",
                                            "()Landroid/os/ParcelFileDescriptor;");
    if (parcelFD.isValid())
        return parcelFD.callMethod<jint>("getFd", "()I");
    return -1;
}

QAndroidBinder QAndroidParcelPrivate::readBinder() const
{
    QJniEnvironment().checkAndClearExceptions();
    auto strongBinder = handle.callObjectMethod("readStrongBinder", "()Landroid/os/IBinder;");
    return QAndroidBinder(strongBinder.object());
}

/*!
    \class QAndroidParcel
    \inheaderfile QtCore/private/qandroidextras_p.h
    \preliminary
    \inmodule QtCorePrivate
    \brief Wraps the most important methods of Android Parcel class.

    The QAndroidParcel is a convenience class that wraps the most important
    \l {https://developer.android.com/reference/android/os/Parcel.html}{Android Parcel}
    methods.

    \include qtcore.qdoc qtcoreprivate-usage

    \since 6.2
*/

/*!
    Creates a new object.
 */
QAndroidParcel::QAndroidParcel()
    : d(new QAndroidParcelPrivate())
{
}

/*!
    Wraps the \a parcel object.
 */
QAndroidParcel::QAndroidParcel(const QJniObject& parcel)
    : d(new QAndroidParcelPrivate(parcel))
{

}

QAndroidParcel::~QAndroidParcel()
{
}

/*!
    Writes the provided \a data as a byte array
 */
void QAndroidParcel::writeData(const QByteArray &data) const
{
    d->writeData(data);
}

/*!
    Writes the provided \a value. The value is converted into a
    QByteArray before is written.
 */
void QAndroidParcel::writeVariant(const QVariant &value) const
{
    QByteArray buff;
    QDataStream stream(&buff, QIODevice::WriteOnly);
    stream << value;
    d->writeData(buff);
}

/*!
    Writes a \a binder object. This is useful for a client to
    send to a server a binder which can be used by the server callback the client.
 */
void QAndroidParcel::writeBinder(const QAndroidBinder &binder) const
{
    d->writeBinder(binder);
}

/*!
    Writes the provided \a fd.
 */
void QAndroidParcel::writeFileDescriptor(int fd) const
{
    d->writeFileDescriptor(fd);
}

/*!
    Returns the data as a QByteArray
 */
QByteArray QAndroidParcel::readData() const
{
    return d->readData();
}

/*!
    Returns the data as a QVariant
 */
QVariant QAndroidParcel::readVariant() const
{
    QDataStream stream(d->readData());
    QVariant res;
    stream >> res;
    return res;
}

/*!
    Returns the binder as a QAndroidBinder
 */
QAndroidBinder QAndroidParcel::readBinder() const
{
    return d->readBinder();
}

/*!
    Returns the file descriptor
 */
int QAndroidParcel::readFileDescriptor() const
{
    return d->readFileDescriptor();
}

/*!
    The return value is useful to call other Java API which are not covered by this wrapper
 */
QJniObject QAndroidParcel::handle() const
{
    return d->handle;
}



/*!
    \class QAndroidBinder
    \inheaderfile QtCore/private/qandroidextras_p.h
    \preliminary
    \inmodule QtCorePrivate
    \brief Wraps the most important methods of Android Binder class.

    The QAndroidBinder is a convenience class that wraps the most important
    \l {https://developer.android.com/reference/android/os/Binder.html}{Android Binder}
    methods.

    \include qtcore.qdoc qtcoreprivate-usage

    \since 6.2
*/


/*!
    \enum QAndroidBinder::CallType

    This enum is used with \l QAndroidBinder::transact() to describe the mode in which the
    IPC call is performed.

    \value Normal normal IPC, meaning that the caller waits the result from the callee
    \value OneWay one-way IPC, meaning that the caller returns immediately, without waiting
    for a result from the callee
*/


class QAndroidBinderPrivate
{
public:
    explicit QAndroidBinderPrivate(QAndroidBinder *binder)
        : handle("org/qtproject/qt/android/extras/QtAndroidBinder", "(J)V", jlong(binder))
        , m_isQtAndroidBinder(true)
    {
        QJniEnvironment().checkAndClearExceptions();
    }

    explicit QAndroidBinderPrivate(const QJniObject &binder)
        : handle(binder), m_isQtAndroidBinder(false) {};
    void setDeleteListener(const std::function<void()> &func) { m_deleteListener = func; }
    ~QAndroidBinderPrivate()
    {
        if (m_isQtAndroidBinder) {
            QJniEnvironment().checkAndClearExceptions();
            handle.callMethod<void>("setId", "(J)V", jlong(0));
            if (m_deleteListener)
                m_deleteListener();
        }
    }

private:
    QJniObject handle;
    std::function<void()> m_deleteListener;
    bool m_isQtAndroidBinder;
    friend class QAndroidBinder;
};

/*!
    Creates a new object which can be used to perform IPC.

    \sa onTransact, transact
 */
QAndroidBinder::QAndroidBinder()
    : d(new QAndroidBinderPrivate(this))
{
}

/*!
    Creates a new object from the \a binder Java object.

    \sa transact
 */
QAndroidBinder::QAndroidBinder(const QJniObject &binder)
    : d(new QAndroidBinderPrivate(binder))
{
}

QAndroidBinder::~QAndroidBinder()
{
}

/*!
    Default implementation is a stub that returns false.
    The user should override this method to get the transact data from the caller.

    The \a code is the action to perform.
    The \a data is the marshaled data sent by the caller.\br
    The \a reply is the marshaled data to be sent to the caller.\br
    The \a flags are the additional operation flags.\br

    \warning This method is called from Binder's thread which is different
    from the thread that this object was created.

    \sa transact
 */
bool QAndroidBinder::onTransact(int /*code*/, const QAndroidParcel &/*data*/,
                                const QAndroidParcel &/*reply*/, CallType /*flags*/)
{
    return false;
}

/*!
    Performs an IPC call

    The \a code is the action to perform. Should be between
                \l {https://developer.android.com/reference/android/os/IBinder.html#FIRST_CALL_TRANSACTION}
                {FIRST_CALL_TRANSACTION} and
                \l {https://developer.android.com/reference/android/os/IBinder.html#LAST_CALL_TRANSACTION}
                {LAST_CALL_TRANSACTION}.\br
    The \a data is the marshaled data to send to the target.\br
    The \a reply (if specified) is the marshaled data to be received from the target.
    May be \b nullptr if you are not interested in the return value.\br
    The \a flags are the additional operation flags.\br

    \return true on success
 */
bool QAndroidBinder::transact(int code, const QAndroidParcel &data,
                              QAndroidParcel *reply, CallType flags) const
{
    QJniEnvironment().checkAndClearExceptions();
    return d->handle.callMethod<jboolean>("transact",
                                          "(ILandroid/os/Parcel;Landroid/os/Parcel;I)Z",
                                          jint(code), data.d->handle.object(),
                                          reply ? reply->d->handle.object() : nullptr,
                                          jint(flags));
}

/*!
    The return value is useful to call other Java API which are not covered by this wrapper
 */
QJniObject QAndroidBinder::handle() const
{
    return d->handle;
}




/*!
    \class QAndroidServiceConnection
    \inheaderfile QtCore/private/qandroidextras_p.h
    \preliminary
    \inmodule QtCorePrivate
    \brief Wraps the most important methods of Android ServiceConnection class.

    The QAndroidServiceConnection is a convenience abstract class which wraps the
    \l {https://developer.android.com/reference/android/content/ServiceConnection.html}{AndroidServiceConnection}
    interface.

    It is useful when you perform a QtAndroidPrivate::bindService operation.

    \include qtcore.qdoc qtcoreprivate-usage

    \since 6.2
*/

/*!
    Creates a new object
 */
QAndroidServiceConnection::QAndroidServiceConnection()
    : m_handle("org/qtproject/qt/android/extras/QtAndroidServiceConnection", "(J)V", jlong(this))
{
}

/*!
    Creates a new object from an existing \a serviceConnection.

    It's useful when you have your own Java implementation.
    Of course onServiceConnected()/onServiceDisconnected()
    will not be called anymore.
 */
QAndroidServiceConnection::QAndroidServiceConnection(const QJniObject &serviceConnection)
    : m_handle(serviceConnection)
{
}

QAndroidServiceConnection::~QAndroidServiceConnection()
{
    m_handle.callMethod<void>("setId", "(J)V", jlong(this));
}

/*!
    returns the underline QJniObject
 */
QJniObject QAndroidServiceConnection::handle() const
{
    return m_handle;
}

/*!
    \fn void QAndroidServiceConnection::onServiceConnected(const QString &name, const QAndroidBinder &serviceBinder)

    This notification is called when the client managed to connect to the service.
    The \a name contains the server name, the \a serviceBinder is the binder that the client
    uses to perform IPC operations.

    \warning This method is called from Binder's thread which is different
    from the thread that this object was created.

    returns the underline QJniObject
 */

/*!
    \fn void QAndroidServiceConnection::onServiceDisconnected(const QString &name)

    Called when a connection to the Service has been lost.
    The \a name parameter specifies which connectioen was lost.

    \warning This method is called from Binder's thread which is different
    from the thread that this object was created.

    returns the underline QJniObject
 */


Q_CONSTINIT static QBasicAtomicInteger<uint> nextUniqueActivityRequestCode = Q_BASIC_ATOMIC_INITIALIZER(0);

// Get a unique activity request code.
static int uniqueActivityRequestCode()
{
    constexpr uint ReservedForQtOffset = 0x1000; // Reserve all request codes under 0x1000 for Qt

    const uint requestCodeBase = nextUniqueActivityRequestCode.fetchAndAddRelaxed(1);
    if (requestCodeBase == uint(INT_MAX) - ReservedForQtOffset)
        qWarning("Unique activity request code has wrapped. Unexpected behavior may occur.");

    const int requestCode = static_cast<int>(requestCodeBase + ReservedForQtOffset);
    return requestCode;
}

class QAndroidActivityResultReceiverPrivate: public QtAndroidPrivate::ActivityResultListener
{
public:
    QAndroidActivityResultReceiver *q;
    mutable QHash<int, int> localToGlobalRequestCode;
    mutable QHash<int, int> globalToLocalRequestCode;

    int globalRequestCode(int localRequestCode) const
    {
        const auto oldSize = localToGlobalRequestCode.size();
        auto &e = localToGlobalRequestCode[localRequestCode];
        if (localToGlobalRequestCode.size() != oldSize) {
            // new entry, populate:
            int globalRequestCode = uniqueActivityRequestCode();
            e = globalRequestCode;
            globalToLocalRequestCode[globalRequestCode] = localRequestCode;
        }
        return e;
    }

    bool handleActivityResult(jint requestCode, jint resultCode, jobject data)
    {
        const auto it = std::as_const(globalToLocalRequestCode).find(requestCode);
        if (it != globalToLocalRequestCode.cend()) {
            q->handleActivityResult(*it, resultCode, QJniObject(data));
            return true;
        }

        return false;
    }

    static QAndroidActivityResultReceiverPrivate *get(QAndroidActivityResultReceiver *publicObject)
    {
        return publicObject->d.data();
    }
};

/*!
  \class QAndroidActivityResultReceiver
  \inheaderfile QtCore/private/qandroidextras_p.h
  \preliminary
  \inmodule QtCorePrivate
  \since 6.2
  \brief Interface used for callbacks from onActivityResult() in the main Android activity.

  Create a subclass of this class to be notified of the results when using the
  \c QtAndroidPrivate::startActivity() and \c QtAndroidPrivate::startIntentSender() APIs.

  \include qtcore.qdoc qtcoreprivate-usage
 */

/*!
   \internal
*/
QAndroidActivityResultReceiver::QAndroidActivityResultReceiver()
    : d(new QAndroidActivityResultReceiverPrivate)
{
    d->q = this;
    QtAndroidPrivate::registerActivityResultListener(d.data());
}

/*!
   \internal
*/
QAndroidActivityResultReceiver::~QAndroidActivityResultReceiver()
{
    QtAndroidPrivate::unregisterActivityResultListener(d.data());
}

/*!
   \fn void QAndroidActivityResultReceiver::handleActivityResult(int receiverRequestCode, int resultCode, const QJniObject &data)

    Reimplement this function to get activity results after starting an activity using
    either QtAndroidPrivate::startActivity() or QtAndroidPrivate::startIntentSender().
    The \a receiverRequestCode is the request code unique to this receiver which was
    originally passed to the startActivity() or startIntentSender() functions. The
    \a resultCode is the result returned by the activity, and \a data is either null
    or a Java object of the class android.content.Intent. Both the last to arguments
    are identical to the arguments passed to onActivityResult().
*/



class QAndroidServicePrivate : public QObject, public QtAndroidPrivate::OnBindListener
{
public:
    QAndroidServicePrivate(QAndroidService *service,
                           const std::function<QAndroidBinder*(const QAndroidIntent&)> &binder ={})
        : m_service(service)
        , m_binder(binder)
    {
        QTimer::singleShot(0,this, [this]{ QtAndroidPrivate::setOnBindListener(this);});
    }

    ~QAndroidServicePrivate()
    {
        QMutexLocker lock(&m_bindersMutex);
        while (!m_binders.empty()) {
            auto it = m_binders.begin();
            lock.unlock();
            delete (*it);
            lock.relock();
        }
    }

    // OnBindListener interface
    jobject onBind(jobject intent) override
    {
        auto qai = QAndroidIntent(intent);
        auto binder = m_binder ? m_binder(qai) : m_service->onBind(qai);
        if (binder) {
            {
                QMutexLocker lock(&m_bindersMutex);
                binder->d->setDeleteListener([this, binder]{binderDestroied(binder);});
                m_binders.insert(binder);
            }
            return binder->handle().object();
        }
        return nullptr;
    }

private:
    void binderDestroied(QAndroidBinder* obj)
    {
        QMutexLocker lock(&m_bindersMutex);
        m_binders.remove(obj);
    }

public:
    QAndroidService *m_service = nullptr;
    std::function<QAndroidBinder *(const QAndroidIntent &)> m_binder;
    QMutex m_bindersMutex;
    QSet<QAndroidBinder*> m_binders;
};

/*!
    \class QAndroidService
    \inheaderfile QtCore/private/qandroidextras_p.h
    \preliminary
    \inmodule QtCorePrivate
    \brief Wraps the most important methods of Android Service class.

    The QAndroidService is a convenience class that wraps the most important
    \l {https://developer.android.com/reference/android/app/Service.html}{Android Service}
    methods.

    \include qtcore.qdoc qtcoreprivate-usage

    \since 6.2
*/


/*!
    \fn QAndroidService::QAndroidService(int &argc, char **argv)

    Creates a new Android service, passing \a argc and \a argv as parameters.

    //! Parameter \a flags is omitted in the documentation.

    \sa QCoreApplication
 */
QAndroidService::QAndroidService(int &argc, char **argv, int flags)
    : QCoreApplication (argc, argv, QtAndroidPrivate::acuqireServiceSetup(flags))
    , d(new QAndroidServicePrivate{this})
{
}

/*!
    \fn QAndroidService::QAndroidService(int &argc, char **argv, const std::function<QAndroidBinder *(const QAndroidIntent &)> &binder)

    Creates a new Android service, passing \a argc and \a argv as parameters.

    \a binder is used to create a \l {QAndroidBinder}{binder} when needed.

    //! Parameter \a flags is omitted in the documentation.

    \sa QCoreApplication
 */
QAndroidService::QAndroidService(int &argc, char **argv,
                                 const std::function<QAndroidBinder*(const QAndroidIntent&)> &binder,
                                 int flags)
    : QCoreApplication (argc, argv, QtAndroidPrivate::acuqireServiceSetup(flags))
    , d(new QAndroidServicePrivate{this, binder})
{
}

QAndroidService::~QAndroidService()
{}

/*!
    The user must override this method and to return a binder.

    The \a intent parameter contains all the caller information.

    The returned binder is used by the caller to perform IPC calls.

    \warning This method is called from Binder's thread which is different
    from the thread that this object was created.

    \sa QAndroidBinder::onTransact, QAndroidBinder::transact
 */
QAndroidBinder* QAndroidService::onBind(const QAndroidIntent &/*intent*/)
{
    return nullptr;
}

static jboolean onTransact(JNIEnv */*env*/, jclass /*cls*/, jlong id, jint code, jobject data,
                           jobject reply, jint flags)
{
    if (!id)
        return false;

    return reinterpret_cast<QAndroidBinder*>(id)->onTransact(
            code, QAndroidParcel(data), QAndroidParcel(reply), QAndroidBinder::CallType(flags));
}

static void onServiceConnected(JNIEnv */*env*/, jclass /*cls*/, jlong id, jstring name,
                               jobject service)
{
    if (!id)
        return;

    return reinterpret_cast<QAndroidServiceConnection *>(id)->onServiceConnected(
            QJniObject(name).toString(), QAndroidBinder(service));
}

static void onServiceDisconnected(JNIEnv */*env*/, jclass /*cls*/, jlong id, jstring name)
{
    if (!id)
        return;

    return reinterpret_cast<QAndroidServiceConnection *>(id)->onServiceDisconnected(
            QJniObject(name).toString());
}

bool QtAndroidPrivate::registerExtrasNatives(QJniEnvironment &env)
{
    static const JNINativeMethod methods[] = {
        {"onTransact", "(JILandroid/os/Parcel;Landroid/os/Parcel;I)Z", (void *)onTransact},
        {"onServiceConnected", "(JLjava/lang/String;Landroid/os/IBinder;)V", (void *)onServiceConnected},
        {"onServiceDisconnected", "(JLjava/lang/String;)V", (void *)onServiceDisconnected}
    };

    return env.registerNativeMethods("org/qtproject/qt/android/extras/QtNative", methods, 3);
}

/*!
    \class QAndroidIntent
    \inheaderfile QtCore/private/qandroidextras_p.h
    \preliminary
    \inmodule QtCorePrivate
    \brief Wraps the most important methods of Android Intent class.

    The QAndroidIntent is a convenience class that wraps the most important
    \l {https://developer.android.com/reference/android/content/Intent.html}{Android Intent}
    methods.

    \include qtcore.qdoc qtcoreprivate-usage

    \since 6.2
*/

/*!
    Create a new intent
 */
QAndroidIntent::QAndroidIntent()
    : m_handle("android.content.Intent", "()V")
{

}

QAndroidIntent::~QAndroidIntent()
{}

/*!
    Wraps the provided \a intent java object.
 */
QAndroidIntent::QAndroidIntent(const QJniObject &intent)
    : m_handle(intent)
{
}

/*!
    Creates a new intent and sets the provided \a action.
 */
QAndroidIntent::QAndroidIntent(const QString &action)
    : m_handle("android.content.Intent", "(Ljava/lang/String;)V",
                        QJniObject::fromString(action).object())
{
    QJniEnvironment().checkAndClearExceptions();
}

/*!
    Creates a new intent and sets the provided \a packageContext and the service \a className.
    Example:
    \code
        auto serviceIntent = QAndroidIntent(QtAndroidPrivate::androidActivity().object(), "com.example.MyService");
    \endcode

    \sa QtAndroidPrivate::bindService
 */
QAndroidIntent::QAndroidIntent(const QJniObject &packageContext, const char *className)
    : m_handle("android/content/Intent", "(Landroid/content/Context;Ljava/lang/Class;)V",
                        packageContext.object(), QJniEnvironment().findClass(className))
{
    QJniEnvironment().checkAndClearExceptions();
}

/*!
    Sets the \a key with the \a data in the Intent extras
 */
void QAndroidIntent::putExtra(const QString &key, const QByteArray &data)
{
    QJniEnvironment().checkAndClearExceptions();
    QJniEnvironment env;
    jbyteArray array = env->NewByteArray(data.size());
    env->SetByteArrayRegion(array, 0, data.length(),
                            reinterpret_cast<const jbyte*>(data.constData()));
    m_handle.callObjectMethod("putExtra", "(Ljava/lang/String;[B)Landroid/content/Intent;",
                              QJniObject::fromString(key).object(), array);
    env->DeleteLocalRef(array);
    QJniEnvironment().checkAndClearExceptions();
}

/*!
    Returns the extra \a key data from the Intent extras
 */
QByteArray QAndroidIntent::extraBytes(const QString &key)
{
    QJniEnvironment().checkAndClearExceptions();
    auto array = m_handle.callObjectMethod("getByteArrayExtra", "(Ljava/lang/String;)[B",
                                           QJniObject::fromString(key).object());
    if (!array.isValid() || !array.object())
        return QByteArray();
    QJniEnvironment env;
    auto sz = env->GetArrayLength(jarray(array.object()));
    QByteArray res(sz, Qt::Initialization::Uninitialized);
    env->GetByteArrayRegion(jbyteArray(array.object()), 0, sz,
                            reinterpret_cast<jbyte *>(res.data()));
    QJniEnvironment().checkAndClearExceptions();

    return res;
}

/*!
    Sets the \a key with the \a value in the Intent extras.
 */
void QAndroidIntent::putExtra(const QString &key, const QVariant &value)
{
    QByteArray buff;
    QDataStream stream(&buff, QIODevice::WriteOnly);
    stream << value;
    putExtra(key, buff);
}

/*!
    Returns the extra \a key data from the Intent extras as a QVariant
 */
QVariant QAndroidIntent::extraVariant(const QString &key)
{
    QDataStream stream(extraBytes(key));
    QVariant res;
    stream >> res;
    return res;
}

/*!
    The return value is useful to call other Java API which are not covered by this wrapper
 */
QJniObject QAndroidIntent::handle() const
{
    return m_handle;
}



/*!
    \namespace QtAndroidPrivate
    \preliminary
    \inmodule QtCorePrivate
    \since 6.2
    \brief The QtAndroidPrivate namespace provides miscellaneous functions
           to aid Android development.
    \inheaderfile QtCore/private/qandroidextras_p.h

    \include qtcore.qdoc qtcoreprivate-usage
*/

/*!
    \since 6.2
    \enum QtAndroidPrivate::BindFlag

    This enum is used with QtAndroidPrivate::bindService to describe the mode in which the
    binding is performed.

    \value None                 No options.
    \value AutoCreate           Automatically creates the service as long as the binding exist.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_AUTO_CREATE}
                                {BIND_AUTO_CREATE} documentation for more details.
    \value DebugUnbind          Include debugging help for mismatched calls to unbind.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_DEBUG_UNBIND}
                                {BIND_DEBUG_UNBIND} documentation for more details.
    \value NotForeground        Don't allow this binding to raise the target service's process to the foreground scheduling priority.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_NOT_FOREGROUND}
                                {BIND_NOT_FOREGROUND} documentation for more details.
    \value AboveClient          Indicates that the client application binding to this service considers the service to be more important than the app itself.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_ABOVE_CLIENT}
                                {BIND_ABOVE_CLIENT} documentation for more details.
    \value AllowOomManagement   Allow the process hosting the bound service to go through its normal memory management.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_ALLOW_OOM_MANAGEMENT}
                                {BIND_ALLOW_OOM_MANAGEMENT} documentation for more details.
    \value WaivePriority        Don't impact the scheduling or memory management priority of the target service's hosting process.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_WAIVE_PRIORITY}
                                {BIND_WAIVE_PRIORITY} documentation for more details.
    \value Important            This service is assigned a higher priority so that it is available to the client when needed.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_IMPORTANT}
                                {BIND_IMPORTANT} documentation for more details.
    \value AdjustWithActivity   If binding from an activity, allow the target service's process importance to be raised based on whether the activity is visible to the user.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_ADJUST_WITH_ACTIVITY}
                                {BIND_ADJUST_WITH_ACTIVITY} documentation for more details.
    \value ExternalService      The service being bound is an isolated, external service.
                                See \l {https://developer.android.com/reference/android/content/Context.html#BIND_EXTERNAL_SERVICE}
                                {BIND_EXTERNAL_SERVICE} documentation for more details.
*/

/*!
  \since 6.2

  Starts the activity given by \a intent and provides the result asynchronously through the
  \a resultReceiver if this is non-null.

  If \a resultReceiver is null, then the \c startActivity() method in the \c androidActivity()
  will be called. Otherwise \c startActivityForResult() will be called.

  The \a receiverRequestCode is a request code unique to the \a resultReceiver, and will be
  returned along with the result, making it possible to use the same receiver for more than
  one intent.

 */
void QtAndroidPrivate::startActivity(const QJniObject &intent,
                              int receiverRequestCode,
                              QAndroidActivityResultReceiver *resultReceiver)
{
    QJniObject activity = QtAndroidPrivate::activity();
    if (resultReceiver != 0) {
        QAndroidActivityResultReceiverPrivate *resultReceiverD =
                QAndroidActivityResultReceiverPrivate::get(resultReceiver);
        activity.callMethod<void>("startActivityForResult",
                                  "(Landroid/content/Intent;I)V",
                                  intent.object<jobject>(),
                                  resultReceiverD->globalRequestCode(receiverRequestCode));
    } else {
        activity.callMethod<void>("startActivity",
                                  "(Landroid/content/Intent;)V",
                                  intent.object<jobject>());
    }
}

/*!
  \since 6.2

  Starts the activity given by \a intent and provides the result asynchronously through the
  \a resultReceiver if this is non-null.

  If \a resultReceiver is null, then the \c startActivity() method in the \c androidActivity()
  will be called. Otherwise \c startActivityForResult() will be called.

  The \a receiverRequestCode is a request code unique to the \a resultReceiver, and will be
  returned along with the result, making it possible to use the same receiver for more than
  one intent.

 */
void QtAndroidPrivate::startActivity(const QAndroidIntent &intent,
                              int receiverRequestCode,
                              QAndroidActivityResultReceiver *resultReceiver)
{
    startActivity(intent.handle(), receiverRequestCode, resultReceiver);
}

/*!
  \since 6.2

  Starts the activity given by \a intent, using the request code \a receiverRequestCode,
  and provides the result by calling \a callbackFunc.
*/
void QtAndroidPrivate::startActivity(const QJniObject &intent,
                              int receiverRequestCode,
                              std::function<void(int, int, const QJniObject &data)> callbackFunc)
{
    QJniObject activity = QtAndroidPrivate::activity();
    QAndroidActivityCallbackResultReceiver::instance()->registerCallback(receiverRequestCode,
                                                                         callbackFunc);
    startActivity(intent, receiverRequestCode, QAndroidActivityCallbackResultReceiver::instance());
}

/*!
  \since 6.2

  Starts the activity given by \a intentSender and provides the result asynchronously through the
  \a resultReceiver if this is non-null.

  If \a resultReceiver is null, then the \c startIntentSender() method in the \c androidActivity()
  will be called. Otherwise \c startIntentSenderForResult() will be called.

  The \a receiverRequestCode is a request code unique to the \a resultReceiver, and will be
  returned along with the result, making it possible to use the same receiver for more than
  one intent.

*/
void QtAndroidPrivate::startIntentSender(const QJniObject &intentSender,
                                  int receiverRequestCode,
                                  QAndroidActivityResultReceiver *resultReceiver)
{
    QJniObject activity = QtAndroidPrivate::activity();
    if (resultReceiver != 0) {
        QAndroidActivityResultReceiverPrivate *resultReceiverD =
                QAndroidActivityResultReceiverPrivate::get(resultReceiver);
        activity.callMethod<void>("startIntentSenderForResult",
                                  "(Landroid/content/IntentSender;ILandroid/content/Intent;III)V",
                                  intentSender.object<jobject>(),
                                  resultReceiverD->globalRequestCode(receiverRequestCode),
                                  0,  // fillInIntent
                                  0,  // flagsMask
                                  0,  // flagsValues
                                  0); // extraFlags
    } else {
        activity.callMethod<void>("startIntentSender",
                                  "(Landroid/content/IntentSender;Landroid/content/Intent;III)V",
                                  intentSender.object<jobject>(),
                                  0,  // fillInIntent
                                  0,  // flagsMask
                                  0,  // flagsValues
                                  0); // extraFlags

    }

}

/*!
    \since 6.2
    \fn bool QtAndroidPrivate::bindService(const QAndroidIntent &serviceIntent, const QAndroidServiceConnection &serviceConnection, BindFlags flags = BindFlag::None)

    Binds the service given by \a serviceIntent, \a serviceConnection and \a flags.
    The \a serviceIntent object identifies the service to connect to.
    The \a serviceConnection is a listener that receives the information as the service
    is started and stopped.

    \return true on success

    See \l {https://developer.android.com/reference/android/content/Context.html#bindService%28android.content.Intent,%20android.content.ServiceConnection,%20int%29}
    {Android documentation} documentation for more details.

    \sa QAndroidIntent, QAndroidServiceConnection, BindFlag
*/
bool QtAndroidPrivate::bindService(const QAndroidIntent &serviceIntent,
                            const QAndroidServiceConnection &serviceConnection, BindFlags flags)
{
    QJniEnvironment().checkAndClearExceptions();
    QJniObject contextObj = QtAndroidPrivate::context();
    return contextObj.callMethod<jboolean>(
                "bindService",
                "(Landroid/content/Intent;Landroid/content/ServiceConnection;I)Z",
                serviceIntent.handle().object(),
                serviceConnection.handle().object(),
                jint(flags));
}

QAndroidActivityCallbackResultReceiver * QAndroidActivityCallbackResultReceiver::s_instance = nullptr;

QAndroidActivityCallbackResultReceiver::QAndroidActivityCallbackResultReceiver()
    : QAndroidActivityResultReceiver()
    , callbackMap()
{
}

void QAndroidActivityCallbackResultReceiver::handleActivityResult(int receiverRequestCode,
                                                                  int resultCode,
                                                                  const QJniObject &intent)
{
    callbackMap[receiverRequestCode](receiverRequestCode, resultCode, intent);
    callbackMap.remove(receiverRequestCode);
}

QAndroidActivityCallbackResultReceiver * QAndroidActivityCallbackResultReceiver::instance() {
    if (!s_instance) {
        s_instance = new QAndroidActivityCallbackResultReceiver();
    }
    return s_instance;
}

void QAndroidActivityCallbackResultReceiver::registerCallback(
        int receiverRequestCode,
        std::function<void(int, int, const QJniObject &data)> callbackFunc)
{
    callbackMap.insert(receiverRequestCode, callbackFunc);
}

// Permissions API

static const char qtNativeClassName[] = "org/qtproject/qt/android/QtNative";

QtAndroidPrivate::PermissionResult resultFromAndroid(jint value)
{
    return value == 0 ? QtAndroidPrivate::Authorized : QtAndroidPrivate::Denied;
}

using PendingPermissionRequestsHash
            = QHash<int, QSharedPointer<QPromise<QtAndroidPrivate::PermissionResult>>>;
Q_GLOBAL_STATIC(PendingPermissionRequestsHash, g_pendingPermissionRequests);
Q_CONSTINIT static QBasicMutex g_pendingPermissionRequestsMutex;

static int nextRequestCode()
{
    Q_CONSTINIT static QBasicAtomicInt counter = Q_BASIC_ATOMIC_INITIALIZER(0);
    return counter.fetchAndAddRelaxed(1);
}

/*!
    \internal

    This function is called when the result of the permission request is available.
    Once a permission is requested, the result is braodcast by the OS and listened
    to by QtActivity which passes it to C++ through a native JNI method call.
 */
static void sendRequestPermissionsResult(JNIEnv *env, jobject *obj, jint requestCode,
                                         jobjectArray permissions, jintArray grantResults)
{
    Q_UNUSED(obj);

    QMutexLocker locker(&g_pendingPermissionRequestsMutex);
    auto it = g_pendingPermissionRequests->constFind(requestCode);
    if (it == g_pendingPermissionRequests->constEnd()) {
        qWarning() << "Found no valid pending permission request for request code" << requestCode;
        return;
    }

    auto request = *it;
    g_pendingPermissionRequests->erase(it);
    locker.unlock();

    const int size = env->GetArrayLength(permissions);
    std::unique_ptr<jint[]> results(new jint[size]);
    env->GetIntArrayRegion(grantResults, 0, size, results.get());

    for (int i = 0 ; i < size; ++i) {
        QtAndroidPrivate::PermissionResult result = resultFromAndroid(results[i]);
        request->addResult(result, i);
    }

    QtAndroidPrivate::releaseAndroidDeadlockProtector();
    request->finish();
}

QFuture<QtAndroidPrivate::PermissionResult>
requestPermissionsInternal(const QStringList &permissions)
{
    // No mechanism to request permission for SDK version below 23, because
    // permissions defined in the manifest are granted at install time.
    if (QtAndroidPrivate::androidSdkVersion() < 23) {
        QList<QtAndroidPrivate::PermissionResult> result;
        result.reserve(permissions.size());
        // ### can we kick off all checkPermission()s, and whenAll() collect results?
        for (const QString &permission : permissions)
            result.push_back(QtAndroidPrivate::checkPermission(permission).result());
        return QtFuture::makeReadyRangeFuture(result);
    }

    if (!QtAndroidPrivate::acquireAndroidDeadlockProtector())
        return QtFuture::makeReadyValueFuture(QtAndroidPrivate::Denied);

    QSharedPointer<QPromise<QtAndroidPrivate::PermissionResult>> promise;
    promise.reset(new QPromise<QtAndroidPrivate::PermissionResult>());
    QFuture<QtAndroidPrivate::PermissionResult> future = promise->future();
    promise->start();

    const int requestCode = nextRequestCode();
    QMutexLocker locker(&g_pendingPermissionRequestsMutex);
    g_pendingPermissionRequests->insert(requestCode, promise);
    locker.unlock();

    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([permissions, requestCode] {
        QJniEnvironment env;
        jclass clazz = env.findClass("java/lang/String");
        auto array = env->NewObjectArray(permissions.size(), clazz, nullptr);
        int index = 0;

        for (auto &perm : permissions)
            env->SetObjectArrayElement(array, index++, QJniObject::fromString(perm).object());

        QJniObject(QtAndroidPrivate::activity()).callMethod<void>("requestPermissions",
                                                                  "([Ljava/lang/String;I)V",
                                                                  array,
                                                                  requestCode);
        env->DeleteLocalRef(array);
    });

    return future;
}

/*!
    \preliminary
    Requests the \a permission and returns a QFuture representing the
    result of the request.

    \since 6.2
    \sa checkPermission()
*/
QFuture<QtAndroidPrivate::PermissionResult>
QtAndroidPrivate::requestPermission(const QString &permission)
{
    return requestPermissions({permission});
}

QFuture<QtAndroidPrivate::PermissionResult>
QtAndroidPrivate::requestPermissions(const QStringList &permissions)
{
    // avoid the uneccessary call and response to an empty permission string
    if (permissions.isEmpty())
        return QtFuture::makeReadyValueFuture(QtAndroidPrivate::Denied);
    return requestPermissionsInternal(permissions);
}

/*!
    \preliminary
    Checks whether this process has the named \a permission and returns a QFuture
    representing the result of the check.

    \since 6.2
    \sa requestPermission()
*/
QFuture<QtAndroidPrivate::PermissionResult>
QtAndroidPrivate::checkPermission(const QString &permission)
{
    QtAndroidPrivate::PermissionResult result = Denied;
    if (!permission.isEmpty()) {
        auto res = QJniObject::callStaticMethod<jint>(qtNativeClassName,
                                                      "checkSelfPermission",
                                                      "(Ljava/lang/String;)I",
                                                      QJniObject::fromString(permission).object());
        result = resultFromAndroid(res);
    }
    return QtFuture::makeReadyValueFuture(result);
}

bool QtAndroidPrivate::registerPermissionNatives(QJniEnvironment &env)
{
    if (QtAndroidPrivate::androidSdkVersion() < 23)
        return true;

    const JNINativeMethod methods[] = {
        {"sendRequestPermissionsResult", "(I[Ljava/lang/String;[I)V",
         reinterpret_cast<void *>(sendRequestPermissionsResult)
        }};

    return env.registerNativeMethods(qtNativeClassName, methods, 1);
}

QT_END_NAMESPACE
