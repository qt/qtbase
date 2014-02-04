/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qloggingregistry_p.h"
#include "qloggingcategory_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qtextstream.h>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QLoggingRegistry, qtLoggingRegistry)

/*!
    \internal
    Constructs a logging rule with default values.
*/
QLoggingRule::QLoggingRule() :
    flags(Invalid),
    enabled(false)
{
}

/*!
    \internal
    Constructs a logging rule.
*/
QLoggingRule::QLoggingRule(const QString &pattern, bool enabled) :
    pattern(pattern),
    flags(Invalid),
    enabled(enabled)
{
    parse();
}

/*!
    \internal
    Return value 1 means filter passed, 0 means filter doesn't influence this
    category, -1 means category doesn't pass this filter.
 */
int QLoggingRule::pass(const QString &categoryName, QtMsgType msgType) const
{
    QString fullCategory = categoryName;
    switch (msgType) {
    case QtDebugMsg:
        fullCategory += QLatin1String(".debug");
        break;
    case QtWarningMsg:
        fullCategory += QLatin1String(".warning");
        break;
    case QtCriticalMsg:
        fullCategory += QLatin1String(".critical");
        break;
    default:
        break;
    }

    if (flags == FullText) {
        // can be
        //   qtproject.org.debug = true
        // or
        //   qtproject.org = true
        if (pattern == categoryName
                || pattern == fullCategory)
            return (enabled ? 1 : -1);
    }

    int idx = 0;
    if (flags == MidFilter) {
        // e.g. *.qtproject*
        idx = fullCategory.indexOf(pattern);
        if (idx >= 0)
            return (enabled ? 1 : -1);
    } else {
        idx = fullCategory.indexOf(pattern);
        if (flags == LeftFilter) {
            // e.g. org.qtproject.*
            if (idx == 0)
                return (enabled ? 1 : -1);
        } else if (flags == RightFilter) {
            // e.g. *.qtproject
            if (idx == (fullCategory.count() - pattern.count()))
                return (enabled ? 1 : -1);
        }
    }
    return 0;
}

/*!
    \internal
    Parses the category and checks which kind of wildcard the filter can contain.
    Allowed is f.ex.:
             org.qtproject.logging FullText
             org.qtproject.*       LeftFilter
             *.qtproject           RightFilter
             *.qtproject*          MidFilter
 */
void QLoggingRule::parse()
{
    int index = pattern.indexOf(QLatin1Char('*'));
    if (index < 0) {
        flags = FullText;
    } else {
        flags = Invalid;
        if (index == 0) {
            flags |= RightFilter;
            pattern = pattern.remove(0, 1);
            index = pattern.indexOf(QLatin1Char('*'));
        }
        if (index == (pattern.length() - 1)) {
            flags |= LeftFilter;
            pattern = pattern.remove(pattern.length() - 1, 1);
        }
    }
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
    QString content_ = content;
    QTextStream stream(&content_, QIODevice::ReadOnly);
    setContent(stream);
}

/*!
    \internal
    Parses configuration from \a stream.
*/
void QLoggingSettingsParser::setContent(QTextStream &stream)
{
    _rules.clear();
    while (!stream.atEnd()) {
        QString line = stream.readLine();

        // Remove all whitespace from line
        line = line.simplified();
        line.remove(QLatin1Char(' '));

        // comment
        if (line.startsWith(QLatin1Char(';')))
            continue;

        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            // new section
            _section = line.mid(1, line.size() - 2);
            continue;
        }

        if (_section == QLatin1String("rules")) {
            int equalPos = line.indexOf(QLatin1Char('='));
            if ((equalPos != -1)
                    && (line.lastIndexOf(QLatin1Char('=')) == equalPos)) {
                const QString pattern = line.left(equalPos);
                const QStringRef value = line.midRef(equalPos + 1);
                bool enabled = (value.compare(QLatin1String("true"),
                                              Qt::CaseInsensitive) == 0);
                _rules.append(QLoggingRule(pattern, enabled));
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
}

/*!
    \internal
    Initializes the rules database by loading
    .config/QtProject/qtlogging.ini and $QT_LOGGING_CONF.
 */
void QLoggingRegistry::init()
{
    // get rules from environment
    const QByteArray rulesFilePath = qgetenv("QT_LOGGING_CONF");
    if (!rulesFilePath.isEmpty()) {
        QFile file(QFile::decodeName(rulesFilePath));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QLoggingSettingsParser parser;
            parser.setContent(stream);
            envRules = parser.rules();
        }
    }

    // get rules from qt configuration
    QString envPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                             QStringLiteral("QtProject/qtlogging.ini"));
    if (!envPath.isEmpty()) {
        QFile file(envPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QLoggingSettingsParser parser;
            parser.setContent(stream);
            configRules = parser.rules();
        }
    }

    if (!envRules.isEmpty() || !configRules.isEmpty()) {
        QMutexLocker locker(&registryMutex);
        updateRules();
    }
}

/*!
    \internal
    Registers a category object.

    This method might be called concurrently for the same category object.
*/
void QLoggingRegistry::registerCategory(QLoggingCategory *cat)
{
    QMutexLocker locker(&registryMutex);

    if (!categories.contains(cat)) {
        categories.append(cat);
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

    categories.removeOne(cat);
}

/*!
    \internal
    Installs logging rules as specified in \a content.
 */
void QLoggingRegistry::setApiRules(const QString &content)
{
    QLoggingSettingsParser parser;
    parser.setSection(QStringLiteral("rules"));
    parser.setContent(content);

    QMutexLocker locker(&registryMutex);
    apiRules = parser.rules();

    updateRules();
}

/*!
    \internal
    Activates a new set of logging rules for the default filter.

    (The caller must lock registryMutex to make sure the API is thread safe.)
*/
void QLoggingRegistry::updateRules()
{
    if (categoryFilter != defaultCategoryFilter)
        return;

    rules = configRules + apiRules + envRules;

    foreach (QLoggingCategory *cat, categories)
        (*categoryFilter)(cat);
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

    foreach (QLoggingCategory *cat, categories)
        (*categoryFilter)(cat);

    return old;
}

QLoggingRegistry *QLoggingRegistry::instance()
{
    return qtLoggingRegistry();
}

/*!
    \internal
    Updates category settings according to rules.
*/
void QLoggingRegistry::defaultCategoryFilter(QLoggingCategory *cat)
{
    // QLoggingCategory() normalizes "default" strings
    // to qtDefaultCategoryName
    bool debug = true;
    char c;
    if (!memcmp(cat->categoryName(), "qt", 2) && (!(c = cat->categoryName()[2]) || c == '.'))
        debug = false;

    bool warning = true;
    bool critical = true;

    QString categoryName = QLatin1String(cat->categoryName());
    QLoggingRegistry *reg = QLoggingRegistry::instance();
    foreach (const QLoggingRule &item, reg->rules) {
        int filterpass = item.pass(categoryName, QtDebugMsg);
        if (filterpass != 0)
            debug = (filterpass > 0);
        filterpass = item.pass(categoryName, QtWarningMsg);
        if (filterpass != 0)
            warning = (filterpass > 0);
        filterpass = item.pass(categoryName, QtCriticalMsg);
        if (filterpass != 0)
            critical = (filterpass > 0);
    }

    cat->setEnabled(QtDebugMsg, debug);
    cat->setEnabled(QtWarningMsg, warning);
    cat->setEnabled(QtCriticalMsg, critical);
}


QT_END_NAMESPACE
