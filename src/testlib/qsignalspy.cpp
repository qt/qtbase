// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsignalspy.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSignalSpy
    \inmodule QtTest

    \brief The QSignalSpy class enables introspection of signal emission.

    QSignalSpy can connect to any signal of any object and records its emission.
    QSignalSpy itself is a list of QVariant lists. Each emission of the signal
    will append one item to the list, containing the arguments of the signal.

    The following example records all signal emissions for the \c clicked() signal
    of a QCheckBox:

    \snippet code/doc_src_qsignalspy.cpp 0

    \c{spy.takeFirst()} returns the arguments for the first emitted signal, as a
    list of QVariant objects. The \c clicked() signal has a single bool argument,
    which is stored as the first entry in the list of arguments.

    The example below catches a signal from a custom object:

    \snippet code/doc_src_qsignalspy.cpp 1

    \note Non-standard data types need to be registered, using
    the qRegisterMetaType() function, before you can create a
    QSignalSpy. For example:

    \snippet code/doc_src_qsignalspy.cpp 2

    To retrieve the instance, you can use qvariant_cast:

    \snippet code/doc_src_qsignalspy.cpp 3

    \section1 Verifying Signal Emissions

    The QSignalSpy class provides an elegant mechanism for capturing the list
    of signals emitted by an object. However, you should verify its validity
    after construction. The constructor does a number of sanity checks, such as
    verifying that the signal to be spied upon actually exists. To make the
    diagnosis of test failures easier, the results of these checks should be
    checked by calling \c QVERIFY(spy.isValid()) before proceeding further with
    a test.

    \sa QVERIFY()
 */

/*! \fn QSignalSpy::QSignalSpy(const QObject *object, const char *signal)

    Constructs a new QSignalSpy that listens for emissions of the \a signal
    from the QObject \a object. If QSignalSpy is not able to listen for a
    valid signal (for example, because \a object is \nullptr or \a signal does
    not denote a valid signal of \a object), an explanatory warning message
    will be output using qWarning() and subsequent calls to \c isValid() will
    return false.

    Example:
    \snippet code/doc_src_qsignalspy.cpp 4
*/

/*! \fn template <typename PointerToMemberFunction> QSignalSpy::QSignalSpy(const QObject *object, PointerToMemberFunction signal)
    \since 5.4

    Constructs a new QSignalSpy that listens for emissions of the \a signal
    from the QObject \a object. If QSignalSpy is not able to listen for a
    valid signal (for example, because \a object is \nullptr or \a signal does
    not denote a valid signal of \a object), an explanatory warning message
    will be output using qWarning() and subsequent calls to \c isValid() will
    return false.

    Example:
    \snippet code/doc_src_qsignalspy.cpp 6
*/

/*! \fn QSignalSpy::QSignalSpy(const QObject *obj, QMetaMethod signal)
    \since 5.14

    Constructs a new QSignalSpy that listens for emissions of the \a signal
    from the QObject \a obj. If QSignalSpy is not able to listen for a
    valid signal (for example, because \a obj is \nullptr or \a signal does
    not denote a valid signal of \a obj), an explanatory warning message
    will be output using qWarning() and subsequent calls to \c isValid() will
    return false.

    This constructor is convenient to use when Qt's meta-object system is
    heavily used in a test.

    Basic usage example:
    \snippet code/doc_src_qsignalspy.cpp 7

    Imagine we need to check whether all properties of the QWindow class
    that represent minimum and maximum dimensions are properly writable.
    The following example demonstrates one of the approaches:
    \snippet code/doc_src_qsignalspy.cpp 8
*/

/*! \fn QSignalSpy::isValid() const

    Returns \c true if the signal spy listens to a valid signal, otherwise false.
*/

/*! \fn QSignalSpy::signal() const

    Returns the normalized signal the spy is currently listening to.
*/

/*! \fn bool QSignalSpy::wait(int timeout)
    \since 5.0

    This is an overloaded function, equivalent passing \a timeout to the
    chrono overload:
    \code
    wait(std::chrono::milliseconds{timeout});
    \endcode

    Returns \c true if the signal was emitted at least once in \a timeout,
    otherwise returns \c false.
*/

/*!
    \since 6.6

    Starts an event loop that runs until the given signal is received
    or \a timeout has passed, whichever happens first.

    \a timeout is any valid std::chrono::duration (std::chrono::seconds,
    std::chrono::milliseconds ...etc).

    Returns \c true if the signal was emitted at least once in \a timeout,
    otherwise returns \c false.

    Example:
    \code
        using namespace std::chrono_literals;
        QSignalSpy spy(object, signal);
        spy.wait(2s);
    \endcode
*/
bool QSignalSpy::wait(std::chrono::milliseconds timeout)
{
    QMutexLocker locker(&m_mutex);
    Q_ASSERT(!m_waiting);
    const qsizetype origCount = size();
    m_waiting = true;
    locker.unlock();

    m_loop.enterLoop(timeout);

    locker.relock();
    m_waiting = false;
    return size() > origCount;
}

