/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOPENGL_H
#define QOPENGL_H

#ifndef QT_NO_OPENGL

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

#if defined(QT_OPENGL_ES_2)
# if defined(Q_OS_MAC)
#  include <OpenGLES/ES2/gl.h>
# else
#  include <GLES2/gl2.h>
# endif

/*
   Some GLES2 implementations (like the one on Harmattan) are missing the
   typedef for GLchar. Work around it here by adding it. The Kkronos headers
   specify GLChar as a typedef to char, so if an implementation already
   provides it, then this doesn't do any harm.
*/
typedef char GLchar;

# include <QtGui/qopengles2ext.h>
# ifndef GL_DOUBLE
#  define GL_DOUBLE GL_FLOAT
# endif
# ifndef GLdouble
typedef GLfloat GLdouble;
# endif
#else
// Mac OSX is a "controlled platform" for OpenGL ABI so we use
// the system provided headers there. Controlled means that the
// headers always match the actual driver implementation so there
// is no possibility of drivers exposing additional functionality
// from the system headers. Also it means that the vendor can
// (and does) make different choices about some OpenGL types. For
// e.g. Apple uses void* for GLhandleARB whereas other platforms
// use unsigned int.
//
// For the "uncontrolled" Windows and Linux platforms we use the
// official Khronos glext.h header. On these platforms this gives
// access to additional functionality the drivers may expose but
// which the system headers do not.
# if defined(Q_OS_MAC)
#  include <OpenGL/gl.h>
#  if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
#   define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#   include <OpenGL/gl3.h>
#  endif
#  include <OpenGL/glext.h>
# else
#  if defined(Q_OS_WIN)
#   include <QtCore/qt_windows.h>
#  endif
#  include <GL/gl.h>
#  include <QtGui/qopenglext.h>
# endif // Q_OS_MAC
#endif

// Desktops, apart from Mac OS X prior to 10.7 can support OpenGL 3
#if !defined(QT_OPENGL_ES_2)
# if !defined(Q_OS_MAC) || (defined(Q_OS_MAC) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)
#  define QT_OPENGL_3
# endif
#endif

QT_BEGIN_NAMESPACE


QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_OPENGL

#endif // QOPENGL_H
