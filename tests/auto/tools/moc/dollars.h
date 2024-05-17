// Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DOLLARS_H
#define DOLLARS_H

/* both GCC and clang allow $ in identifiers
 * So moc should not throw a parse error if it parses a file that contains such identifiers
 */

#include <QObject>

#define macro$1 function1
#define $macro2 function2

namespace $NS {
    class $CLS : public QObject
    {
        Q_PROPERTY(int rich$ MEMBER m_$rich$)
        Q_PROPERTY(int money$$$ READ $$$money$$$ WRITE $$$setMoney$$$)
        Q_OBJECT

        int m_$rich$;
        int m_money;
        int $$$money$$$() { return m_money; }
        int $$$setMoney$$$(int m) { return m_money = m; }

    Q_SIGNALS:
        void macro$1 ();
        void $macro2 ();

        void function$3 ($CLS * cl$s);
    };
}

#endif // DOLLARS_H
