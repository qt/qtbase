// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "tabletcanvas.h"

#include <QCoreApplication>
#include <QPainter>
#include <QtMath>
#include <cstdlib>

//! [0]
TabletCanvas::TabletCanvas()
    : QWidget(nullptr), m_brush(m_color)
    , m_pen(m_brush, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
{
    resize(500, 500);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_TabletTracking);
}
//! [0]

//! [1]
bool TabletCanvas::saveImage(const QString &file)
{
    return m_pixmap.save(file);
}
//! [1]

//! [2]
bool TabletCanvas::loadImage(const QString &file)
{
    bool success = m_pixmap.load(file);

    if (success) {
        update();
        return true;
    }
    return false;
}
//! [2]

void TabletCanvas::clear()
{
    m_pixmap.fill(Qt::white);
    update();
}

//! [3]
void TabletCanvas::tabletEvent(QTabletEvent *event)
{
    switch (event->type()) {
        case QEvent::TabletPress:
            if (!m_deviceDown) {
                m_deviceDown = true;
                lastPoint.pos = event->position();
                lastPoint.pressure = event->pressure();
                lastPoint.rotation = event->rotation();
            }
            break;
        case QEvent::TabletMove:
#ifndef Q_OS_IOS
            if (event->pointingDevice() && event->pointingDevice()->capabilities().testFlag(QPointingDevice::Capability::Rotation))
                updateCursor(event);
#endif
            if (m_deviceDown) {
                updateBrush(event);
                QPainter painter(&m_pixmap);
                paintPixmap(painter, event);
                lastPoint.pos = event->position();
                lastPoint.pressure = event->pressure();
                lastPoint.rotation = event->rotation();
            }
            break;
        case QEvent::TabletRelease:
            if (m_deviceDown && event->buttons() == Qt::NoButton)
                m_deviceDown = false;
            update();
            break;
        default:
            break;
    }
    event->accept();
}
//! [3]

//! [4]
void TabletCanvas::initPixmap()
{
    qreal dpr = devicePixelRatio();
    QPixmap newPixmap = QPixmap(qRound(width() * dpr), qRound(height() * dpr));
    newPixmap.setDevicePixelRatio(dpr);
    newPixmap.fill(Qt::white);
    QPainter painter(&newPixmap);
    if (!m_pixmap.isNull())
        painter.drawPixmap(0, 0, m_pixmap);
    painter.end();
    m_pixmap = newPixmap;
}

void TabletCanvas::paintEvent(QPaintEvent *event)
{
    if (m_pixmap.isNull())
        initPixmap();
    QPainter painter(this);
    QRect pixmapPortion = QRect(event->rect().topLeft() * devicePixelRatio(),
                                event->rect().size() * devicePixelRatio());
    painter.drawPixmap(event->rect().topLeft(), m_pixmap, pixmapPortion);
}
//! [4]

//! [5]
void TabletCanvas::paintPixmap(QPainter &painter, QTabletEvent *event)
{
    static qreal maxPenRadius = pressureToWidth(1.0);
    painter.setRenderHint(QPainter::Antialiasing);

    switch (event->deviceType()) {
//! [6]
        case QInputDevice::DeviceType::Airbrush:
            {
                painter.setPen(Qt::NoPen);
                QRadialGradient grad(lastPoint.pos, m_pen.widthF() * 10.0);
                QColor color = m_brush.color();
                color.setAlphaF(color.alphaF() * 0.25);
                grad.setColorAt(0, m_brush.color());
                grad.setColorAt(0.5, Qt::transparent);
                painter.setBrush(grad);
                qreal radius = grad.radius();
                painter.drawEllipse(event->position(), radius, radius);
                update(QRect(event->position().toPoint() - QPoint(radius, radius), QSize(radius * 2, radius * 2)));
            }
            break;
//! [6]
        case QInputDevice::DeviceType::Puck:
        case QInputDevice::DeviceType::Mouse:
            {
                const QString error(tr("This input device is not supported by the example."));
#if QT_CONFIG(statustip)
                QStatusTipEvent status(error);
                QCoreApplication::sendEvent(this, &status);
#else
                qWarning() << error;
#endif
            }
            break;
        default:
            {
                const QString error(tr("Unknown tablet device - treating as stylus"));
#if QT_CONFIG(statustip)
                QStatusTipEvent status(error);
                QCoreApplication::sendEvent(this, &status);
#else
                qWarning() << error;
#endif
            }
            Q_FALLTHROUGH();
        case QInputDevice::DeviceType::Stylus:
            if (event->pointingDevice()->capabilities().testFlag(QPointingDevice::Capability::Rotation)) {
                m_brush.setStyle(Qt::SolidPattern);
                painter.setPen(Qt::NoPen);
                painter.setBrush(m_brush);
                QPolygonF poly;
                qreal halfWidth = pressureToWidth(lastPoint.pressure);
                QPointF brushAdjust(qSin(qDegreesToRadians(-lastPoint.rotation)) * halfWidth,
                                    qCos(qDegreesToRadians(-lastPoint.rotation)) * halfWidth);
                poly << lastPoint.pos + brushAdjust;
                poly << lastPoint.pos - brushAdjust;
                halfWidth = m_pen.widthF();
                brushAdjust = QPointF(qSin(qDegreesToRadians(-event->rotation())) * halfWidth,
                                      qCos(qDegreesToRadians(-event->rotation())) * halfWidth);
                poly << event->position() - brushAdjust;
                poly << event->position() + brushAdjust;
                painter.drawConvexPolygon(poly);
                update(poly.boundingRect().toRect());
            } else {
                painter.setPen(m_pen);
                painter.drawLine(lastPoint.pos, event->position());
                update(QRect(lastPoint.pos.toPoint(), event->position().toPoint()).normalized()
                       .adjusted(-maxPenRadius, -maxPenRadius, maxPenRadius, maxPenRadius));
            }
            break;
    }
}
//! [5]

