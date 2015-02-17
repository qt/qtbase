/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef QPEPPERSCREEN_H
#define QPEPPERSCREEN_H

#include <QtCore/QScopedPointer>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class QPlatformCursor;
class QPepperScreen : public QPlatformScreen
{
public:
    QPepperScreen();
    QRect geometry() const;
    int depth() const { return m_depth; }
    QImage::Format format() const { return m_format; }
    qreal devicePixelRatio() const;
    QPlatformCursor *cursor() const;
    void resizeMaximizedWindows();

public:
    int m_depth;
    QImage::Format m_format;
    QScopedPointer<QPlatformCursor> m_cursor;
};

QT_END_NAMESPACE

#endif
