// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PROJECT_H
#define PROJECT_H

#include <qmakeevaluator.h>

QT_BEGIN_NAMESPACE

class QMakeProject : private QMakeEvaluator
{
    QString m_projectFile;
    QString m_projectDir;

public:
    QMakeProject();
    QMakeProject(QMakeProject *p);

    bool read(const QString &project, LoadFlags what = LoadAll);

    QString projectFile() const { return m_projectFile; }
    QString projectDir() const { return m_projectDir; }
    QString sourceRoot() const { return m_sourceRoot.isEmpty() ? m_buildRoot : m_sourceRoot; }
    QString buildRoot() const { return m_buildRoot; }
    QString confFile() const { return m_conffile; }
    QString cacheFile() const { return m_cachefile; }
    QString specDir() const { return m_qmakespec; }

    ProString expand(const QString &v, const QString &file, int line);
    QStringList expand(const ProKey &func, const QList<ProStringList> &args);
    bool test(const QString &v, const QString &file, int line)
        { m_current.clear(); return evaluateConditional(QStringView(v), file, line) == ReturnTrue; }
    bool test(const ProKey &func, const QList<ProStringList> &args);

    bool isSet(const ProKey &v) const { return m_valuemapStack.front().contains(v); }
    bool isEmpty(const ProKey &v) const;
    ProStringList &values(const ProKey &v) { return valuesRef(v); }
    int intValue(const ProKey &v, int defaultValue = 0) const;
    const ProValueMap &variables() const { return m_valuemapStack.front(); }
    ProValueMap &variables() { return m_valuemapStack.front(); }
    bool isActiveConfig(const QString &config, bool regex = false)
        { return QMakeEvaluator::isActiveConfig(QStringView(config), regex); }

    void dump() const;

    using QMakeEvaluator::LoadFlags;
    using QMakeEvaluator::VisitReturn;
    using QMakeEvaluator::setExtraVars;
    using QMakeEvaluator::setExtraConfigs;
    using QMakeEvaluator::loadSpec;
    using QMakeEvaluator::evaluateFeatureFile;
    using QMakeEvaluator::evaluateConfigFeatures;
    using QMakeEvaluator::evaluateExpression;
    using QMakeEvaluator::propertyValue;
    using QMakeEvaluator::values;
    using QMakeEvaluator::first;
    using QMakeEvaluator::isHostBuild;
    using QMakeEvaluator::dirSep;

private:
    static bool boolRet(VisitReturn vr);
};

/*!
 * For variables that are supposed to contain a single int,
 * this method returns the numeric value.
 * Only the first value of the variable is taken into account.
 * The string representation is assumed to look like a C int literal.
 */
inline int QMakeProject::intValue(const ProKey &v, int defaultValue) const
{
    const ProString &str = first(v);
    if (!str.isEmpty()) {
        bool ok;
        int i = str.toInt(&ok, 0);
        if (ok)
            return i;
    }
    return defaultValue;
}

QT_END_NAMESPACE

#endif // PROJECT_H
