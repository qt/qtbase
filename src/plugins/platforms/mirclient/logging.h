/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUBUNTULOGGING_H
#define QUBUNTULOGGING_H

// Logging and assertion macros.
#define LOG(...) qDebug(__VA_ARGS__)
#define LOG_IF(cond,...) do { if (cond) qDebug(__VA_ARGS__); } while(0)
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

#endif  // QUBUNTULOGGING_H
