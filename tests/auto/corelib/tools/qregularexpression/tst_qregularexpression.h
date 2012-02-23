/****************************************************************************
**
** Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qobject.h>
#include <qregularexpression.h>

Q_DECLARE_METATYPE(QRegularExpression::PatternOptions)
Q_DECLARE_METATYPE(QRegularExpression::MatchType)
Q_DECLARE_METATYPE(QRegularExpression::MatchOptions)

class tst_QRegularExpression : public QObject
{
    Q_OBJECT

private slots:
    void gettersSetters_data();
    void gettersSetters();
    void escape_data();
    void escape();
    void validity_data();
    void validity();
    void patternOptions_data();
    void patternOptions();
    void normalMatch_data();
    void normalMatch();
    void partialMatch_data();
    void partialMatch();
    void globalMatch_data();
    void globalMatch();
    void serialize_data();
    void serialize();
    void operatoreq_data();
    void operatoreq();
    void captureCount_data();
    void captureCount();
    void pcreJitStackUsage_data();
    void pcreJitStackUsage();

private:
    void provideRegularExpressions();
};
