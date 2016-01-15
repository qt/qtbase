/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef TRIGRAPHS_H
#define TRIGRAPHS_H

#include <QObject>

namespace AAA {
    struct BaseA {};
}

namespace BBB {
    class Foo : public QObject, public ::AAA::BaseA
    {
        Q_OBJECT
        Q_SIGNALS:
        // don't turn "> >" into ">>"
        void foo(QList<QList<int> >);
        void foo2(const QList<QList<int> > &);

        // don't turn "< :" into "<:"
        void bar(QList< ::AAA::BaseA*>);
        void bar2(const QList< ::AAA::BaseA*> &);
        void bar3(QList< ::AAA::BaseA const*>);
    };
}

#endif // TRIGRAPHS_H
