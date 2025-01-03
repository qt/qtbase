// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHINULL_H
#define QRHINULL_H

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

#include <private/qrhi_p.h>

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QRhiNullInitParams : public QRhiInitParams
{
};

struct Q_GUI_EXPORT QRhiNullNativeHandles : public QRhiNativeHandles
{
};

QT_END_NAMESPACE

#endif
