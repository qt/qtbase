/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QBBSCREEN_H
#define QBBSCREEN_H

#include <QtGui/QPlatformScreen>

#include "qqnxrootwindow.h"

#include <QtCore/QByteArray>
#include <QtCore/QScopedPointer>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxWindow;

class QQnxScreen : public QPlatformScreen
{
public:
    static QList<QPlatformScreen *> screens() { return ms_screens; }
    static void createDisplays(screen_context_t context);
    static void destroyDisplays();
    static QQnxScreen *primaryDisplay() { return static_cast<QQnxScreen*>(ms_screens.at(0)); }
    static int defaultDepth();

    virtual QRect geometry() const { return m_currentGeometry; }
    virtual QRect availableGeometry() const;
    virtual int depth() const { return defaultDepth(); }
    virtual QImage::Format format() const { return (depth() == 32) ? QImage::Format_RGB32 : QImage::Format_RGB16; }
    virtual QSizeF physicalSize() const { return m_currentPhysicalSize; }

    bool isPrimaryScreen() const { return m_primaryScreen; }

    int rotation() const { return m_currentRotation; }
    void setRotation(int rotation);

    int nativeFormat() const { return (depth() == 32) ? SCREEN_FORMAT_RGBA8888 : SCREEN_FORMAT_RGB565; }
    screen_display_t nativeDisplay() const { return m_display; }
    screen_context_t nativeContext() const { return m_screenContext; }
    const char *windowGroupName() const { return m_rootWindow->groupName().constData(); }

    /* Window hierarchy management */
    static void addWindow(QQnxWindow *child);
    static void removeWindow(QQnxWindow *child);
    static void raiseWindow(QQnxWindow *window);
    static void lowerWindow(QQnxWindow *window);
    static void updateHierarchy();

    void onWindowPost(QQnxWindow *window);

    QSharedPointer<QQnxRootWindow> rootWindow() const { return m_rootWindow; }

private:
    QQnxScreen(screen_context_t context, screen_display_t display, bool primaryScreen);
    virtual ~QQnxScreen();

    static bool orthogonal(int rotation1, int rotation2);

    screen_context_t m_screenContext;
    screen_display_t m_display;
    QSharedPointer<QQnxRootWindow> m_rootWindow;
    bool m_primaryScreen;
    bool m_posted;
    bool m_usingOpenGL;

    int m_initialRotation;
    int m_currentRotation;
    QSize m_initialPhysicalSize;
    QSize m_currentPhysicalSize;
    QRect m_initialGeometry;
    QRect m_currentGeometry;
    QPlatformOpenGLContext *m_platformContext;

    static QList<QPlatformScreen *> ms_screens;
    static QList<QQnxWindow *> ms_childWindows;
};

QT_END_NAMESPACE

#endif // QBBSCREEN_H
