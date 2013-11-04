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
    \internal
    Creates a new QLoggingRules object.
*/
QLoggingRulesParser::QLoggingRulesParser(QLoggingRegistry *registry)  :
    registry(registry)
{
}

/*!
    \internal
    Sets logging rules string.
*/
void QLoggingRulesParser::setRules(const QString &content)
{
    QString content_ = content;
    QTextStream stream(&content_, QIODevice::ReadOnly);
    parseRules(stream);
}

/*!
    \internal
    Parses rules out of a QTextStream.
*/
void QLoggingRulesParser::parseRules(QTextStream &stream)
{
    QVector<QLoggingRule> rules;

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        // Remove all whitespace from line
        line = line.simplified();
        line.remove(QLatin1Char(' '));

        const QStringList pair = line.split(QLatin1Char('='));
        if (pair.count() == 2) {
            const QString pattern = pair.at(0);
            bool enabled = (QString::compare(pair.at(1),
                                             QLatin1String("true"),
                                             Qt::CaseInsensitive) == 0);
            rules.append(QLoggingRule(pattern, enabled));
        }
    }

    registry->setRules(rules);
}

/*!
    \internal
    QLoggingPrivate constructor
 */
QLoggingRegistry::QLoggingRegistry()
    : rulesParser(this),
      categoryFilter(defaultCategoryFilter)
{
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
    Activates a new set of logging rules for the default filter.
*/
void QLoggingRegistry::setRules(const QVector<QLoggingRule> &rules_)
{
    QMutexLocker locker(&registryMutex);

    rules = rules_;

    if (categoryFilter != defaultCategoryFilter)
        return;

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
    // QLoggingCategory() normalizes all "default" strings
    // to qtDefaultCategoryName
    bool debug = (cat->categoryName() == qtDefaultCategoryName);
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
