/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PLATFORMQUIRKS_H
#define PLATFORMQUIRKS_H

#include <qglobal.h>

#ifdef QT_GUI_LIB
#include <qapplication.h>
#endif

#ifdef Q_WS_X11
#include <private/qt_x11_p.h>
#endif

struct PlatformQuirks
{
    enum MediaFileTypes
    {
        mp3,
        wav,
        ogg
    };

    /* On some platforms, libpng or libjpeg sacrifice precision for speed.
       Esp. with NEON support, color values after decoding can be off by up
       to three bytes.
     */
    static inline bool isImageLoaderImprecise()
    {
#if defined(Q_WS_X11)
        // ### this is a very bad assumption, we should really check the version of libjpeg
        return X11->desktopEnvironment == DE_MEEGO_COMPOSITOR;
#else
        return false;
#endif
    }

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

    /* On some systems an ogg codec is not installed by default.
    The autotests have to know which fileType is the default on the system*/
    static inline MediaFileTypes defaultMediaFileType()
    {
#ifdef Q_WS_X11
        // ### very bad assumption
        if (X11->desktopEnvironment == DE_MEEGO_COMPOSITOR)
            return PlatformQuirks::mp3;
#endif
        return PlatformQuirks::ogg;
    }
};

#endif

