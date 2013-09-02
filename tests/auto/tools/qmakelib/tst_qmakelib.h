/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qmakeevaluator.h>

#include <QObject>
#include <QProcessEnvironment>
#include <QtTest/QtTest>

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
    virtual void message(int type, const QString &msg, const QString &fileName, int lineNo)
        { print(fileName, lineNo, type, msg); }

    virtual void fileMessage(int type, const QString &msg)
        { Q_UNUSED(type) doPrint(msg); }

    virtual void aboutToEval(ProFile *, ProFile *, EvalFileType) {}
    virtual void doneWithEval(ProFile *) {}

    void setExpectedMessages(const QStringList &msgs) { expected = msgs; }
    QStringList expectedMessages() const { return expected; }

    bool printedMessages() const { return printed; }

private:
    void print(const QString &fileName, int lineNo, int type, const QString &msg);
    void doPrint(const QString &msg);

    QStringList expected;
    bool printed;
};

