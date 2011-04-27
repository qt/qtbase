/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include <QtCore/qdebug.h>

#define QT_DEBUG_SHADER_CACHE
#define QT_MEEGO_EXPERIMENTAL_SHADERCACHE
#define QT_OPENGL_ES_2
#define QT_BOOTSTRAPPED

typedef int GLsizei;
typedef unsigned int GLenum;

#include "../../gl2paintengineex/qglshadercache_meego_p.h"

#include <stdlib.h>
#include <stdio.h>

int main()
{
    ShaderCacheSharedMemory shm;

    if (!shm.isAttached()) {
        fprintf(stderr, "Unable to attach to shared memory\n");
        return EXIT_FAILURE;
    }

    ShaderCacheLocker locker(&shm);
    if (!locker.isLocked()) {
        fprintf(stderr, "Unable to lock shared memory\n");
        return EXIT_FAILURE;
    }

    void *data = shm.data();
    Q_ASSERT(data);

    CachedShaders *cache = reinterpret_cast<CachedShaders *>(data);

    for (int i = 0; i < cache->shaderCount; ++i) {
        printf("Shader %d: %d bytes\n", i, cache->headers[i].size);
    }

    printf("\nSummary:\n\n"
           "    Amount of cached shaders: %d\n"
           "                  Bytes used: %d\n"
           "             Bytes available: %d\n",
           cache->shaderCount, cache->dataSize, cache->availableSize());

    return EXIT_SUCCESS;
}

