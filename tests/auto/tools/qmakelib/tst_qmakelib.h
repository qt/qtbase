// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qmakeevaluator.h>

#include <QObject>
#include <QProcessEnvironment>
#include <QTest>

class tst_qmakelib : public QObject
{
    Q_OBJECT

public:
    tst_qmakelib() {}
    virtual ~tst_qmakelib() {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void quoteArgUnix_data();
    void quoteArgUnix();
    void quoteArgWin_data();
    void quoteArgWin();

    void pathUtils();
    void ioUtilRelativity_data();
    void ioUtilRelativity();
    void ioUtilResolve_data();
    void ioUtilResolve();

    void proString();
    void proStringList();

    void proParser_data();
    void proParser();

    void proEval_data();
    void proEval();

private:
    void addParseOperators();
    void addParseValues();
    void addParseConditions();
    void addParseControlStatements();
    void addParseBraces();
    void addParseCustomFunctions();
    void addParseAbuse();

    void addAssignments();
    void addExpansions();
    void addControlStructs();
    void addReplaceFunctions(const QString &qindir);
    void addTestFunctions(const QString &qindir);

    QProcessEnvironment m_env;
    QHash<ProKey, ProString> m_prop;
    QString m_indir, m_outdir;
};

class QMakeTestHandler : public QMakeHandler {
public:
    QMakeTestHandler() : QMakeHandler(), printed(false) {}
    virtual void message(int type, const QString &msg, const QString &fileName, int lineNo) override
        { print(fileName, lineNo, type, msg); }

    virtual void fileMessage(int type, const QString &msg) override
        { Q_UNUSED(type); doPrint(msg); }

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) override {}
    virtual void doneWithEval(ProFile *) override {}

    void setExpectedMessages(const QStringList &msgs) { expected = msgs; }
    QStringList expectedMessages() const { return expected; }

    bool printedMessages() const { return printed; }

private:
    void print(const QString &fileName, int lineNo, int type, const QString &msg);
    void doPrint(const QString &msg);

    QStringList expected;
    bool printed;
};

