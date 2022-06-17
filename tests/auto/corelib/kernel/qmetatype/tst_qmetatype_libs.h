// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TST_QMETATYPE_LIBS_H
#define TST_QMETATYPE_LIBS_H

#include <qmetatype.h>

#include <stdlib.h>         // for div_t

// void:        builtin metatype, special
// int:         builtin metatype, primitive type
// QString:     builtin metatype, class
// QCollator:   not builtin, class, Q_CORE_EXPORT
// div_t:       not builtin, class, no export
#define FOR_EACH_METATYPE_LIBS(F)           \
    F(void, QMetaType::Void)                \
    F(int, QMetaType::Int)                  \
    F(QString, QMetaType::QString)          \
    F(QCollator, QMetaType::UnknownType)    \
    F(div_t, QMetaType::UnknownType)        \
    /**/

#endif // TST_QMETATYPE_LIBS_H
