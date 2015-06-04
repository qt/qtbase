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
#include <QtCore/qloggingcategory.h>
#include <QtGui/qregion.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcScaling);

class QScreen;
class QPlatformScreen;

class Q_GUI_EXPORT QHighDpiScaling {
public:
    static void initHighDPiScaling();
    static void setGlobalFactor(qreal factor);
    static void setScreenFactor(QScreen *window, qreal factor);

    static bool isActive() { return m_active; }
    static qreal factor(const QWindow *window);
    static qreal factor(const QScreen *screen);
    static qreal factor(const QPlatformScreen *platformScreen);
    static QPoint origin(const QScreen *screen);
    static QPoint origin(const QPlatformScreen *platformScreen);
    static QPoint mapPositionFromNative(const QPoint &pos, const QPlatformScreen *platformScreen);
    static QPoint mapPositionToNative(const QPoint &pos, const QPlatformScreen *platformScreen);
private:
    static qreal screenSubfactor(const QPlatformScreen *screen);

    static qreal m_factor;
    static bool m_active;
    static bool m_perScreenActive;
    static bool m_usePixelDensity;
};

// Coordinate system conversion functions:
// QHighDpi::fromNativePixels   : from physical(screen/backing) to logical pixels
// QHighDpi::toNativePixels              : from logical to physical pixels

