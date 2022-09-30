// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2019 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGLOBAL_H
#define QGLOBAL_H

#if 0
#pragma qt_class(QtGlobal)
#endif

#ifdef __cplusplus
#  include <type_traits>
#  include <cstddef>
#  include <utility>
#  include <cstdint>
#endif
#ifndef __ASSEMBLER__
#  include <assert.h>
#  include <stdbool.h>
#  include <stddef.h>
#endif

#include <QtCore/qtversionchecks.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>

/* These two macros makes it possible to turn the builtin line expander into a
 * string literal. */
#define QT_STRINGIFY2(x) #x
#define QT_STRINGIFY(x) QT_STRINGIFY2(x)

inline void qt_noop(void) {}

#include <QtCore/qsystemdetection.h>
#include <QtCore/qprocessordetection.h>
#include <QtCore/qcompilerdetection.h>

#include <QtCore/qassert.h>
#include <QtCore/qtypes.h>
#include <QtCore/qtclasshelpermacros.h>

/*
   Avoid "unused parameter" warnings
*/
#define Q_UNUSED(x) (void)x;

#ifndef __ASSEMBLER__
QT_BEGIN_NAMESPACE

#if defined(__cplusplus)

#if !defined(Q_UNIMPLEMENTED)
#  define Q_UNIMPLEMENTED() qWarning("Unimplemented code.")
#endif

QT_END_NAMESPACE

// We need to keep QTypeInfo, QSysInfo, QFlags, qDebug & family in qglobal.h for compatibility with Qt 4.
// Be careful when changing the order of these files.
#include <QtCore/qtypeinfo.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qlogging.h>

#include <QtCore/qflags.h>

#include <QtCore/qatomic.h>
#include <QtCore/qconstructormacros.h>
#include <QtCore/qdarwinhelpers.h>
#include <QtCore/qexceptionhandling.h>
#include <QtCore/qforeach.h>
#include <QtCore/qfunctionpointer.h>
#include <QtCore/qglobalstatic.h>
#include <QtCore/qmalloc.h>
#include <QtCore/qminmax.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qoverload.h>
#include <QtCore/qswap.h>
#include <QtCore/qtdeprecationmarkers.h>
#include <QtCore/qtenvironmentvariables.h>
#include <QtCore/qtresource.h>
#include <QtCore/qttranslation.h>
#include <QtCore/qttypetraits.h>
#include <QtCore/qversiontagging.h>

#endif /* __cplusplus */
#endif /* !__ASSEMBLER__ */

#endif /* QGLOBAL_H */
