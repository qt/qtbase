/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWAYLANDXCOMPOSITEGLXCONTEXT_H
#define QWAYLANDXCOMPOSITEGLXCONTEXT_H

#include <QtGui/QPlatformGLContext>

#include <QtCore/QWaitCondition>

#include "qwaylandbuffer.h"
#include "qwaylandxcompositeglxintegration.h"

#include "qglxconvenience.h"

class QWaylandXCompositeGLXWindow;
class QWaylandShmBuffer;

class QWaylandXCompositeGLXContext : public QPlatformGLContext
{
public:
    QWaylandXCompositeGLXContext(QWaylandXCompositeGLXIntegration *glxIntegration, QWaylandXCompositeGLXWindow *window);

    void makeCurrent();
    void doneCurrent();
    void swapBuffers();
    void* getProcAddress(const QString& procName);

    QPlatformWindowFormat platformWindowFormat() const;

    void geometryChanged();

private:
    QWaylandXCompositeGLXIntegration *mGlxIntegration;
    QWaylandXCompositeGLXWindow *mWindow;
    QWaylandBuffer *mBuffer;

    Window mXWindow;
    GLXFBConfig mConfig;
    GLXContext mContext;

    static void sync_function(void *data);
    void waitForSync();
    bool mWaitingForSyncCallback;
};

#endif // QWAYLANDXCOMPOSITEGLXCONTEXT_H
