// Copyright (C) 2011 Olivier Goffart.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CXX17_NAMESPACES_H
#define CXX17_NAMESPACES_H
#include <QtCore/QObject>

#if defined(__cpp_nested_namespace_definitions) || defined(Q_MOC_RUN)
namespace CXX17Namespace::A::B {
namespace C::D {
namespace E::F::G { } // don't confuse moc
#else
namespace CXX17Namespace { namespace A { namespace B {
namespace C { namespace D {
#endif

Q_NAMESPACE

class ClassInNamespace
{
    Q_GADGET
public:
    enum GadEn { Value = 3 };
    Q_ENUM(GadEn)
};

enum NamEn { Value = 4 };
Q_ENUM_NS(NamEn);


#if defined(__cpp_nested_namespace_definitions) || defined(Q_MOC_RUN)
}
}
#else
} } }
} }
#endif

#endif
