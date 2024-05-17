// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef NAMESPACE_NO_MERGE_H
#define NAMESPACE_NO_MERGE_H

#include <QObject>

namespace FooNamespace {
    Q_NAMESPACE
    enum class MEnum1 {
        Key1,
        Key2
    };
    Q_ENUM_NS(MEnum1)

    namespace FooNestedNamespace {
        Q_NAMESPACE
        enum class MEnum2 {
            Key3,
            Key4
        };
        Q_ENUM_NS(MEnum2)
    }

    using namespace FooNamespace;
    namespace Bar = FooNamespace;

    // Moc should merge this namespace with the previous one
    namespace FooNestedNamespace {
        Q_NAMESPACE
        enum class MEnum3 {
            Key5,
            Key6
        };
        Q_ENUM_NS(MEnum3)

        namespace FooMoreNestedNamespace {
            Q_NAMESPACE
            enum class MEnum4 {
                Key7,
                Key8
            };
            Q_ENUM_NS(MEnum4)
        }
    }
}

#endif // NAMESPACE_NO_MERGE_H