namespace QHighDpi {

// inline QRect fromNativeGeometry(const QRect &pixelRect, const QPlatformScreen *platformScreen)
// {
//     return QRect(pixelRect.topLeft(), pixelRect.size() / QHighDpiScaling::factor(platformScreen));
// }

// inline QRect toNativeGeometry(const QRect &pointRect, const QPlatformScreen *platformScreen)
// {
//     return QRect(pointRect.topLeft(), pointRect.size() * QHighDpiScaling::factor(platformScreen));
// }

inline QPoint fromNative(const QPoint &pos, qreal scaleFactor, const QPoint &origin)
{
     return (pos - origin) / scaleFactor + origin;
}

inline QPoint toNative(const QPoint &pos, qreal scaleFactor, const QPoint &origin)
{
     return (pos - origin) * scaleFactor + origin;
}

inline QPoint fromNative(const QPoint &pos, qreal scaleFactor)
{
     return pos / scaleFactor;
}

inline QPoint toNative(const QPoint &pos, qreal scaleFactor)
{
    return pos * scaleFactor;
}

inline QSize fromNative(const QSize &size, qreal scaleFactor)
{
    return size / scaleFactor; // TODO: ### round up ###
}

inline QSize toNative(const QSize &size, qreal scaleFactor)
{
    return size * scaleFactor;
}

inline QRect fromNative(const QRect &rect, qreal scaleFactor, const QPoint &origin)
{
    return QRect(fromNative(rect.topLeft(), scaleFactor, origin), fromNative(rect.size(), scaleFactor));
}

inline QRect toNative(const QRect &rect, qreal scaleFactor, const QPoint &origin)
{
    return QRect(toNative(rect.topLeft(), scaleFactor, origin), toNative(rect.size(), scaleFactor));

}

inline QRect fromNative(const QRect &rect, const QScreen *screen, const QPoint &screenOrigin)
{
    return toNative(rect, QHighDpiScaling::factor(screen), screenOrigin);
}

inline QRect fromNativeScreenGeometry(const QRect &nativeScreenGeometry, const QScreen *screen)
{
    return QRect(nativeScreenGeometry.topLeft(), fromNative(nativeScreenGeometry.size(), QHighDpiScaling::factor(screen)));
}

inline QPoint fromNativeLocalPosition(const QPoint &pos, const QWindow *window)
{
    const qreal scaleFactor = QHighDpiScaling::factor(window);
    return pos / scaleFactor;
}

inline QPoint toNativeLocalPosition(const QPoint &pos, const QWindow *window)
{
    const qreal scaleFactor = QHighDpiScaling::factor(window);
    return pos * scaleFactor;
}

inline QPointF fromNativeLocalPosition(const QPointF &pos, const QWindow *window)
{
    const qreal scaleFactor = QHighDpiScaling::factor(window);
    return pos / scaleFactor;
}

inline QPointF toNativeLocalPosition(const QPointF &pos, const QWindow *window)
{
    const qreal scaleFactor = QHighDpiScaling::factor(window);
    return pos * scaleFactor;
}

inline QRect fromNativePixels(const QRect &pixelRect, const QPlatformScreen *platformScreen)
{
    const qreal scaleFactor = QHighDpiScaling::factor(platformScreen);
    const QPoint origin = QHighDpiScaling::origin(platformScreen);
    return QRect(fromNative(pixelRect.topLeft(), scaleFactor, origin),
                 fromNative(pixelRect.size(), scaleFactor));
}

inline QRect toNativePixels(const QRect &pointRect, const QPlatformScreen *platformScreen)
{
    const qreal scaleFactor = QHighDpiScaling::factor(platformScreen);
    const QPoint origin = QHighDpiScaling::origin(platformScreen);
    return QRect(toNative(pointRect.topLeft(), scaleFactor, origin),
                 toNative(pointRect.size(), scaleFactor));
}

inline QRect fromNativePixels(const QRect &pixelRect, const QScreen *screen)
{
    const qreal scaleFactor = QHighDpiScaling::factor(screen);
    const QPoint origin = QHighDpiScaling::origin(screen);
    return QRect(fromNative(pixelRect.topLeft(), scaleFactor, origin),
                 fromNative(pixelRect.size(), scaleFactor));
}

inline QRect toNativePixels(const QRect &pointRect, const QScreen *screen)
{
    const qreal scaleFactor = QHighDpiScaling::factor(screen);
    const QPoint origin = QHighDpiScaling::origin(screen);
    return QRect(toNative(pointRect.topLeft(), scaleFactor, origin),
                 toNative(pointRect.size(), scaleFactor));
}

inline QRect fromNativePixels(const QRect &pixelRect, const QWindow *window)
{
    if (window && window->isTopLevel() && window->screen()) {
        return fromNativePixels(pixelRect, window->screen());
    } else {
        const qreal scaleFactor = QHighDpiScaling::factor(window);
        return QRect(pixelRect.topLeft() / scaleFactor, fromNative(pixelRect.size(), scaleFactor));
    }
}

inline QRectF toNativePixels(const QRectF &pointRect, const QScreen *screen)
{
    //########
    return toNativePixels(pointRect.toRect(), screen);
}

inline QRect toNativePixels(const QRect &pointRect, const QWindow *window)
{
    if (window && window->isTopLevel() && window->screen()) {
        return toNativePixels(pointRect, window->screen());
    } else {
        const qreal scaleFactor = QHighDpiScaling::factor(window);
        return QRect(pointRect.topLeft() * scaleFactor, toNative(pointRect.size(), scaleFactor));
    }
}

inline QRectF fromNativePixels(const QRectF &pixelRect, const QScreen *screen)
{
    //########
    return fromNativePixels(pixelRect.toRect(), screen);
}

inline QRectF fromNativePixels(const QRectF &pixelRect, const QWindow *window)
{
    if (window && window->isTopLevel() && window->screen()) {
        return fromNativePixels(pixelRect, window->screen());
    } else {
        const qreal scaleFactor = QHighDpiScaling::factor(window);
        return QRectF(pixelRect.topLeft() / scaleFactor, pixelRect.size() / scaleFactor);
    }
}

inline QRectF toNativePixels(const QRectF &pointRect, const QWindow *window)
{
    if (window && window->isTopLevel() && window->screen()) {
        return toNativePixels(pointRect, window->screen());
    } else {
        const qreal scaleFactor = QHighDpiScaling::factor(window);
        return QRectF(pointRect.topLeft() * scaleFactor, pointRect.size() * scaleFactor);
    }
}

inline QSize fromNativePixels(const QSize &pixelSize, const QWindow *window)
{
    return pixelSize / QHighDpiScaling::factor(window);
}

inline QSize toNativePixels(const QSize &pointSize, const QWindow *window)
{
    return pointSize * QHighDpiScaling::factor(window);
}

inline QSizeF fromNativePixels(const QSizeF &pixelSize, const QWindow *window)
{
    return pixelSize / QHighDpiScaling::factor(window);
}

inline QSizeF toNativePixels(const QSizeF &pointSize, const QWindow *window)
{
    return pointSize * QHighDpiScaling::factor(window);
}

inline QPoint fromNativePixels(const QPoint &pixelPoint, const QScreen *screen)
{
    return fromNative(pixelPoint, QHighDpiScaling::factor(screen), QHighDpiScaling::origin(screen));
}

inline QPoint fromNativePixels(const QPoint &pixelPoint, const QWindow *window)
{
    if (window && window->isTopLevel() && window->screen())
        return fromNativePixels(pixelPoint, window->screen());
    else
        return pixelPoint / QHighDpiScaling::factor(window);
}

inline QPoint toNativePixels(const QPoint &pointPoint, const QScreen *screen)
{
    return toNative(pointPoint, QHighDpiScaling::factor(screen), QHighDpiScaling::origin(screen));
}

inline QPoint toNativePixels(const QPoint &pointPoint, const QWindow *window)
{
    if (window && window->isTopLevel() && window->screen())
        return toNativePixels(pointPoint, window->screen());
    else
        return pointPoint * QHighDpiScaling::factor(window);
}

inline QPointF fromNativePixels(const QPointF &pixelPoint, const QScreen *screen)
{
    return fromNativePixels(pixelPoint.toPoint(), screen); //###############
}

inline QPointF fromNativePixels(const QPointF &pixelPoint, const QWindow *window)
{
    if (window && window->isTopLevel() && window->screen())
        return fromNativePixels(pixelPoint, window->screen());
    else
        return pixelPoint / QHighDpiScaling::factor(window);
}

inline QPointF toNativePixels(const QPointF &pointPoint, const QScreen *screen)
{
    return toNativePixels(pointPoint.toPoint(), screen); //###########
}

inline QPointF toNativePixels(const QPointF &pointPoint, const QWindow *window)
{
     if (window && window->isTopLevel() && window->screen())
        return toNativePixels(pointPoint, window->screen());
    else
        return pointPoint * QHighDpiScaling::factor(window);
}

inline QMargins fromNativePixels(const QMargins &pixelMargins, const QWindow *window)
{
    const qreal scaleFactor = QHighDpiScaling::factor(window);
    return QMargins(pixelMargins.left() / scaleFactor, pixelMargins.top() / scaleFactor,
                    pixelMargins.right() / scaleFactor, pixelMargins.bottom() / scaleFactor);
}

inline QMargins toNativePixels(const QMargins &pointMargins, const QWindow *window)
{
    const qreal scaleFactor = QHighDpiScaling::factor(window);
    return QMargins(pointMargins.left() * scaleFactor, pointMargins.top() * scaleFactor,
                    pointMargins.right() * scaleFactor, pointMargins.bottom() * scaleFactor);
}
#if 1
    //############## expose regions need special handling
inline QRegion fromNativeLocalRegion(const QRegion &pixelRegion, const QWindow *window)
{
    if (!QHighDpiScaling::isActive())
        return pixelRegion;

    qreal scaleFactor = QHighDpiScaling::factor(window);
    QRegion pointRegion;
    foreach (const QRect &rect, pixelRegion.rects())
        pointRegion += QRect(fromNative(rect.topLeft(), scaleFactor),
                             fromNative(rect.size(), scaleFactor));
    return pointRegion;
}

inline QRegion toNativeLocalRegion(const QRegion &pointRegion, const QWindow *window)
{
    if (!QHighDpiScaling::isActive())
        return pointRegion;

    qreal scaleFactor = QHighDpiScaling::factor(window);
    QRegion pixelRegon;
    foreach (const QRect &rect, pointRegion.rects())
        pixelRegon += QRect(toNative(rect.topLeft(), scaleFactor),
                             toNative(rect.size(), scaleFactor));
    return pixelRegon;
}
#endif
// Any T that has operator/()
template <typename T>
T fromNativePixels(const T &pixelValue, const QWindow *window)
{
    if (!QHighDpiScaling::isActive())
        return pixelValue;

    return pixelValue / QHighDpiScaling::factor(window);

}

