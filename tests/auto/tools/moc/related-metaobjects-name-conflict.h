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
