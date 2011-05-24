/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#ifndef QSCREENEGL_P_H
#define QSCREENEGL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QScreenEGL class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/QScreen>
#include <QtOpenGL/qgl.h>
#if defined(QT_OPENGL_ES_2)
#include <EGL/egl.h>
#else
#include <GLES/egl.h>
#endif
#if !defined(EGL_VERSION_1_3) && !defined(QEGL_NATIVE_TYPES_DEFINED)
#undef EGLNativeWindowType
#undef EGLNativePixmapType
#undef EGLNativeDisplayType
typedef NativeWindowType EGLNativeWindowType;
typedef NativePixmapType EGLNativePixmapType;
typedef NativeDisplayType EGLNativeDisplayType;
#define QEGL_NATIVE_TYPES_DEFINED 1
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(OpenGL)

class QGLScreenPrivate;

class Q_OPENGL_EXPORT QGLScreenSurfaceFunctions
{
public:
    virtual bool createNativeWindow(QWidget *widget, EGLNativeWindowType *native);
    virtual bool createNativePixmap(QPixmap *pixmap, EGLNativePixmapType *native);
    virtual bool createNativeImage(QImage *image, EGLNativePixmapType *native);
};

class Q_OPENGL_EXPORT QGLScreen : public QScreen
{
    Q_DECLARE_PRIVATE(QGLScreen)
public:
    QGLScreen(int displayId);
    virtual ~QGLScreen();

    enum Option
    {
        NoOptions       = 0,
        NativeWindows   = 1,
        NativePixmaps   = 2,
        NativeImages    = 4,
        Overlays        = 8
    };
    Q_DECLARE_FLAGS(Options, Option)

    QGLScreen::Options options() const;

    virtual bool chooseContext(QGLContext *context, const QGLContext *shareContext);
    virtual bool hasOpenGL() = 0;

    QGLScreenSurfaceFunctions *surfaceFunctions() const;

protected:
    void setOptions(QGLScreen::Options value);
    void setSurfaceFunctions(QGLScreenSurfaceFunctions *functions);

private:
    QGLScreenPrivate *d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGLScreen::Options)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSCREENEGL_P_H
