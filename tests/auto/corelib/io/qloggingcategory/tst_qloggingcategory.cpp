/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QMutexLocker>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(TST_LOG, "tst.log")
Q_LOGGING_CATEGORY(TST_LOG1, "tst.log1")
Q_LOGGING_CATEGORY(Digia_Oslo_Office_com, "Digia.Oslo.Office.com")
Q_LOGGING_CATEGORY(Digia_Oulu_Office_com, "Digia.Oulu.Office.com")
Q_LOGGING_CATEGORY(Digia_Berlin_Office_com, "Digia.Berlin.Office.com")

QT_USE_NAMESPACE

QtMessageHandler oldMessageHandler;
QString logMessage;
bool multithreadtest = false;
QStringList threadtest;
QMutex threadmutex;
bool usedefaultformat = false;

QByteArray qMyMessageFormatString(QtMsgType type, const QMessageLogContext &context,
                                              const QString &str)
{
    QByteArray message;
    if (!usedefaultformat) {
        message.append(context.category);
        switch (type) {
        case QtDebugMsg:   message.append(".debug"); break;
        case QtWarningMsg: message.append(".warning"); break;
        case QtCriticalMsg:message.append(".critical"); break;
        case QtFatalMsg:   message.append(".fatal"); break;
        }
        message.append(": ");
        message.append(qPrintable(str));
    } else {
        message.append(qPrintable(str));
    }

    return message.simplified();
}

static void myCustomMessageHandler(QtMsgType type,
                                   const QMessageLogContext &context,
                                   const QString &msg)
{
    QMutexLocker locker(&threadmutex);
    logMessage = qMyMessageFormatString(type, context, msg);
    if (multithreadtest)
        threadtest.append(logMessage);
}

class Configuration
{
public:
    Configuration()
    {
    }

    void addKey(const QString &key, bool val){
        // Old key values gets updated
        _values.insert(key, (val ? "true" : "false"));
        if (!_configitemEntryOrder.contains(key))
            _configitemEntryOrder.append(key);
    }

    void addKey(const QString &key, const QString &val){
        // Old key values gets updated
        _values.insert(key, val);
        if (!_configitemEntryOrder.contains(key))
            _configitemEntryOrder.append(key);
    }

    QByteArray array()
    {
        QString ret;
        QTextStream out(&ret);
        for (int a = 0; a < _configitemEntryOrder.count(); a++) {
            out << _configitemEntryOrder[a]
                   << " = "
                   << _values.value(_configitemEntryOrder[a]) << endl;
        }
        out.flush();
        return ret.toLatin1();
    }

    void clear()
    {
        _values.clear();
        _configitemEntryOrder.clear();
    }

private:
    QMap<QString, QString> _values;
    QStringList _configitemEntryOrder;
};

static Configuration configuration1;
static Configuration configuration2;

class LogThread : public QThread
{
    Q_OBJECT

public:
    LogThread(const QString &logtext, Configuration *configuration)
        : _logtext(logtext), _configuration(configuration)
    {}
protected:
    void run()
    {
        for (int i = 0; i < 2000; i++) {
            _configuration->addKey("Digia*", true);
            QByteArray arr = _configuration->array();
            QLoggingCategory::setFilterRules(arr);
            qCDebug(Digia_Oslo_Office_com) << "Oslo " << _logtext << " :true";
            _configuration->addKey("Digia*", false);
            arr = _configuration->array();
            QLoggingCategory::setFilterRules(arr);
            qCDebug(Digia_Oslo_Office_com) << "Oslo " << _logtext << " :false";

            _configuration->addKey("Digia*", true);
            arr = _configuration->array();
            QLoggingCategory::setFilterRules(arr);
            qCDebug(Digia_Berlin_Office_com) << "Berlin " << _logtext << " :true";
            _configuration->addKey("Digia*", false);
            arr = _configuration->array();
            QLoggingCategory::setFilterRules(arr);
            qCDebug(Digia_Berlin_Office_com) << "Berlin " << _logtext << " :false";

            _configuration->addKey("Digia*", true);
            arr = _configuration->array();
            QLoggingCategory::setFilterRules(arr);
            qCDebug(Digia_Oulu_Office_com) << "Oulu " << _logtext << " :true";
            _configuration->addKey("Digia*", false);
            arr = _configuration->array();
            QLoggingCategory::setFilterRules(arr);
            qCDebug(Digia_Oulu_Office_com) << "Oulu " << _logtext << " :false";
        }
    }

public:
    QString _logtext;
    Configuration *_configuration;
};

