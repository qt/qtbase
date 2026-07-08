// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
// Despite its file name, this really is not a public header.
// It is an implementation detail of the private bootstrap library.
//
// Qt-Security score:significant reason:default

#if 0
// silence syncqt warnings
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifdef QT_BOOTSTRAPPED

#include <stdlib.h> // for __GLIBC_PREREQ

#ifndef QT_NO_EXCEPTIONS
#define QT_NO_EXCEPTIONS
#endif

#undef QT_DEBUG
#undef QT_FORCE_ASSERTS
#ifndef QT_NO_DEBUG
#  define QT_NO_DEBUG
#endif
#define QT_NO_DEBUG_OUTPUT
#define QT_NO_DEBUG_STREAM
#define QT_NO_INFO_OUTPUT
#define QT_NO_WARNING_OUTPUT

#define QT_NO_USING_NAMESPACE
#define QT_NO_DEPRECATED

// Keep feature-test macros in alphabetic order by feature name:
#define QT_FEATURE_cborstreamreader -1
#define QT_FEATURE_cborstreamwriter 1
#define QT_FEATURE_commandlineparser 1
#define QT_NO_COMPRESS
#define QT_FEATURE_copy_file_range -1
#define QT_FEATURE_cxx17_filesystem -1
#define QT_NO_DATASTREAM
#define QT_FEATURE_datestring 1
#define QT_FEATURE_datetimeparser -1
#define QT_FEATURE_dirfd (_POSIX_VERSION >= 200809L ? 1 : -1)
#define QT_FEATURE_dup3 -1
#define QT_FEATURE_easingcurve -1
#define QT_FEATURE_etw -1
#define QT_FEATURE_futimens -1
#undef QT_FEATURE_future
#define QT_FEATURE_future -1
#define QT_FEATURE_hijricalendar -1
#define QT_FEATURE_icu -1
#define QT_FEATURE_itemmodel -1
#define QT_FEATURE_islamiccivilcalendar -1
#define QT_FEATURE_jalalicalendar -1
#define QT_FEATURE_jemalloc -1
#define QT_FEATURE_journald -1
#define QT_FEATURE_library -1
#ifdef __linux__
# define QT_FEATURE_linkat 1
#else
# define QT_FEATURE_linkat -1
#endif
#define QT_FEATURE_lttng -1
#define QT_FEATURE_memmem -1
#define QT_FEATURE_memrchr -1
#define QT_NO_QOBJECT
#define QT_FEATURE_permissions -1
#define QT_FEATURE_process -1
#define QT_FEATURE_regularexpression 1
#ifdef __GLIBC_PREREQ
# define QT_FEATURE_renameat2 (__GLIBC_PREREQ(2, 28) ? 1 : -1)
#else
# define QT_FEATURE_renameat2 -1
#endif
#define QT_FEATURE_shortcut -1
#define QT_FEATURE_slog2 -1
#define QT_FEATURE_syslog -1
#define QT_NO_SYSTEMLOCALE
#define QT_FEATURE_temporaryfile -1
#define QT_FEATURE_textdate 1
#undef QT_FEATURE_thread
#define QT_FEATURE_thread -1
#define QT_FEATURE_timezone -1
#define QT_FEATURE_topleveldomain -1
#define QT_NO_TRANSLATION
#define QT_FEATURE_translation -1
#define QT_NO_VARIANT -1
#define QT_FEATURE_winsdkicu -1

#endif // QT_BOOTSTRAPPED
