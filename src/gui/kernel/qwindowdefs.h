/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QWINDOWDEFS_H
#define QWINDOWDEFS_H

#include <QtCore/qobjectdefs.h>
#include <QtCore/qnamespace.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE


// Class forward definitions

class QPaintDevice;
class QWidget;
class QWindow;
class QDialog;
class QColor;
class QPalette;
class QCursor;
class QPoint;
class QSize;
class QRect;
class QPolygon;
class QPainter;
class QRegion;
class QFont;
class QFontMetrics;
class QFontInfo;
class QPen;
class QBrush;
class QMatrix;
class QPixmap;
class QBitmap;
class QMovie;
class QImage;
class QPicture;
class QTimer;
class QTime;
class QClipboard;
class QString;
class QByteArray;
class QApplication;

template<typename T> class QList;
typedef QList<QWidget *> QWidgetList;
typedef QList<QWindow *> QWindowList;

QT_END_NAMESPACE
QT_END_HEADER

// Window system dependent definitions


#if defined(Q_OS_WIN)
#  include <QtGui/qwindowdefs_win.h>
#endif // Q_OS_WIN




typedef unsigned long  WId;



QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

template<class K, class V> class QHash;
typedef QHash<WId, QWidget *> QWidgetMapper;

template<class V> class QSet;
typedef QSet<QWidget *> QWidgetSet;

QT_END_NAMESPACE
QT_END_HEADER

#if defined(QT_NEEDS_QMAIN)
#define main qMain
#endif

// Global platform-independent types and functions

#endif // QWINDOWDEFS_H
