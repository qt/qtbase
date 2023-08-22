// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QPermission>

#include <QTest>

struct DummyPermission // a minimal QPermission-compatible type
{
    using QtPermissionHelper = void;
    int state = 0;
};
Q_DECLARE_METATYPE(DummyPermission)

class tst_QPermission : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void converting_Dummy() const { return converting_impl<DummyPermission>(); }
    void converting_Location() const { return converting_impl<QLocationPermission>(); }
    void converting_Calendar() const { return converting_impl<QCalendarPermission>(); }
    void converting_Contacts() const { return converting_impl<QContactsPermission>(); }
    void converting_Camera() const { return converting_impl<QCameraPermission>(); }
    void converting_Microphone() const { return converting_impl<QMicrophonePermission>(); }
    void converting_Bluetooth() const { return converting_impl<QBluetoothPermission>(); }

    void conversionMaintainsState() const;

    void functorWithoutContext();
    void functorWithContextInThread();
    void receiverInThread();
    void destroyedContextObject();
private:
    template <typename T>
    void converting_impl() const;
};

template <typename T>
void tst_QPermission::converting_impl() const
{
    T concrete;
    const T cconcrete = concrete;
    const auto metaType = QMetaType::fromType<T>();

    // construction is implicit:
    // from rvalue:
    {
        QPermission p = T();
        QCOMPARE_EQ(p.type(), metaType);
    }
    // from mutable lvalue:
    {
        QPermission p = concrete;
        QCOMPARE_EQ(p.type(), metaType);
    }
    // from const lvalue:
    {
        QPermission p = cconcrete;
        QCOMPARE_EQ(p.type(), metaType);
    }

    // value<>() compiles:
    {
        const QPermission p = concrete;
        auto v = p.value<T>();
        static_assert(std::is_same_v<decltype(v), std::optional<T>>);
        QCOMPARE_NE(v, std::nullopt);
    }
}

void tst_QPermission::conversionMaintainsState() const
{
    DummyPermission dummy{42}, dummy_default;
    QCOMPARE_NE(dummy.state, dummy_default.state);

    QLocationPermission loc, loc_default;
    QCOMPARE_EQ(loc_default.accuracy(), QLocationPermission::Accuracy::Approximate);
    QCOMPARE_EQ(loc_default.availability(), QLocationPermission::Availability::WhenInUse);

    loc.setAccuracy(QLocationPermission::Accuracy::Precise);
    loc.setAvailability(QLocationPermission::Availability::Always);

    QCOMPARE_EQ(loc.accuracy(), QLocationPermission::Accuracy::Precise);
    QCOMPARE_EQ(loc.availability(), QLocationPermission::Availability::Always);

    QCalendarPermission cal, cal_default;
    QCOMPARE_EQ(cal_default.accessMode(), QCalendarPermission::AccessMode::ReadOnly);

    cal.setAccessMode(QCalendarPermission::AccessMode::ReadWrite);

    QCOMPARE_EQ(cal.accessMode(), QCalendarPermission::AccessMode::ReadWrite);

    QContactsPermission con, con_default;
    QCOMPARE_EQ(con_default.accessMode(), QContactsPermission::AccessMode::ReadOnly);

    con.setAccessMode(QContactsPermission::AccessMode::ReadWrite);

    QCOMPARE_EQ(con.accessMode(), QContactsPermission::AccessMode::ReadWrite);

    //
    // QCameraPermission, QMicrophonePermission, QBluetoothPermission don't have
    // state at the time of writing
    //

    QPermission p; // maintain state between the blocks below to test reset behavior

    {
        p = dummy;
        auto v = p.value<DummyPermission>();
        QCOMPARE_NE(v, std::nullopt);
        auto &r = *v;
        QCOMPARE_EQ(r.state, dummy.state);
        // check mismatched returns nullopt:
        QCOMPARE_EQ(p.value<QCalendarPermission>(), std::nullopt);
    }

    {
        p = loc;
        auto v = p.value<QLocationPermission>();
        QCOMPARE_NE(v, std::nullopt);
        auto &r = *v;
        QCOMPARE_EQ(r.accuracy(), loc.accuracy());
        QCOMPARE_EQ(r.availability(), loc.availability());
        // check mismatched returns nullopt:
        QCOMPARE_EQ(p.value<DummyPermission>(), std::nullopt);
    }

    {
        p = con;
        auto v = p.value<QContactsPermission>();
        QCOMPARE_NE(v, std::nullopt);
        auto &r = *v;
        QCOMPARE_EQ(r.accessMode(), con.accessMode());
        // check mismatched returns nullopt:
        QCOMPARE_EQ(p.value<QLocationPermission>(), std::nullopt);
    }

    {
        p = cal;
        auto v = p.value<QCalendarPermission>();
        QCOMPARE_NE(v, std::nullopt);
        auto &r = *v;
        QCOMPARE_EQ(r.accessMode(), cal.accessMode());
        // check mismatched returns nullopt:
        QCOMPARE_EQ(p.value<QContactsPermission>(), std::nullopt);
    }
}

