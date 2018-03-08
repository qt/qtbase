/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qloggingregistry_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qdir.h>
#include <QtCore/qcoreapplication.h>

// We can't use the default macros because this would lead to recursion.
// Instead let's define our own one that unconditionally logs...
#define debugMsg QMessageLogger(__FILE__, __LINE__, __FUNCTION__, "qt.core.logging").debug
#define warnMsg QMessageLogger(__FILE__, __LINE__, __FUNCTION__, "qt.core.logging").warning


QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QLoggingRegistry, qtLoggingRegistry)

/*!
    \internal
    Constructs a logging rule with default values.
*/
QLoggingRule::QLoggingRule() :
    enabled(false)
{
}

/*!
    \internal
    Constructs a logging rule.
*/
QLoggingRule::QLoggingRule(const QStringRef &pattern, bool enabled) :
    messageType(-1),
    enabled(enabled)
{
    parse(pattern);
}

/*!
    \internal
    Return value 1 means filter passed, 0 means filter doesn't influence this
    category, -1 means category doesn't pass this filter.
 */
int QLoggingRule::pass(const QString &cat, QtMsgType msgType) const
{
    // check message type
    if (messageType > -1 && messageType != msgType)
        return 0;

    if (flags == FullText) {
        // full match
        if (category == cat)
            return (enabled ? 1 : -1);
        else
            return 0;
    }

    const int idx = cat.indexOf(category);
    if (idx >= 0) {
        if (flags == MidFilter) {
            // matches somewhere
            if (idx >= 0)
                return (enabled ? 1 : -1);
        } else if (flags == LeftFilter) {
            // matches left
            if (idx == 0)
                return (enabled ? 1 : -1);
        } else if (flags == RightFilter) {
            // matches right
            if (idx == (cat.count() - category.count()))
                return (enabled ? 1 : -1);
        }
    }
    return 0;
}

/*!
    \internal
    Parses \a pattern.
    Allowed is f.ex.:
             qt.core.io.debug      FullText, QtDebugMsg
             qt.core.*             LeftFilter, all types
             *.io.warning          RightFilter, QtWarningMsg
             *.core.*              MidFilter
 */
void QLoggingRule::parse(const QStringRef &pattern)
{
    QStringRef p;

    // strip trailing ".messagetype"
    if (pattern.endsWith(QLatin1String(".debug"))) {
        p = QStringRef(pattern.string(), pattern.position(),
                       pattern.length() - 6); // strlen(".debug")
        messageType = QtDebugMsg;
    } else if (pattern.endsWith(QLatin1String(".info"))) {
        p = QStringRef(pattern.string(), pattern.position(),
                       pattern.length() - 5); // strlen(".info")
        messageType = QtInfoMsg;
    } else if (pattern.endsWith(QLatin1String(".warning"))) {
        p = QStringRef(pattern.string(), pattern.position(),
                       pattern.length() - 8); // strlen(".warning")
        messageType = QtWarningMsg;
    } else if (pattern.endsWith(QLatin1String(".critical"))) {
        p = QStringRef(pattern.string(), pattern.position(),
                       pattern.length() - 9); // strlen(".critical")
        messageType = QtCriticalMsg;
    } else {
        p = pattern;
    }

    if (!p.contains(QLatin1Char('*'))) {
        flags = FullText;
    } else {
        if (p.endsWith(QLatin1Char('*'))) {
            flags |= LeftFilter;
            p = QStringRef(p.string(), p.position(), p.length() - 1);
        }
        if (p.startsWith(QLatin1Char('*'))) {
            flags |= RightFilter;
            p = QStringRef(p.string(), p.position() + 1, p.length() - 1);
        }
        if (p.contains(QLatin1Char('*'))) // '*' only supported at start/end
            flags = 0;
    }

    category = p.toString();
}

/*!
    \class QLoggingSettingsParser
    \since 5.3
    \internal

    Parses a .ini file with the following format:

    [rules]
    rule1=[true|false]
    rule2=[true|false]
    ...

    [rules] is the default section, and therefore optional.
*/

/*!
    \internal
    Parses configuration from \a content.
*/
void QLoggingSettingsParser::setContent(const QString &content)
{
    _rules.clear();
    const auto lines = content.splitRef(QLatin1Char('\n'));
    for (const auto &line : lines)
        parseNextLine(line);
}

