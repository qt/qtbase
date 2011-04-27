/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QEGLCONTEXT_P_H
#define QEGLCONTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience of
// the QtOpenGL and QtOpenVG modules.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qsize.h>
#include <QtGui/qimage.h>

#include <QtGui/private/qegl_p.h>
#include <QtGui/private/qeglproperties_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QEglContext
{
public:
    QEglContext();
    ~QEglContext();

    bool isValid() const;
    bool isCurrent() const;
    bool isSharing() const { return sharing; }

    QEgl::API api() const { return apiType; }
    void setApi(QEgl::API api) { apiType = api; }

    bool chooseConfig(const QEglProperties& properties, QEgl::PixelFormatMatch match = QEgl::ExactPixelFormat);
    bool createContext(QEglContext *shareContext = 0, const QEglProperties *properties = 0);
    void destroyContext();
    EGLSurface createSurface(QPaintDevice *device, const QEglProperties *properties = 0);
    void destroySurface(EGLSurface surface);

    bool makeCurrent(EGLSurface surface);
    bool doneCurrent();
    bool lazyDoneCurrent();
    bool swapBuffers(EGLSurface surface);
    bool swapBuffersRegion2NOK(EGLSurface surface, const QRegion *region);

    int  configAttrib(int name) const;

    EGLContext context() const { return ctx; }
    void setContext(EGLContext context) { ctx = context; ownsContext = false;}

    EGLDisplay display() {return QEgl::display();}

    EGLConfig config() const { return cfg; }
    void setConfig(EGLConfig config) { cfg = config; }

private:
    QEgl::API apiType;
    EGLContext ctx;
    EGLConfig cfg;
    EGLSurface currentSurface;
    bool current;
    bool ownsContext;
    bool sharing;

    static QEglContext *currentContext(QEgl::API api);
    static void setCurrentContext(QEgl::API api, QEglContext *context);

    friend class QMeeGoGraphicsSystem;
    friend class QMeeGoPixmapData;
};

QT_END_NAMESPACE

#endif // QEGLCONTEXT_P_H
