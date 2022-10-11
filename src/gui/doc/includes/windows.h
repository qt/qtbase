// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// Dummy declarations for generating docs on non-Windows platforms
#if !defined(Q_OS_WIN) && defined(Q_QDOC)
typedef struct _FORMATETC {} FORMATETC;
typedef struct _STGMEDIUM {} STGMEDIUM;
typedef void *IDataObject;
#endif
