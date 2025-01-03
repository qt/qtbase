// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIXMAP_WIN_P_H
#define QPIXMAP_WIN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class QBitmap;
class QImage;
class QPixmap;

Q_GUI_EXPORT HBITMAP qt_createIconMask(const QBitmap &bitmap);
Q_GUI_EXPORT HBITMAP qt_imageToWinHBITMAP(const QImage &imageIn, int hbitmapFormat = 0);
Q_GUI_EXPORT HBITMAP qt_pixmapToWinHBITMAP(const QPixmap &p, int hbitmapFormat = 0);
Q_GUI_EXPORT QImage qt_imageFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &p);
Q_GUI_EXPORT QImage qt_imageFromWinHBITMAP(HDC hdc, HBITMAP bitmap, int w, int h);
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);

QT_END_NAMESPACE

#endif // QPIXMAP_WIN_P_H
