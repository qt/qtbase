/****************************************************************************
**
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

