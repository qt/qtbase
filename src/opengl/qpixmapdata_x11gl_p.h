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

#ifndef QPIXMAPDATA_X11GL_P_H
#define QPIXMAPDATA_X11GL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qpixmapdata_p.h>
#include <private/qpixmap_x11_p.h>
#include <private/qglpaintdevice_p.h>

#include <qgl.h>

#ifndef QT_NO_EGL
#include <QtGui/private/qeglcontext_p.h>
#endif

QT_BEGIN_NAMESPACE

class QX11GLSharedContexts;

class QX11GLPixmapData : public QX11PixmapData, public QGLPaintDevice
{
public:
    QX11GLPixmapData();
    virtual ~QX11GLPixmapData();

    // Re-implemented from QX11PixmapData:
    void fill(const QColor &color);
    void copy(const QPixmapData *data, const QRect &rect);
    bool scroll(int dx, int dy, const QRect &rect);

    // Re-implemented from QGLPaintDevice
    QPaintEngine* paintEngine() const; // Also re-implements QX11PixmapData::paintEngine
    void beginPaint();
    QGLContext* context() const;
    QSize size() const;

    static bool hasX11GLPixmaps();
    static QGLFormat glFormat();
    static QX11GLSharedContexts* sharedContexts();

private:
    mutable QGLContext* ctx;
};


QT_END_NAMESPACE

#endif // QPIXMAPDATA_X11GL_P_H
