/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QOPENWFDSCREEN_H
#define QOPENWFDSCREEN_H

#include <qpa/qplatformscreen.h>


#include "qopenwfdoutputbuffer.h"

#include <WF/wfd.h>

#include <QtCore/QVarLengthArray>
#include <QtCore/QLinkedList>

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
