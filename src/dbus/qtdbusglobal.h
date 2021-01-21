/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTDBUSGLOBAL_H
#define QTDBUSGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qvariant.h>

#ifndef Q_MOC_RUN
# define Q_NOREPLY
#endif

#ifdef Q_CC_MSVC
#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#if QT_DEPRECATED_SINCE(5, 5)
#include <QtCore/qhash.h>
#endif
#include <QtCore/qvector.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DBUS

#ifndef QT_STATIC
#  if defined(QT_BUILD_DBUS_LIB)
#    define Q_DBUS_EXPORT Q_DECL_EXPORT
#  else
#    define Q_DBUS_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define Q_DBUS_EXPORT
#endif

#endif // QT_NO_DBUS

QT_END_NAMESPACE

#endif
