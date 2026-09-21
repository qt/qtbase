// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Bare-include forwarder in a plain -I directory, mirroring Qt's framework
// build layout (include/QtFoo/qfoo.h -> <QtFoo/qfoo.h>). The forwarder can't
// assume the framework path sits after it, so it probes with
// __has_include_next and continues into the framework when it does, and
// otherwise restarts the search with a plain #include.
#if __has_include_next(<Test/testinterface.h>)
#  include_next <Test/testinterface.h> // IWYU pragma: export
#else
#  include <Test/testinterface.h> // IWYU pragma: export
#endif
