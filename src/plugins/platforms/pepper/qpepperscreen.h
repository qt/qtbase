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

    QRect geometry() const Q_DECL_OVERRIDE;
    int depth() const Q_DECL_OVERRIDE;
    QImage::Format format() const Q_DECL_OVERRIDE;
    qreal devicePixelRatio() const Q_DECL_OVERRIDE;
    QPlatformCursor *cursor() const Q_DECL_OVERRIDE;

    void resizeMaximizedWindows();

private:
    QScopedPointer<QPlatformCursor> m_cursor;
};

QT_END_NAMESPACE

#endif
