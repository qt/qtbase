// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcolrpaintgraphrenderer_p.h"


QT_BEGIN_NAMESPACE

QColrPaintGraphRenderer::~QColrPaintGraphRenderer()
{
    Q_ASSERT(m_oldPaths.isEmpty());
    Q_ASSERT(m_oldTransforms.isEmpty());

    delete m_painter;
}

void QColrPaintGraphRenderer::save()
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[enter scope]";

    m_oldPaths.append(m_currentPath);
    m_oldTransforms.append(m_currentTransform);

    if (m_painter != nullptr)
        m_painter->save();
}

void QColrPaintGraphRenderer::restore()
{
    m_currentPath = m_oldPaths.takeLast();
    m_currentTransform = m_oldTransforms.takeLast();

    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[exit scope]";

    if (m_painter != nullptr)
        m_painter->restore();
}

void QColrPaintGraphRenderer::setClip(QRect rect)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[set clip: " << rect
        << "]";
    if (!isRendering())
        m_boundingRect = m_boundingRect.united(rect);
    if (m_painter != nullptr)
        m_painter->setClipRect(rect);
}

void QColrPaintGraphRenderer::prependTransform(const QTransform &transform)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[prepend transform: " << transform
        << "]";
    m_currentTransform = transform * m_currentTransform;
}

void QColrPaintGraphRenderer::appendPath(const QPainterPath &path)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[append path: " << path.controlPointRect()
        << "]";

    QPainterPath transformedPath(m_currentTransform.map(path));
    m_currentPath = m_currentPath.united(transformedPath);
    if (!isRendering())
        m_boundingRect = m_boundingRect.united(transformedPath.boundingRect());
}

void QColrPaintGraphRenderer::setPath(const QPainterPath &path)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[set path]";

    m_currentPath.clear();
    appendPath(path);
}

void QColrPaintGraphRenderer::setSolidColor(QColor color)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[solid " << color
        << "]";

    if (m_painter != nullptr)
        m_painter->setBrush(color);
}

void QColrPaintGraphRenderer::setLinearGradient(QPointF p0, QPointF p1, QPointF p2,
                                                QGradient::Spread spread,
                                                const QGradientStops &gradientStops)
{
    if (m_painter != nullptr) {
        qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[linear gradient " << p0 << ", " << p1 << ", " << p2
        << ", spread: " << spread
        << ", stop count: " << gradientStops.size()
        << "]";

        // Calculate new start and end point for single vector gradient preferred by Qt
        // Find vector perpendicular to p0p2 and project p0p1 onto this to find p3 (final
        // stop)
        // https://learn.microsoft.com/en-us/typography/opentype/spec/colr#linear-gradients
        QVector2D p0p2(p2 - p0);
        if (qFuzzyIsNull(p0p2.lengthSquared())) {
            qCWarning(lcColrv1) << "Malformed linear gradient in COLRv1 graph. Points:"
                                << p0
                                << p1
                                << p2;
            return;
        }

        // v is perpendicular to p0p2
        QVector2D v(p0p2.y(), -p0p2.x());

        // u is the vector from p0 to p1
        QVector2D u(p1 - p0);
        if (qFuzzyIsNull(u.lengthSquared())) {
            qCWarning(lcColrv1) << "Malformed linear gradient in COLRv1 graph. Points:"
                                << p0
                                << p1
                                << p2;
            return;
        }

        // We find the projected point p3
        QPointF p3((QVector2D(p0) + v * QVector2D::dotProduct(u, v) / v.lengthSquared()).toPointF());

        p0 = m_currentTransform.map(p0);
        p3 = m_currentTransform.map(p1);

        QLinearGradient linearGradient(p0, p3);
        linearGradient.setSpread(spread);
        linearGradient.setStops(gradientStops);
        linearGradient.setCoordinateMode(QGradient::LogicalMode);

        m_painter->setBrush(linearGradient);
    }
}

void QColrPaintGraphRenderer::setConicalGradient(QPointF center,
                                                 qreal startAngle, qreal endAngle,
                                                 QGradient::Spread spread,
                                                 const QGradientStops &gradientStops)
{
    if (m_painter != nullptr) {
        qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
            << "[conical gradient " << center
            << ", startAngle=" << startAngle
            << ", endAngle=" << endAngle
            << ", spread: " << spread
            << ", stop count: " << gradientStops.size()
            << "]";

        QConicalGradient conicalGradient(center, startAngle);
        conicalGradient.setSpread(spread);

        // Adapt stops to actual span since Qt always assumes end angle of 360
        // Note: This does not give accurate results for the colors outside the angle span.
        // To do this correctly, we would have to insert stops at 0°, 360° and if the spread
        // is reflect/repeat, also throughout the uncovered area to get the correct
        // rendering. It might however be easier to support this in QConicalGradient itself.
        // For now, this is left only semi-supported, as sweep gradients are currently rare.
        const qreal multiplier = qFuzzyCompare(endAngle, startAngle)
                                     ? 1.0
                                     : (endAngle - startAngle) / 360.0;

        QGradientStops adaptedStops;
        adaptedStops.reserve(gradientStops.size());

        for (const QGradientStop &gradientStop : gradientStops)
            adaptedStops.append(qMakePair(gradientStop.first * multiplier, gradientStop.second));

        conicalGradient.setStops(adaptedStops);
        conicalGradient.setCoordinateMode(QGradient::LogicalMode);

        m_painter->setBrush(conicalGradient);
    }
}

void QColrPaintGraphRenderer::setRadialGradient(QPointF c0, qreal r0,
                                                QPointF c1, qreal r1,
                                                QGradient::Spread spread,
                                                const QGradientStops &gradientStops)
{
    if (m_painter != nullptr) {
        qCDebug(lcColrv1).noquote().nospace()
            << QByteArray().fill(' ', m_oldPaths.size() * 2)
            << "[radial gradient " << c0
            << ", rad=" << r0
            << ", " << c1
            << ", rad=" << r1
            << ", spread: " << spread
            << ", stop count: " << gradientStops.size()
            << "]";

        QPointF c0e(c0 + QPointF(r0, 0.0));
        QPointF c1e(c1 + QPointF(r1, 0.0));

        c0 = m_currentTransform.map(c0);
        c0e = m_currentTransform.map(c0e);
        c1 = m_currentTransform.map(c1);
        c1e = m_currentTransform.map(c1e);

        const QVector2D d0(c0e - c0);
        const QVector2D d1(c1e - c1);

        QRadialGradient gradient(c1, d1.length(), c0, d0.length());
        gradient.setCoordinateMode(QGradient::LogicalMode);
        gradient.setSpread(spread);
        gradient.setStops(gradientStops);
        m_painter->setBrush(gradient);
    }
}

void QColrPaintGraphRenderer::drawCurrentPath()
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
            << "[draw path " << m_currentPath.controlPointRect() << m_currentTransform
        << "]";

    if (m_painter != nullptr)
        m_painter->drawPath(m_currentPath);
}

