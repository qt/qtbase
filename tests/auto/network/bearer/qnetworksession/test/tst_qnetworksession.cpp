/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include "../../qbearertestcommon.h"

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/qnetworkconfigmanager.h>
#include <QtNetwork/qnetworksession.h>
#include <private/qnetworksession_p.h>
#endif

QT_USE_NAMESPACE

// Can be used to configure tests that require manual attention (such as roaming)
//#define QNETWORKSESSION_MANUAL_TESTS 1

#ifndef QT_NO_BEARERMANAGEMENT
Q_DECLARE_METATYPE(QNetworkConfiguration::Type)
#endif

class tst_QNetworkSession : public QObject
{
    Q_OBJECT

#ifndef QT_NO_BEARERMANAGEMENT
public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void robustnessBombing();

    void sessionClosing_data();
    void sessionClosing();

    void outOfProcessSession();
    void invalidSession();

    void repeatedOpenClose_data();
    void repeatedOpenClose();

    void sessionProperties_data();
    void sessionProperties();

    void userChoiceSession_data();
    void userChoiceSession();

    void sessionOpenCloseStop_data();
    void sessionOpenCloseStop();

    void sessionAutoClose_data();
    void sessionAutoClose();

    void usagePolicies();

private:
    QNetworkConfigurationManager manager;
    int inProcessSessionManagementCount;
    QString lackeyDir;
#endif
};

#ifndef QT_NO_BEARERMANAGEMENT
// Helper functions
bool openSession(QNetworkSession *session);
bool closeSession(QNetworkSession *session, bool lastSessionOnConfiguration = true);
void updateConfigurations();
void printConfigurations();
QNetworkConfiguration suitableConfiguration(QString bearerType, QNetworkConfiguration::Type configType);

void tst_QNetworkSession::initTestCase()
{
    qRegisterMetaType<QNetworkConfiguration>("QNetworkConfiguration");
    qRegisterMetaType<QNetworkConfiguration::Type>("QNetworkConfiguration::Type");

    inProcessSessionManagementCount = -1;

    QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
    manager.updateConfigurations();
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, TestTimeOut);

    lackeyDir = QFINDTESTDATA("lackey");
    QVERIFY2(!lackeyDir.isEmpty(), qPrintable(
        QString::fromLatin1("Couldn't find lackey dir starting from %1.").arg(QDir::currentPath())));
}

void tst_QNetworkSession::cleanupTestCase()
{
    if (!(manager.capabilities() & QNetworkConfigurationManager::SystemSessionSupport) &&
        (manager.capabilities() & QNetworkConfigurationManager::CanStartAndStopInterfaces) &&
        inProcessSessionManagementCount == 0) {
        qWarning("No usable configurations found to complete all possible tests in "
                 "inProcessSessionManagement()");
    }
}

// Robustness test for calling interfaces in nonsense order / with nonsense parameters
void tst_QNetworkSession::robustnessBombing()
{
    QNetworkConfigurationManager mgr;
    QNetworkSession testSession(mgr.defaultConfiguration());
    // Should not reset even session is not opened
    testSession.migrate();
    testSession.accept();
    testSession.ignore();
    testSession.reject();
}

void tst_QNetworkSession::sessionClosing_data() {
    QTest::addColumn<QString>("bearerType");
    QTest::addColumn<QNetworkConfiguration::Type>("configurationType");

    QTest::newRow("WLAN_IAP") << "WLAN" << QNetworkConfiguration::InternetAccessPoint;
    QTest::newRow("Cellular_IAP") << "cellular" << QNetworkConfiguration::InternetAccessPoint;
    QTest::newRow("SNAP") << "bearer_type_not_relevant_with_SNAPs" << QNetworkConfiguration::ServiceNetwork;
}

// Testcase for closing the session at unexpected times
void tst_QNetworkSession::sessionClosing()
{
    QFETCH(QString, bearerType);
    QFETCH(QNetworkConfiguration::Type, configurationType);

    // Update configurations so that WLANs are discovered too.
    updateConfigurations();

    // First check that opening once succeeds and determine if test is doable
    QNetworkConfiguration config = suitableConfiguration(bearerType, configurationType);
    if (!config.isValid())
        QSKIP("No suitable configurations, skipping this round of repeated open-close test.");
    qDebug() << "Using following configuration to bomb with close(): " << config.name();
    QNetworkSession session(config);
    if (!openSession(&session) || !closeSession(&session))
        QSKIP("Unable to open/close session, skipping this round of close() bombing.");

    qDebug() << "Closing without issuing open()";
    session.close();

    for (int i = 0; i < 25; i++) {
        qDebug() << "Opening and then waiting: " << i * 100 << " ms before closing.";
        session.open();
        QTest::qWait(i*100);
        session.close();
        // Sooner or later session must end in Disconnected state,
        // no matter what the phase was.
        QTRY_VERIFY_WITH_TIMEOUT(session.state() == QNetworkSession::Disconnected, TestTimeOut);
        QTest::qWait(200); // Give platform a breathe, otherwise we'll be catching other errors
    }
}

