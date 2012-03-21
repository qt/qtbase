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

#include "qqnxscreen.h"
#include "qqnxvirtualkeyboard.h"
#include "qqnxwindow.h"

#include <QtCore/QDebug>
#include <QtCore/QUuid>

#include <errno.h>

QT_BEGIN_NAMESPACE

QList<QPlatformScreen *> QQnxScreen::ms_screens;
QList<QQnxWindow*> QQnxScreen::ms_childWindows;

QQnxScreen::QQnxScreen(screen_context_t screenContext, screen_display_t display, bool primaryScreen)
    : m_screenContext(screenContext),
      m_display(display),
      m_rootWindow(),
      m_primaryScreen(primaryScreen),
      m_posted(false),
      m_platformContext(0)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Cache initial orientation of this display
    // TODO: use ORIENTATION environment variable?
    errno = 0;
    int result = screen_get_display_property_iv(m_display, SCREEN_PROPERTY_ROTATION, &m_initialRotation);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query display rotation, errno=%d", errno);
    }
    m_currentRotation = m_initialRotation;

    // Cache size of this display in pixels
    // TODO: use WIDTH and HEIGHT environment variables?
    errno = 0;
    int val[2];
    result = screen_get_display_property_iv(m_display, SCREEN_PROPERTY_SIZE, val);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query display size, errno=%d", errno);
    }

    m_currentGeometry = m_initialGeometry = QRect(0, 0, val[0], val[1]);

    // Cache size of this display in millimeters
    errno = 0;
    result = screen_get_display_property_iv(m_display, SCREEN_PROPERTY_PHYSICAL_SIZE, val);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query display physical size, errno=%d", errno);
    }

    // Peg the DPI to 96 (for now) so fonts are a reasonable size. We'll want to match
    // everything with a QStyle later, and at that point the physical size can be used
    // instead.
    {
        static const int dpi = 96;
        int width = m_currentGeometry.width() / dpi * qreal(25.4) ;
        int height = m_currentGeometry.height() / dpi * qreal(25.4) ;

        m_currentPhysicalSize = m_initialPhysicalSize = QSize(width,height);
    }

    // We only create the root window if we are the primary display.
    if (primaryScreen)
        m_rootWindow = QSharedPointer<QQnxRootWindow>(new QQnxRootWindow(this));
}

QQnxScreen::~QQnxScreen()
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
}

/* static */
void QQnxScreen::createDisplays(screen_context_t context)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // Query number of displays
    errno = 0;
    int displayCount;
    int result = screen_get_context_property_iv(context, SCREEN_PROPERTY_DISPLAY_COUNT, &displayCount);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query display count, errno=%d", errno);
    }

    // Get all displays
    errno = 0;
    screen_display_t *displays = (screen_display_t *)alloca(sizeof(screen_display_t) * displayCount);
    result = screen_get_context_property_pv(context, SCREEN_PROPERTY_DISPLAYS, (void **)displays);
    if (result != 0) {
        qFatal("QQnxScreen: failed to query displays, errno=%d", errno);
    }

    for (int i=0; i<displayCount; i++) {
#if defined(QQNXSCREEN_DEBUG)
        qDebug() << "QQnxScreen::Creating screen for display " << i;
#endif
        QQnxScreen *screen = new QQnxScreen(context, displays[i], i==0);
        ms_screens.append(screen);
    }
}

/* static */
void QQnxScreen::destroyDisplays()
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    qDeleteAll(ms_screens);
    ms_screens.clear();

    // We're not managing the child windows anymore so we need to clear the list.
    ms_childWindows.clear();
}

/* static */
int QQnxScreen::defaultDepth()
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    static int defaultDepth = 0;
    if (defaultDepth == 0) {
        // check if display depth was specified in environment variable;
        // use default value if no valid value found
        defaultDepth = qgetenv("QQNX_DISPLAY_DEPTH").toInt();
        if (defaultDepth != 16 && defaultDepth != 32) {
            defaultDepth = 32;
        }
    }
    return defaultDepth;
}

QRect QQnxScreen::availableGeometry() const
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    // available geometry = total geometry - keyboard
    int keyboardHeight = QQnxVirtualKeyboard::instance().height();
    return QRect(m_currentGeometry.x(), m_currentGeometry.y(),
                 m_currentGeometry.width(), m_currentGeometry.height() - keyboardHeight);
}

/*!
    Check if the supplied angles are perpendicular to each other.
*/
static bool isOrthogonal(int angle1, int angle2)
{
    return ((angle1 - angle2) % 180) != 0;
}

void QQnxScreen::setRotation(int rotation)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "orientation =" << rotation;
#endif
    // Check if rotation changed
    if (m_currentRotation != rotation) {
        // Update rotation of root window
        if (m_rootWindow)
            m_rootWindow->setRotation(rotation);

        // Swap dimensions if we've rotated 90 or 270 from initial orientation
        if (isOrthogonal(m_initialRotation, rotation)) {
            m_currentGeometry = QRect(0, 0, m_initialGeometry.height(), m_initialGeometry.width());
            m_currentPhysicalSize = QSize(m_initialPhysicalSize.height(), m_initialPhysicalSize.width());
        } else {
            m_currentGeometry = QRect(0, 0, m_initialGeometry.width(), m_initialGeometry.height());
            m_currentPhysicalSize = m_initialPhysicalSize;
        }

        // Resize root window if we've rotated 90 or 270 from previous orientation
        if (isOrthogonal(m_currentRotation, rotation)) {
#if defined(QQNXSCREEN_DEBUG)
            qDebug() << Q_FUNC_INFO << "resize, size =" << m_currentGeometry.size();
#endif
            if (m_rootWindow)
                m_rootWindow->resize(m_currentGeometry.size());
        } else {
            // TODO: Find one global place to flush display updates
            // Force immediate display update if no geometry changes required
            if (m_rootWindow)
                m_rootWindow->flush();
        }

        // Save new rotation
        m_currentRotation = rotation;
    }
}

void QQnxScreen::addWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    if (ms_childWindows.contains(window))
        return;

    ms_childWindows.push_back(window);
    QQnxScreen::updateHierarchy();
}

void QQnxScreen::removeWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    ms_childWindows.removeAll(window);
    QQnxScreen::updateHierarchy();
}

void QQnxScreen::raiseWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    removeWindow(window);
    ms_childWindows.push_back(window);
    QQnxScreen::updateHierarchy();
}

void QQnxScreen::lowerWindow(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO << "window =" << window;
#endif

    removeWindow(window);
    ms_childWindows.push_front(window);
    QQnxScreen::updateHierarchy();
}

void QQnxScreen::updateHierarchy()
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif

    QList<QQnxWindow*>::iterator it;
    int topZorder = 1; // root window is z-order 0, all "top" level windows are "above" it

    for (it = ms_childWindows.begin(); it != ms_childWindows.end(); it++)
        (*it)->updateZorder(topZorder);

    // After a hierarchy update, we need to force a flush on all screens.
    // Right now, all screens share a context.
    screen_flush_context( primaryDisplay()->m_screenContext, 0 );
}

void QQnxScreen::onWindowPost(QQnxWindow *window)
{
#if defined(QQNXSCREEN_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    Q_UNUSED(window)

    // post app window (so navigator will show it) after first child window
    // has posted; this only needs to happen once as the app window's content
    // never changes
    if (!m_posted && m_rootWindow) {
        m_rootWindow->post();
        m_posted = true;
    }
}

QT_END_NAMESPACE
