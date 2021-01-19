/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QLOGGINGREGISTRY_P_H
#define QLOGGINGREGISTRY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qvector.h>

class tst_QLoggingRegistry;

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QLoggingRule
{
public:
    QLoggingRule();
    QLoggingRule(QStringView pattern, bool enabled);
    int pass(QLatin1String categoryName, QtMsgType type) const;

    enum PatternFlag {
        FullText = 0x1,
        LeftFilter = 0x2,
        RightFilter = 0x4,
        MidFilter = LeftFilter |  RightFilter
    };
    Q_DECLARE_FLAGS(PatternFlags, PatternFlag)

    QString category;
    int messageType;
    PatternFlags flags;
    bool enabled;

private:
    void parse(QStringView pattern);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QLoggingRule::PatternFlags)
Q_DECLARE_TYPEINFO(QLoggingRule, Q_MOVABLE_TYPE);

class Q_AUTOTEST_EXPORT QLoggingSettingsParser
{
public:
    void setImplicitRulesSection(bool inRulesSection) { m_inRulesSection = inRulesSection; }

    void setContent(const QString &content);
    void setContent(QTextStream &stream);

    QVector<QLoggingRule> rules() const { return _rules; }

private:
    void parseNextLine(QStringView line);

private:
    bool m_inRulesSection = false;
    QVector<QLoggingRule> _rules;
};

class Q_AUTOTEST_EXPORT QLoggingRegistry
{
public:
    QLoggingRegistry();

    void initializeRules();

    void registerCategory(QLoggingCategory *category, QtMsgType enableForLevel);
    void unregisterCategory(QLoggingCategory *category);

    void setApiRules(const QString &content);

    QLoggingCategory::CategoryFilter
    installFilter(QLoggingCategory::CategoryFilter filter);

    static QLoggingRegistry *instance();

private:
    void updateRules();

    static void defaultCategoryFilter(QLoggingCategory *category);

    enum RuleSet {
        // sorted by order in which defaultCategoryFilter considers them:
        QtConfigRules,
        ConfigRules,
        ApiRules,
        EnvironmentRules,

        NumRuleSets
    };

    QMutex registryMutex;

    // protected by mutex:
    QVector<QLoggingRule> ruleSets[NumRuleSets];
    QHash<QLoggingCategory*,QtMsgType> categories;
    QLoggingCategory::CategoryFilter categoryFilter;

    friend class ::tst_QLoggingRegistry;
};

QT_END_NAMESPACE

#endif // QLOGGINGREGISTRY_P_H