void tst_QNetworkSession::invalidSession()
{
    // 1. Verify that session created with invalid configuration remains in invalid state
    QNetworkSession session(QNetworkConfiguration(), 0);
    QVERIFY(!session.isOpen());
    QCOMPARE(session.state(), QNetworkSession::Invalid);
    QCOMPARE(session.error(), QNetworkSession::InvalidConfigurationError);

    // 2. Verify that opening session with invalid configuration both 1) emits invalidconfigurationerror and 2) sets session's state as invalid.
    QSignalSpy errorSpy(&session, SIGNAL(error(QNetworkSession::SessionError)));
    session.open();
    session.waitForOpened(1000); // Should bail out right away
    QCOMPARE(errorSpy.count(), 1);
    QNetworkSession::SessionError error =
           qvariant_cast<QNetworkSession::SessionError> (errorSpy.first().at(0));
    QCOMPARE(error, QNetworkSession::InvalidConfigurationError);
    QCOMPARE(session.error(), QNetworkSession::InvalidConfigurationError);
    QCOMPARE(session.state(), QNetworkSession::Invalid);

#ifdef QNETWORKSESSION_MANUAL_TESTS

    QNetworkConfiguration invalidatedConfig = suitableConfiguration("WLAN",QNetworkConfiguration::InternetAccessPoint);
    if (invalidatedConfig.isValid()) {
        // 3. Verify that invalidating a session after its successfully configured works
        QNetworkSession invalidatedSession(invalidatedConfig);
        qDebug() << "Delete the WLAN IAP from phone now (waiting 60 seconds): " << invalidatedConfig.name();
        QTest::qWait(60000);
        QVERIFY(!invalidatedConfig.isValid());
        QCOMPARE(invalidatedSession.state(), QNetworkSession::Invalid);
        qDebug() << "Add the WLAN IAP back (waiting 60 seconds): " << invalidatedConfig.name();
        QTest::qWait(60000);
    }

    QNetworkConfiguration definedConfig = suitableConfiguration("WLAN",QNetworkConfiguration::InternetAccessPoint);
    if (definedConfig.isValid()) {
        // 4. Verify that opening a session with defined configuration emits error and enters notavailable-state
        // TODO these timer waits should be changed to waiting appropriate signals, now these wait excessively
        qDebug() << "Shutdown WLAN IAP (waiting 60 seconds): " << definedConfig.name();
        QTest::qWait(60000);
        // Shutting down WLAN should bring back to defined -state.
        QVERIFY((definedConfig.state() & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined);
        QNetworkSession definedSession(definedConfig);
        QSignalSpy errorSpy(&definedSession, SIGNAL(error(QNetworkSession::SessionError)));
        QNetworkSession::SessionError sessionError;
        updateConfigurations();

        definedSession.open();
        updateConfigurations();

        QVERIFY(definedConfig.isValid()); // Session remains valid
        QVERIFY(definedSession.state() == QNetworkSession::NotAvailable); // State is not available because WLAN is not in coverage
        QVERIFY(!errorSpy.isEmpty()); // Session tells with error about invalidated configuration
        sessionError = qvariant_cast<QNetworkSession::SessionError> (errorSpy.first().at(0));
        QCOMPARE(sessionError, QNetworkSession::InvalidConfigurationError);
        qDebug() << "Turn the WLAN IAP back on (waiting 60 seconds): " << definedConfig.name();
        QTest::qWait(60000);
        updateConfigurations();
        QCOMPARE(definedConfig.state(), QNetworkConfiguration::Discovered);
    }
#endif
}

void tst_QNetworkSession::sessionProperties_data()
{
    QTest::addColumn<QNetworkConfiguration>("configuration");

    QTest::newRow("invalid configuration") << QNetworkConfiguration();

    foreach (const QNetworkConfiguration &config, manager.allConfigurations()) {
        const QString name = config.name().isEmpty() ? QString("<Hidden>") : config.name();
        QTest::newRow(name.toLocal8Bit().constData()) << config;
    }
}

void tst_QNetworkSession::sessionProperties()
{
    QFETCH(QNetworkConfiguration, configuration);
    QNetworkSession session(configuration);
    QCOMPARE(session.configuration(), configuration);
    QStringList validBearerNames = QStringList() << QLatin1String("Unknown")
                                                 << QLatin1String("Ethernet")
                                                 << QLatin1String("WLAN")
                                                 << QLatin1String("2G")
                                                 << QLatin1String("CDMA2000")
                                                 << QLatin1String("WCDMA")
                                                 << QLatin1String("HSPA")
                                                 << QLatin1String("Bluetooth")
                                                 << QLatin1String("WiMAX")
                                                 << QLatin1String("BearerEVDO")
                                                 << QLatin1String("BearerLTE")
                                                 << QLatin1String("Bearer3G")
                                                 << QLatin1String("Bearer4G");

    if (!configuration.isValid()) {
        QVERIFY(configuration.bearerTypeName().isEmpty());
    } else {
        switch (configuration.type())
        {
            case QNetworkConfiguration::ServiceNetwork:
            case QNetworkConfiguration::UserChoice:
            default:
                QVERIFY(configuration.bearerTypeName().isEmpty());
                break;
            case QNetworkConfiguration::InternetAccessPoint:
                QVERIFY(validBearerNames.contains(configuration.bearerTypeName()));
                break;
        }
    }

    // QNetworkSession::interface() should return an invalid interface unless
    // session is in the connected state.
#ifndef QT_NO_NETWORKINTERFACE
    QCOMPARE(session.state() == QNetworkSession::Connected, session.interface().isValid());
#endif

    if (!configuration.isValid()) {
        QVERIFY(configuration.state() == QNetworkConfiguration::Undefined &&
                session.state() == QNetworkSession::Invalid);
    } else {
        switch (configuration.state()) {
        case QNetworkConfiguration::Undefined:
            QCOMPARE(session.state(), QNetworkSession::NotAvailable);
            break;
        case QNetworkConfiguration::Defined:
            QCOMPARE(session.state(), QNetworkSession::NotAvailable);
            break;
        case QNetworkConfiguration::Discovered:
            QVERIFY(session.state() == QNetworkSession::Connecting ||
                    session.state() == QNetworkSession::Disconnected);
            break;
        case QNetworkConfiguration::Active:
            QVERIFY(session.state() == QNetworkSession::Connected ||
                    session.state() == QNetworkSession::Closing ||
                    session.state() == QNetworkSession::Roaming);
            break;
        default:
            QFAIL("Invalid configuration state");
        };
    }
}

void tst_QNetworkSession::repeatedOpenClose_data() {
    QTest::addColumn<QString>("bearerType");
    QTest::addColumn<QNetworkConfiguration::Type>("configurationType");
    QTest::addColumn<int>("repeatTimes");

    QTest::newRow("WLAN_IAP") << "WLAN" << QNetworkConfiguration::InternetAccessPoint << 3;
    // QTest::newRow("Cellular_IAP") << "cellular" << QNetworkConfiguration::InternetAccessPoint << 3;
    // QTest::newRow("SNAP") << "bearer_type_not_relevant_with_SNAPs" << QNetworkConfiguration::ServiceNetwork << 3;
}

// Tests repeated-open close.
void tst_QNetworkSession::repeatedOpenClose()
{
    QFETCH(QString, bearerType);
    QFETCH(QNetworkConfiguration::Type, configurationType);
    QFETCH(int, repeatTimes);

    // First check that opening once succeeds and determine if repeatable testing is doable
    QNetworkConfiguration config = suitableConfiguration(bearerType, configurationType);
    if (!config.isValid())
        QSKIP("No suitable configurations, skipping this round of repeated open-close test.");
    qDebug() << "Using following configuratio to repeatedly open and close: " << config.name();
    QNetworkSession permanentSession(config);
    if (!openSession(&permanentSession) || !closeSession(&permanentSession))
        QSKIP("Unable to open/close session, skipping this round of repeated open-close test.");
    for (int i = 0; i < repeatTimes; i++) {
       qDebug() << "Opening, loop number " << i;
       QVERIFY(openSession(&permanentSession));
       qDebug() << "Closing, loop number, then waiting 5 seconds: " << i;
       QVERIFY(closeSession(&permanentSession));
       QTest::qWait(5000);
    }
}

void tst_QNetworkSession::userChoiceSession_data()
{
    QTest::addColumn<QNetworkConfiguration>("configuration");

    QNetworkConfiguration config = manager.defaultConfiguration();
    if (config.type() == QNetworkConfiguration::UserChoice)
        QTest::newRow("UserChoice") << config;
    else
        QSKIP("Default configuration is not a UserChoice configuration.");
}

void tst_QNetworkSession::userChoiceSession()
{
    QFETCH(QNetworkConfiguration, configuration);

    QCOMPARE(configuration.type(), QNetworkConfiguration::UserChoice);

    QNetworkSession session(configuration);

    // Check that configuration was really set
    QCOMPARE(session.configuration(), configuration);

    QVERIFY(!session.isOpen());

    // Check that session is not active
    QVERIFY(session.sessionProperty("ActiveConfiguration").toString().isEmpty());

    // The remaining tests require the session to be not NotAvailable.
    if (session.state() == QNetworkSession::NotAvailable)
        QSKIP("Network is not available.");

    QSignalSpy sessionOpenedSpy(&session, SIGNAL(opened()));
    QSignalSpy sessionClosedSpy(&session, SIGNAL(closed()));
    QSignalSpy stateChangedSpy(&session, SIGNAL(stateChanged(QNetworkSession::State)));
    QSignalSpy errorSpy(&session, SIGNAL(error(QNetworkSession::SessionError)));

    // Test opening the session.
    {
        bool expectStateChange = session.state() != QNetworkSession::Connected;

        session.open();
        session.waitForOpened();

        if (session.isOpen())
            QVERIFY(!sessionOpenedSpy.isEmpty() || !errorSpy.isEmpty());
        if (!errorSpy.isEmpty()) {
            QNetworkSession::SessionError error =
                qvariant_cast<QNetworkSession::SessionError>(errorSpy.first().at(0));
            if (error == QNetworkSession::OperationNotSupportedError) {
                // The session needed to bring up the interface,
                // but the operation is not supported.
                QSKIP("Configuration does not support open().");
            } else if (error == QNetworkSession::InvalidConfigurationError) {
                // The session needed to bring up the interface, but it is not possible for the
                // specified configuration.
                if ((session.configuration().state() & QNetworkConfiguration::Discovered) ==
                    QNetworkConfiguration::Discovered) {
                    QFAIL("Failed to open session for Discovered configuration.");
                } else {
                    QSKIP("Cannot test session for non-Discovered configuration.");
                }
            } else if (error == QNetworkSession::UnknownSessionError) {
                QSKIP("Unknown session error.");
            } else {
                QFAIL("Error opening session.");
            }
        } else if (!sessionOpenedSpy.isEmpty()) {
            QCOMPARE(sessionOpenedSpy.count(), 1);
            QVERIFY(sessionClosedSpy.isEmpty());
            QVERIFY(errorSpy.isEmpty());

            if (expectStateChange)
                QTRY_VERIFY_WITH_TIMEOUT(!stateChangedSpy.isEmpty(), TestTimeOut);

            QCOMPARE(session.state(), QNetworkSession::Connected);
#ifndef QT_NO_NETWORKINTERFACE
            QVERIFY(session.interface().isValid());
#endif
            const QString userChoiceIdentifier =
                session.sessionProperty("UserChoiceConfiguration").toString();

            QVERIFY(!userChoiceIdentifier.isEmpty());
            QVERIFY(userChoiceIdentifier != configuration.identifier());

            QNetworkConfiguration userChoiceConfiguration =
                manager.configurationFromIdentifier(userChoiceIdentifier);

            QVERIFY(userChoiceConfiguration.isValid());
            QVERIFY(userChoiceConfiguration.type() != QNetworkConfiguration::UserChoice);

            const QString testIdentifier("abc");
            //resetting UserChoiceConfiguration is ignored (read only property)
            session.setSessionProperty("UserChoiceConfiguration", testIdentifier);
            QVERIFY(session.sessionProperty("UserChoiceConfiguration").toString() != testIdentifier);

            const QString activeIdentifier =
                session.sessionProperty("ActiveConfiguration").toString();

            QVERIFY(!activeIdentifier.isEmpty());
            QVERIFY(activeIdentifier != configuration.identifier());

            QNetworkConfiguration activeConfiguration =
                manager.configurationFromIdentifier(activeIdentifier);

            QVERIFY(activeConfiguration.isValid());
            QCOMPARE(activeConfiguration.type(), QNetworkConfiguration::InternetAccessPoint);

            //resetting ActiveConfiguration is ignored (read only property)
            session.setSessionProperty("ActiveConfiguration", testIdentifier);
            QVERIFY(session.sessionProperty("ActiveConfiguration").toString() != testIdentifier);

            if (userChoiceConfiguration.type() == QNetworkConfiguration::InternetAccessPoint) {
                QCOMPARE(userChoiceConfiguration, activeConfiguration);
            } else {
                QCOMPARE(userChoiceConfiguration.type(), QNetworkConfiguration::ServiceNetwork);
                QVERIFY(userChoiceConfiguration.children().contains(activeConfiguration));
            }
        } else {
            QFAIL("Timeout waiting for session to open.");
        }
    }
}

void tst_QNetworkSession::sessionOpenCloseStop_data()
{
    QTest::addColumn<QNetworkConfiguration>("configuration");
    QTest::addColumn<bool>("forceSessionStop");

    foreach (const QNetworkConfiguration &config, manager.allConfigurations()) {
        const QString name = config.name().isEmpty() ? QString("<Hidden>") : config.name();
        QTest::newRow((name + QLatin1String(" close")).toLocal8Bit().constData())
            << config << false;
        QTest::newRow((name + QLatin1String(" stop")).toLocal8Bit().constData())
            << config << true;
    }

    inProcessSessionManagementCount = 0;
}

void tst_QNetworkSession::sessionOpenCloseStop()
{
    QFETCH(QNetworkConfiguration, configuration);
    QFETCH(bool, forceSessionStop);
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QSKIP("Deadlocks on Linux due to QTBUG-45655");
#endif

    QNetworkSession session(configuration);

    // Test initial state of the session.
    {
        QCOMPARE(session.configuration(), configuration);
        QVERIFY(!session.isOpen());
        // session may be invalid if configuration is removed between when
        // sessionOpenCloseStop_data() is called and here.
        QVERIFY((configuration.isValid() && (session.state() != QNetworkSession::Invalid)) ||
                (!configuration.isValid() && (session.state() == QNetworkSession::Invalid)));
        QCOMPARE(session.error(), QNetworkSession::UnknownSessionError);
    }

    // The remaining tests require the session to be not NotAvailable.
    if (session.state() == QNetworkSession::NotAvailable)
        QSKIP("Network is not available.");

    QSignalSpy sessionOpenedSpy(&session, SIGNAL(opened()));
    QSignalSpy sessionClosedSpy(&session, SIGNAL(closed()));
    QSignalSpy stateChangedSpy(&session, SIGNAL(stateChanged(QNetworkSession::State)));
    QSignalSpy errorSpy(&session, SIGNAL(error(QNetworkSession::SessionError)));

    // Test opening the session.
    {
        QNetworkSession::State previousState = session.state();
        bool expectStateChange = previousState != QNetworkSession::Connected;

        session.open();
        session.waitForOpened();

        // Wait until the configuration is uptodate as well, it may be signaled 'connected'
        // bit later than the session
        QTRY_VERIFY_WITH_TIMEOUT(configuration.state() == QNetworkConfiguration::Active, TestTimeOut);

        if (session.isOpen())
            QVERIFY(!sessionOpenedSpy.isEmpty() || !errorSpy.isEmpty());
        if (!errorSpy.isEmpty()) {
            QNetworkSession::SessionError error =
                qvariant_cast<QNetworkSession::SessionError>(errorSpy.first().at(0));

            QCOMPARE(session.state(), previousState);

            if (error == QNetworkSession::OperationNotSupportedError) {
                // The session needed to bring up the interface,
                // but the operation is not supported.
                QSKIP("Configuration does not support open().");
            } else if (error == QNetworkSession::InvalidConfigurationError) {
                // The session needed to bring up the interface, but it is not possible for the
                // specified configuration.
                if ((session.configuration().state() & QNetworkConfiguration::Discovered) ==
                    QNetworkConfiguration::Discovered) {
                    QFAIL("Failed to open session for Discovered configuration.");
                } else {
                    QSKIP("Cannot test session for non-Discovered configuration.");
                }
            } else if (error == QNetworkSession::UnknownSessionError) {
                QSKIP("Unknown Session error.");
            } else {
                QFAIL("Error opening session.");
            }
        } else if (!sessionOpenedSpy.isEmpty()) {
            QCOMPARE(sessionOpenedSpy.count(), 1);
            QVERIFY(sessionClosedSpy.isEmpty());
            QVERIFY(errorSpy.isEmpty());

            if (expectStateChange) {
                QTRY_VERIFY_WITH_TIMEOUT(stateChangedSpy.count() >= 2, TestTimeOut);

                QNetworkSession::State state =
                    qvariant_cast<QNetworkSession::State>(stateChangedSpy.at(0).at(0));
                QCOMPARE(state, QNetworkSession::Connecting);

                state = qvariant_cast<QNetworkSession::State>(stateChangedSpy.at(1).at(0));
                QCOMPARE(state, QNetworkSession::Connected);
            }

            QCOMPARE(session.state(), QNetworkSession::Connected);
#ifndef QT_NO_NETWORKINTERFACE
            QVERIFY(session.interface().isValid());
#endif
        } else {
            QFAIL("Timeout waiting for session to open.");
        }
    }

    sessionOpenedSpy.clear();
    sessionClosedSpy.clear();
    stateChangedSpy.clear();
    errorSpy.clear();

    QNetworkSession session2(configuration);

    QSignalSpy sessionOpenedSpy2(&session2, SIGNAL(opened()));
    QSignalSpy sessionClosedSpy2(&session2, SIGNAL(closed()));
    QSignalSpy stateChangedSpy2(&session2, SIGNAL(stateChanged(QNetworkSession::State)));
    QSignalSpy errorSpy2(&session2, SIGNAL(error(QNetworkSession::SessionError)));

    // Test opening a second session.
    {
        QCOMPARE(session2.configuration(), configuration);
        QVERIFY(!session2.isOpen());
        QCOMPARE(session2.state(), QNetworkSession::Connected);
        QCOMPARE(session.error(), QNetworkSession::UnknownSessionError);

        session2.open();

        QTRY_VERIFY_WITH_TIMEOUT(!sessionOpenedSpy2.isEmpty() || !errorSpy2.isEmpty(), TestTimeOut);

        if (errorSpy2.isEmpty()) {
            QVERIFY(session2.isOpen());
            QCOMPARE(session2.state(), QNetworkSession::Connected);
        }
        QVERIFY(session.isOpen());
        QCOMPARE(session.state(), QNetworkSession::Connected);
#ifndef QT_NO_NETWORKINTERFACE
        QVERIFY(session.interface().isValid());
        if (errorSpy2.isEmpty()) {
            QCOMPARE(session.interface().hardwareAddress(), session2.interface().hardwareAddress());
            QCOMPARE(session.interface().index(), session2.interface().index());
        }
#endif
    }

    sessionOpenedSpy2.clear();

    if (forceSessionStop && session2.isOpen()) {
        // Test forcing the second session to stop the interface.
        QNetworkSession::State previousState = session.state();
        bool expectStateChange = previousState != QNetworkSession::Disconnected;
        session2.stop();

        // QNetworkSession::stop() must result either closed() signal
        // or error() signal
        QTRY_VERIFY_WITH_TIMEOUT(!sessionClosedSpy2.isEmpty() || !errorSpy2.isEmpty(), TestTimeOut);
        QVERIFY(!session2.isOpen());

        if (!errorSpy2.isEmpty()) {
            // QNetworkSession::stop() resulted error() signal for session2
            // => also session should emit error() signal
            QTRY_VERIFY_WITH_TIMEOUT(!errorSpy.isEmpty(), TestTimeOut);

            // check for SessionAbortedError
            QNetworkSession::SessionError error =
                qvariant_cast<QNetworkSession::SessionError>(errorSpy.first().at(0));
            QNetworkSession::SessionError error2 =
                qvariant_cast<QNetworkSession::SessionError>(errorSpy2.first().at(0));

            QCOMPARE(error, QNetworkSession::SessionAbortedError);
            QCOMPARE(error2, QNetworkSession::SessionAbortedError);

            QCOMPARE(errorSpy.count(), 1);
            QCOMPARE(errorSpy2.count(), 1);

            errorSpy.clear();
            errorSpy2.clear();
        }

        QVERIFY(errorSpy.isEmpty());
        QVERIFY(errorSpy2.isEmpty());

        // Wait for Disconnected state
        QTRY_NOOP(session2.state() == QNetworkSession::Disconnected);

        if (expectStateChange)
            QTRY_VERIFY_WITH_TIMEOUT(stateChangedSpy2.count() >= 1 || !errorSpy2.isEmpty(), TestTimeOut);

        if (!errorSpy2.isEmpty()) {
            QCOMPARE(session2.state(), previousState);
            QCOMPARE(session.state(), previousState);

            QNetworkSession::SessionError error =
                qvariant_cast<QNetworkSession::SessionError>(errorSpy2.first().at(0));
            if (error == QNetworkSession::OperationNotSupportedError) {
                // The session needed to bring down the interface,
                // but the operation is not supported.
                QSKIP("Configuration does not support stop().");
            } else if (error == QNetworkSession::InvalidConfigurationError) {
                // The session needed to bring down the interface, but it is not possible for the
                // specified configuration.
                if ((session.configuration().state() & QNetworkConfiguration::Discovered) ==
                    QNetworkConfiguration::Discovered) {
                    QFAIL("Failed to stop session for Discovered configuration.");
                } else {
                    QSKIP("Cannot test session for non-Discovered configuration.");
                }
            } else {
                QFAIL("Error stopping session.");
            }
        } else if (!sessionClosedSpy2.isEmpty()) {
            if (expectStateChange) {
                if (configuration.type() == QNetworkConfiguration::ServiceNetwork) {
                    bool roamedSuccessfully = false;

                    QNetworkSession::State state;
                    if (stateChangedSpy2.count() == 4) {
                        state = qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(0).at(0));
                        QCOMPARE(state, QNetworkSession::Connecting);

                        state = qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(1).at(0));
                        QCOMPARE(state, QNetworkSession::Connected);

                        state = qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(2).at(0));
                        QCOMPARE(state, QNetworkSession::Closing);

                        state = qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(3).at(0));
                        QCOMPARE(state, QNetworkSession::Disconnected);
                    } else if (stateChangedSpy2.count() == 2) {
                        state = qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(0).at(0));
                        QCOMPARE(state, QNetworkSession::Closing);

                        state = qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(1).at(0));
                        QCOMPARE(state, QNetworkSession::Disconnected);
                    } else {
                        QFAIL("Unexpected amount of state changes when roaming.");
                    }

                    QTRY_VERIFY_WITH_TIMEOUT(session.state() == QNetworkSession::Roaming ||
                                session.state() == QNetworkSession::Connected ||
                                session.state() == QNetworkSession::Disconnected, TestTimeOut);

                    QTRY_VERIFY_WITH_TIMEOUT(stateChangedSpy.count() > 0, TestTimeOut);
                    state = qvariant_cast<QNetworkSession::State>(stateChangedSpy.at(stateChangedSpy.count() - 1).at(0));

                    for (int i = 0; i < stateChangedSpy.count(); i++) {
                        QNetworkSession::State state_temp =
                                qvariant_cast<QNetworkSession::State>(stateChangedSpy.at(i).at(0));
                        // Extra debug because a fragile point in testcase because statuses vary.
                        qDebug() << "------- Statechange spy at: " << i << " is " << state_temp;
                    }

                    if (state == QNetworkSession::Roaming) {
                        QTRY_VERIFY_WITH_TIMEOUT(session.state() == QNetworkSession::Connected, TestTimeOut);
                        QTRY_VERIFY_WITH_TIMEOUT(session2.state() == QNetworkSession::Connected, TestTimeOut);
                        roamedSuccessfully = true;
                    } else if (state == QNetworkSession::Closing) {
                        QTRY_VERIFY_WITH_TIMEOUT(session2.state() == QNetworkSession::Disconnected, TestTimeOut);
                        QTRY_VERIFY_WITH_TIMEOUT(session.state() == QNetworkSession::Connected ||
                                    session.state() == QNetworkSession::Disconnected, TestTimeOut );
                        roamedSuccessfully = false;
                    } else if (state == QNetworkSession::Disconnected) {
                        QTRY_VERIFY_WITH_TIMEOUT(!errorSpy.isEmpty(), TestTimeOut);
                        QTRY_VERIFY_WITH_TIMEOUT(session2.state() == QNetworkSession::Disconnected, TestTimeOut);
                    } else if (state == QNetworkSession::Connected) {
                        QTRY_VERIFY_WITH_TIMEOUT(errorSpy.isEmpty(),TestTimeOut);

                        if (stateChangedSpy.count() > 1) {
                            state = qvariant_cast<QNetworkSession::State>(stateChangedSpy.at(stateChangedSpy.count() - 2).at(0));
                            QCOMPARE(state, QNetworkSession::Roaming);
                        }
                        roamedSuccessfully = true;
                    }

                    if (roamedSuccessfully) {
                        // Verify that you can open session based on the disconnected configuration
                        QString configId = session.sessionProperty("ActiveConfiguration").toString();
                        QNetworkConfiguration config = manager.configurationFromIdentifier(configId);
                        QNetworkSession session3(config);
                        QSignalSpy errorSpy3(&session3, SIGNAL(error(QNetworkSession::SessionError)));
                        QSignalSpy sessionOpenedSpy3(&session3, SIGNAL(opened()));
                        session3.open();
                        session3.waitForOpened();
                        QTest::qWait(1000); // Wait awhile to get all signals from platform
                        if (session.isOpen())
                            QVERIFY(!sessionOpenedSpy3.isEmpty() || !errorSpy3.isEmpty());
                        session.stop();
                        QTRY_VERIFY_WITH_TIMEOUT(session.state() == QNetworkSession::Disconnected, TestTimeOut);
                    }
                    if (!roamedSuccessfully)
                        QVERIFY(!errorSpy.isEmpty());
                } else {
                    QTest::qWait(2000); // Wait awhile to get all signals from platform

                    if (stateChangedSpy2.count() == 2)  {
                        QNetworkSession::State state =
                            qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(0).at(0));
                        QCOMPARE(state, QNetworkSession::Closing);
                        state = qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(1).at(0));
                        QCOMPARE(state, QNetworkSession::Disconnected);
                    } else {
                        QVERIFY(stateChangedSpy2.count() >= 1);

                        for (int i = 0; i < stateChangedSpy2.count(); i++) {
                            QNetworkSession::State state_temp =
                                    qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(i).at(0));
                            // Extra debug because a fragile point in testcase.
                            qDebug() << "+++++ Statechange spy at: " << i << " is " << state_temp;
                        }

                        QNetworkSession::State state =
                                qvariant_cast<QNetworkSession::State>(stateChangedSpy2.at(stateChangedSpy2.count() - 1).at(0));
                        QCOMPARE(state, QNetworkSession::Disconnected);
                    }
                }

                QTRY_VERIFY_WITH_TIMEOUT(!sessionClosedSpy.isEmpty(), TestTimeOut);
                QTRY_VERIFY_WITH_TIMEOUT(session.state() == QNetworkSession::Disconnected, TestTimeOut);
            }

            QVERIFY(errorSpy2.isEmpty());

            ++inProcessSessionManagementCount;
        } else {
            QFAIL("Timeout waiting for session to stop.");
        }

        QVERIFY(!sessionClosedSpy.isEmpty());
        QVERIFY(!sessionClosedSpy2.isEmpty());

        QVERIFY(!session.isOpen());
        QVERIFY(!session2.isOpen());
    } else if (session2.isOpen()) {
        // Test closing the second session.
        {
            int stateChangedCountBeforeClose = stateChangedSpy2.count();
            session2.close();

            QTRY_VERIFY_WITH_TIMEOUT(!sessionClosedSpy2.isEmpty(), TestTimeOut);
            QCOMPARE(stateChangedSpy2.count(), stateChangedCountBeforeClose);

            QVERIFY(sessionClosedSpy.isEmpty());

            QVERIFY(session.isOpen());
            QVERIFY(!session2.isOpen());
            QCOMPARE(session.state(), QNetworkSession::Connected);
            QCOMPARE(session2.state(), QNetworkSession::Connected);
#ifndef QT_NO_NETWORKINTERFACE
            QVERIFY(session.interface().isValid());
            QCOMPARE(session.interface().hardwareAddress(), session2.interface().hardwareAddress());
            QCOMPARE(session.interface().index(), session2.interface().index());
#endif
        }

        sessionClosedSpy2.clear();

        // Test closing the first session.
        {
            bool expectStateChange = session.state() != QNetworkSession::Disconnected &&
                                     manager.capabilities() & QNetworkConfigurationManager::SystemSessionSupport;

            session.close();

            QTRY_VERIFY_WITH_TIMEOUT(!sessionClosedSpy.isEmpty() || !errorSpy.isEmpty(), TestTimeOut);

            QVERIFY(!session.isOpen());

            if (expectStateChange)
                QTRY_VERIFY_WITH_TIMEOUT(!stateChangedSpy.isEmpty() || !errorSpy.isEmpty(), TestTimeOut);

            if (!errorSpy.isEmpty()) {
                QNetworkSession::SessionError error =
                    qvariant_cast<QNetworkSession::SessionError>(errorSpy.first().at(0));
                if (error == QNetworkSession::OperationNotSupportedError) {
                    // The session needed to bring down the interface,
                    // but the operation is not supported.
                    QSKIP("Configuration does not support close().");
                } else if (error == QNetworkSession::InvalidConfigurationError) {
                    // The session needed to bring down the interface, but it is not possible for the
                    // specified configuration.
                    if ((session.configuration().state() & QNetworkConfiguration::Discovered) ==
                        QNetworkConfiguration::Discovered) {
                        QFAIL("Failed to close session for Discovered configuration.");
                    } else {
                        QSKIP("Cannot test session for non-Discovered configuration.");
                    }
                } else {
                    QFAIL("Error closing session.");
                }
            } else if (!sessionClosedSpy.isEmpty()) {
                QVERIFY(sessionOpenedSpy.isEmpty());
                QCOMPARE(sessionClosedSpy.count(), 1);
                if (expectStateChange)
                    QVERIFY(!stateChangedSpy.isEmpty());
                QVERIFY(errorSpy.isEmpty());

                if (expectStateChange)
                    QTRY_VERIFY_WITH_TIMEOUT(session.state() == QNetworkSession::Disconnected, TestTimeOut);

                ++inProcessSessionManagementCount;
            } else {
                QFAIL("Timeout waiting for session to close.");
            }
        }
    }
}

