/****************************************************************************
**
** Copyright (C) 2022 Giuseppe D'Angelo <dangelog@gmail.com>.
** Copyright (C) 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:FDL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Free Documentation License Usage
** Alternatively, this file may be used under the terms of the GNU Free
** Documentation License version 1.3 as published by the Free Software
** Foundation and appearing in the file included in the packaging of
** this file. Please review the following information to ensure
** the GNU Free Documentation License version 1.3 requirements
** will be met: https://www.gnu.org/licenses/fdl-1.3.html.
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
QString p("a .*|pattern");

// re matches exactly the pattern string p
QRegularExpression re(QRegularExpression::anchoredPattern(p));
//! [0]

//! [1]
QString subject("the quick fox");

int offset = 0;
QRegExp re("(\\w+)");
while ((offset = re.indexIn(subject, offset)) != -1) {
    offset += re.matchedLength();
    // ...
}
//! [1]

//! [2]
QString subject("the quick fox");

QRegularExpression re("(\\w+)");
QRegularExpressionMatchIterator i = re.globalMatch(subject);
while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    // ...
}
//! [2]

//! [3]
QRegExp wildcard("*.txt");
wildcard.setPatternSyntax(QRegExp::Wildcard);
//! [3]

//! [4]
auto wildcard = QRegularExpression(QRegularExpression::wildcardToRegularExpression("*.txt"));
//! [4]

//! [5]
const QString fp1("C:/Users/dummy/files/content.txt");
const QString fp2("/home/dummy/files/content.txt");

QRegExp re1("*/files/*");
re1.setPatternSyntax(QRegExp::Wildcard);
re1.exactMatch(fp1); // returns true
re1.exactMatch(fp2); // returns true

// but converted with QRegularExpression::wildcardToRegularExpression()

QRegularExpression re2(QRegularExpression::wildcardToRegularExpression("*/files/*"));
re2.match(fp1).hasMatch(); // returns false
re2.match(fp2).hasMatch(); // returns false
//! [5]

//! [6]
QRegularExpression re3(QRegularExpression::wildcardToRegularExpression(
                           "*/files/*", QRegularExpression::UnanchoredWildcardConversion));
re3.match(fp1).hasMatch(); // returns true
re3.match(fp2).hasMatch(); // returns true
//! [6]
