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

#include <qpa/qplatformscreen.h>

#include "qqnxrootwindow.h"

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxWindow;

class QQnxScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:
    QQnxScreen(screen_context_t context, screen_display_t display, bool primaryScreen);
    ~QQnxScreen();

    QRect geometry() const { return m_currentGeometry; }
    QRect availableGeometry() const;
    int depth() const;
    QImage::Format format() const { return (depth() == 32) ? QImage::Format_RGB32 : QImage::Format_RGB16; }
    QSizeF physicalSize() const { return m_currentPhysicalSize; }

    qreal refreshRate() const;

    Qt::ScreenOrientation nativeOrientation() const;
    Qt::ScreenOrientation orientation() const;

    bool isPrimaryScreen() const { return m_primaryScreen; }

    int rotation() const { return m_currentRotation; }

    int nativeFormat() const { return (depth() == 32) ? SCREEN_FORMAT_RGBA8888 : SCREEN_FORMAT_RGB565; }
    screen_display_t nativeDisplay() const { return m_display; }
    screen_context_t nativeContext() const { return m_screenContext; }
    const char *windowGroupName() const { return m_rootWindow->groupName().constData(); }

    QQnxWindow *findWindow(screen_window_t windowHandle);

    /* Window hierarchy management */
    void addWindow(QQnxWindow *child);
    void removeWindow(QQnxWindow *child);
    void raiseWindow(QQnxWindow *window);
    void lowerWindow(QQnxWindow *window);
    void updateHierarchy();

    void onWindowPost(QQnxWindow *window);

    QSharedPointer<QQnxRootWindow> rootWindow() const { return m_rootWindow; }

public Q_SLOTS:
    void setRotation(int rotation);
    void newWindowCreated(void *window);
    void windowClosed(void *window);
    void activateWindowGroup(const QByteArray &id);
    void deactivateWindowGroup(const QByteArray &id);

private Q_SLOTS:
    void keyboardHeightChanged(int height);

private:
    void addOverlayWindow(screen_window_t window);
    void removeOverlayWindow(screen_window_t window);

    screen_context_t m_screenContext;
    screen_display_t m_display;
    QSharedPointer<QQnxRootWindow> m_rootWindow;
    bool m_primaryScreen;
    bool m_posted;

    int m_initialRotation;
    int m_currentRotation;
    int m_keyboardHeight;
    QSize m_initialPhysicalSize;
    QSize m_currentPhysicalSize;
    Qt::ScreenOrientation m_nativeOrientation;
    QRect m_initialGeometry;
    QRect m_currentGeometry;
    QPlatformOpenGLContext *m_platformContext;

    QList<QQnxWindow *> m_childWindows;
    QList<screen_window_t> m_overlays;
};

QT_END_NAMESPACE

#endif // QBBSCREEN_H
