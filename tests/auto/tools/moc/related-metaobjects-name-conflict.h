// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RELATED_METAOBJECTS_NAME_CONFLICT_H
#define RELATED_METAOBJECTS_NAME_CONFLICT_H

#include <QObject>

#define DECLARE_GADGET_AND_OBJECT_CLASSES \
    class Gadget { \
        Q_GADGET \
        Q_ENUMS(SomeEnum) \
    public: \
        enum SomeEnum { SomeEnumValue = 0 }; \
    }; \
    class Object : public QObject{ \
        Q_OBJECT \
        Q_ENUMS(SomeEnum) \
    public: \
        enum SomeEnum { SomeEnumValue = 0 }; \
    };

#define DECLARE_DEPENDING_CLASSES \
    class DependingObject : public QObject \
    { \
        Q_OBJECT \
        Q_PROPERTY(Gadget::SomeEnum gadgetPoperty READ gadgetPoperty) \
        Q_PROPERTY(Object::SomeEnum objectPoperty READ objectPoperty) \
    public: \
        Gadget::SomeEnum gadgetPoperty() const { return Gadget::SomeEnumValue; } \
        Object::SomeEnum objectPoperty() const { return Object::SomeEnumValue; } \
    };\
    struct DependingNestedGadget : public QObject \
    { \
        Q_OBJECT \
        Q_PROPERTY(Nested::Gadget::SomeEnum nestedGadgetPoperty READ nestedGadgetPoperty) \
        Nested::Gadget::SomeEnum nestedGadgetPoperty() const { return Nested::Gadget::SomeEnumValue; } \
    };\
    struct DependingNestedObject : public QObject \
    { \
        Q_OBJECT \
        Q_PROPERTY(Nested::Object::SomeEnum nestedObjectPoperty READ nestedObjectPoperty) \
        Nested::Object::SomeEnum nestedObjectPoperty() const { return Nested::Object::SomeEnumValue; } \
    };\


namespace Unsused {
    DECLARE_GADGET_AND_OBJECT_CLASSES
} // Unused

namespace NS1 {
namespace Nested {
    DECLARE_GADGET_AND_OBJECT_CLASSES
} // Nested

namespace NestedUnsused {
    DECLARE_GADGET_AND_OBJECT_CLASSES
} // NestedUnused

DECLARE_GADGET_AND_OBJECT_CLASSES
DECLARE_DEPENDING_CLASSES

} // NS1

namespace NS2 {
namespace Nested {
    DECLARE_GADGET_AND_OBJECT_CLASSES
} // Nested

namespace NestedUnsused {
    DECLARE_GADGET_AND_OBJECT_CLASSES
} // NestedUnused

DECLARE_GADGET_AND_OBJECT_CLASSES
DECLARE_DEPENDING_CLASSES

} // NS2

#endif // RELATED_METAOBJECTS_NAME_CONFLICT_H
