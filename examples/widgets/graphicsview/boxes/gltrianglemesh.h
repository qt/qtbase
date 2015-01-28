/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
