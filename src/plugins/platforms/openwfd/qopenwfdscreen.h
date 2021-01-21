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

#ifndef QOPENWFDSCREEN_H
#define QOPENWFDSCREEN_H

#include <qpa/qplatformscreen.h>


#include "qopenwfdoutputbuffer.h"

#include <WF/wfd.h>

#include <QtCore/QVarLengthArray>

#define BUFFER_NUM 4

class QOpenWFDPort;
class QOpenWFDScreen : public QPlatformScreen
{
public:
    QOpenWFDScreen(QOpenWFDPort *port);
    ~QOpenWFDScreen();

    QRect geometry() const;
    int depth() const;
    QImage::Format format() const;
    QSizeF physicalSize() const;

    QOpenWFDPort *port() const;

    void swapBuffers();
    void bindFramebuffer();
    void pipelineBindSourceComplete();

private:
    void setStagedBackBuffer(int bufferIndex);
    void commitStagedOutputBuffer();
    int nextAvailableRenderBuffer() const;

    QOpenWFDPort *mPort;

    GLuint mFbo;

    QVarLengthArray<QOpenWFDOutputBuffer *, BUFFER_NUM> mOutputBuffers;
    int mCurrentRenderBufferIndex;
    int mStagedBackBufferIndex;
    int mCommitedBackBufferIndex;
    int mBackBufferIndex;
};

#endif