/*!
    \internal
    Parses configuration from \a stream.
*/
void QLoggingSettingsParser::setContent(QTextStream &stream)
{
    _rules.clear();
    QString line;
    while (stream.readLineInto(&line))
        parseNextLine(QStringRef(&line));
}

/*!
    \internal
    Parses one line of the configuation file
*/

void QLoggingSettingsParser::parseNextLine(QStringRef line)
{
    // Remove whitespace at start and end of line:
    line = line.trimmed();

    // comment
    if (line.startsWith(QLatin1Char(';')))
        return;

    if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
        // new section
        auto sectionName = line.mid(1, line.size() - 2).trimmed();
        m_inRulesSection = sectionName.compare(QLatin1String("rules"), Qt::CaseInsensitive) == 0;
        return;
    }

    if (m_inRulesSection) {
        int equalPos = line.indexOf(QLatin1Char('='));
        if (equalPos != -1) {
            if (line.lastIndexOf(QLatin1Char('=')) == equalPos) {
                const auto pattern = line.left(equalPos).trimmed();
                const auto valueStr = line.mid(equalPos + 1).trimmed();
                int value = -1;
                if (valueStr == QLatin1String("true"))
                    value = 1;
                else if (valueStr == QLatin1String("false"))
                    value = 0;
                QLoggingRule rule(pattern, (value == 1));
                if (rule.flags != 0 && (value != -1))
                    _rules.append(rule);
                else
                    warnMsg("Ignoring malformed logging rule: '%s'", line.toUtf8().constData());
            } else {
                warnMsg("Ignoring malformed logging rule: '%s'", line.toUtf8().constData());
            }
        }
    }
}

/*!
    \internal
    QLoggingRegistry constructor
 */
QLoggingRegistry::QLoggingRegistry()
    : categoryFilter(defaultCategoryFilter)
{
#if defined(Q_OS_ANDROID)
    // Unless QCoreApplication has been constructed we can't be sure that
    // we are on Qt's main thread. If we did allow logging here, we would
    // potentially set Qt's main thread to Android's thread 0, which would
    // confuse Qt later when running main().
    if (!qApp)
        return;
#endif

    initializeRules(); // Init on first use
}

static bool qtLoggingDebug()
{
    static const bool debugEnv = qEnvironmentVariableIsSet("QT_LOGGING_DEBUG");
    return debugEnv;
}

static QVector<QLoggingRule> loadRulesFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (qtLoggingDebug())
            debugMsg("Loading \"%s\" ...",
                     QDir::toNativeSeparators(file.fileName()).toUtf8().constData());
        QTextStream stream(&file);
        QLoggingSettingsParser parser;
        parser.setContent(stream);
        return parser.rules();
    }
    return QVector<QLoggingRule>();
}

/*!
    \internal
    Initializes the rules database by loading
    $QT_LOGGING_CONF, $QT_LOGGING_RULES, and .config/QtProject/qtlogging.ini.
 */
void QLoggingRegistry::initializeRules()
{
    QVector<QLoggingRule> er, qr, cr;
    // get rules from environment
    const QByteArray rulesFilePath = qgetenv("QT_LOGGING_CONF");
    if (!rulesFilePath.isEmpty())
        er = loadRulesFromFile(QFile::decodeName(rulesFilePath));

    const QByteArray rulesSrc = qgetenv("QT_LOGGING_RULES").replace(';', '\n');
    if (!rulesSrc.isEmpty()) {
         QTextStream stream(rulesSrc);
         QLoggingSettingsParser parser;
         parser.setImplicitRulesSection(true);
         parser.setContent(stream);
         er += parser.rules();
    }

    const QString configFileName = QStringLiteral("qtlogging.ini");

#if !defined(QT_BOOTSTRAPPED)
    // get rules from Qt data configuration path
    const QString qtConfigPath
            = QDir(QLibraryInfo::location(QLibraryInfo::DataPath)).absoluteFilePath(configFileName);
    qr = loadRulesFromFile(qtConfigPath);
#endif

    // get rules from user's/system configuration
    const QString envPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                                   QString::fromLatin1("QtProject/") + configFileName);
    if (!envPath.isEmpty())
        cr = loadRulesFromFile(envPath);

    const QMutexLocker locker(&registryMutex);

    ruleSets[EnvironmentRules] = std::move(er);
    ruleSets[QtConfigRules] = std::move(qr);
    ruleSets[ConfigRules] = std::move(cr);

    if (!ruleSets[EnvironmentRules].isEmpty() || !ruleSets[QtConfigRules].isEmpty() || !ruleSets[ConfigRules].isEmpty())
        updateRules();
}

