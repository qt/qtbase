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

#ifndef QOPENWFDOUTPUTBUFFER_H
#define QOPENWFDOUTPUTBUFFER_H

#include "qopenwfdport.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

class QOpenWFDOutputBuffer
{
public:
    QOpenWFDOutputBuffer(const QSize &size, QOpenWFDPort *port);
    ~QOpenWFDOutputBuffer();
    void bindToCurrentFbo();
    bool isAvailable() const { return mAvailable; }
    void setAvailable(bool available) { mAvailable = available; }

    WFDSource wfdSource() const { return mWfdSource; }
private:
    QOpenWFDPort *mPort;
    WFDSource mWfdSource;
    GLuint mRbo;
    EGLImageKHR mEglImage;
    struct gbm_bo *mGbm_buffer;
    bool mAvailable;
};

#endif // QOPENWFDOUTPUTBUFFER_H
