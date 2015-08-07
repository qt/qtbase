/****************************************************************************
**
** Copyright (C) 2014 Canonical, Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QMIRCLIENTLOGGING_H
#define QMIRCLIENTLOGGING_H

// Logging and assertion macros.
#define LOG(...) qDebug(__VA_ARGS__)
#define LOG_IF(cond,...) do { if (cond) qDebug(__VA_ARGS__); } while (0)
#define ASSERT(cond) ((!(cond)) ? qt_assert(#cond,__FILE__,__LINE__) : qt_noop())
#define NOT_REACHED() qt_assert("Not reached!",__FILE__,__LINE__)

// Logging and assertion macros are compiled out for release builds.
#if !defined(QT_NO_DEBUG)
#define DLOG(...) LOG(__VA_ARGS__)
#define DLOG_IF(cond,...) LOG_IF((cond), __VA_ARGS__)
#define DASSERT(cond) ASSERT((cond))
#define DNOT_REACHED() NOT_REACHED()
#else
#define DLOG(...) qt_noop()
#define DLOG_IF(cond,...) qt_noop()
#define DASSERT(cond) qt_noop()
#define DNOT_REACHED() qt_noop()
#endif

#endif  // QMIRCLIENTLOGGING_H