QDebug operator<<(QDebug debug, const QList<QNetworkConfiguration> &list)
{
    debug.nospace() << "( ";
    foreach (const QNetworkConfiguration &config, list)
        debug.nospace() << config.identifier() << ", ";
    debug.nospace() << ")\n";
    return debug;
}

// Note: outOfProcessSession requires that at least one configuration is
// at Discovered -state.
void tst_QNetworkSession::outOfProcessSession()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    updateConfigurations();
    QTest::qWait(2000);

    QNetworkConfigurationManager manager;
    // Create a QNetworkConfigurationManager to detect configuration changes made in Lackey. This
    // is actually the essence of this testcase - to check that platform mediates/reflects changes
    // regardless of process boundaries. The interprocess communication is more like a way to get
    // this test-case act correctly and timely.
    QList<QNetworkConfiguration> before = manager.allConfigurations(QNetworkConfiguration::Active);
    QSignalSpy spy(&manager, SIGNAL(configurationChanged(QNetworkConfiguration)));

    // Cannot read/write to processes on WinCE.
    // Easiest alternative is to use sockets for IPC.
    QLocalServer oopServer;
    // First remove possible earlier listening address which would cause listen to fail
    // (e.g. previously abruptly ended unit test might cause this)
    QLocalServer::removeServer("tst_qnetworksession");
    oopServer.listen("tst_qnetworksession");

    QProcess lackey;
    QString lackeyExe = lackeyDir + "/lackey";
    lackey.start(lackeyExe);
    QVERIFY2(lackey.waitForStarted(), qPrintable(
        QString::fromLatin1("Could not start %1: %2").arg(lackeyExe, lackey.errorString())));

    QVERIFY(oopServer.waitForNewConnection(-1));
    QLocalSocket *oopSocket = oopServer.nextPendingConnection();

    do {
        QByteArray output;

        if (oopSocket->waitForReadyRead())
            output = oopSocket->readLine().trimmed();

        if (output.startsWith("Started session ")) {
            QString identifier = QString::fromLocal8Bit(output.mid(20).constData());
            QNetworkConfiguration changed;

            do {
                QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), TestTimeOut);
                changed = qvariant_cast<QNetworkConfiguration>(spy.takeFirst().at(0));
            } while (changed.identifier() != identifier);

            QVERIFY((changed.state() & QNetworkConfiguration::Active) ==
                    QNetworkConfiguration::Active);

            QVERIFY(!before.contains(changed));

            QList<QNetworkConfiguration> after =
                manager.allConfigurations(QNetworkConfiguration::Active);

            QVERIFY(after.contains(changed));

            spy.clear();

            oopSocket->write("stop\n");
            oopSocket->waitForBytesWritten();

            do {
                QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), TestTimeOut);

                changed = qvariant_cast<QNetworkConfiguration>(spy.takeFirst().at(0));
            } while (changed.identifier() != identifier);

            QVERIFY((changed.state() & QNetworkConfiguration::Active) !=
                    QNetworkConfiguration::Active);

            QList<QNetworkConfiguration> afterStop =
                    manager.allConfigurations(QNetworkConfiguration::Active);

            QVERIFY(!afterStop.contains(changed));

            oopSocket->disconnectFromServer();
            oopSocket->waitForDisconnected(-1);

            lackey.waitForFinished();
        }
    // This is effected by QTBUG-4903, process will always report as running
    //} while (lackey.state() == QProcess::Running);

    // Workaround: the socket in the lackey will disconnect on exit
    } while (oopSocket->state() == QLocalSocket::ConnectedState);

    switch (lackey.exitCode()) {
    case 0:
        qDebug("Lackey returned exit success (0)");
        break;
    case 1:
        QSKIP("No discovered configurations found.");
    case 2:
        QSKIP("Lackey could not start session.");
    default:
        QSKIP("Lackey failed");
    }
