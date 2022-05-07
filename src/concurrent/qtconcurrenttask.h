/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QTCONCURRENTTASK_H
#define QTCONCURRENTTASK_H

#if !defined(QT_NO_CONCURRENT)

#include <QtConcurrent/qtaskbuilder.h>

QT_BEGIN_NAMESPACE

#ifdef Q_CLANG_QDOC

namespace QtConcurrent {

template <class Task>
[[nodiscard]]
QTaskBuilder<Task> task(Task &&task);

} // namespace QtConcurrent

#else

namespace QtConcurrent {

template <class Task>
[[nodiscard]]
constexpr auto task(Task &&t) { return QTaskBuilder(std::forward<Task>(t)); }

} // namespace QtConcurrent

#endif // Q_CLANG_QDOC

QT_END_NAMESPACE

#endif // !defined(QT_NO_CONCURRENT)

#endif // QTCONCURRENTTASK_H
