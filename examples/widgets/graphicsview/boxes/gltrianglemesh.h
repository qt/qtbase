/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GLTRIANGLEMESH_H
#define GLTRIANGLEMESH_H

//#include <GL/glew.h>
#include "glextensions.h"

#include <QtWidgets>
#include <QtOpenGL>

#include "glbuffers.h"

template<class TVertex, class TIndex>
class GLTriangleMesh
{
public:
    GLTriangleMesh(int vertexCount, int indexCount) : m_vb(vertexCount), m_ib(indexCount)
    {
    }

    virtual ~GLTriangleMesh()
    {
    }

    virtual void draw()
    {
        if (failed())
            return;

        int type = GL_UNSIGNED_INT;
        if (sizeof(TIndex) == sizeof(char)) type = GL_UNSIGNED_BYTE;
        if (sizeof(TIndex) == sizeof(short)) type = GL_UNSIGNED_SHORT;

        m_vb.bind();
        m_ib.bind();
        glDrawElements(GL_TRIANGLES, m_ib.length(), type, BUFFER_OFFSET(0));
        m_vb.unbind();
        m_ib.unbind();
    }

    bool failed()
    {
        return m_vb.failed() || m_ib.failed();
    }
protected:
    GLVertexBuffer<TVertex> m_vb;
    GLIndexBuffer<TIndex> m_ib;
};


#endif
