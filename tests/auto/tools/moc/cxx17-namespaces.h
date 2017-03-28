/****************************************************************************
**
** Copyright (C) 2011 Olivier Goffart.
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
