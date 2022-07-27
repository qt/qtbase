// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTVERSIONCHECKS_H
#define QTVERSIONCHECKS_H

#if 0
#pragma qt_class(QtVersionChecks)
#pragma qt_sync_stop_processing
#endif

/*
   QT_VERSION is (major << 16) | (minor << 8) | patch.
*/
#define QT_VERSION      QT_VERSION_CHECK(QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH)
/*
   can be used like #if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
*/
#define QT_VERSION_CHECK(major, minor, patch) ((major<<16)|(minor<<8)|(patch))

#endif /* QTVERSIONCHECKS_H */
