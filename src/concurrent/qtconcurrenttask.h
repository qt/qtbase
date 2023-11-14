// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QTCONCURRENTTASK_H
#define QTCONCURRENTTASK_H

#if !defined(QT_NO_CONCURRENT)

#include <QtConcurrent/qtaskbuilder.h>

QT_BEGIN_NAMESPACE

#ifdef Q_QDOC

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

#endif // Q_QDOC

QT_END_NAMESPACE

#endif // !defined(QT_NO_CONCURRENT)

#endif // QTCONCURRENTTASK_H
