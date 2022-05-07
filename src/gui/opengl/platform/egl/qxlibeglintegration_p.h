/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QXLIBEGLINTEGRATION_H
#define QXLIBEGLINTEGRATION_H

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

#include <QtGui/private/qeglconvenience_p.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QXlibEglIntegration
{
public:
    static VisualID getCompatibleVisualId(Display *display, EGLDisplay eglDisplay, EGLConfig config);
};

QT_END_NAMESPACE

#endif // QXLIBEGLINTEGRATION_H