inline QString cleanLogLine(const QString &qstring)
{
    QString buf = qstring;
    buf.remove("../");
    buf.remove("qlog/");
    QString ret;
    for (int i = 0; i < buf.length(); i++) {
        if (buf[i] >= '!' && buf[i] <= 'z')
            ret += buf[i];
    }
    return ret;
}


QStringList customCategoryFilterArgs;
static void customCategoryFilter(QLoggingCategory *category)
{
    customCategoryFilterArgs << QLatin1String(category->categoryName());
    // invert debug
    category->setEnabled(QtDebugMsg, !category->isEnabled(QtDebugMsg));
}

class tst_QLogging : public QObject
{
    Q_OBJECT

private:
    Configuration *_config;
    QStringList logEntries;

private slots:
    void initTestCase()
    {
        qputenv("QT_MESSAGE_PATTERN", QByteArray("%{category}: %{type},%{message}"));
        oldMessageHandler = qInstallMessageHandler(myCustomMessageHandler);
        // Create configuration
        _config = new Configuration();
    }

    void QLoggingCategory_categoryName()
    {
        logMessage.clear();
        QCOMPARE(QString::fromLatin1(QLoggingCategory::defaultCategory()->categoryName()),
                 QStringLiteral("default"));

        QLoggingCategory defaultCategory("default");
        QCOMPARE(QString::fromLatin1(defaultCategory.categoryName()),
                 QStringLiteral("default"));

        QLoggingCategory nullCategory(0);
        QCOMPARE(QByteArray(nullCategory.categoryName()), QByteArray("default"));

        // we rely on the same pointer for any "default" category
        QCOMPARE(QLoggingCategory::defaultCategory()->categoryName(),
                 defaultCategory.categoryName());
        QCOMPARE(defaultCategory.categoryName(),
                 nullCategory.categoryName());

        QLoggingCategory customCategory("custom");
        QCOMPARE(QByteArray(customCategory.categoryName()), QByteArray("custom"));

        QLoggingCategory emptyCategory("");
        QCOMPARE(QByteArray(emptyCategory.categoryName()), QByteArray(""));

        // make sure nothing has printed warnings
        QVERIFY(logMessage.isEmpty());
    }

    void QLoggingCategory_isEnabled()
    {
        logMessage.clear();

        QCOMPARE(QLoggingCategory::defaultCategory()->isDebugEnabled(), true);
        QCOMPARE(QLoggingCategory::defaultCategory()->isEnabled(QtDebugMsg), true);
        QCOMPARE(QLoggingCategory::defaultCategory()->isWarningEnabled(), true);
        QCOMPARE(QLoggingCategory::defaultCategory()->isEnabled(QtWarningMsg), true);
        QCOMPARE(QLoggingCategory::defaultCategory()->isCriticalEnabled(), true);
        QCOMPARE(QLoggingCategory::defaultCategory()->isEnabled(QtCriticalMsg), true);

        QLoggingCategory defaultCategory("default");
        QCOMPARE(defaultCategory.isDebugEnabled(), true);
        QCOMPARE(defaultCategory.isEnabled(QtDebugMsg), true);
        QCOMPARE(defaultCategory.isWarningEnabled(), true);
        QCOMPARE(defaultCategory.isEnabled(QtWarningMsg), true);
        QCOMPARE(defaultCategory.isCriticalEnabled(), true);
        QCOMPARE(defaultCategory.isEnabled(QtCriticalMsg), true);

        QLoggingCategory customCategory("custom");
        QCOMPARE(customCategory.isDebugEnabled(), true);
        QCOMPARE(customCategory.isEnabled(QtDebugMsg), true);
        QCOMPARE(customCategory.isWarningEnabled(), true);
        QCOMPARE(customCategory.isEnabled(QtWarningMsg), true);
        QCOMPARE(customCategory.isCriticalEnabled(), true);
        QCOMPARE(customCategory.isEnabled(QtCriticalMsg), true);

        // make sure nothing has printed warnings
        QVERIFY(logMessage.isEmpty());
    }

