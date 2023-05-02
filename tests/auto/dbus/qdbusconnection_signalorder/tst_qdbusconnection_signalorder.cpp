// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>
#include <QTest>
#include <QTimer>

using namespace Qt::StringLiterals;

static constexpr int MAX_TEST_DURATION_MS = 10000;
static constexpr int NUM_MESSAGES = 20;

class SignalReceiver : public QObject
{
    Q_OBJECT
public:
    explicit SignalReceiver(QDBusConnection &connection, const QString &serviceName);

    int nextValue() const { return m_nextValue; }
    bool inOrder() const { return m_inOrder; }

Q_SIGNALS:
    void testSignal(int);
    void done();

private Q_SLOTS:
    void testSlot(int number);

private:
    int m_nextValue = 0;
    bool m_inOrder = true;
};

SignalReceiver::SignalReceiver(QDBusConnection &connection, const QString &serviceName)
{
    connection.connect(serviceName, "/", {}, "testSignal", this, SLOT(testSlot(int)));
}

void SignalReceiver::testSlot(int number)
{
    if (m_nextValue != number) {
        m_inOrder = false;
        qWarning("Message out of sequence, expected: %d, received: %d", m_nextValue, number);
    }

    m_nextValue++;

    if (m_nextValue == NUM_MESSAGES) {
        Q_EMIT done();
    }
}

class tst_QDBusConnection_SignalOrder : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void signalOrder();
};

// This is a regression test for QTBUG-105457. The bug is a race condition,
// so it cannot be reliably triggered at each test execution.
void tst_QDBusConnection_SignalOrder::signalOrder()
{
    int argc = 1;
    static char appName[] = "tst_qdbusconnection_signalorder";
    char *argv[] = { appName, 0 };
    QCoreApplication app(argc, argv);

    const QString serviceName =
            u"org.qtproject.tst_dbusconnection_signalorder_%1"_s.arg(app.applicationPid());

    auto connection = QDBusConnection::sessionBus();

    QVERIFY(connection.isConnected());
    QVERIFY(connection.registerService(serviceName));

    // Limit the test execution time in case if something goes wrong inside
    // the signal receiver.
    QTimer::singleShot(MAX_TEST_DURATION_MS, &app, &QCoreApplication::quit);

    SignalReceiver signalReceiver(connection, serviceName);
    connect(&signalReceiver, &SignalReceiver::done, &app, &QCoreApplication::quit);

    QVERIFY(connection.registerObject("/", &signalReceiver, QDBusConnection::ExportAllSlots));

    for (int i = 0; i < NUM_MESSAGES; i++) {
        auto testSignal = QDBusMessage::createSignal("/", serviceName, "testSignal");
        testSignal << i;
        QVERIFY(connection.send(testSignal));
    }

    app.exec();

    QVERIFY(signalReceiver.inOrder());
    QCOMPARE(signalReceiver.nextValue(), NUM_MESSAGES);
}

QTEST_APPLESS_MAIN(tst_QDBusConnection_SignalOrder)

#include "tst_qdbusconnection_signalorder.moc"
