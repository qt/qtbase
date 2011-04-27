/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef Q_GLX_CONTEXT_H
#define Q_GLX_CONTEXT_H

#include "qxlibwindow.h"

#include <QtGui/QPlatformGLContext>
#include <QtGui/QPlatformWindowFormat>

#include <QtCore/QMutex>

#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
#include <GL/glx.h>

QT_BEGIN_NAMESPACE

class QGLXContext : public QPlatformGLContext
{
public:
    QGLXContext(Window window, QXlibScreen *xd, const QPlatformWindowFormat &format);
    ~QGLXContext();

    virtual void makeCurrent();
    virtual void doneCurrent();
    virtual void swapBuffers();
    virtual void* getProcAddress(const QString& procName);

    GLXContext glxContext() const {return m_context;}

    QPlatformWindowFormat platformWindowFormat() const;

private:
    QXlibScreen  *m_screen;
    Drawable    m_drawable;
    GLXContext  m_context;
    QPlatformWindowFormat m_windowFormat;

    QGLXContext (QXlibScreen *screen, Drawable drawable, GLXContext context);
};

QT_END_NAMESPACE

#endif //!defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)

#endif
