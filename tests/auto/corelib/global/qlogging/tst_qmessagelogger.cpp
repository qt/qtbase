// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qlogging.h>
#include <qloggingcategory.h>
#include <QtTest/QTest>

Q_LOGGING_CATEGORY(debugTestCategory, "debug", QtDebugMsg)
Q_LOGGING_CATEGORY(infoTestCategory, "info", QtInfoMsg)
Q_LOGGING_CATEGORY(warningTestCategory, "warning", QtWarningMsg)
Q_LOGGING_CATEGORY(criticalTestCategory, "critical", QtCriticalMsg)

struct LoggerMessageInfo
{
    QtMsgType messageType { QtFatalMsg };
    QString message;
    const char *file { nullptr };
    int line { 0 };
    const char *function { nullptr };
    const char *category { nullptr };
};

LoggerMessageInfo messageInfo;

static void customMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                 const QString &message)
{
    messageInfo.messageType = type;
    messageInfo.message = message;
    messageInfo.file = context.file;
    messageInfo.line = context.line;
    messageInfo.function = context.function;
    messageInfo.category = context.category;
}

class tst_QMessageLogger : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase_data();

    void init();
    void cleanup();

    void logMessage();
    void logMessageWithLoggingCategory();
    void logMessageWithLoggingCategoryDisabled();
    void logMessageWithCategoryFunction();
    void logMessageWithNoDebug();

private:
    void logWithLoggingCategoryHelper(bool messageTypeEnabled);
};

void tst_QMessageLogger::initTestCase_data()
{
    QTest::addColumn<QtMsgType>("messageType");
    QTest::addColumn<QByteArray>("categoryName");
    QTest::addColumn<QByteArray>("messageText");
    QTest::addColumn<bool>("useDebugStream");

    // not testing QtFatalMsg, as it terminates the application
    QTest::newRow("debug") << QtDebugMsg << QByteArray("categoryDebug")
                           << QByteArray("debug message") << false;
    QTest::newRow("info") << QtInfoMsg << QByteArray("categoryInfo") << QByteArray("info message")
                          << false;
    QTest::newRow("warning") << QtWarningMsg << QByteArray("categoryWarning")
                             << QByteArray("warning message") << false;
    QTest::newRow("critical") << QtCriticalMsg << QByteArray("categoryCritical")
                              << QByteArray("critical message") << false;

#ifndef QT_NO_DEBUG_STREAM
    QTest::newRow("stream debug") << QtDebugMsg << QByteArray("categoryDebug")
                                  << QByteArray("debug message") << true;
    QTest::newRow("stream info") << QtInfoMsg << QByteArray("categoryInfo")
                                 << QByteArray("info message") << true;
    QTest::newRow("stream warning") << QtWarningMsg << QByteArray("categoryWarning")
                                    << QByteArray("warning message") << true;
    QTest::newRow("stream critical") << QtCriticalMsg << QByteArray("categoryCritical")
                                     << QByteArray("critical message") << true;
#endif
}

void tst_QMessageLogger::init()
{
    qInstallMessageHandler(customMessageHandler);
}

void tst_QMessageLogger::cleanup()
{
    qInstallMessageHandler((QtMessageHandler)0);
    messageInfo.messageType = QtFatalMsg;
    messageInfo.message.clear();
    messageInfo.file = nullptr;
    messageInfo.line = 0;
    messageInfo.function = nullptr;
    messageInfo.category = nullptr;
}

void tst_QMessageLogger::logMessage()
{
    const int line = QT_MESSAGELOG_LINE;
    QMessageLogger logger(QT_MESSAGELOG_FILE, line, QT_MESSAGELOG_FUNC);

    QFETCH_GLOBAL(QtMsgType, messageType);
    QFETCH_GLOBAL(QByteArray, messageText);
    QFETCH_GLOBAL(bool, useDebugStream);
    if (useDebugStream) {
#ifndef QT_NO_DEBUG_STREAM
        switch (messageType) {
        case QtDebugMsg:
            logger.debug().noquote() << messageText;
            break;
        case QtInfoMsg:
            logger.info().noquote() << messageText;
            break;
        case QtWarningMsg:
            logger.warning().noquote() << messageText;
            break;
        case QtCriticalMsg:
            logger.critical().noquote() << messageText;
            break;
        default:
            QFAIL("Invalid message type");
            break;
        }
#else
        QSKIP("Qt debug stream disabled");
#endif
    } else {
        switch (messageType) {
        case QtDebugMsg:
            logger.debug("%s", messageText.constData());
            break;
        case QtInfoMsg:
            logger.info("%s", messageText.constData());
            break;
        case QtWarningMsg:
            logger.warning("%s", messageText.constData());
            break;
        case QtCriticalMsg:
            logger.critical("%s", messageText.constData());
            break;
        default:
            QFAIL("Invalid message type");
            break;
        }
    }

    QCOMPARE(messageInfo.messageType, messageType);
    QCOMPARE(messageInfo.message, messageText);
    QCOMPARE(messageInfo.file, __FILE__);
    QCOMPARE(messageInfo.line, line);
    QCOMPARE(messageInfo.function, Q_FUNC_INFO);
}

void tst_QMessageLogger::logMessageWithLoggingCategory()
{
    logWithLoggingCategoryHelper(true);
}

void tst_QMessageLogger::logMessageWithLoggingCategoryDisabled()
{
    logWithLoggingCategoryHelper(false);
}

