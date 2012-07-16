/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef PLATFORMQUIRKS_H
#define PLATFORMQUIRKS_H

#include <qglobal.h>

#ifdef QT_GUI_LIB
#include <qapplication.h>
#endif


struct PlatformQuirks
{
    /* Some windowing systems automatically maximize apps on startup (e.g. Maemo)
       "Normal" fixed-sized windows do not work, the WM ignores their size settings.
    */
    static inline bool isAutoMaximizing()
    {
#if defined(Q_WS_X11)
        return X11->desktopEnvironment == DE_MEEGO_COMPOSITOR;
#else
        return false;
#endif
    }

    static inline bool haveMouseCursor()
    {
#if defined(Q_WS_X11)
        return X11->desktopEnvironment != DE_MEEGO_COMPOSITOR;
#else
        return true;
#endif
    }
};

#endif

