// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMAKEEVALUATOR_P_H
#define QMAKEEVALUATOR_P_H

#include "proitems.h"

#define debugMsg if (!m_debugLevel) {} else debugMsgInternal
#define traceMsg if (!m_debugLevel) {} else traceMsgInternal
#ifdef PROEVALUATOR_DEBUG
#  define dbgBool(b) (b ? "true" : "false")
#  define dbgReturn(r) \
    (r == ReturnError ? "error" : \
     r == ReturnBreak ? "break" : \
     r == ReturnNext ? "next" : \
     r == ReturnReturn ? "return" : \
     "<invalid>")
#  define dbgKey(s) s.toString().toQStringView().toLocal8Bit().constData()
#  define dbgStr(s) qPrintable(formatValue(s, true))
#  define dbgStrList(s) qPrintable(formatValueList(s))
#  define dbgSepStrList(s) qPrintable(formatValueList(s, true))
#  define dbgStrListList(s) qPrintable(formatValueListList(s))
#  define dbgQStr(s) dbgStr(ProString(s))
#else
#  define dbgBool(b) 0
#  define dbgReturn(r) 0
#  define dbgKey(s) 0
#  define dbgStr(s) 0
#  define dbgStrList(s) 0
#  define dbgSepStrList(s) 0
#  define dbgStrListList(s) 0
#  define dbgQStr(s) 0
#endif

QT_BEGIN_NAMESPACE

namespace QMakeInternal {

struct QMakeBuiltinInit
{
    const char *name;
    int func;
    enum { VarArgs = 1000 };
    int min_args, max_args;
    const char *args;
};

struct QMakeBuiltin
{
    QMakeBuiltin(const QMakeBuiltinInit &data);
    QString usage;
    int index, minArgs, maxArgs;
};

struct QMakeStatics {
    QString field_sep;
    QString strtrue;
    QString strfalse;
    ProKey strCONFIG;
    ProKey strARGS;
    ProKey strARGC;
    QString strDot;
    QString strDotDot;
    QString strever;
    QString strforever;
    QString strhost_build;
    ProKey strTEMPLATE;
    ProKey strQMAKE_PLATFORM;
    ProKey strQMAKE_DIR_SEP;
    ProKey strQMAKESPEC;
#ifdef PROEVALUATOR_FULL
    ProKey strREQUIRES;
#endif
    QHash<ProKey, QMakeBuiltin> expands;
    QHash<ProKey, QMakeBuiltin> functions;
    QHash<ProKey, ProKey> varMap;
    ProStringList fakeValue;
};

extern QMakeStatics statics;

}

Q_DECLARE_TYPEINFO(QMakeInternal::QMakeBuiltin, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QMAKEEVALUATOR_P_H
