// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Found first on the include path. __has_include_next must search *past* this
// directory: <probe.h> exists in the next directory (true), while
// <nonexistent-next.h> exists nowhere (false).
#if __has_include_next(<probe.h>)
int probe_has_next;
#else
int probe_no_next;
#endif

#if __has_include_next(<nonexistent-next.h>)
int nonexistent_has_next;
#else
int nonexistent_no_next;
#endif

// The argument is macro expanded unless it already is a header-name, for
// __has_include_next just as for __has_include.
#define PROBE_ANGLE_WRAP(x) <x>
#define PROBE_HEADER_NAME probe.h
#define PROBE_WRAPPED_NAME PROBE_ANGLE_WRAP(PROBE_HEADER_NAME)

#if __has_include_next(PROBE_WRAPPED_NAME)
int wrapped_has_next;
#else
int wrapped_no_next;
#endif

#if __has_include_next(PROBE_ANGLE_WRAP(PROBE_HEADER_NAME))
int funcmacro_has_next;
#else
int funcmacro_no_next;
#endif
