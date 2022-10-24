// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_CTFPLUGIN_P_H
#define Q_CTFPLUGIN_P_H

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
//

#include "qctf_p.h"
#include <qplugin.h>

QT_BEGIN_NAMESPACE

class QCtfLib
{
public:
    virtual ~QCtfLib() = default;
    virtual bool tracepointEnabled(const QCtfTracePointEvent &point) = 0;
    virtual void doTracepoint(const QCtfTracePointEvent &point, const QByteArray &arr) = 0;
    virtual bool sessionEnabled() = 0;
    virtual QCtfTracePointPrivate *initializeTracepoint(const QCtfTracePointEvent &point) = 0;
};

Q_DECLARE_INTERFACE(QCtfLib, "org.qt-project.Qt.QCtfLib");

QT_END_NAMESPACE

#endif