qreal TabletCanvas::pressureToWidth(qreal pressure)
{
    return pressure * 10 + 1;
}

//! [7]
void TabletCanvas::updateBrush(const QTabletEvent *event)
{
    int hue, saturation, value, alpha;
    m_color.getHsv(&hue, &saturation, &value, &alpha);

    int vValue = int(((event->yTilt() + 60.0) / 120.0) * 255);
    int hValue = int(((event->xTilt() + 60.0) / 120.0) * 255);
//! [7] //! [8]

    switch (m_alphaChannelValuator) {
        case PressureValuator:
            m_color.setAlphaF(event->pressure());
            break;
        case TangentialPressureValuator:
            if (event->deviceType() == QInputDevice::DeviceType::Airbrush)
                m_color.setAlphaF(qMax(0.01, (event->tangentialPressure() + 1.0) / 2.0));
            else
                m_color.setAlpha(255);
            break;
        case TiltValuator:
            m_color.setAlpha(std::max(std::abs(vValue - 127),
                                      std::abs(hValue - 127)));
            break;
        default:
            m_color.setAlpha(255);
    }

//! [8] //! [9]
    switch (m_colorSaturationValuator) {
        case VTiltValuator:
            m_color.setHsv(hue, vValue, value, alpha);
            break;
        case HTiltValuator:
            m_color.setHsv(hue, hValue, value, alpha);
            break;
        case PressureValuator:
            m_color.setHsv(hue, int(event->pressure() * 255.0), value, alpha);
            break;
        default:
            ;
    }

//! [9] //! [10]
    switch (m_lineWidthValuator) {
        case PressureValuator:
            m_pen.setWidthF(pressureToWidth(event->pressure()));
            break;
        case TiltValuator:
            m_pen.setWidthF(std::max(std::abs(vValue - 127),
                                     std::abs(hValue - 127)) / 12);
            break;
        default:
            m_pen.setWidthF(1);
    }

//! [10] //! [11]
    if (event->pointerType() == QPointingDevice::PointerType::Eraser) {
        m_brush.setColor(Qt::white);
        m_pen.setColor(Qt::white);
        m_pen.setWidthF(event->pressure() * 10 + 1);
    } else {
        m_brush.setColor(m_color);
        m_pen.setColor(m_color);
    }
}
//! [11]

//! [12]
void TabletCanvas::updateCursor(const QTabletEvent *event)
{
    QCursor cursor;
    if (event->type() != QEvent::TabletLeaveProximity) {
        if (event->pointerType() == QPointingDevice::PointerType::Eraser) {
            cursor = QCursor(QPixmap(":/images/cursor-eraser.png"), 3, 28);
        } else {
            switch (event->deviceType()) {
            case QInputDevice::DeviceType::Stylus:
                if (event->pointingDevice()->capabilities().testFlag(QPointingDevice::Capability::Rotation)) {
                    QImage origImg(QLatin1String(":/images/cursor-felt-marker.png"));
                    QImage img(32, 32, QImage::Format_ARGB32);
                    QColor solid = m_color;
                    solid.setAlpha(255);
                    img.fill(solid);
                    QPainter painter(&img);
                    QTransform transform = painter.transform();
                    transform.translate(16, 16);
                    transform.rotate(event->rotation());
                    painter.setTransform(transform);
                    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
                    painter.drawImage(-24, -24, origImg);
                    painter.setCompositionMode(QPainter::CompositionMode_HardLight);
                    painter.drawImage(-24, -24, origImg);
                    painter.end();
                    cursor = QCursor(QPixmap::fromImage(img), 16, 16);
                } else {
                    cursor = QCursor(QPixmap(":/images/cursor-pencil.png"), 0, 0);
                }
                break;
            case QInputDevice::DeviceType::Airbrush:
                cursor = QCursor(QPixmap(":/images/cursor-airbrush.png"), 3, 4);
                break;
            default:
                break;
            }
        }
    }
    setCursor(cursor);
}
//! [12]

void TabletCanvas::resizeEvent(QResizeEvent *)
{
    initPixmap();
}
