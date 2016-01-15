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

#ifndef CXX11_ENUMS_H
#define CXX11_ENUMS_H
#include <QtCore/QObject>

#if defined(Q_COMPILER_CLASS_ENUM) || defined(Q_MOC_RUN)
class CXX11Enums
{
    Q_GADGET
public:
    enum class EnumClass { A0, A1, A2, A3 };
    enum TypedEnum : char { B0, B1 , B2, B3 };
    enum class TypedEnumClass : char { C0, C1, C2, C3 };
    enum NormalEnum { D2 = 2, D3, D0 =0 , D1 };
    Q_ENUM(EnumClass)
    Q_ENUM(TypedEnum)
    Q_ENUM(TypedEnumClass)
    Q_ENUM(NormalEnum)
};

// Also test the Q_ENUMS macro
class CXX11Enums2
{
    Q_GADGET
public:
    enum class EnumClass { A0, A1, A2, A3 };
    enum TypedEnum : char { B0, B1 , B2, B3 };
    enum class TypedEnumClass : char { C0, C1, C2, C3 };
    enum NormalEnum { D2 = 2, D3, D0 =0 , D1 };
    Q_ENUMS(EnumClass TypedEnum TypedEnumClass NormalEnum)
};

#else
//workaround to get the moc compiled code to compile
class CXX11Enums
{
    Q_GADGET
public:
    struct EnumClass { enum { A0, A1, A2, A3 }; };
    struct TypedEnumClass { enum { C0, C1, C2, C3 }; };
    enum NormalEnum { D2 = 2, D3, D0 =0 , D1 };
    enum TypedEnum { B0, B1 , B2, B3 };
};

class CXX11Enums2 : public CXX11Enums
{
    Q_GADGET
};
#endif
#endif // CXX11_ENUMS_H