/*!
    \internal
    Registers a category object.

    This method might be called concurrently for the same category object.
*/
void QLoggingRegistry::registerCategory(QLoggingCategory *cat, QtMsgType enableForLevel)
{
    QMutexLocker locker(&registryMutex);

    if (!categories.contains(cat)) {
        categories.insert(cat, enableForLevel);
        (*categoryFilter)(cat);
    }
}

/*!
    \internal
    Unregisters a category object.
*/
void QLoggingRegistry::unregisterCategory(QLoggingCategory *cat)
{
    QMutexLocker locker(&registryMutex);
    categories.remove(cat);
}

/*!
    \internal
    Installs logging rules as specified in \a content.
 */
void QLoggingRegistry::setApiRules(const QString &content)
{
    QLoggingSettingsParser parser;
    parser.setImplicitRulesSection(true);
    parser.setContent(content);

    if (qtLoggingDebug())
        debugMsg("Loading logging rules set by QLoggingCategory::setFilterRules ...");

    const QMutexLocker locker(&registryMutex);

    ruleSets[ApiRules] = parser.rules();

    updateRules();
}

/*!
    \internal
    Activates a new set of logging rules for the default filter.

    (The caller must lock registryMutex to make sure the API is thread safe.)
*/
void QLoggingRegistry::updateRules()
{
    for (auto it = categories.keyBegin(), end = categories.keyEnd(); it != end; ++it)
        (*categoryFilter)(*it);
}

/*!
    \internal
    Installs a custom filter rule.
*/
QLoggingCategory::CategoryFilter
QLoggingRegistry::installFilter(QLoggingCategory::CategoryFilter filter)
{
    QMutexLocker locker(&registryMutex);

    if (filter == 0)
        filter = defaultCategoryFilter;

    QLoggingCategory::CategoryFilter old = categoryFilter;
    categoryFilter = filter;

    updateRules();

    return old;
}

QLoggingRegistry *QLoggingRegistry::instance()
{
    return qtLoggingRegistry();
}

/*!
    \internal
    Updates category settings according to rules.

    As a category filter, it is run with registryMutex held.
*/
void QLoggingRegistry::defaultCategoryFilter(QLoggingCategory *cat)
{
    const QLoggingRegistry *reg = QLoggingRegistry::instance();
    Q_ASSERT(reg->categories.contains(cat));
    QtMsgType enableForLevel = reg->categories.value(cat);

    // NB: note that the numeric values of the Qt*Msg constants are
    //     not in severity order.
    bool debug = (enableForLevel == QtDebugMsg);
    bool info = debug || (enableForLevel == QtInfoMsg);
    bool warning = info || (enableForLevel == QtWarningMsg);
    bool critical = warning || (enableForLevel == QtCriticalMsg);

    // hard-wired implementation of
    //   qt.*.debug=false
    //   qt.debug=false
    if (const char *categoryName = cat->categoryName()) {
        // == "qt" or startsWith("qt.")
        if (strcmp(categoryName, "qt") == 0 || strncmp(categoryName, "qt.", 3) == 0)
            debug = false;
    }

    QString categoryName = QLatin1String(cat->categoryName());

    for (const auto &ruleSet : reg->ruleSets) {
        for (const auto &rule : ruleSet) {
            int filterpass = rule.pass(categoryName, QtDebugMsg);
            if (filterpass != 0)
                debug = (filterpass > 0);
            filterpass = rule.pass(categoryName, QtInfoMsg);
            if (filterpass != 0)
                info = (filterpass > 0);
            filterpass = rule.pass(categoryName, QtWarningMsg);
            if (filterpass != 0)
                warning = (filterpass > 0);
            filterpass = rule.pass(categoryName, QtCriticalMsg);
            if (filterpass != 0)
                critical = (filterpass > 0);
        }
    }

    cat->setEnabled(QtDebugMsg, debug);
    cat->setEnabled(QtInfoMsg, info);
    cat->setEnabled(QtWarningMsg, warning);
    cat->setEnabled(QtCriticalMsg, critical);
}


QT_END_NAMESPACE