    void QLoggingCategory_setEnabled()
    {
        logMessage.clear();

        QCOMPARE(QLoggingCategory::defaultCategory()->isDebugEnabled(), true);

        QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, false);
        QCOMPARE(QLoggingCategory::defaultCategory()->isDebugEnabled(), false);
        QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);

        // make sure nothing has printed warnings
        QVERIFY(logMessage.isEmpty());

    }

    void QLoggingCategory_installFilter()
    {
        QVERIFY(QLoggingCategory::defaultCategory()->isDebugEnabled());

        QLoggingCategory::CategoryFilter defaultFilter =
                QLoggingCategory::installFilter(customCategoryFilter);
        QVERIFY(defaultFilter);
        customCategoryFilterArgs.clear();
        QVERIFY(!QLoggingCategory::defaultCategory()->isDebugEnabled());

        QLoggingCategory cat("custom");
        QCOMPARE(customCategoryFilterArgs, QStringList() << "custom");
        QVERIFY(!cat.isDebugEnabled());
        customCategoryFilterArgs.clear();

        // install default filter
        QLoggingCategory::CategoryFilter currentFilter =
                QLoggingCategory::installFilter(defaultFilter);
        QCOMPARE((void*)currentFilter, (void*)customCategoryFilter);
        QCOMPARE(customCategoryFilterArgs.size(), 0);

        QVERIFY(QLoggingCategory::defaultCategory()->isDebugEnabled());
        QVERIFY(cat.isDebugEnabled());

        // install default filter
        currentFilter =
                QLoggingCategory::installFilter(0);
        QCOMPARE((void*)defaultFilter, (void*)currentFilter);
        QCOMPARE(customCategoryFilterArgs.size(), 0);

        QVERIFY(QLoggingCategory::defaultCategory()->isDebugEnabled());
        QVERIFY(cat.isDebugEnabled());
    }

    void qDebugMacros()
    {
        QString buf;

        // Check default debug
        buf = QStringLiteral("default.debug: Check debug with no filter active");
        qDebug("%s", "Check debug with no filter active");
        QCOMPARE(logMessage, buf);

        // Check default warning
        buf = QStringLiteral("default.warning: Check warning with no filter active");
        qWarning("%s", "Check warning with no filter active");
        QCOMPARE(logMessage, buf);

        // Check default critical
        buf = QStringLiteral("default.critical: Check critical with no filter active");
        qCritical("%s", "Check critical with no filter active");
        QCOMPARE(logMessage, buf);

        // install filter (inverts rules for qtdebug)
        QLoggingCategory::installFilter(customCategoryFilter);

        // Check default debug
        logMessage.clear();
        qDebug("%s", "Check debug with filter active");
        QCOMPARE(logMessage, QString());

        // reset to default filter
        QLoggingCategory::installFilter(0);

        // Check default debug
        buf = QStringLiteral("default.debug: Check debug with no filter active");
        qDebug("%s", "Check debug with no filter active");
        QCOMPARE(logMessage, buf);
    }

    void qCDebugMacros()
    {
        QString buf;

        QLoggingCategory defaultCategory("default");
        // Check default debug
        buf = QStringLiteral("default.debug: Check debug with no filter active");
        qCDebug(defaultCategory) << "Check debug with no filter active";
        QCOMPARE(logMessage, buf);
        qCDebug(defaultCategory, "Check debug with no filter active");
        QCOMPARE(logMessage, buf);

        // Check default warning
        buf = QStringLiteral("default.warning: Check warning with no filter active");
        qCWarning(defaultCategory) << "Check warning with no filter active";
        QCOMPARE(logMessage, buf);
        qCWarning(defaultCategory, "Check warning with no filter active");
        QCOMPARE(logMessage, buf);

        // Check default critical
        buf = QStringLiteral("default.critical: Check critical with no filter active");
        qCCritical(defaultCategory) << "Check critical with no filter active";
        QCOMPARE(logMessage, buf);
        qCCritical(defaultCategory, "Check critical with no filter active");
        QCOMPARE(logMessage, buf);


        QLoggingCategory customCategory("custom");
        // Check custom debug
        logMessage.clear();
        buf = QStringLiteral("custom.debug: Check debug with no filter active");
        qCDebug(customCategory, "Check debug with no filter active");
        QCOMPARE(logMessage, buf);

        qCDebug(customCategory) << "Check debug with no filter active";
        QCOMPARE(logMessage, buf);

        // Check custom warning
        buf = QStringLiteral("custom.warning: Check warning with no filter active");
        qCWarning(customCategory) << "Check warning with no filter active";
        QCOMPARE(logMessage, buf);

        // Check custom critical
        buf = QStringLiteral("custom.critical: Check critical with no filter active");
        qCCritical(customCategory) << "Check critical with no filter active";
        QCOMPARE(logMessage, buf);

        // install filter (inverts rules for qtdebug)
        QLoggingCategory::installFilter(customCategoryFilter);

        // Check custom debug
        logMessage.clear();
        qCDebug(customCategory) << "Check debug with filter active";
        QCOMPARE(logMessage, QString());

        // Check different macro/category variants
        buf = QStringLiteral("tst.log.debug: Check debug with no filter active");
        qCDebug(TST_LOG) << "Check debug with no filter active";
        QCOMPARE(logMessage, QString());
        qCDebug(TST_LOG, "Check debug with no filter active");
        QCOMPARE(logMessage, QString());
        qCDebug(TST_LOG(), "Check debug with no filter active");
        QCOMPARE(logMessage, QString());
        buf = QStringLiteral("tst.log.warning: Check warning with no filter active");
        qCWarning(TST_LOG) << "Check warning with no filter active";
        QCOMPARE(logMessage, buf);
        qCWarning(TST_LOG, "Check warning with no filter active");
        QCOMPARE(logMessage, buf);
        buf = QStringLiteral("tst.log.critical: Check critical with no filter active");
        qCCritical(TST_LOG) << "Check critical with no filter active";
        QCOMPARE(logMessage, buf);
        qCCritical(TST_LOG, "Check critical with no filter active");
        QCOMPARE(logMessage, buf);


        // reset to default filter
        QLoggingCategory::installFilter(0);

        // Check custom debug
        logMessage.clear();
        buf = QStringLiteral("custom.debug: Check debug with no filter active");
        qCDebug(customCategory) << "Check debug with no filter active";
        QCOMPARE(logMessage, buf);
    }

    void checkLegacyMessageLogger()
    {
        usedefaultformat = true;
        // This should just not crash.
        QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).debug() << "checkLegacyMessageLogger1";
        QCOMPARE(logMessage, QStringLiteral("checkLegacyMessageLogger1"));
        QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).warning() << "checkLegacyMessageLogger2";
        QCOMPARE(logMessage, QStringLiteral("checkLegacyMessageLogger2"));
        QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO).critical() << "checkLegacyMessageLogger3";
        QCOMPARE(logMessage, QStringLiteral("checkLegacyMessageLogger3"));
        usedefaultformat = false;
    }

    // Check the Debug, Warning and critical without having category active. should be active.
    void checkNoCategoryLogActive()
    {
        // Check default debug
        QString buf = QStringLiteral("default.debug: Check default Debug with no log active");
        qDebug() << "Check default Debug with no log active";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check default warning
        buf = QStringLiteral("default.warning: Check default Warning with no log active");
        qWarning() << "Check default Warning with no log active";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check default critical
        buf = QStringLiteral("default.critical: Check default Critical with no log active");
        qCritical() << "Check default Critical with no log active";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check category debug
        buf = QStringLiteral("tst.log.debug: Check category Debug with no log active");
        qCDebug(TST_LOG) << "Check category Debug with no log active";
        QCOMPARE(logMessage, buf);


        // Check default warning
        buf = QStringLiteral("tst.log.warning: Check category Warning with no log active");
        qCWarning(TST_LOG) << "Check category Warning with no log active";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check default critical
        buf = QStringLiteral("tst.log.critical: Check category Critical with no log active");
        qCCritical(TST_LOG) << "Check category Critical with no log active";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
    }

    void writeCategoryLogs()
    {
        usedefaultformat = false;
        // Activate TST_LOG category
        logMessage = "";
        _config->addKey("tst.log", true);
        QLoggingCategory::setFilterRules(_config->array());
        QString buf = QStringLiteral("tst.log.debug: Check for default messagePattern");
        qCDebug(TST_LOG) << "Check for default messagePattern";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Activate TST_LOG category with default enabled function info
        _config->addKey("tst.log1", true);
        QLoggingCategory::setFilterRules(_config->array());
        qCDebug(TST_LOG) << "1";
        buf = QStringLiteral("tst.log.debug: 1");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Write out all different types
        qCDebug(TST_LOG) << "DebugType";
        buf = QStringLiteral("tst.log.debug: DebugType");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCWarning(TST_LOG) << "WarningType";
        buf = QStringLiteral("tst.log.warning: WarningType");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCCritical(TST_LOG) << "CriticalType";
        buf = QStringLiteral("tst.log.critical: CriticalType");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
    }

    void checkLegacyLogs()
    {
        logMessage = "";
        qDebug() << "DefaultDebug";
        QString buf = QStringLiteral("default.debug: DefaultDebug");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // debug off by default, warning and critical are on
        qWarning() << "DefaultWarning";
        buf = QStringLiteral("default.warning: DefaultWarning");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCritical() << "DefaultCritical";
        buf = QStringLiteral("default.critical: DefaultCritical");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Enable debug
        _config->addKey("default.debug", true);
        QLoggingCategory::setFilterRules(_config->array());

        qDebug() << "DefaultDebug1";
        buf = QStringLiteral("default.debug: DefaultDebug1");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qWarning() << "DefaultWarning1";
        buf = QStringLiteral("default.warning: DefaultWarning1");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCritical() << "DefaultCritical1";
        buf = QStringLiteral("default.critical: DefaultCritical1");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Disable warning
        _config->addKey("default.warning", false);
        QLoggingCategory::setFilterRules(_config->array());

        qDebug() << "DefaultDebug2";
        buf = QStringLiteral("default.debug: DefaultDebug2");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        logMessage = "no change";
        qWarning() << "DefaultWarning2";
        buf = QStringLiteral("no change");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCritical() << "DefaultCritical2";
        buf = QStringLiteral("default.critical: DefaultCritical2");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Disable critical
        _config->addKey("default.critical", false);
        _config->addKey("default.debug", false);
        QLoggingCategory::setFilterRules(_config->array());

        logMessage = "no change";
        qDebug() << "DefaultDebug3";
        buf = QStringLiteral("no change");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qWarning() << "DefaultWarning3";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCritical() << "DefaultCritical3";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Enable default logs
        _config->addKey("default.critical", true);
        _config->addKey("default.warning", true);
        _config->addKey("default.debug", true);
        QLoggingCategory::setFilterRules(_config->array());

        // Ensure all are on
        qDebug() << "DefaultDebug4";
        buf = QStringLiteral("default.debug: DefaultDebug4");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qWarning() << "DefaultWarning4";
        buf = QStringLiteral("default.warning: DefaultWarning4");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCritical() << "DefaultCritical4";
        buf = QStringLiteral("default.critical: DefaultCritical4");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Disable default log
        _config->addKey("default", false);
        QLoggingCategory::setFilterRules(_config->array());

        // Ensure all are off
        logMessage = "no change";
        buf = QStringLiteral("no change");
        qDebug() << "DefaultDebug5";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qWarning() << "DefaultWarning5";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCritical() << "DefaultCritical5";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Reset
        _config->clear();
        QLoggingCategory::setFilterRules(_config->array());
    }

    void checkFiltering()
    {
        // Enable default logs
        _config->clear();
        _config->addKey("Digia.Oslo.Office.com", false);
        _config->addKey("Digia.Oulu.Office.com", false);
        _config->addKey("Digia.Berlin.Office.com", false);
        _config->addKey("MessagePattern", QString("%{category}: %{message}"));
        QLoggingCategory::setFilterRules(_config->array());

        logMessage = "no change";
        QString buf = QStringLiteral("no change");
        qCDebug(Digia_Oslo_Office_com) << "Digia.Oslo.Office.com 1";
        qCDebug(Digia_Oulu_Office_com) << "Digia.Oulu.Office.com 1";
        qCDebug(Digia_Berlin_Office_com) << "Digia.Berlin.Office.com 1";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        _config->addKey("Digia.Oslo.Office.com", true);
        _config->addKey("Digia.Oulu.Office.com", true);
        _config->addKey("Digia.Berlin.Office.com", true);
        QLoggingCategory::setFilterRules(_config->array());

        qCDebug(Digia_Oslo_Office_com) << "Digia.Oslo.Office.com 2";
        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Digia.Oslo.Office.com 2");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCDebug(Digia_Oulu_Office_com) << "Digia.Oulu.Office.com 2";
        buf = QStringLiteral("Digia.Oulu.Office.com.debug: Digia.Oulu.Office.com 2");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCDebug(Digia_Berlin_Office_com) << "Digia.Berlin.Office.com 2";
        buf = QStringLiteral("Digia.Berlin.Office.com.debug: Digia.Berlin.Office.com 2");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check right filter
        _config->addKey("Digia.Oslo.Office.com", false);
        _config->addKey("Digia.Oulu.Office.com", false);
        _config->addKey("Digia.Berlin.Office.com", false);
        _config->addKey("*Office.com*", true);
        QLoggingCategory::setFilterRules(_config->array());

        qCDebug(Digia_Oslo_Office_com) << "Digia.Oslo.Office.com 3";
        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Digia.Oslo.Office.com 3");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCDebug(Digia_Oulu_Office_com) << "Digia.Oulu.Office.com 3";
        buf = QStringLiteral("Digia.Oulu.Office.com.debug: Digia.Oulu.Office.com 3");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCDebug(Digia_Berlin_Office_com) << "Digia.Berlin.Office.com 3";
        buf = QStringLiteral("Digia.Berlin.Office.com.debug: Digia.Berlin.Office.com 3");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check left filter
        _config->addKey("*Office.com*", false);
        _config->addKey("*Office.com.debug", true);
        QLoggingCategory::setFilterRules(_config->array());

        qCDebug(Digia_Oslo_Office_com) << "Debug: Digia.Oslo.Office.com 4";
        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Debug: Digia.Oslo.Office.com 4");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        logMessage = "no change";
        buf = QStringLiteral("no change");
        qCWarning(Digia_Oulu_Office_com) << "Warning: Digia.Oulu.Office.com 4";
        qCCritical(Digia_Berlin_Office_com) << "Critical: Digia.Berlin.Office.com 4";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check right filter
        _config->addKey("*Office.com.debug", false);
        _config->addKey("Digia.*", true);
        QLoggingCategory::setFilterRules(_config->array());

        qCDebug(Digia_Oslo_Office_com) << "Debug: Digia.Oslo.Office.com 5";
        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Debug: Digia.Oslo.Office.com 5");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCWarning(Digia_Oulu_Office_com) << "Warning: Digia.Oulu.Office.com 5";
        buf = QStringLiteral("Digia.Oulu.Office.com.warning: Warning: Digia.Oulu.Office.com 5");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCCritical(Digia_Berlin_Office_com) << "Critical: Digia.Berlin.Office.com 5";
        buf = QStringLiteral("Digia.Berlin.Office.com.critical: Critical: Digia.Berlin.Office.com 5");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // Check mid filter
        _config->addKey("Digia.*", false);
        QLoggingCategory::setFilterRules(_config->array());

        logMessage = "no change";
        buf = QStringLiteral("no change");
        qCDebug(Digia_Oslo_Office_com) << "Debug: Digia.Oslo.Office.com 6";
        qCWarning(Digia_Oulu_Office_com) << "Warning: Digia.Oulu.Office.com 6";
        qCCritical(Digia_Berlin_Office_com) << "Critical: Digia.Berlin.Office.com 6";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        _config->addKey("*.Office.*", true);
        QLoggingCategory::setFilterRules(_config->array());

        qCDebug(Digia_Oslo_Office_com) << "Debug: Digia.Oslo.Office.com 7";
        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Debug: Digia.Oslo.Office.com 7");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCWarning(Digia_Oulu_Office_com) << "Warning: Digia.Oulu.Office.com 7";
        buf = QStringLiteral("Digia.Oulu.Office.com.warning: Warning: Digia.Oulu.Office.com 7");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        qCCritical(Digia_Berlin_Office_com) << "Critical: Digia.Berlin.Office.com 7";
        buf = QStringLiteral("Digia.Berlin.Office.com.critical: Critical: Digia.Berlin.Office.com 7");
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
    }

    void checkLogWithCategoryObject()
    {
        _config->clear();
        _config->addKey("LoggingCategoryObject", true);
        QLoggingCategory *pcategorybject = 0;
        QLoggingCategory::setFilterRules(_config->array());
        {
            QLoggingCategory mycategoryobject("LoggingCategoryObject");
            pcategorybject = &mycategoryobject;
            logMessage = "no change";

            QString buf = QStringLiteral("LoggingCategoryObject.debug: My Category Object");
            qCDebug(mycategoryobject) << "My Category Object";
            QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

            buf = QStringLiteral("LoggingCategoryObject.warning: My Category Object");
            qCWarning(mycategoryobject) << "My Category Object";
            QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

            buf = QStringLiteral("LoggingCategoryObject.critical: My Category Object");
            qCCritical(mycategoryobject) << "My Category Object";
            QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

            QLoggingCategory mycategoryobject2("LoggingCategoryObject");
            buf = QStringLiteral("LoggingCategoryObject.debug: My Category Object");
            qCDebug(mycategoryobject) << "My Category Object";
            QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

            buf = QStringLiteral("LoggingCategoryObject.warning: My Category Object");
            qCWarning(mycategoryobject) << "My Category Object";
            QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

            buf = QStringLiteral("LoggingCategoryObject.critical: My Category Object");
            qCCritical(mycategoryobject) << "My Category Object";
            QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
        }
        Q_UNUSED(pcategorybject);
    }

    void checkEmptyCategoryName()
    {
        // "" -> custom category
        QLoggingCategory mycategoryobject1("");
        QString buf = QStringLiteral(".debug: My Category Object");
        qCDebug(mycategoryobject1) << "My Category Object";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));

        // 0 -> default category
        QLoggingCategory mycategoryobject2(0);
        buf = QStringLiteral("default.debug:MyCategoryObject");
        qCDebug(mycategoryobject2) << "My Category Object";
        QCOMPARE(cleanLogLine(logMessage), cleanLogLine(buf));
    }

    void checkMultithreading()
    {
        multithreadtest = true;
        // Init two configurations, one for each thread
        configuration1.addKey("Digia*", true);
        configuration2.addKey("Digia*", true);
        QByteArray arr = configuration1.array();
        QLoggingCategory::setFilterRules(arr);

        LogThread thgread1(QString("from Thread 1"), &configuration1);
        LogThread thgread2(QString("from Thread 2"), &configuration2);

        // Writing out stuff from 2 different threads into the same areas
        thgread1.start();
        thgread2.start();
        thgread1.wait();
        thgread2.wait();

        // Check if each log line is complete
        QStringList compareagainst;
        QString buf = QStringLiteral("Digia.Oslo.Office.com.debug: Oslo  \"from Thread 1\"  :true");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Oulu.Office.com.debug: Oulu  \"from Thread 1\"  :true");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Berlin.Office.com.debug: Berlin  \"from Thread 1\"  :true");
        compareagainst.append(cleanLogLine(buf));

        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Oslo  \"from Thread 1\"  :false");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Oulu.Office.com.debug: Oulu  \"from Thread 1\"  :false");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Berlin.Office.com.debug: Berlin  \"from Thread 1\"  :false");
        compareagainst.append(cleanLogLine(buf));

        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Oslo  \"from Thread 2\"  :true");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Oulu.Office.com.debug: Oulu  \"from Thread 2\"  :true");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Berlin.Office.com.debug: Berlin  \"from Thread 2\"  :true");
        compareagainst.append(cleanLogLine(buf));

        buf = QStringLiteral("Digia.Oslo.Office.com.debug: Oslo  \"from Thread 2\"  :false");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Oulu.Office.com.debug: Oulu  \"from Thread 2\"  :false");
        compareagainst.append(cleanLogLine(buf));
        buf = QStringLiteral("Digia.Berlin.Office.com.debug: Berlin  \"from Thread 2\"  :false");
        compareagainst.append(cleanLogLine(buf));

        for (int i = 0; i < threadtest.count(); i++) {
            if (!compareagainst.contains(cleanLogLine(threadtest[i]))){
                fprintf(stdout, "%s\r\n", threadtest[i].toLatin1().constData());
                QVERIFY2(false, "Multithread log is not complete!");
            }
        }
    }

    void cleanupTestCase()
    {
        delete _config;
        qInstallMessageHandler(oldMessageHandler);
    }
};

QTEST_MAIN(tst_QLogging)

#include "tst_qloggingcategory.moc"
