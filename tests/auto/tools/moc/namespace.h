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

#ifndef NAMESPACE_H
#define NAMESPACE_H

#include <QObject>

#include "namespace_no_merge.h"
// moc should not merge namespace_no_merge.h content with this one !

namespace FooNamespace {
    Q_NAMESPACE
    enum class Enum1 {
        Key1,
        Key2
    };
    Q_ENUM_NS(Enum1)

    namespace FooNestedNamespace {
        Q_NAMESPACE
        enum class Enum2 {
            Key3,
            Key4
        };
        Q_ENUM_NS(Enum2)
    }

    using namespace FooNamespace;
    namespace Bar = FooNamespace;

    // Moc should merge this namespace with the previous one
    namespace FooNestedNamespace {
        Q_NAMESPACE
        enum class Enum3 {
            Key5,
            Key6
        };
        Q_ENUM_NS(Enum3)

        namespace FooMoreNestedNamespace {
            Q_NAMESPACE
            enum class Enum4 {
                Key7,
                Key8
            };
            Q_ENUM_NS(Enum4)
        }
    }
}

#ifdef Q_MOC_RUN
namespace __identifier("<AtlImplementationDetails>") {} // QTBUG-56634
using namespace __identifier("<AtlImplementationDetails>"); // QTBUG-63772
#endif

#endif // NAMESPACE_H