static bool isSignalMetaMethodValid(QMetaMethod signal)
{
    if (!signal.isValid()) {
        qWarning("QSignalSpy: Null signal is not valid");
        return false;
    }

    if (signal.methodType() != QMetaMethod::Signal) {
        qWarning("QSignalSpy: Not a signal: '%s'", signal.methodSignature().constData());
        return false;
    }

    return true;
}

static bool isObjectValid(const QObject *object)
{
    const bool valid = !!object;

    if (!valid)
        qWarning("QSignalSpy: Cannot spy on a null object");

    return valid;
}

QSignalSpy::ObjectSignal QSignalSpy::verify(const QObject *obj, const char *aSignal)
{
    if (!isObjectValid(obj))
        return {};

    if (!aSignal) {
        qWarning("QSignalSpy: Null signal name is not valid");
        return {};
    }

    if (((aSignal[0] - '0') & 0x03) != QSIGNAL_CODE) {
        qWarning("QSignalSpy: Not a valid signal, use the SIGNAL macro");
        return {};
    }

    const QByteArray ba = QMetaObject::normalizedSignature(aSignal + 1);
    const QMetaObject * const mo = obj->metaObject();
    const int sigIndex = mo->indexOfMethod(ba.constData());
    if (sigIndex < 0) {
        qWarning("QSignalSpy: No such signal: '%s'", ba.constData());
        return {};
    }

    return verify(obj, mo->method(sigIndex));
}

QSignalSpy::ObjectSignal QSignalSpy::verify(const QObject *obj, QMetaMethod signal)
{
    if (isObjectValid(obj) && isSignalMetaMethodValid(signal))
        return {obj, signal};
    else
        return {};
}

static QList<int> makeArgs(QMetaMethod member, const QObject *obj)
{
    QList<int> result;
    result.reserve(member.parameterCount());
    for (int i = 0; i < member.parameterCount(); ++i) {
        QMetaType tp = member.parameterMetaType(i);
        if (!tp.isValid() && obj) {
            void *argv[] = { &tp, &i };
            QMetaObject::metacall(const_cast<QObject*>(obj),
                                  QMetaObject::RegisterMethodArgumentMetaType,
                                  member.methodIndex(), argv);
        }
        if (!tp.isValid()) {
            qWarning("QSignalSpy: Unable to handle parameter '%s' of type '%s' of method '%s',"
                     " use qRegisterMetaType to register it.",
                     member.parameterNames().at(i).constData(),
                     member.parameterTypes().at(i).constData(),
                     member.name().constData());
        }
        result.append(tp.id());
    }
    return result;
}

class QSignalSpyPrivate : public QObject
{
    QSignalSpy * const q;
public:
    explicit QSignalSpyPrivate(QSignalSpy *qq) : q(qq) {}

    int qt_metacall(QMetaObject::Call call, int methodId, void **a) override;
};

QSignalSpy::QSignalSpy(ObjectSignal os)
    : sig(os.sig.methodSignature()),
      args(os.obj ? makeArgs(os.sig, os.obj) : QList<int>{})
{
    if (!os.obj)
        return;

    auto i = std::make_unique<QSignalSpyPrivate>(this);

    const auto signalIndex = os.sig.methodIndex();
    const auto slotIndex = QObject::staticMetaObject.methodCount();
    if (!QMetaObject::connect(os.obj, signalIndex,
                              i.get(), slotIndex, Qt::DirectConnection)) {
        qWarning("QSignalSpy: QMetaObject::connect returned false. Unable to connect.");
        return;
    }

    d_ptr = std::move(i);
}

/*!
    Destructor.
*/
QSignalSpy::~QSignalSpy()
    = default;

void QSignalSpy::appendArgs(void **a)
{
    QList<QVariant> list;
    list.reserve(args.size());
    for (qsizetype i = 0; i < args.size(); ++i) {
        const QMetaType::Type type = static_cast<QMetaType::Type>(args.at(i));
        if (type == QMetaType::QVariant)
            list << *reinterpret_cast<QVariant *>(a[i + 1]);
        else
            list << QVariant(QMetaType(type), a[i + 1]);
    }
    QMutexLocker locker(&m_mutex);
    append(std::move(list));

    if (m_waiting) {
        locker.unlock();
        m_loop.exitLoop();
    }
}

/*!
    \reimp
    \internal
*/
int QSignalSpyPrivate::qt_metacall(QMetaObject::Call call, int methodId, void **a)
{
    methodId = QObject::qt_metacall(call, methodId, a);
    if (methodId < 0)
        return methodId;

    if (call == QMetaObject::InvokeMetaMethod) {
        if (methodId == 0) {
            q->appendArgs(a);
        }
        --methodId;
    }
    return methodId;
}

QT_END_NAMESPACE