template <typename Func,
          typename T = std::void_t<decltype(qApp->requestPermission(std::declval<DummyPermission>(),
                                                                    std::declval<Func>()))>
         >
void wrapRequestPermission(const QPermission &p, Func &&f)
{
    qApp->requestPermission(p, std::forward<Func>(f));
}

template <typename Functor>
using CompatibleTest = decltype(wrapRequestPermission(std::declval<QPermission>(), std::declval<Functor>()));


// Compile test for context-less functor overloads
void tst_QPermission::functorWithoutContext()
{
    int argc = 0;
    char *argv = nullptr;
    QCoreApplication app(argc, &argv);

    DummyPermission dummy;
#ifdef Q_OS_DARWIN
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Could not find permission plugin for DummyPermission.*"));
#endif

    qApp->requestPermission(dummy, [](const QPermission &permission){
        QVERIFY(permission.value<DummyPermission>());
    });
    wrapRequestPermission(dummy, [](const QPermission &permission){
        QVERIFY(permission.value<DummyPermission>());
    });

    auto compatible = [](const QPermission &) {};
    using Compatible = decltype(compatible);
    auto incompatible = [](const QString &) {};
    using Incompatible = decltype(incompatible);

    static_assert(qxp::is_detected_v<CompatibleTest, Compatible>);
    static_assert(!qxp::is_detected_v<CompatibleTest, Incompatible>);
}

void tst_QPermission::functorWithContextInThread()
{
    int argc = 0;
    char *argv = nullptr;
    QCoreApplication app(argc, &argv);
    QThread::currentThread()->setObjectName("main thread");
    QThread receiverThread;
    receiverThread.setObjectName("receiverThread");
    QObject receiver;
    receiver.moveToThread(&receiverThread);
    receiverThread.start();
    auto guard = qScopeGuard([&receiverThread]{
        receiverThread.quit();
        QVERIFY(receiverThread.wait(1000));
    });

    DummyPermission dummy;
#ifdef Q_OS_DARWIN
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Could not find permission plugin for DummyPermission.*"));
#endif
    QThread *permissionReceiverThread = nullptr;
    qApp->requestPermission(dummy, &receiver, [&](const QPermission &permission){
        auto dummy = permission.value<DummyPermission>();
        QVERIFY(dummy);
        permissionReceiverThread = QThread::currentThread();
    });
    QTRY_COMPARE(permissionReceiverThread, &receiverThread);
}

void tst_QPermission::receiverInThread()
{
    int argc = 0;
    char *argv = nullptr;
    QCoreApplication app(argc, &argv);
    QThread::currentThread()->setObjectName("main thread");
    QThread receiverThread;
    receiverThread.setObjectName("receiverThread");
    class Receiver : public QObject
    {
    public:
        using QObject::QObject;
        void handlePermission(const QPermission &permission)
        {
            auto dummy = permission.value<DummyPermission>();
            QVERIFY(dummy);
            permissionReceiverThread = QThread::currentThread();
        }

        QThread *permissionReceiverThread = nullptr;
    } receiver;
    receiver.moveToThread(&receiverThread);
    receiverThread.start();
    auto guard = qScopeGuard([&receiverThread]{
        receiverThread.quit();
        QVERIFY(receiverThread.wait(1000));
    });

    DummyPermission dummy;
#ifdef Q_OS_DARWIN
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Could not find permission plugin for DummyPermission.*"));
#endif

    qApp->requestPermission(dummy, &receiver, &Receiver::handlePermission);
    QTRY_COMPARE(receiver.permissionReceiverThread, &receiverThread);

    // compile tests: none of these work and the error output isn't horrible
    // qApp->requestPermission(dummy, &receiver, "&tst_QPermission::receiverInThread");
    // qApp->requestPermission(dummy, &receiver, &tst_QPermission::receiverInThread);
    // qApp->requestPermission(dummy, &receiver, &QObject::destroyed);
}

void tst_QPermission::destroyedContextObject()
{
    int argc = 0;
    char *argv = nullptr;
    QCoreApplication app(argc, &argv);

    QObject *context = new QObject;

    DummyPermission dummy;
#ifdef Q_OS_DARWIN
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Could not find permission plugin for DummyPermission.*"));
#endif
    bool permissionReceived = false;
    qApp->requestPermission(dummy, context, [&]{
        permissionReceived = true;
    });
    QVERIFY2(!permissionReceived, "Permission received synchronously");
    delete context;
    QTest::qWait(100);
    QVERIFY(!permissionReceived);
}

QTEST_APPLESS_MAIN(tst_QPermission)
#include "tst_qpermission.moc"
