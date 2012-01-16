/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "animation.h"

#include <QPointF>
#include <QIODevice>
#include <QDataStream>

class Frame 
{
public:
    Frame() {
    }

    int nodeCount() const 
    {
        return m_nodePositions.size();
    }

    void setNodeCount(int nodeCount)
    {
        while (nodeCount > m_nodePositions.size())
            m_nodePositions.append(QPointF());            
        
        while (nodeCount < m_nodePositions.size())
            m_nodePositions.removeLast();
    }

    QPointF nodePos(int idx) const
    {
        return m_nodePositions.at(idx);
    }

    void setNodePos(int idx, const QPointF &pos)
    {
        m_nodePositions[idx] = pos;
    }
    
private:
    QList<QPointF> m_nodePositions;
};

Animation::Animation()
{
    m_currentFrame = 0;
    m_frames.append(new Frame);
}

Animation::~Animation() 
{
    qDeleteAll(m_frames);
}

void Animation::setTotalFrames(int totalFrames)
{
    while (m_frames.size() < totalFrames)
        m_frames.append(new Frame);    

    while (totalFrames < m_frames.size())
        delete m_frames.takeLast();    
}

int Animation::totalFrames() const
{
    return m_frames.size();
}

void Animation::setCurrentFrame(int currentFrame)
{
    m_currentFrame = qMax(qMin(currentFrame, totalFrames()-1), 0);
}

int Animation::currentFrame() const
{
    return m_currentFrame;
}

void Animation::setNodeCount(int nodeCount)
{
    Frame *frame = m_frames.at(m_currentFrame);
    frame->setNodeCount(nodeCount);
}

int Animation::nodeCount() const
{
    Frame *frame = m_frames.at(m_currentFrame);
    return frame->nodeCount();
}

void Animation::setNodePos(int idx, const QPointF &pos)
{
    Frame *frame = m_frames.at(m_currentFrame);
    frame->setNodePos(idx, pos);
}

QPointF Animation::nodePos(int idx) const
{
    Frame *frame = m_frames.at(m_currentFrame);
    return frame->nodePos(idx);
}

QString Animation::name() const
{
    return m_name;
}

void Animation::setName(const QString &name)
{
    m_name = name;
}

void Animation::save(QIODevice *device) const
{
    QDataStream stream(device);
    stream << m_name;
    stream << m_frames.size();
    foreach (Frame *frame, m_frames) {
        stream << frame->nodeCount();
        for (int i=0; i<frame->nodeCount(); ++i)
            stream << frame->nodePos(i);
    }
}

void Animation::load(QIODevice *device)
{
    if (!m_frames.isEmpty())
        qDeleteAll(m_frames);

    m_frames.clear();

    QDataStream stream(device);
    stream >> m_name;
    
    int frameCount;
    stream >> frameCount;

    for (int i=0; i<frameCount; ++i) {
        
        int nodeCount;
        stream >> nodeCount;
        
        Frame *frame = new Frame;
        frame->setNodeCount(nodeCount);

        for (int j=0; j<nodeCount; ++j) {
            QPointF pos;
            stream >> pos;

            frame->setNodePos(j, pos);
        }

        m_frames.append(frame);
    }
}
