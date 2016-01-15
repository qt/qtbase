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

#ifndef NAMESPACED_FLAGS_H
#define NAMESPACED_FLAGS_H
#include <QObject>

namespace Foo {
    class Bar : public QObject {
        Q_OBJECT
        Q_PROPERTY( Flags flags READ flags WRITE setFlags )
    public:
        explicit Bar( QObject * parent=0 ) : QObject( parent ), mFlags() {}

        enum Flag { Read=1, Write=2 };
        Q_DECLARE_FLAGS( Flags, Flag )
        Q_FLAG(Flags)


        void setFlags( Flags f ) { mFlags = f; }
        Flags flags() const { return mFlags; }

    private:
        Flags mFlags;
    };

    class Baz : public QObject {
        Q_OBJECT
        //Q_PROPERTY( Bar::Flags flags READ flags WRITE setFlags ) // triggers assertion
        Q_PROPERTY( Foo::Bar::Flags flags READ flags WRITE setFlags ) // fails to compile, or with the same assertion if moc fix is applied
        Q_PROPERTY( QList<Foo::Bar::Flags> flagsList READ flagsList WRITE setFlagsList )
    public:
        explicit Baz( QObject * parent=0 ) : QObject( parent ), mFlags() {}

        void setFlags( Bar::Flags f ) { mFlags = f; }
        Bar::Flags flags() const { return mFlags; }

        void setFlagsList( const QList<Bar::Flags> &f ) { mList = f; }
        QList<Bar::Flags> flagsList() const { return mList; }
    private:
        Bar::Flags mFlags;
        QList<Bar::Flags> mList;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS( Foo::Bar::Flags )
#endif // NAMESPACED_FLAGS_H
