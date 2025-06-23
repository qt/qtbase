// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qloggingregistry_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/private/qlocking_p.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qstringtokenizer.h>
#include <QtCore/qdir.h>
#include <QtCore/qcoreapplication.h>
#include <qplatformdefs.h>

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
alignas(QLoggingCategory) static unsigned char defaultLoggingCategory[sizeof(QLoggingCategory)];

/*!
    \internal
    Constructs a logging rule with default values.
*/
QLoggingRule::QLoggingRule()
{
}

/*!
    \internal
    Constructs a logging rule.
*/
QLoggingRule::QLoggingRule(QStringView pattern, bool enabled) : enabled(enabled)
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
void QLoggingSettingsParser::setContent(QStringView content, char16_t separator)
{
    _rules.clear();
    for (auto line : qTokenize(content, separator))
        parseNextLine(line);
}

/*!
    \internal
    Parses configuration from \a stream.
*/
void QLoggingSettingsParser::setContent(FILE *stream)
{
    _rules.clear();

    constexpr size_t ChunkSize = 240;
    QByteArray buffer(ChunkSize, Qt::Uninitialized);
    auto readline = [&](FILE *stream) {
        // Read one line into the buffer

        // fgets() always writes the terminating null into the buffer, so we'll
        // allow it to write to the QByteArray's null (thus the off by 1).
        char *s = fgets(buffer.begin(), buffer.size() + 1, stream);
        if (!s)
            return QByteArrayView{};

        qsizetype len = strlen(s);
        while (len == buffer.size()) {
            // need to grow the buffer
            buffer.resizeForOverwrite(buffer.size() + ChunkSize);
            s = fgets(buffer.end() - ChunkSize, ChunkSize + 1, stream);
            if (!s)
                break;
            len += strlen(s);
        }
        QByteArrayView result(buffer.constBegin(), len);
        if (result.endsWith('\n'))
            result.chop(1);
        return result;
    };

    QByteArrayView line;
    while (!(line = readline(stream)).isNull())
        parseNextLine(QString::fromUtf8(line));
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
    using U = QLoggingCategory::UnregisteredInitialization;
    Q_ASSERT_X(!self, "QLoggingRegistry", "Singleton recreated");
    self = this;

    // can't use std::construct_at here - private constructor
    auto cat = new (defaultLoggingCategory) QLoggingCategory(U{}, defaultCategoryName);
    categories.emplace(cat, QtDebugMsg);

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
    static const bool debugEnv = [] {
        bool debug = qEnvironmentVariableIsSet("QT_LOGGING_DEBUG");
        if (debug)
            debugMsg("QT_LOGGING_DEBUG environment variable is set.");
        return debug;
    }();
    return Q_UNLIKELY(debugEnv);
}

static QList<QLoggingRule> loadRulesFromFile(const QString &filePath)
{
    Q_ASSERT(!filePath.isEmpty());
    if (qtLoggingDebug()) {
        debugMsg("Checking \"%s\" for rules",
                 QDir::toNativeSeparators(filePath).toUtf8().constData());
    }

    // We bypass QFile here because QFile is a QObject.
    if (Q_UNLIKELY(filePath.at(0) == u':')) {
        if (qtLoggingDebug()) {
            warnMsg("Attempted to load config rules from Qt resource path \"%ls\"",
                    qUtf16Printable(filePath));
        }
        return {};
    }

#ifdef Q_OS_WIN
    // text mode: let the runtime do CRLF translation
    FILE *f = _wfopen(reinterpret_cast<const wchar_t *>(filePath.constBegin()), L"rtN");
#else
    FILE *f = QT_FOPEN(QFile::encodeName(filePath).constBegin(), "re");
#endif
    if (f) {
        QLoggingSettingsParser parser;
        parser.setContent(f);
        fclose(f);
        if (qtLoggingDebug())
            debugMsg("Loaded %td rules from \"%ls\"", static_cast<ptrdiff_t>(parser.rules().size()),
                     qUtf16Printable(filePath));
        return parser.rules();
    } else if (int err = errno; err != ENOENT) {
        warnMsg("Failed to load file \"%ls\": %ls", qUtf16Printable(filePath),
                qUtf16Printable(qt_error_string(err)));
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
    if (qtLoggingDebug()) {
        debugMsg("Initializing the rules database ...");
        debugMsg("Checking %s environment variable", "QT_LOGGING_CONF");
    }
    QList<QLoggingRule> er, qr, cr;
    // get rules from environment
    if (QString rulesFilePath = qEnvironmentVariable("QT_LOGGING_CONF"); !rulesFilePath.isEmpty())
        er = loadRulesFromFile(rulesFilePath);

    if (qtLoggingDebug())
        debugMsg("Checking %s environment variable", "QT_LOGGING_RULES");

    const QString rulesSrc = qEnvironmentVariable("QT_LOGGING_RULES");
    if (!rulesSrc.isEmpty()) {
        QLoggingSettingsParser parser;
        parser.setImplicitRulesSection(true);
        parser.setContent(rulesSrc, u';');

        if (qtLoggingDebug())
            debugMsg("Loaded %td rules", static_cast<ptrdiff_t>(parser.rules().size()));

        er += parser.rules();
    }

    const QString configFileName = u"QtProject/qtlogging.ini"_s;
    QStringView baseConfigFileName = QStringView(configFileName).sliced(strlen("QtProject"));
    Q_ASSERT(baseConfigFileName.startsWith(u'/'));

    // get rules from Qt data configuration path
    qr = loadRulesFromFile(QLibraryInfo::path(QLibraryInfo::DataPath) + baseConfigFileName);

    // get rules from user's/system configuration
    // locateAll() returns the user's file (most overriding) first
    const QStringList configPaths =
            QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation, configFileName);
    for (qsizetype i = configPaths.size(); i > 0; --i)
        cr += loadRulesFromFile(configPaths[i - 1]);

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

    auto r = categories.tryEmplace(cat, enableForLevel);
    if (r.inserted) {
        // new entry
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
void QLoggingRegistry::registerEnvironmentOverrideForCategory(const char *categoryName,
                                                              const char *environment)
{
    qtCategoryEnvironmentOverrides.insert_or_assign(categoryName, environment);
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
    Q_CONSTINIT thread_local bool recursionGuard = false;
    if (recursionGuard)
        return nullptr;
    QScopedValueRollback<bool> rollback(recursionGuard, true);
    return qtLoggingRegistry();
}

QLoggingCategory *QLoggingRegistry::defaultCategory()
{
    // Initialize the defaultLoggingCategory global static, if necessary. Note
    // how it remains initialized forever, even if the QLoggingRegistry
    // instance() is destroyed.
    instance();

    // std::launder() to be on the safe side, but it's unnecessary because the
    // object is never recreated.
    return std::launder(reinterpret_cast<QLoggingCategory *>(defaultLoggingCategory));
}

/*!
    \internal
    Updates category settings according to rules.

    As a category filter, it is run with registryMutex held.
*/
void QLoggingRegistry::defaultCategoryFilter(QLoggingCategory *cat)
{
    const QLoggingRegistry *reg = self;
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
                debug = qEnvironmentVariableIntValue(it->second);
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
