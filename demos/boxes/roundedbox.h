/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ROUNDEDBOX_H
#define ROUNDEDBOX_H

//#include <GL/glew.h>
#include "glextensions.h"

#include <QtWidgets>
#include <QtOpenGL>

#include "gltrianglemesh.h"
#include <QtGui/qvector3d.h>
#include <QtGui/qvector2d.h>
#include "glbuffers.h"

struct P3T2N3Vertex
{
    QVector3D position;
    QVector2D texCoord;
    QVector3D normal;
    static VertexDescription description[];
};

class GLRoundedBox : public GLTriangleMesh<P3T2N3Vertex, unsigned short>
{
public:
    // 0 < r < 0.5, 0 <= n <= 125
    GLRoundedBox(float r = 0.25f, float scale = 1.0f, int n = 10);
};


#endif
