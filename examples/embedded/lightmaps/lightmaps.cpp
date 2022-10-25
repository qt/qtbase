// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

#include "lightmaps.h"
#include "slippymap.h"

// how long (milliseconds) the user need to hold (after a tap on the screen)
// before triggering the magnifying glass feature
// 701, a prime number, is the sum of 229, 233, 239
// (all three are also prime numbers, consecutive!)
#define HOLD_TIME 701

// maximum size of the magnifier
// Hint: see above to find why I picked this one :)
#define MAX_MAGNIFIER 229

LightMaps::LightMaps(QWidget *parent)
    : QWidget(parent), pressed(false), snapped(false), zoomed(false),
      invert(false)
{
    m_normalMap = new SlippyMap(this);
    m_largeMap = new SlippyMap(this);
    connect(m_normalMap, &SlippyMap::updated, this, &LightMaps::updateMap);
    connect(m_largeMap, &SlippyMap::updated, this, &LightMaps::updateMap);
}

void LightMaps::setCenter(qreal lat, qreal lng)
{
    m_normalMap->latitude = lat;
    m_normalMap->longitude = lng;
    m_normalMap->invalidate();
    m_largeMap->latitude = lat;
    m_largeMap->longitude = lng;
    m_largeMap->invalidate();
}

void LightMaps::toggleNightMode()
{
    invert = !invert;
    update();
}

void LightMaps::updateMap(const QRect &r)
{
    update(r);
}

void LightMaps::activateZoom()
{
    zoomed = true;
    tapTimer.stop();
    m_largeMap->zoom = m_normalMap->zoom + 1;
    m_largeMap->width = m_normalMap->width * 2;
    m_largeMap->height = m_normalMap->height * 2;
    m_largeMap->latitude = m_normalMap->latitude;
    m_largeMap->longitude = m_normalMap->longitude;
    m_largeMap->invalidate();
    update();
}

void LightMaps::resizeEvent(QResizeEvent *)
{
    m_normalMap->width = width();
    m_normalMap->height = height();
    m_normalMap->invalidate();
    m_largeMap->width = m_normalMap->width * 2;
    m_largeMap->height = m_normalMap->height * 2;
    m_largeMap->invalidate();
}

void LightMaps::paintEvent(QPaintEvent *event)
{
    QPainter p;
    p.begin(this);
    m_normalMap->render(&p, event->rect());
    p.setPen(Qt::black);
    p.drawText(rect(),  Qt::AlignBottom | Qt::TextWordWrap,
                "Map data CCBYSA 2009 OpenStreetMap.org contributors");
    p.end();

    if (zoomed) {
        int dim = qMin(width(), height());
        int magnifierSize = qMin(MAX_MAGNIFIER, dim * 2 / 3);
        int radius = magnifierSize / 2;
        int ring = radius - 15;
        QSize box = QSize(magnifierSize, magnifierSize);

        // reupdate our mask
        if (maskPixmap.size() != box) {
            maskPixmap = QPixmap(box);
            maskPixmap.fill(Qt::transparent);

            QRadialGradient g;
            g.setCenter(radius, radius);
            g.setFocalPoint(radius, radius);
            g.setRadius(radius);
            g.setColorAt(1.0, QColor(255, 255, 255, 0));
            g.setColorAt(0.5, QColor(128, 128, 128, 255));

            QPainter mask(&maskPixmap);
            mask.setRenderHint(QPainter::Antialiasing);
            mask.setCompositionMode(QPainter::CompositionMode_Source);
            mask.setBrush(g);
            mask.setPen(Qt::NoPen);
            mask.drawRect(maskPixmap.rect());
            mask.setBrush(QColor(Qt::transparent));
            mask.drawEllipse(g.center(), ring, ring);
            mask.end();
        }

        QPoint center = dragPos - QPoint(0, radius);
        center = center + QPoint(0, radius / 2);
        QPoint corner = center - QPoint(radius, radius);

        QPoint xy = center * 2 - QPoint(radius, radius);

        // only set the dimension to the magnified portion
        if (zoomPixmap.size() != box) {
            zoomPixmap = QPixmap(box);
            zoomPixmap.fill(Qt::lightGray);
        }
        if (true) {
            QPainter p(&zoomPixmap);
            p.translate(-xy);
            m_largeMap->render(&p, QRect(xy, box));
            p.end();
        }

        QPainterPath clipPath;
        clipPath.addEllipse(center, ring, ring);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setClipPath(clipPath);
        p.drawPixmap(corner, zoomPixmap);
        p.setClipping(false);
        p.drawPixmap(corner, maskPixmap);
        p.setPen(Qt::gray);
        p.drawPath(clipPath);
    }
    if (invert) {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Difference);
        p.fillRect(event->rect(), Qt::white);
        p.end();
    }
}

void LightMaps::timerEvent(QTimerEvent *)
{
    if (!zoomed)
        activateZoom();
    update();
}

void LightMaps::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() != Qt::LeftButton)
        return;
    pressed = snapped = true;
    pressPos = dragPos = event->position().toPoint();
    tapTimer.stop();
    tapTimer.start(HOLD_TIME, this);
}

void LightMaps::mouseMoveEvent(QMouseEvent *event)
{
    if (!event->buttons())
        return;
    if (!zoomed) {
        if (!pressed || !snapped) {
            QPoint delta = event->position().toPoint() - pressPos;
            pressPos = event->position().toPoint();
            m_normalMap->pan(delta);
            return;
        } else {
            const int threshold = 10;
            QPoint delta = event->position().toPoint() - pressPos;
            if (snapped) {
                snapped &= delta.x() < threshold;
                snapped &= delta.y() < threshold;
                snapped &= delta.x() > -threshold;
                snapped &= delta.y() > -threshold;
            }
            if (!snapped)
                tapTimer.stop();
        }
    } else {
        dragPos = event->position().toPoint();
        update();
    }
}

void LightMaps::mouseReleaseEvent(QMouseEvent *)
{
    zoomed = false;
    update();
}

void LightMaps::keyPressEvent(QKeyEvent *event)
{
    if (!zoomed) {
        if (event->key() == Qt::Key_Left)
            m_normalMap->pan(QPoint(20, 0));
        if (event->key() == Qt::Key_Right)
            m_normalMap->pan(QPoint(-20, 0));
        if (event->key() == Qt::Key_Up)
            m_normalMap->pan(QPoint(0, 20));
        if (event->key() == Qt::Key_Down)
            m_normalMap->pan(QPoint(0, -20));
        if (event->key() == Qt::Key_Z || event->key() == Qt::Key_Select) {
            dragPos = QPoint(width() / 2, height() / 2);
            activateZoom();
        }
    } else {
        if (event->key() == Qt::Key_Z || event->key() == Qt::Key_Select) {
            zoomed = false;
            update();
        }
        QPoint delta(0, 0);
        if (event->key() == Qt::Key_Left)
            delta = QPoint(-15, 0);
        if (event->key() == Qt::Key_Right)
            delta = QPoint(15, 0);
        if (event->key() == Qt::Key_Up)
            delta = QPoint(0, -15);
        if (event->key() == Qt::Key_Down)
            delta = QPoint(0, 15);
        if (delta != QPoint(0, 0)) {
            dragPos += delta;
            update();
        }
    }
}