#endif
}

// A convenience / helper function for testcases. Return the first matching configuration.
// Ignores configurations in other than 'discovered' -state. Returns invalid (QNetworkConfiguration())
// if none found.
QNetworkConfiguration suitableConfiguration(QString bearerType, QNetworkConfiguration::Type configType) {

    // Refresh configurations and derive configurations matching given parameters.
    QNetworkConfigurationManager mgr;
    QSignalSpy updateSpy(&mgr, SIGNAL(updateCompleted()));

    mgr.updateConfigurations();
    QTRY_NOOP(updateSpy.count() >= 1);
    if (updateSpy.count() != 1) {
        qDebug("tst_QNetworkSession::suitableConfiguration() failure: unable to update configurations");
        return QNetworkConfiguration();
    }
    QList<QNetworkConfiguration> discoveredConfigs = mgr.allConfigurations(QNetworkConfiguration::Discovered);
    foreach(QNetworkConfiguration config, discoveredConfigs) {
        if ((config.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
            discoveredConfigs.removeOne(config);
        } else if (config.type() != configType) {
            // qDebug() << "Dumping config because type (IAP/SNAP) mismatches: " << config.name();
            discoveredConfigs.removeOne(config);
        } else if ((config.type() == QNetworkConfiguration::InternetAccessPoint) &&
                    bearerType == "cellular") { // 'cellular' bearertype is for convenience
            if (config.bearerTypeName() != "2G" &&
                config.bearerTypeName() != "CDMA2000" &&
                config.bearerTypeName() != "WCDMA" &&
                config.bearerTypeName() != "HSPA" &&
                config.bearerTypeName() != "EVDO" &&
                config.bearerTypeName() != "LTE" &&
                config.bearerTypeName() != "3G" &&
                config.bearerTypeName() != "4G") {
                // qDebug() << "Dumping config because bearer mismatches (cellular): " << config.name();
                discoveredConfigs.removeOne(config);
            }
        } else if ((config.type() == QNetworkConfiguration::InternetAccessPoint) &&
                    bearerType != config.bearerTypeName()) {
            // qDebug() << "Dumping config because bearer mismatches (WLAN): " << config.name();
            discoveredConfigs.removeOne(config);
        }
    }
    if (discoveredConfigs.isEmpty()) {
        qDebug("tst_QNetworkSession::suitableConfiguration() failure: no suitable configurations present.");
        return QNetworkConfiguration();
    } else {
        return discoveredConfigs.first();
    }
}

// A convenience-function: updates configurations and waits that they are updated.
void updateConfigurations()
{
    QNetworkConfigurationManager mgr;
    QSignalSpy updateSpy(&mgr, SIGNAL(updateCompleted()));
    mgr.updateConfigurations();
    QTRY_NOOP(updateSpy.count() >= 1);
}

// A convenience-function: updates and prints all available confiurations and their states
void printConfigurations()
{
    QNetworkConfigurationManager manager;
    QList<QNetworkConfiguration> allConfigs =
    manager.allConfigurations();
    qDebug("tst_QNetworkSession::printConfigurations QNetworkConfigurationManager gives following configurations: ");
    foreach(QNetworkConfiguration config, allConfigs) {
        qDebug() << "Name of the configuration: " << config.name();
        qDebug() << "State of the configuration: " << config.state();
    }
}

// A convenience function for test-cases: opens the given configuration and return
// true if it was done gracefully.
bool openSession(QNetworkSession *session) {
    bool result = true;
    QNetworkConfigurationManager mgr;
    QSignalSpy openedSpy(session, SIGNAL(opened()));
    QSignalSpy stateChangeSpy(session, SIGNAL(stateChanged(QNetworkSession::State)));
    QSignalSpy errorSpy(session, SIGNAL(error(QNetworkSession::SessionError)));
    QSignalSpy configChangeSpy(&mgr, SIGNAL(configurationChanged(QNetworkConfiguration)));
    // Store some initial statuses, because expected signals differ if the config is already
    // active by some other session
    QNetworkConfiguration::StateFlags configInitState = session->configuration().state();
    QNetworkSession::State sessionInitState = session->state();
    qDebug() << "tst_QNetworkSession::openSession() name of the configuration to be opened:  " << session->configuration().name();
    qDebug() << "tst_QNetworkSession::openSession() state of the configuration to be opened:  " << session->configuration().state();
    qDebug() << "tst_QNetworkSession::openSession() state of the session to be opened:  " << session->state();

    if (session->isOpen() ||
        !session->sessionProperty("ActiveConfiguration").toString().isEmpty()) {
        qDebug("tst_QNetworkSession::openSession() failure: session was already open / active.");
        result = false;
    } else {
        session->open();
        session->waitForOpened(120000); // Bringing interfaces up and down may take time at platform
    }
    QTest::qWait(5000); // Wait a moment to ensure all signals are propagated
    // Check that connection opening went by the book. Add checks here if more strictness needed.
    if (!session->isOpen()) {
        qDebug("tst_QNetworkSession::openSession() failure: QNetworkSession::open() failed.");
        result =  false;
    }
    if (openedSpy.count() != 1) {
        qDebug("tst_QNetworkSession::openSession() failure: QNetworkSession::opened() - signal not received.");
        result =  false;
    }
    if (!errorSpy.isEmpty()) {
        qDebug("tst_QNetworkSession::openSession() failure: QNetworkSession::error() - signal was detected.");
        result = false;
    }
    if (sessionInitState != QNetworkSession::Connected &&
        stateChangeSpy.isEmpty()) {
        qDebug("tst_QNetworkSession::openSession() failure: QNetworkSession::stateChanged() - signals not detected.");
        result =  false;
    }
    if (configInitState != QNetworkConfiguration::Active &&
        configChangeSpy.isEmpty()) {
        qDebug("tst_QNetworkSession::openSession() failure: QNetworkConfigurationManager::configurationChanged() - signals not detected.");
        result =  false;
    }
    if (session->configuration().state() != QNetworkConfiguration::Active) {
        qDebug("tst_QNetworkSession::openSession() failure: session's configuration is not in 'Active' -state.");
        qDebug() << "tst_QNetworkSession::openSession() state is:  " << session->configuration().state();
        result =  false;
    }
    if (result == false) {
        qDebug() << "tst_QNetworkSession::openSession() opening session failed.";
    } else {
        qDebug() << "tst_QNetworkSession::openSession() opening session succeeded.";
    }
    qDebug() << "tst_QNetworkSession::openSession() name of the configuration is:  " << session->configuration().name();
    qDebug() << "tst_QNetworkSession::openSession() configuration state is:  " << session->configuration().state();
    qDebug() << "tst_QNetworkSession::openSession() session state is:  " << session->state();

    return result;
}

// Helper function for closing opened session. Performs checks that
// session is closed gradefully (e.g. signals). Function does not delete
// the session. The lastSessionOnConfiguration (true by default) is used to
// tell if there are more sessions open, basing on same configuration. This
// impacts the checks made.
bool closeSession(QNetworkSession *session, bool lastSessionOnConfiguration) {
    if (!session) {
        qDebug("tst_QNetworkSession::closeSession() failure: NULL session given");
        return false;
    }

    qDebug() << "tst_QNetworkSession::closeSession() name of the configuration to be closed:  " << session->configuration().name();
    qDebug() << "tst_QNetworkSession::closeSession() state of the configuration to be closed:  " << session->configuration().state();
    qDebug() << "tst_QNetworkSession::closeSession() state of the session to be closed:  " << session->state();

    if (session->state() != QNetworkSession::Connected ||
        !session->isOpen()) {
        qDebug("tst_QNetworkSession::closeSession() failure: session is not opened.");
        return false;
    }
    QNetworkConfigurationManager mgr;
    QSignalSpy sessionClosedSpy(session, SIGNAL(closed()));
    QSignalSpy sessionStateChangedSpy(session, SIGNAL(stateChanged(QNetworkSession::State)));
    QSignalSpy sessionErrorSpy(session, SIGNAL(error(QNetworkSession::SessionError)));
    QSignalSpy configChangeSpy(&mgr, SIGNAL(configurationChanged(QNetworkConfiguration)));

    bool result = true;
    session->close();
    QTest::qWait(5000); // Wait a moment so that all signals are propagated

    if (!sessionErrorSpy.isEmpty()) {
        qDebug("tst_QNetworkSession::closeSession() failure: QNetworkSession::error() received.");
        result = false;
    }
    if (sessionClosedSpy.count() != 1) {
        qDebug("tst_QNetworkSession::closeSession() failure: QNetworkSession::closed() signal not received.");
        result = false;
    }
    if (lastSessionOnConfiguration &&
        sessionStateChangedSpy.isEmpty()) {
        qDebug("tst_QNetworkSession::closeSession() failure: QNetworkSession::stateChanged() signals not received.");
        result = false;
    }
    if (lastSessionOnConfiguration &&
        session->state() != QNetworkSession::Disconnected) {
        qDebug("tst_QNetworkSession::closeSession() failure: QNetworkSession is not in Disconnected -state");
        result = false;
    }
    QTRY_NOOP(!configChangeSpy.isEmpty());
    if (lastSessionOnConfiguration &&
        configChangeSpy.isEmpty()) {
        qDebug("tst_QNetworkSession::closeSession() failure: QNetworkConfigurationManager::configurationChanged() - signal not detected.");
        result = false;
    }
    if (lastSessionOnConfiguration &&
        session->configuration().state() == QNetworkConfiguration::Active) {
         qDebug("tst_QNetworkSession::closeSession() failure: session's configuration is still in active state.");
         result = false;
    }
    if (result == false) {
        qDebug() << "tst_QNetworkSession::closeSession() closing session failed.";
    } else {
        qDebug() << "tst_QNetworkSession::closeSession() closing session succeeded.";
    }
    qDebug() << "tst_QNetworkSession::closeSession() name of the configuration is:  " << session->configuration().name();
    qDebug() << "tst_QNetworkSession::closeSession() configuration state is:  " << session->configuration().state();
    qDebug() << "tst_QNetworkSession::closeSession() session state is:  " << session->state();
    return result;
}

void tst_QNetworkSession::sessionAutoClose_data()
{
    QTest::addColumn<QNetworkConfiguration>("configuration");

    bool testData = false;
    foreach (const QNetworkConfiguration &config,
             manager.allConfigurations(QNetworkConfiguration::Discovered)) {
        QNetworkSession session(config);
        if (!session.sessionProperty(QLatin1String("AutoCloseSessionTimeout")).isValid())
            continue;

        testData = true;

        const QString name = config.name().isEmpty() ? QString("<Hidden>") : config.name();
        QTest::newRow(name.toLocal8Bit().constData()) << config;
    }

    if (!testData)
        QSKIP("No applicable configurations to test");
}

void tst_QNetworkSession::sessionAutoClose()
{
    QFETCH(QNetworkConfiguration, configuration);

    QNetworkSession session(configuration);

    QCOMPARE(session.configuration(), configuration);

    QVariant autoCloseSession = session.sessionProperty(QLatin1String("AutoCloseSessionTimeout"));

    QVERIFY(autoCloseSession.isValid());

    // property defaults to false
    QCOMPARE(autoCloseSession.toInt(), -1);

    QSignalSpy closeSpy(&session, SIGNAL(closed()));

    session.open();
    session.waitForOpened();

    if (!session.isOpen())
        QSKIP("Session not open");

    // set session to auto close at next polling interval.
    session.setSessionProperty(QLatin1String("AutoCloseSessionTimeout"), 0);

    QTRY_VERIFY_WITH_TIMEOUT(!closeSpy.isEmpty(), TestTimeOut);

    QCOMPARE(session.state(), QNetworkSession::Connected);

    QVERIFY(!session.isOpen());

    QCOMPARE(session.configuration(), configuration);

    autoCloseSession = session.sessionProperty(QLatin1String("AutoCloseSessionTimeout"));

    QVERIFY(autoCloseSession.isValid());

    QCOMPARE(autoCloseSession.toInt(), -1);
}

void tst_QNetworkSession::usagePolicies()
{
    QNetworkSession session(manager.defaultConfiguration());
    QNetworkSession::UsagePolicies initial;
    initial = session.usagePolicies();
    if (initial != 0)
        QNetworkSessionPrivate::setUsagePolicies(session, 0);
    QSignalSpy spy(&session, SIGNAL(usagePoliciesChanged(QNetworkSession::UsagePolicies)));
    QNetworkSessionPrivate::setUsagePolicies(session, QNetworkSession::NoBackgroundTrafficPolicy);
    QCOMPARE(spy.count(), 1);
    QNetworkSession::UsagePolicies policies = qvariant_cast<QNetworkSession::UsagePolicies>(spy.at(0).at(0));
    QCOMPARE(policies, QNetworkSession::NoBackgroundTrafficPolicy);
    QCOMPARE(session.usagePolicies(), QNetworkSession::NoBackgroundTrafficPolicy);
    QNetworkSessionPrivate::setUsagePolicies(session, initial);
    spy.clear();

    session.open();
    QVERIFY(session.waitForOpened());

    //policies may be changed when session is opened, if so, signal should have been emitted
    if (session.usagePolicies() != initial)
        QCOMPARE(spy.count(), 1);
    else
        QCOMPARE(spy.count(), 0);
}


#endif

QTEST_MAIN(tst_QNetworkSession)
#include "tst_qnetworksession.moc"
