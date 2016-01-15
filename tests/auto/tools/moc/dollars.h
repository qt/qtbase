/****************************************************************************
**
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
