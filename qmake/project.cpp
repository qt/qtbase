// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "project.h"

#include "cachekeys.h"
#include "option.h"
#include <qmakeevaluator_p.h>

#include <qdir.h>

#include <stdio.h>

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

QMakeProject::QMakeProject()
    : QMakeEvaluator(Option::globals, Option::parser, Option::vfs, &Option::evalHandler)
{
}

QMakeProject::QMakeProject(QMakeProject *p)
    : QMakeEvaluator(Option::globals, Option::parser, Option::vfs, &Option::evalHandler)
{
    initFrom(p);
}

bool QMakeProject::boolRet(VisitReturn vr)
{
    if (vr == ReturnError) {
        qmakeClearCaches();
        exit(3);
    }
    Q_ASSERT(vr == ReturnTrue || vr == ReturnFalse);
    return vr != ReturnFalse;
}

bool QMakeProject::read(const QString &project, LoadFlags what)
{
    m_projectFile = project;
    setOutputDir(Option::output_dir);
    QString absproj = (project == QLatin1String("-"))
            ? QLatin1String("(stdin)")
            : QDir::cleanPath(QDir(qmake_getpwd()).absoluteFilePath(project));
    m_projectDir = QFileInfo(absproj).path();
    return boolRet(evaluateFile(absproj, QMakeHandler::EvalProjectFile, what));
}

static ProStringList prepareBuiltinArgs(const QList<ProStringList> &args)
{
    ProStringList ret;
    ret.reserve(args.size());
    for (const ProStringList &arg : args)
        ret << arg.join(' ');
    return ret;
}

bool QMakeProject::test(const ProKey &func, const QList<ProStringList> &args)
{
    m_current.clear();

    auto adef = statics.functions.constFind(func);
    if (adef != statics.functions.constEnd())
        return boolRet(evaluateBuiltinConditional(*adef, func, prepareBuiltinArgs(args)));

    QHash<ProKey, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(func);
    if (it != m_functionDefs.testFunctions.constEnd())
        return boolRet(evaluateBoolFunction(*it, args, func));

    evalError(QStringLiteral("'%1' is not a recognized test function.")
              .arg(func.toQStringView()));
    return false;
}

QStringList QMakeProject::expand(const ProKey &func, const QList<ProStringList> &args)
{
    m_current.clear();

    auto adef = statics.expands.constFind(func);
    if (adef != statics.expands.constEnd()) {
        ProStringList ret;
        if (evaluateBuiltinExpand(*adef, func, prepareBuiltinArgs(args), ret) == ReturnError)
            exit(3);
        return ret.toQStringList();
    }

    QHash<ProKey, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd()) {
        ProStringList ret;
        if (evaluateFunction(*it, args, &ret) == QMakeProject::ReturnError)
            exit(3);
        return ret.toQStringList();
    }

    evalError(QStringLiteral("'%1' is not a recognized replace function.")
              .arg(func.toQStringView()));
    return QStringList();
}

ProString QMakeProject::expand(const QString &expr, const QString &where, int line)
{
    ProString ret;
    ProFile *pro = m_parser->parsedProBlock(QStringView(expr), 0, where, line,
                                            QMakeParser::ValueGrammar);
    if (pro->isOk()) {
        m_current.pro = pro;
        m_current.line = 0;
        const ushort *tokPtr = pro->tokPtr();
        ProStringList result;
        if (expandVariableReferences(tokPtr, 1, &result, true) == ReturnError)
            exit(3);
        if (!result.isEmpty())
            ret = result.at(0);
    }
    pro->deref();
    return ret;
}

bool QMakeProject::isEmpty(const ProKey &v) const
{
    ProValueMap::ConstIterator it = m_valuemapStack.front().constFind(v);
    return it == m_valuemapStack.front().constEnd() || it->isEmpty();
}

void QMakeProject::dump() const
{
    QStringList out;
    for (ProValueMap::ConstIterator it = m_valuemapStack.front().begin();
         it != m_valuemapStack.front().end(); ++it) {
        if (!it.key().startsWith('.')) {
            QString str = it.key() + " =";
            for (const ProString &v : it.value())
                str += ' ' + formatValue(v);
            out << str;
        }
    }
    out.sort();
    for (const QString &v : std::as_const(out))
        puts(qPrintable(v));
}

QT_END_NAMESPACE
