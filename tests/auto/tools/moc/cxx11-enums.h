// Copyright (C) 2011 Olivier Goffart.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CXX11_ENUMS_H
#define CXX11_ENUMS_H
#include <QtCore/QObject>

class CXX11Enums
{
    Q_GADGET
public:
    enum class EnumClass { A0, A1, A2, A3 };
    enum TypedEnum : char { B0, B1 , B2, B3 };
    enum class TypedEnumClass : char { C0, C1, C2, C3 };
    enum NormalEnum { D2 = 2, D3, D0 =0 , D1 };
    enum class ClassFlag { F0 = 1, F1 = 2, F2 = 4, F3 = 8};

    enum struct EnumStruct { G0, G1, G2, G3 };
    enum struct TypedEnumStruct : char { H0, H1, H2, H3 };
    enum struct StructFlag { I0 = 1, I1 = 2, I2 = 4, I3 = 8};

    Q_DECLARE_FLAGS(ClassFlags, ClassFlag)
    Q_DECLARE_FLAGS(StructFlags, StructFlag)

    Q_ENUM(EnumClass)
    Q_ENUM(TypedEnum)
    Q_ENUM(TypedEnumClass)
    Q_ENUM(NormalEnum)
    Q_ENUM(EnumStruct)
    Q_ENUM(TypedEnumStruct)
    Q_FLAG(ClassFlags)
    Q_FLAG(StructFlags)
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
    enum class ClassFlag { F0 = 1, F1 = 2, F2 = 4, F3 = 8 };
    Q_DECLARE_FLAGS(ClassFlags, ClassFlag)
    Q_ENUMS(EnumClass TypedEnum TypedEnumClass NormalEnum)
    Q_FLAGS(ClassFlags)
};

class CXX11Enums3 : public QObject
{
    Q_OBJECT
public:
    enum class EnumClass { A0, A1, A2, A3 };
    enum TypedEnum : char { B0, B1 , B2, B3 };
    enum class TypedEnumClass : char { C0, C1, C2, C3 };
    enum NormalEnum { D2 = 2, D3, D0 =0 , D1 };
    enum class ClassFlag { F0 = 1, F1 = 2, F2 = 4, F3 = 8 };

    Q_ENUM(EnumClass)
    Q_ENUM(TypedEnum)
    Q_ENUM(TypedEnumClass)
    Q_ENUM(NormalEnum)
};

#endif // CXX11_ENUMS_H
