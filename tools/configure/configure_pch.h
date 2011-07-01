/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#if (defined(_WIN32) || defined(__NT__))
#  define QT_UNDEF_MACROS_IN_PCH
#  define _WINSCARD_H_
#  define _POSIX_         /* Make sure PATH_MAX et al. are defined    */
#  include <limits.h>
#  undef _POSIX_          /* Don't polute                             */

   /* Make sure IP v6 is defined first of all, before windows.h     */
#     include <winsock2.h>
#  include <stdlib.h>
#endif

#if defined __cplusplus
#include <qglobal.h>
#include <qlist.h>
#include <qvariant.h>  // All moc genereated code has this include
#include <qplatformdefs.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextcodec.h>

#include <limits.h>
#include <stdlib.h>
#endif

#if defined(QT_UNDEF_MACROS_IN_PCH)
#  undef max /*  These are defined in windef.h, but                   */
#  undef min /*  we don't want them when building Qt                  */
#  undef _WINSCARD_H_
#endif
