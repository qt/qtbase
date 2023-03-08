// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qloggingregistry_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/private/qlocking_p.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qstringtokenizer.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qdir.h>
#include <QtCore/qcoreapplication.h>

#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#include <QtCore/private/qsettings_p.h>
#endif

// We can't use the default macros because this would lead to recursion.
// Instead let's define our own one that unconditionally logs...
#define debugMsg QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, "qt.core.logging").debug
#define warnMsg QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, "qt.core.logging").warning

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
QLoggingRule::QLoggingRule(QStringView pattern, bool enabled) :
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
int QLoggingRule::pass(QLatin1StringView cat, QtMsgType msgType) const
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

    const qsizetype idx = cat.indexOf(category);
    if (idx >= 0) {
        if (flags == MidFilter) {
            // matches somewhere
            return (enabled ? 1 : -1);
        } else if (flags == LeftFilter) {
            // matches left
            if (idx == 0)
                return (enabled ? 1 : -1);
        } else if (flags == RightFilter) {
            // matches right
            if (idx == (cat.size() - category.size()))
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
void QLoggingRule::parse(QStringView pattern)
{
    QStringView p;

    // strip trailing ".messagetype"
    if (pattern.endsWith(".debug"_L1)) {
        p = pattern.chopped(6); // strlen(".debug")
        messageType = QtDebugMsg;
    } else if (pattern.endsWith(".info"_L1)) {
        p = pattern.chopped(5); // strlen(".info")
        messageType = QtInfoMsg;
    } else if (pattern.endsWith(".warning"_L1)) {
        p = pattern.chopped(8); // strlen(".warning")
        messageType = QtWarningMsg;
    } else if (pattern.endsWith(".critical"_L1)) {
        p = pattern.chopped(9); // strlen(".critical")
        messageType = QtCriticalMsg;
    } else {
        p = pattern;
    }

    const QChar asterisk = u'*';
    if (!p.contains(asterisk)) {
        flags = FullText;
    } else {
        if (p.endsWith(asterisk)) {
            flags |= LeftFilter;
            p = p.chopped(1);
        }
        if (p.startsWith(asterisk)) {
            flags |= RightFilter;
            p = p.mid(1);
        }
        if (p.contains(asterisk)) // '*' only supported at start/end
            flags = PatternFlags();
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
void QLoggingSettingsParser::setContent(QStringView content)
{
    _rules.clear();
    for (auto line : qTokenize(content, u'\n'))
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
        parseNextLine(qToStringViewIgnoringNull(line));
}

/*!
    \internal
    Parses one line of the configuration file
*/

void QLoggingSettingsParser::parseNextLine(QStringView line)
{
    // Remove whitespace at start and end of line:
    line = line.trimmed();

    // comment
    if (line.startsWith(u';'))
        return;

    if (line.startsWith(u'[') && line.endsWith(u']')) {
        // new section
        auto sectionName = line.mid(1).chopped(1).trimmed();
        m_inRulesSection = sectionName.compare("rules"_L1, Qt::CaseInsensitive) == 0;
        return;
    }

    if (m_inRulesSection) {
        const qsizetype equalPos = line.indexOf(u'=');
        if (equalPos != -1) {
            if (line.lastIndexOf(u'=') == equalPos) {
                const auto key = line.left(equalPos).trimmed();
#if QT_CONFIG(settings)
                QString tmp;
                QSettingsPrivate::iniUnescapedKey(key.toUtf8(), tmp);
                QStringView pattern = qToStringViewIgnoringNull(tmp);
#else
                QStringView pattern = key;
#endif
                const auto valueStr = line.mid(equalPos + 1).trimmed();
                int value = -1;
                if (valueStr == "true"_L1)
                    value = 1;
                else if (valueStr == "false"_L1)
                    value = 0;
                QLoggingRule rule(pattern, (value == 1));
                if (rule.flags != 0 && (value != -1))
                    _rules.append(std::move(rule));
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

static QList<QLoggingRule> loadRulesFromFile(const QString &filePath)
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
    return QList<QLoggingRule>();
}

/*!
    \internal
    Initializes the rules database by loading
    $QT_LOGGING_CONF, $QT_LOGGING_RULES, and .config/QtProject/qtlogging.ini.
 */
void QLoggingRegistry::initializeRules()
{
    QList<QLoggingRule> er, qr, cr;
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
            = QDir(QLibraryInfo::path(QLibraryInfo::DataPath)).absoluteFilePath(configFileName);
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
    const auto locker = qt_scoped_lock(registryMutex);

    const auto oldSize = categories.size();
    auto &e = categories[cat];
    if (categories.size() != oldSize) {
        // new entry
        e = enableForLevel;
        (*categoryFilter)(cat);
    }
}

/*!
    \internal
    Unregisters a category object.
*/
void QLoggingRegistry::unregisterCategory(QLoggingCategory *cat)
{
    const auto locker = qt_scoped_lock(registryMutex);
    categories.remove(cat);
}

/*!
    \since 6.3
    \internal

    Registers the environment variable \a environment as the control variable
    for enabling debugging by default for category \a categoryName. The
    category name must start with "qt."
*/
void QLoggingRegistry::registerEnvironmentOverrideForCategory(QByteArrayView categoryName,
                                                              QByteArrayView environment)
{
    qtCategoryEnvironmentOverrides.insert(categoryName, environment);
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
    const auto locker = qt_scoped_lock(registryMutex);

    if (!filter)
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
        if (strcmp(categoryName, "qt") == 0) {
            debug = false;
        } else if (strncmp(categoryName, "qt.", 3) == 0) {
            // may be overridden
            auto it = reg->qtCategoryEnvironmentOverrides.find(categoryName);
            if (it == reg->qtCategoryEnvironmentOverrides.end())
                debug = false;
            else
                debug = qEnvironmentVariableIntValue(it.value().data());
        }
    }

    const auto categoryName = QLatin1StringView(cat->categoryName());

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
