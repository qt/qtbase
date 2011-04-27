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

#include <private/qgraphicssystemplugin_p.h>
#include <private/qgraphicssystem_gl_p.h>
#include <qgl.h>

QT_BEGIN_NAMESPACE

class QGLGraphicsSystemPlugin : public QGraphicsSystemPlugin
{
public:
    QStringList keys() const;
    QGraphicsSystem *create(const QString&);
};

QStringList QGLGraphicsSystemPlugin::keys() const
{
    QStringList list;
    list << QLatin1String("OpenGL") << QLatin1String("OpenGL1");
#if !defined(QT_OPENGL_ES_1)
    list << QLatin1String("OpenGL2");
#endif
#if defined(Q_WS_X11) && !defined(QT_NO_EGL)
    list << QLatin1String("X11GL");
#endif
    return list;
}

QGraphicsSystem* QGLGraphicsSystemPlugin::create(const QString& system)
{
    if (system.toLower() == QLatin1String("opengl1")) {
        QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
        return new QGLGraphicsSystem(false);
    }

#if !defined(QT_OPENGL_ES_1)
    if (system.toLower() == QLatin1String("opengl2")) {
        QGL::setPreferredPaintEngine(QPaintEngine::OpenGL2);
        return new QGLGraphicsSystem(false);
    }
#endif

#if defined(Q_WS_X11) && !defined(QT_NO_EGL)
    if (system.toLower() == QLatin1String("x11gl"))
        return new QGLGraphicsSystem(true);
#endif

    if (system.toLower() == QLatin1String("opengl"))
        return new QGLGraphicsSystem(false);

    return 0;
}

Q_EXPORT_PLUGIN2(opengl, QGLGraphicsSystemPlugin)

QT_END_NAMESPACE