    //##### ?????
template <typename T>
T fromNativePixels(const T &pixelValue, const QScreen *screen)
{
    if (!QHighDpiScaling::isActive())
        return pixelValue;

    return pixelValue / QHighDpiScaling::factor(screen);

}

// Any T that has operator*()
template <typename T>
T toNativePixels(const T &pointValue, const QWindow *window)
{
    if (!QHighDpiScaling::isActive())
        return pointValue;

    return pointValue * QHighDpiScaling::factor(window);
}

template <typename T>
T toNativePixels(const T &pointValue, const QScreen *screen)
{
    if (!QHighDpiScaling::isActive())
        return pointValue;

    return pointValue * QHighDpiScaling::factor(screen);
}


// Any QVector<T> where T has operator/()
template <typename T>
QVector<T> fromNativePixels(const QVector<T> &pixelValues, const QWindow *window)
{
    if (!QHighDpiScaling::isActive())
        return pixelValues;

    QVector<T> pointValues;
    foreach (const T& pixelValue, pixelValues)
        pointValues.append(pixelValue / QHighDpiScaling::factor(window));
    return pointValues;
}

// Any QVector<T> where T has operator*()
template <typename T>
QVector<T> toNativePixels(const QVector<T> &pointValues, const QWindow *window)
{
    if (!QHighDpiScaling::isActive())
        return pointValues;

    QVector<T> pixelValues;
    foreach (const T& pointValue, pointValues)
        pixelValues.append(pointValue * QHighDpiScaling::factor(window));
    return pixelValues;
}

#if 0
// Any QPair<T, U> where T and U has operator/()
template <typename T, typename U>
QPair<T, U> fromNativePixels(const QPair<T, U> &pixelPair, const QWindow *window)
{
    return qMakePair(fromNativePixels(pixelPair.first, window),
                     fromNativePixels(pixelPair.second, window));
}

// Any QPair<T, U> where T and U has operator*()
template <typename T, typename U>
QPair<T, U> toNativePixels(const QPair<T, U> &pointPair, const QWindow *window)
{
    return qMakePair(QHighDpi::toNativePixels(pointPair.first, window),
                     QHighDpi::toNativePixels(pointPair.second, window));
}
#endif
}

QT_END_NAMESPACE

#endif
