// Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QString>
#include <QStringIterator>
#include <QDebug>

int main()
{

{
//! [0]
QString string(QStringLiteral("a string"));
QStringIterator i(string); // implicitly converted to QStringView
//! [0]

//! [1]
while (i.hasNext())
    char32_t c = i.next();
//! [1]
}

{
//! [2]
QStringIterator i(u"𝄞 is the G clef");
qDebug() << Qt::hex << i.next(); // will print '𝄞' (U+1D11E, MUSICAL SYMBOL G CLEF)
qDebug() << Qt::hex << i.next(); // will print ' ' (U+0020, SPACE)
qDebug() << Qt::hex << i.next(); // will print 'i' (U+0069, LATIN SMALL LETTER I)
//! [2]
}

}
