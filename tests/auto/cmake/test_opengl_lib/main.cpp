/****************************************************************************
**
** Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <qglobal.h>
#ifndef QT_OPENGL_DYNAMIC
#  if defined(GL_IMPLEMENTATION_GLES2)
#    include <GLES2/gl2.h>
#  elif defined(GL_IMPLEMENTATION_GL)
#    ifdef Q_OS_WIN
#      include <qt_windows.h>
#    endif
#    ifdef Q_OS_MAC
#      include <OpenGL/gl.h>
#    else
#      include <GL/gl.h>
#    endif
#  endif
#else
#  include <QOpenGLFunctions>
#endif

int main(int argc, char **argv)
{
#ifndef QT_OPENGL_DYNAMIC
    glGetError();
#else
    QOpenGLFunctions functions;
    functions.glGetError();
#endif
    return 0;
}