void QColrPaintGraphRenderer::beginCalculateBoundingBox()
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[begin calculate bounding box]";

    Q_ASSERT(m_painter == nullptr);
    m_boundingRect = QRect{};
}

void QColrPaintGraphRenderer::drawImage(const QImage &image)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[draw image: " << image.size() << "]";

    if (m_painter != nullptr) {
        m_painter->setWorldMatrixEnabled(false);
        m_painter->drawImage(QPoint(0, 0), image);
        m_painter->setWorldMatrixEnabled(true);
    }
}

void QColrPaintGraphRenderer::setCompositionMode(QPainter::CompositionMode mode)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[set composition mode: " << mode << "]";

    if (m_painter != nullptr)
        m_painter->setCompositionMode(mode);
}

void QColrPaintGraphRenderer::beginRender(qreal pixelSizeScale, const QTransform &transform)
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[begin render scale: " << pixelSizeScale
        << ", transform: " << transform
        << "]";

    if (m_boundingRect.isEmpty())
        return;

    QRect alignedRect(m_boundingRect.toAlignedRect());

    m_image = QImage(alignedRect.size(), QImage::Format_ARGB32_Premultiplied);
    m_image.fill(Qt::transparent);

    Q_ASSERT(m_painter == nullptr);
    m_painter = new QPainter;
    m_painter->begin(&m_image);
    m_painter->setRenderHint(QPainter::Antialiasing);
    m_painter->setPen(Qt::NoPen);
    m_painter->setBrush(Qt::NoBrush);

    m_painter->translate(-alignedRect.topLeft());

    // Scale from normalized coordinates
    m_painter->scale(pixelSizeScale, pixelSizeScale);
    m_painter->setWorldTransform(transform, true);
}

QImage QColrPaintGraphRenderer::endRender()
{
    qCDebug(lcColrv1).noquote().nospace()
        << QByteArray().fill(' ', m_oldPaths.size() * 2)
        << "[end image size: " << m_image.size()
        << "]";

    Q_ASSERT(m_painter != nullptr);
    m_painter->end();
    delete m_painter;
    m_painter = nullptr;

    return m_image;
}

QT_END_NAMESPACE
