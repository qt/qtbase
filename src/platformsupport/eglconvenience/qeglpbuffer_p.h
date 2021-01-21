/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
****************************************************************************/

#ifndef QEGLPBUFFER_H
#define QEGLPBUFFER_H

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

#include <qpa/qplatformoffscreensurface.h>
#include <QtEglSupport/private/qeglplatformcontext_p.h>

QT_BEGIN_NAMESPACE

class QEGLPbuffer : public QPlatformOffscreenSurface
{
public:
    QEGLPbuffer(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface,
                QEGLPlatformContext::Flags flags = { });
    ~QEGLPbuffer();

    QSurfaceFormat format() const override { return m_format; }
    bool isValid() const override;

    EGLSurface pbuffer() const { return m_pbuffer; }

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_pbuffer;
    bool m_hasSurfaceless;
};

QT_END_NAMESPACE

#endif // QEGLPBUFFER_H
