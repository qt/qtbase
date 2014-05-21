/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHIGHDPISCALING_P_H
#define QHIGHDPISCALING_P_H

#include <QtCore/qglobal.h>
#include <QtCore/qmargins.h>
#include <QtCore/qrect.h>
#include <QtCore/qvector.h>
#include <QtGui/qregion.h>

// This file implmements utility functions for high-dpi scaling on operating
// systems that do not provide native scaling support.
//
// The functions support creating a logical device-independent
// coordinate system which is related to the device pixel coordinate
// through a scaling factor. The scaling factor is set by the
// QT_HIGHDPI_SCALE_FACTOR environment variable.
//
// With these functions in use  most of the Qt API will then operate in
// the device-independent coordinate system. For example, setting
// the scale factor to 2.0 will make Qt see half of the "device"
// window geometry. Desktop and event geometry will be scaled
// to match.
//
// Integer scaling factors work best. Glitch-free graphics at non-integer
// scaling factors can not be guaranteed.

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QHighDpiScaling {
public:
    static bool isActive() { return m_active; }
    static qreal factor()  { return m_factor; }
    static void setFactor(qreal factor);

private:
    static qreal m_factor;
    static bool m_active;
};

// Coordinate system conversion functions:
// qHighDpiToDeviceIndependentPixels   : from physical(screen/backing) to logical pixels
// qHighDpiToDevicePixels              : from logical to physical pixels
inline QRect qHighDpiToDeviceIndependentPixels(const QRect &pixelRect)
{
    const qreal scaleFactor = QHighDpiScaling::factor();
    return QRect(pixelRect.topLeft() / scaleFactor, pixelRect.size() / scaleFactor);
}

inline QRect qHighDpiToDevicePixels(const QRect &pointRect)
{
    const qreal scaleFactor = QHighDpiScaling::factor();
    return QRect(pointRect.topLeft() * scaleFactor, pointRect.size() * scaleFactor);
}

inline QRectF qHighDpiToDeviceIndependentPixels(const QRectF &pixelRect)
{
    const qreal scaleFactor = QHighDpiScaling::factor();
    return QRectF(pixelRect.topLeft() / scaleFactor, pixelRect.size() / scaleFactor);
}

inline QRectF qHighDpiToDevicePixels(const QRectF &pointRect)
{
   const qreal scaleFactor = QHighDpiScaling::factor();
   return QRectF(pointRect.topLeft() * scaleFactor, pointRect.size() * scaleFactor);
}

inline QSize qHighDpiToDeviceIndependentPixels(const QSize &pixelSize)
{
    return pixelSize / QHighDpiScaling::factor();
}

// For converting minimum/maximum sizes of QWindow, limits to 0..QWINDOWSIZE_MAX
Q_GUI_EXPORT QSize qHighDpiToDevicePixelsConstrained(const QSize &size);

inline QSize qHighDpiToDevicePixels(const QSize &pointSize)
{
    return pointSize * QHighDpiScaling::factor();
}

inline QSizeF qHighDpiToDeviceIndependentPixels(const QSizeF &pixelSize)
{
    return pixelSize / QHighDpiScaling::factor();
}

inline QSizeF qHighDpiToDevicePixels(const QSizeF &pointSize)
{
    return pointSize * QHighDpiScaling::factor();
}

inline QPoint qHighDpiToDeviceIndependentPixels(const QPoint &pixelPoint)
{
    return pixelPoint / QHighDpiScaling::factor();
}

inline QPoint qHighDpiToDevicePixels(const QPoint &pointPoint)
{
    return pointPoint * QHighDpiScaling::factor();
}

inline QPointF qHighDpiToDeviceIndependentPixels(const QPointF &pixelPoint)
{
    return pixelPoint / QHighDpiScaling::factor();
}

inline QPointF qHighDpiToDevicePixels(const QPointF &pointPoint)
{
    return pointPoint * QHighDpiScaling::factor();
}

inline QMargins qHighDpiToDeviceIndependentPixels(const QMargins &pixelMargins)
{
    const qreal scaleFactor = QHighDpiScaling::factor();
    return QMargins(pixelMargins.left() / scaleFactor, pixelMargins.top() / scaleFactor,
                    pixelMargins.right() / scaleFactor, pixelMargins.bottom() / scaleFactor);
}

inline QMargins qHighDpiToDevicePixels(const QMargins &pointMargins)
{
    const qreal scaleFactor = QHighDpiScaling::factor();
    return QMargins(pointMargins.left() * scaleFactor, pointMargins.top() * scaleFactor,
                    pointMargins.right() * scaleFactor, pointMargins.bottom() * scaleFactor);
}

inline QRegion qHighDpiToDeviceIndependentPixels(const QRegion &pixelRegion)
{
    if (!QHighDpiScaling::isActive())
        return pixelRegion;

    QRegion pointRegion;
    foreach (const QRect &rect, pixelRegion.rects())
        pointRegion += qHighDpiToDeviceIndependentPixels(rect);
    return pointRegion;
}

inline QRegion qHighDpiToDevicePixels(const QRegion &pointRegion)
{
    if (!QHighDpiScaling::isActive())
        return pointRegion;

    QRegion pixelRegon;
    foreach (const QRect &rect, pointRegion.rects())
        pixelRegon += qHighDpiToDevicePixels(rect);
    return pixelRegon;
}

// Any T that has operator/()
template <typename T>
T qHighDpiToDeviceIndependentPixels(const T &pixelValue)
{
    if (!QHighDpiScaling::isActive())
        return pixelValue;

    return pixelValue / QHighDpiScaling::factor();

}

// Any T that has operator*()
template <typename T>
T qHighDpiToDevicePixels(const T &pointValue)
{
    if (!QHighDpiScaling::isActive())
        return pointValue;

    return pointValue * QHighDpiScaling::factor();
}

// Any QVector<T> where T has operator/()
template <typename T>
QVector<T> qHighDpiToDeviceIndependentPixels(const QVector<T> &pixelValues)
{
    if (!QHighDpiScaling::isActive())
        return pixelValues;

    QVector<T> pointValues;
    foreach (const T& pixelValue, pixelValues)
        pointValues.append(pixelValue / QHighDpiScaling::factor());
    return pointValues;
}

// Any QVector<T> where T has operator*()
template <typename T>
QVector<T> qHighDpiToDevicePixels(const QVector<T> &pointValues)
{
    if (!QHighDpiScaling::isActive())
        return pointValues;

    QVector<T> pixelValues;
    foreach (const T& pointValue, pointValues)
        pixelValues.append(pointValue * QHighDpiScaling::factor());
    return pixelValues;
}


// Any QPair<T, U> where T and U has operator/()
template <typename T, typename U>
QPair<T, U> qHighDpiToDeviceIndependentPixels(const QPair<T, U> &pixelPair)
{
    return qMakePair(qHighDpiToDeviceIndependentPixels(pixelPair.first), qHighDpiToDeviceIndependentPixels(pixelPair.second));
}

// Any QPair<T, U> where T and U has operator*()
template <typename T, typename U>
QPair<T, U> qHighDpiToDevicePixels(const QPair<T, U> &pointPair)
{
    return qMakePair(qHighDpiToDevicePixels(pointPair.first), qHighDpiToDevicePixels(pointPair.second));
}

QT_END_NAMESPACE

#endif
