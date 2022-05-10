// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
