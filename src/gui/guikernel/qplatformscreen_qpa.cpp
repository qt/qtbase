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

#include "qplatformscreen_qpa.h"
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qplatformintegration_qpa.h>
#include <QtGui/qwindow.h>

/*!
    Return the given top level window for a given position.

    Default implementation retrieves a list of all top level windows and finds the first window
    which contains point \a pos
*/
QWindow *QPlatformScreen::topLevelAt(const QPoint & pos) const
{
    QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = list.size()-1; i >= 0; --i) {
        QWindow *w = list[i];
        if (w->visible() && w->geometry().contains(pos))
            return w;
    }

    return 0;
}

/*! \fn physicalSize() const
    Reimplement in subclass to return the physical size of the screen. This function is used by
    QFont to convert point sizes to pixel sizes.

    Default implementation takes the pixel size of the screen, considers a dpi of 100 and returns
    the calculated (and probably wrong) physical size
*/
QSize QPlatformScreen::physicalSize() const
{
    static const int dpi = 100;
    int width = geometry().width() / dpi * qreal(25.4) ;
    int height = geometry().height() / dpi * qreal(25.4) ;
    return QSize(width,height);
}

QPlatformScreen * QPlatformScreen::platformScreenForWindow(const QWindow *)
{
    return QGuiApplicationPrivate::platformIntegration()->screens().at(0);
}

/*!
    \class QPlatformScreen
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformScreen class provides an abstraction for visual displays.

    Many window systems has support for retrieving information on the attached displays. To be able
    to query the display QPA uses QPlatformScreen. Qt its self is most dependent on the
    physicalSize() function, since this is the function it uses to calculate the dpi to use when
    converting point sizes to pixels sizes. However, this is unfortunate on some systems, as the
    native system fakes its dpi size.

    QPlatformScreen is also used by the public api QDesktopWidget for information about the desktop.
 */

/*! \fn geometry() const
    Reimplement in subclass to return the pixel geometry of the screen
*/

/*! \fn availableGeometry() const
    Reimplement in subclass to return the pixel geometry of the available space
    This normally is the desktop screen minus the task manager, global menubar etc.
*/

/*! \fn depth() const
    Reimplement in subclass to return current depth of the screen
*/

/*! \fn format() const
    Reimplement in subclass to return the image format which corresponds to the screen format
*/

