/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
#ifdef Q_WS_MAEMO_5
        return true;
#elif defined(Q_WS_X11)
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
#ifdef Q_WS_MAEMO_5
        return true;
#elif defined(Q_WS_X11)
        return X11->desktopEnvironment == DE_MEEGO_COMPOSITOR;
#else
        return false;
#endif
    }

    static inline bool haveMouseCursor()
    {
#ifdef Q_WS_MAEMO_5
        return false;
#elif defined(Q_WS_X11)
        return X11->desktopEnvironment != DE_MEEGO_COMPOSITOR;
#else
        return true;
#endif
    }

    /* On some systems an ogg codec is not installed by default.
    The autotests have to know which fileType is the default on the system*/
    static inline MediaFileTypes defaultMediaFileType()
    {
#ifdef Q_WS_MAEMO_5
        return PlatformQuirks::mp3;
#endif
#ifdef Q_WS_X11
        // ### very bad assumption
        if (X11->desktopEnvironment == DE_MEEGO_COMPOSITOR)
            return PlatformQuirks::mp3;
#endif
        return PlatformQuirks::ogg;
    }
};

#endif

