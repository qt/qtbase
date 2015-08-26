/****************************************************************************
**
** Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
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

#include <qobject.h>
#include <qregularexpression.h>

Q_DECLARE_METATYPE(QRegularExpression::PatternOptions)
Q_DECLARE_METATYPE(QRegularExpression::MatchType)
Q_DECLARE_METATYPE(QRegularExpression::MatchOptions)

class tst_QRegularExpression : public QObject
{
    Q_OBJECT

private slots:
    void defaultConstructors();
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
    void captureNames_data();
    void captureNames();
    void pcreJitStackUsage_data();
    void pcreJitStackUsage();
    void regularExpressionMatch_data();
    void regularExpressionMatch();
    void JOptionUsage_data();
    void JOptionUsage();
    void QStringAndQStringRefEquivalence();

private:
    void provideRegularExpressions();
};