void tst_QMessageLogger::logMessageWithCategoryFunction()
{
    const int line = QT_MESSAGELOG_LINE;
    QMessageLogger logger(QT_MESSAGELOG_FILE, line, QT_MESSAGELOG_FUNC);

    const QLoggingCategory *category = nullptr;
    QFETCH_GLOBAL(QtMsgType, messageType);
    QFETCH_GLOBAL(QByteArray, messageText);
    QFETCH_GLOBAL(bool, useDebugStream);
    if (useDebugStream) {
#ifndef QT_NO_DEBUG_STREAM
        switch (messageType) {
        case QtDebugMsg:
            logger.debug(debugTestCategory()).noquote() << messageText;
            category = &debugTestCategory();
            break;
        case QtInfoMsg:
            logger.info(infoTestCategory()).noquote() << messageText;
            category = &infoTestCategory();
            break;
        case QtWarningMsg:
            logger.warning(warningTestCategory()).noquote() << messageText;
            category = &warningTestCategory();
            break;
        case QtCriticalMsg:
            logger.critical(criticalTestCategory()).noquote() << messageText;
            category = &criticalTestCategory();
            break;
        default:
            QFAIL("Invalid message type");
            break;
        }
#else
        QSKIP("Qt debug stream disabled");
#endif
    } else {
        switch (messageType) {
        case QtDebugMsg:
            logger.debug(debugTestCategory(), "%s", messageText.constData());
            category = &debugTestCategory();
            break;
        case QtInfoMsg:
            logger.info(infoTestCategory(), "%s", messageText.constData());
            category = &infoTestCategory();
            break;
        case QtWarningMsg:
            logger.warning(warningTestCategory(), "%s", messageText.constData());
            category = &warningTestCategory();
            break;
        case QtCriticalMsg:
            logger.critical(criticalTestCategory(), "%s", messageText.constData());
            category = &criticalTestCategory();
            break;
        default:
            QFAIL("Invalid message type");
            break;
        }
    }

    QCOMPARE(messageInfo.messageType, messageType);
    QCOMPARE(messageInfo.message, messageText);
    QCOMPARE(messageInfo.file, __FILE__);
    QCOMPARE(messageInfo.line, line);
    QCOMPARE(messageInfo.function, Q_FUNC_INFO);
    QCOMPARE(messageInfo.category, category->categoryName());
}

void tst_QMessageLogger::logMessageWithNoDebug()
{
    const int line = QT_MESSAGELOG_LINE;
    QMessageLogger logger(QT_MESSAGELOG_FILE, line, QT_MESSAGELOG_FUNC);

    QFETCH_GLOBAL(QByteArray, messageText);
    QFETCH_GLOBAL(bool, useDebugStream);
    if (useDebugStream) {
#ifndef QT_NO_DEBUG_STREAM
        logger.noDebug().noquote() << messageText;
#else
        QSKIP("Qt debug stream disabled");
#endif
    } else {
        logger.noDebug("%s", messageText.constData());
    }

    // the callback was not called
    QVERIFY(messageInfo.messageType == QtFatalMsg);
    QVERIFY(messageInfo.message.isEmpty());
    QVERIFY(messageInfo.file == nullptr);
    QVERIFY(messageInfo.line == 0);
    QVERIFY(messageInfo.function == nullptr);
    QVERIFY(messageInfo.category == nullptr);
}

void tst_QMessageLogger::logWithLoggingCategoryHelper(bool messageTypeEnabled)
{
    QFETCH_GLOBAL(QtMsgType, messageType);
    QFETCH_GLOBAL(QByteArray, categoryName);
    QLoggingCategory category(categoryName.constData(), messageType);
    if (!messageTypeEnabled)
        category.setEnabled(messageType, false);

    const int line = QT_MESSAGELOG_LINE;
    QMessageLogger logger(QT_MESSAGELOG_FILE, line, QT_MESSAGELOG_FUNC);

    QFETCH_GLOBAL(QByteArray, messageText);
    QFETCH_GLOBAL(bool, useDebugStream);
    if (useDebugStream) {
#ifndef QT_NO_DEBUG_STREAM
        switch (messageType) {
        case QtDebugMsg:
            logger.debug(category).noquote() << messageText;
            break;
        case QtInfoMsg:
            logger.info(category).noquote() << messageText;
            break;
        case QtWarningMsg:
            logger.warning(category).noquote() << messageText;
            break;
        case QtCriticalMsg:
            logger.critical(category).noquote() << messageText;
            break;
        default:
            QFAIL("Invalid message type");
            break;
        }
#else
        QSKIP("Qt debug stream disabled");
#endif
    } else {
        switch (messageType) {
        case QtDebugMsg:
            logger.debug(category, "%s", messageText.constData());
            break;
        case QtInfoMsg:
            logger.info(category, "%s", messageText.constData());
            break;
        case QtWarningMsg:
            logger.warning(category, "%s", messageText.constData());
            break;
        case QtCriticalMsg:
            logger.critical(category, "%s", messageText.constData());
            break;
        default:
            QFAIL("Invalid message type");
            break;
        }
    }

    if (messageTypeEnabled) {
        QCOMPARE(messageInfo.messageType, messageType);
        QCOMPARE(messageInfo.message, messageText);
        QCOMPARE(messageInfo.file, __FILE__);
        QCOMPARE(messageInfo.line, line);
        QCOMPARE(messageInfo.function, Q_FUNC_INFO);
        QCOMPARE(messageInfo.category, categoryName);
    } else {
        // the callback was not called
        QVERIFY(messageInfo.messageType == QtFatalMsg);
        QVERIFY(messageInfo.message.isEmpty());
        QVERIFY(messageInfo.file == nullptr);
        QVERIFY(messageInfo.line == 0);
        QVERIFY(messageInfo.function == nullptr);
        QVERIFY(messageInfo.category == nullptr);
    }
}

QTEST_MAIN(tst_QMessageLogger)
#include "tst_qmessagelogger.moc"
