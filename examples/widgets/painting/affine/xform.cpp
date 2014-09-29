/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "xform.h"
#include "hoverpoints.h"

#include <QLayout>
#include <QPainter>
#include <QPainterPath>

const int alpha = 155;

XFormView::XFormView(QWidget *parent)
    : ArthurFrame(parent)
{
    setAttribute(Qt::WA_MouseTracking);
    m_type = VectorType;
    m_rotation = 0.0;
    m_scale = 1.0;
    m_shear = 0.0;

    m_pixmap = QPixmap(":res/affine/bg1.jpg");
    pts = new HoverPoints(this, HoverPoints::CircleShape);
    pts->setConnectionType(HoverPoints::LineConnection);
    pts->setEditable(false);
    pts->setPointSize(QSize(15, 15));
    pts->setShapeBrush(QBrush(QColor(151, 0, 0, alpha)));
    pts->setShapePen(QPen(QColor(255, 100, 50, alpha)));
    pts->setConnectionPen(QPen(QColor(151, 0, 0, 50)));
    pts->setBoundingRect(QRectF(0, 0, 500, 500));
    ctrlPoints << QPointF(250, 250) << QPointF(350, 250);
    pts->setPoints(ctrlPoints);
    connect(pts, SIGNAL(pointsChanged(QPolygonF)),
            this, SLOT(updateCtrlPoints(QPolygonF)));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

XFormView::XFormType XFormView::type() const
{
    return m_type;
}

QPixmap XFormView::pixmap() const
{
    return m_pixmap;
}

QString XFormView::text() const
{
    return m_text;
}

void XFormView::setText(const QString &t)
{
    m_text = t;
    update();
}

void XFormView::setPixmap(const QPixmap &p)
{
    m_pixmap = p;
    update();
}

void XFormView::setType(XFormType t)
{
    m_type = t;
    update();
}

void XFormView::mousePressEvent(QMouseEvent *)
{
    setDescriptionEnabled(false);
}

void XFormView::resizeEvent(QResizeEvent *e)
{
    pts->setBoundingRect(rect());
    ArthurFrame::resizeEvent(e);
}

void XFormView::paint(QPainter *p)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    switch (m_type) {
    case VectorType:
        drawVectorType(p);
        break;
    case PixmapType:
        drawPixmapType(p);
        break;
    case TextType:
        drawTextType(p);
        break;
    }
    p->restore();
}

void XFormView::updateCtrlPoints(const QPolygonF &points)
{
    QPointF trans = points.at(0) - ctrlPoints.at(0);

    if (qAbs(points.at(0).x() - points.at(1).x()) < 10
        && qAbs(points.at(0).y() - points.at(1).y()) < 10)
        pts->setPoints(ctrlPoints);
    if (!trans.isNull()) {
        ctrlPoints[0] = points.at(0);
        ctrlPoints[1] += trans;
        pts->setPoints(ctrlPoints);
    }
    ctrlPoints = points;

    QLineF line(ctrlPoints.at(0), ctrlPoints.at(1));
    m_rotation = line.angle(QLineF(0, 0, 1, 0));
    if (line.dy() < 0)
        m_rotation = 360 - m_rotation;

    if (trans.isNull())
        emit rotationChanged(int(m_rotation*10));
}

void XFormView::setVectorType()
{
    m_type = VectorType;
    update();
}

void XFormView::setPixmapType()
{
    m_type = PixmapType;
    update();
}

void XFormView::setTextType()
{
    m_type = TextType;
    update();
}

void XFormView::setAnimation(bool animate)
{
    timer.stop();
    if (animate)
        timer.start(25, this);
}

void XFormView::changeRotation(int r)
{
    setRotation(qreal(r) / 10);
}

void XFormView::changeScale(int s)
{
    setScale(qreal(s) / 1000);
}

void XFormView::changeShear(int s)
{
    setShear(qreal(s) / 1000);
}

void XFormView::setShear(qreal s)
{
    m_shear = s;
    update();
}

void XFormView::setScale(qreal s)
{
    m_scale = s;
    update();
}

void XFormView::setRotation(qreal r)
{
    qreal old_rot = m_rotation;
    m_rotation = r;

    QPointF center(pts->points().at(0));
    QMatrix m;
    m.translate(center.x(), center.y());
    m.rotate(m_rotation - old_rot);
    m.translate(-center.x(), -center.y());
    pts->setPoints(pts->points() * m);

    update();
}

void XFormView::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == timer.timerId()) {
        QPointF center(pts->points().at(0));
        QMatrix m;
        m.translate(center.x(), center.y());
        m.rotate(0.2);
        m.translate(-center.x(), -center.y());
        pts->setPoints(pts->points() * m);

        setUpdatesEnabled(false);
        static qreal scale_inc = 0.003;
        static qreal shear_inc = -0.001;
        emit scaleChanged(int((m_scale + scale_inc) * 1000));
        emit shearChanged(int((m_shear + shear_inc) * 1000));
        if (m_scale >= 4.0 || m_scale <= 0.1)
            scale_inc = -scale_inc;
        if (m_shear >= 1.0 || m_shear <= -1.0)
            shear_inc = -shear_inc;
        setUpdatesEnabled(true);

        pts->firePointChange();
    }
}

#ifndef QT_NO_WHEELEVENT
void XFormView::wheelEvent(QWheelEvent *e)
{
    m_scale += e->delta() / qreal(600);
    m_scale = qMax(qreal(0.1), qMin(qreal(4), m_scale));
    emit scaleChanged(int(m_scale*1000));
}
#endif

void XFormView::reset()
{
    emit rotationChanged(0);
    emit scaleChanged(1000);
    emit shearChanged(0);
    ctrlPoints = QPolygonF();
    ctrlPoints << QPointF(250, 250) << QPointF(350, 250);
    pts->setPoints(ctrlPoints);
    pts->firePointChange();
}

void XFormView::drawPixmapType(QPainter *painter)
{
    QPointF center(m_pixmap.width() / qreal(2), m_pixmap.height() / qreal(2));
    painter->translate(ctrlPoints.at(0) - center);

    painter->translate(center);
    painter->rotate(m_rotation);
    painter->scale(m_scale, m_scale);
    painter->shear(0, m_shear);
    painter->translate(-center);

    painter->drawPixmap(QPointF(0, 0), m_pixmap);
    painter->setPen(QPen(QColor(255, 0, 0, alpha), 0.25, Qt::SolidLine, Qt::FlatCap, Qt::BevelJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(0, 0, m_pixmap.width(), m_pixmap.height()).adjusted(-2, -2, 2, 2));
}

void XFormView::drawTextType(QPainter *painter)
{
    QPainterPath path;
    QFont f("times new roman,utopia");
    f.setStyleStrategy(QFont::ForceOutline);
    f.setPointSize(72);
    f.setStyleHint(QFont::Times);
    path.addText(0, 0, f, m_text);

    QFontMetrics fm(f);
    QRectF br(fm.boundingRect(m_text));
    QPointF center(br.center());
    painter->translate(ctrlPoints.at(0) - center);

    painter->translate(center);
    painter->rotate(m_rotation);
    painter->scale(m_scale, m_scale);
    painter->shear(0, m_shear);
    painter->translate(-center);

    painter->fillPath(path, Qt::black);

    painter->setPen(QPen(QColor(255, 0, 0, alpha), 0.25, Qt::SolidLine, Qt::FlatCap, Qt::BevelJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(br.adjusted(-1, -1, 1, 1));
}

void XFormView::drawVectorType(QPainter *painter)
{
    QPainterPath path;
    painter->translate(ctrlPoints.at(0) - QPointF(250,250));

    painter->scale(0.77, 0.77);
    painter->translate(98.9154 + 30 , -217.691 - 20);

    QRect br(-55, 275, 500, 590);
    QPoint center = br.center();
    painter->translate(center.x(), center.y());
    painter->rotate(m_rotation);
    painter->scale(m_scale, m_scale);
    painter->shear(0, m_shear);
    painter->translate(-center.x(), -center.y());

    painter->setPen(Qt::NoPen);
    path.moveTo(120, 470);
    path.lineTo(60+245, 470);
    path.lineTo(60+245, 470+350);
    path.lineTo(60, 470+350);
    path.lineTo(60, 470+80);

    painter->setBrush(Qt::white);
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor( 193, 193, 191, 255));
    path.moveTo(329.336, 727.552);
    path.cubicTo(QPointF(315.224, 726.328), QPointF(304.136, 715.816), QPointF(303.128, 694.936));
    path.cubicTo(QPointF(306.368, 639.496), QPointF(309.608, 582.112), QPointF(271.232, 545.104));
    path.cubicTo(QPointF(265.256, 499.024), QPointF(244.016, 482.104), QPointF(234.008, 452.512));
    path.lineTo(218.24, 441.208);
    path.lineTo(237.104, 411.688);
    path.lineTo(245.168, 411.904);
    path.lineTo(323.936, 571.168);
    path.lineTo(340.424, 651.448);
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(136.232, 439.696);
    path.cubicTo(QPointF(133.856, 455.248), QPointF(132.56, 470.512), QPointF(134.792, 485.272));
    path.cubicTo(QPointF(118.376, 507.592), QPointF(105.92, 530.128), QPointF(104.48, 553.312));
    path.cubicTo(QPointF(92.024, 586.504), QPointF(62.432, 614.584), QPointF(67.544, 680.104));
    path.cubicTo(QPointF(84.176, 697.456), QPointF(107.432, 713.584), QPointF(127.376, 730.36));
    path.cubicTo(QPointF(152.432, 751.312), QPointF(137.528, 778.96), QPointF(102.248, 772.408));
    path.cubicTo(QPointF(94.4, 763.768), QPointF(76.616, 709.624), QPointF(42.92, 676.288));
    path.lineTo(49.544, 632.584);
    path.lineTo(81.368, 547.408);
    path.lineTo(120.968, 484.048);
    path.lineTo(125.36, 456.688);
    path.lineTo(119.816, 386.776);
    path.lineTo(124.424, 361.216);
    path.lineTo(136.232, 439.696);
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(115.64, 341.416);
    path.cubicTo(QPointF(116.576, 336.376), QPointF(117.8, 331.624), QPointF(119.312, 327.16));
    path.lineTo(121.688, 342.784);
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(120.968, 500.464);
    path.cubicTo(QPointF(108.368, 523.792), QPointF(103.976, 546.256), QPointF(132.92, 550.216));
    path.cubicTo(QPointF(117.008, 553.888), QPointF(97.208, 568.648), QPointF(77.192, 593.488));
    path.lineTo(77.624, 543.016);
    path.lineTo(101.456, 503.272);
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(-33.256, 818.488);
    path.cubicTo(QPointF(10.52, 838.144), QPointF(41.408, 837.064), QPointF(69.272, 850.96));
    path.cubicTo(QPointF(91.304, 862.552), QPointF(113.552, 861.184), QPointF(126.944, 847.144));
    path.cubicTo(QPointF(138.32, 832.456), QPointF(146.744, 831.736), QPointF(163.52, 830.224));
    path.cubicTo(QPointF(190.952, 828.568), QPointF(217.736, 828.28), QPointF(241.928, 830.8));
    path.lineTo(269.576, 833.032);
    path.cubicTo(QPointF(269.072, 864.064), QPointF(328.04, 867.88), QPointF(345.392, 844.336));
    path.cubicTo(QPointF(366.344, 819.424), QPointF(395.144, 808.264), QPointF(419.84, 790.192));
    path.lineTo(289.304, 725.536);
    path.cubicTo(QPointF(255.824, 806.464), QPointF(131.048, 827.632), QPointF(113.768, 763.264));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(286.424, 711.568);
    path.cubicTo(QPointF(273.824, 711.496), QPointF(260.936, 715.6), QPointF(261.944, 732.16));
    path.lineTo(266.192, 776.44);
    path.lineTo(304.424, 756.64);
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(0, 0, 0, 255));
    path.moveTo(-37.36, 821.224);
    path.cubicTo(QPointF(7.136, 840.88), QPointF(38.6, 839.728), QPointF(66.968, 853.696));
    path.cubicTo(QPointF(89.36, 865.216), QPointF(111.968, 863.92), QPointF(125.648, 849.808));
    path.cubicTo(QPointF(137.24, 835.192), QPointF(145.808, 834.472), QPointF(162.872, 832.96));
    path.cubicTo(QPointF(190.736, 831.232), QPointF(218.024, 831.016), QPointF(242.648, 833.464));
    path.lineTo(270.728, 835.768);
    path.cubicTo(QPointF(270.224, 866.8), QPointF(330.272, 870.544), QPointF(347.912, 847));
    path.cubicTo(QPointF(369.224, 822.088), QPointF(398.528, 811), QPointF(423.656, 792.856));
    path.lineTo(290.816, 728.272);
    path.cubicTo(QPointF(256.76, 809.128), QPointF(129.824, 830.296), QPointF(112.256, 766));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(183, 114, 0, 255));
    path.moveTo(382.328, 691.984);
    path.cubicTo(QPointF(403.64, 698.968), QPointF(389.888, 720.28), QPointF(400.76, 732.52));
    path.cubicTo(QPointF(405.44, 742.888), QPointF(415.304, 752.032), QPointF(431.792, 760.528));
    path.cubicTo(QPointF(459.368, 774.424), QPointF(426.248, 799.336), QPointF(392.768, 812.08));
    path.cubicTo(QPointF(351.944, 825.616), QPointF(344.024, 862.912), QPointF(299.312, 851.896));
    path.cubicTo(QPointF(283.112, 846.496), QPointF(278.36, 831.808), QPointF(278.864, 809.128));
    path.cubicTo(QPointF(284.264, 762.76), QPointF(277.784, 730.432), QPointF(278.792, 698.824));
    path.cubicTo(QPointF(278.72, 686.152), QPointF(283.544, 684.64), QPointF(307.232, 687.952));
    path.cubicTo(QPointF(310.04, 726.328), QPointF(352.376, 727.336), QPointF(382.328, 691.984));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(242, 183, 0, 255));
    path.moveTo(339.632, 826.624);
    path.cubicTo(QPointF(371.6, 814.312), QPointF(403.856, 798.112), QPointF(429.848, 782.128));
    path.cubicTo(QPointF(437.84, 777.448), QPointF(438.92, 765.928), QPointF(427.688, 762.328));
    path.cubicTo(QPointF(403.352, 748.504), QPointF(390.104, 731.224), QPointF(392.912, 708.76));
    path.cubicTo(QPointF(393.344, 700.912), QPointF(383.696, 692.56), QPointF(381.104, 700.048));
    path.cubicTo(QPointF(359.864, 771.472), QPointF(291.32, 767.656), QPointF(300.752, 696.952));
    path.cubicTo(QPointF(301.256, 694.864), QPointF(301.76, 692.776), QPointF(302.264, 690.76));
    path.cubicTo(QPointF(289.952, 688.24), QPointF(285.2, 690.976), QPointF(285.776, 700.408));
    path.lineTo(295.28, 806.608);
    path.cubicTo(QPointF(297.656, 830.8), QPointF(317.312, 836.128), QPointF(339.632, 826.624));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(0, 0, 0, 255));
    path.moveTo(354.464, 537.544);
    path.cubicTo(QPointF(379.16, 569.8), QPointF(404.432, 651.088), QPointF(384.416, 691.552));
    path.cubicTo(QPointF(360.944, 737.776), QPointF(307.808, 743.248), QPointF(305.504, 695.8));
    path.cubicTo(QPointF(308.816, 639.64), QPointF(311.984, 581.536), QPointF(273.68, 544.096));
    path.cubicTo(QPointF(267.704, 497.368), QPointF(246.392, 480.232), QPointF(236.384, 450.28));
    path.lineTo(203.12, 426.088);
    path.lineTo(133.568, 435.088);
    path.cubicTo(QPointF(130.76, 452.152), QPointF(129.104, 468.784), QPointF(131.552, 484.912));
    path.cubicTo(QPointF(115.064, 507.376), QPointF(102.608, 530.056), QPointF(101.168, 553.312));
    path.cubicTo(QPointF(88.712, 586.648), QPointF(59.12, 614.944), QPointF(64.232, 680.752));
    path.cubicTo(QPointF(80.864, 698.248), QPointF(104.12, 714.448), QPointF(124.064, 731.296));
    path.cubicTo(QPointF(149.12, 752.392), QPointF(135.512, 776.296), QPointF(100.232, 769.672));
    path.cubicTo(QPointF(78.848, 746.056), QPointF(56.744, 722.872), QPointF(35.288, 699.328));
    path.cubicTo(QPointF(12.392, 683.056), QPointF(3.896, 662.176), QPointF(27.368, 630.496));
    path.cubicTo(QPointF(43.424, 609.04), QPointF(47.96, 562.456), QPointF(62, 543.664));
    path.cubicTo(QPointF(74.312, 525.16), QPointF(92.24, 508.6), QPointF(105.272, 490.096));
    path.cubicTo(QPointF(112.184, 477.928), QPointF(114.344, 468.568), QPointF(113.264, 454.456));
    path.lineTo(110.312, 369.136);
    path.cubicTo(QPointF(108.368, 307.216), QPointF(142.424, 274.24), QPointF(189.8, 275.248));
    path.cubicTo(QPointF(243.512, 275.752), QPointF(287.576, 312.472), QPointF(288.152, 378.28));
    path.cubicTo(QPointF(292.688, 410.32), QPointF(283.256, 428.68), QPointF(308.672, 474.472));
    path.cubicTo(QPointF(334.52, 522.712), QPointF(338.552, 520.12), QPointF(354.464, 537.544));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(261.296, 503.632);
    path.lineTo(263.528, 512.2);
    path.cubicTo(QPointF(257.696, 501.688), QPointF(250.712, 483.616), QPointF(241.928, 475.696));
    path.cubicTo(QPointF(239.264, 473.536), QPointF(235.808, 473.608), QPointF(233.72, 475.624));
    path.cubicTo(QPointF(222.056, 486.928), QPointF(193.112, 510.112), QPointF(169.928, 507.088));
    path.cubicTo(QPointF(152.072, 505.288), QPointF(134.648, 493.264), QPointF(130.832, 480.232));
    path.cubicTo(QPointF(128.816, 470.872), QPointF(129.752, 463.168), QPointF(130.976, 455.32));
    path.lineTo(240.704, 453.52);
    path.cubicTo(QPointF(238.472, 463.168), QPointF(253.088, 487), QPointF(261.296, 503.632));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(143.144, 363.232);
    path.cubicTo(QPointF(154.088, 363.232), QPointF(163.88, 376.84), QPointF(163.808, 395.632));
    path.cubicTo(QPointF(163.736, 408.232), QPointF(155.528, 411.472), QPointF(149.336, 417.016));
    path.cubicTo(QPointF(146.6, 419.536), QPointF(145.952, 433.144), QPointF(142.568, 433.144));
    path.cubicTo(QPointF(131.696, 433.144), QPointF(123.488, 413.776), QPointF(123.488, 395.632));
    path.cubicTo(QPointF(123.488, 377.56), QPointF(132.272, 363.232), QPointF(143.144, 363.232));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(255, 255, 255, 255));
    path.moveTo(144.368, 375.04);
    path.cubicTo(QPointF(154.088, 375.04), QPointF(160.856, 379.936), QPointF(161.648, 391.312));
    path.cubicTo(QPointF(162.224, 399.16), QPointF(160.136, 411.76), QPointF(154.664, 414.424));
    path.cubicTo(QPointF(152.144, 415.648), QPointF(143.432, 426.664), QPointF(140.408, 426.52));
    path.cubicTo(QPointF(128.096, 425.944), QPointF(125, 402.112), QPointF(125.936, 390.736));
    path.cubicTo(QPointF(126.8, 379.36), QPointF(134.72, 375.04), QPointF(144.368, 375.04));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(0, 0, 0, 255));
    path.moveTo(141.848, 382.672);
    path.cubicTo(QPointF(148.544, 382.096), QPointF(154.736, 389.728), QPointF(155.6, 399.664));
    path.cubicTo(QPointF(156.464, 409.6), QPointF(151.64, 418.24), QPointF(144.944, 418.816));
    path.cubicTo(QPointF(138.248, 419.392), QPointF(132.056, 411.76), QPointF(131.192, 401.752));
    path.cubicTo(QPointF(130.328, 391.816), QPointF(135.152, 383.248), QPointF(141.848, 382.672));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(151.064, 397.288);
    path.cubicTo(QPointF(151.424, 399.088), QPointF(149.408, 400.024), QPointF(148.832, 398.224));
    path.cubicTo(QPointF(148.256, 395.992), QPointF(146.888, 393.328), QPointF(145.088, 391.168));
    path.cubicTo(QPointF(143.936, 389.872), QPointF(145.088, 388.432), QPointF(146.528, 389.44));
    path.cubicTo(QPointF(149.048, 391.528), QPointF(150.488, 394.12), QPointF(151.064, 397.288));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(216.944, 360.712);
    path.cubicTo(QPointF(232.712, 360.712), QPointF(245.6, 377.416), QPointF(245.6, 397.792));
    path.cubicTo(QPointF(245.6, 418.24), QPointF(232.712, 434.872), QPointF(216.944, 434.872));
    path.cubicTo(QPointF(201.176, 434.872), QPointF(188.432, 418.24), QPointF(188.432, 397.792));
    path.cubicTo(QPointF(188.432, 377.416), QPointF(201.176, 360.712), QPointF(216.944, 360.712));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(255, 255, 255, 255));
    path.moveTo(224.792, 374.968);
    path.cubicTo(QPointF(235.664, 378.856), QPointF(241.928, 387.424), QPointF(242.72, 396.568));
    path.cubicTo(QPointF(243.656, 407.08), QPointF(239.408, 418.96), QPointF(230.264, 425.944));
    path.cubicTo(QPointF(227.672, 427.888), QPointF(197.72, 416.08), QPointF(195.992, 411.616));
    path.cubicTo(QPointF(193.4, 405.208), QPointF(191.816, 392.896), QPointF(193.76, 385.624));
    path.cubicTo(QPointF(194.552, 382.744), QPointF(197.216, 378.568), QPointF(201.176, 376.336));
    path.cubicTo(QPointF(207.44, 372.808), QPointF(216.656, 372.088), QPointF(224.792, 374.968));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(0, 0, 0, 255));
    path.moveTo(216.872, 380.944);
    path.cubicTo(QPointF(225.584, 380.944), QPointF(232.712, 389.296), QPointF(232.712, 399.448));
    path.cubicTo(QPointF(232.712, 409.672), QPointF(225.584, 418.024), QPointF(216.872, 418.024));
    path.cubicTo(QPointF(208.16, 418.024), QPointF(201.032, 409.672), QPointF(201.032, 399.448));
    path.cubicTo(QPointF(201.032, 389.296), QPointF(208.16, 380.944), QPointF(216.872, 380.944));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(227.096, 392.392);
    path.cubicTo(QPointF(228.104, 394.048), QPointF(226.448, 395.776), QPointF(225.224, 394.12));
    path.cubicTo(QPointF(223.784, 392.104), QPointF(221.408, 389.944), QPointF(218.888, 388.432));
    path.cubicTo(QPointF(217.232, 387.568), QPointF(217.808, 385.624), QPointF(219.68, 386.2));
    path.cubicTo(QPointF(222.92, 387.28), QPointF(225.368, 389.368), QPointF(227.096, 392.392));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(183, 114, 0, 255));
    path.moveTo(164.96, 404.488);
    path.cubicTo(QPointF(172.376, 402.328), QPointF(184.112, 403.048), QPointF(192.248, 404.632));
    path.cubicTo(QPointF(200.384, 406.792), QPointF(222.056, 418.24), QPointF(245.024, 430.696));
    path.cubicTo(QPointF(247.976, 432.208), QPointF(248.84, 437.104), QPointF(245.024, 438.688));
    path.cubicTo(QPointF(239.12, 439.12), QPointF(249.272, 453.664), QPointF(238.904, 458.848));
    path.cubicTo(QPointF(223.352, 462.88), QPointF(198.44, 485.992), QPointF(186.128, 487.864));
    path.cubicTo(QPointF(179.288, 489.376), QPointF(172.232, 489.592), QPointF(164.6, 487.864));
    path.cubicTo(QPointF(140.552, 482.968), QPointF(134.216, 455.608), QPointF(122.912, 450.064));
    path.cubicTo(QPointF(119.816, 446.824), QPointF(121.4, 441.208), QPointF(122.408, 440.056));
    path.cubicTo(QPointF(123.632, 434.224), QPointF(149.696, 406.216), QPointF(164.96, 404.488));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(242, 183, 0, 255));
    path.moveTo(185.408, 405.856);
    path.cubicTo(QPointF(198.44, 407.296), QPointF(226.088, 423.928), QPointF(239.408, 430.624));
    path.cubicTo(QPointF(242.72, 432.424), QPointF(242.504, 437.824), QPointF(239.552, 438.688));
    path.cubicTo(QPointF(236.384, 440.488), QPointF(235.448, 438.256), QPointF(232.928, 437.896));
    path.cubicTo(QPointF(228.896, 435.736), QPointF(222.272, 440.92), QPointF(217.016, 444.88));
    path.cubicTo(QPointF(186.704, 467.776), QPointF(180.656, 465.256), QPointF(156.176, 462.664));
    path.cubicTo(QPointF(147.68, 460.576), QPointF(142.136, 457.984), QPointF(139.688, 455.968));
    path.cubicTo(QPointF(141.488, 445.888), QPointF(160.496, 407.656), QPointF(166.76, 406.792));
    path.cubicTo(QPointF(168.344, 404.704), QPointF(179.936, 404.632), QPointF(185.408, 405.856));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(183, 114, 0, 255));
    path.moveTo(190.664, 412.048);
    path.lineTo(193.76, 413.416);
    path.cubicTo(QPointF(196.064, 414.712), QPointF(193.256, 418.168), QPointF(190.736, 417.088));
    path.lineTo(186.2, 415.504);
    path.cubicTo(QPointF(183.536, 413.272), QPointF(186.704, 410.104), QPointF(190.664, 412.048));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(268.568, 452.368);
    path.cubicTo(QPointF(273.032, 454.384), QPointF(279.224, 457.192), QPointF(282.536, 460.144));
    path.cubicTo(QPointF(285.488, 464.104), QPointF(286.784, 468.064), QPointF(286.424, 472.024));
    path.cubicTo(QPointF(285.776, 474.544), QPointF(284.12, 476.344), QPointF(281.24, 477.424));
    path.cubicTo(QPointF(277.856, 478.216), QPointF(273.68, 477.424), QPointF(271.376, 474.112));
    path.cubicTo(QPointF(269.864, 471.448), QPointF(265.256, 462.16), QPointF(263.96, 460.576));
    path.cubicTo(QPointF(262.232, 457.12), QPointF(261.944, 454.456), QPointF(262.88, 452.368));
    path.cubicTo(QPointF(264.032, 451.288), QPointF(266.048, 451), QPointF(268.568, 452.368));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(255, 255, 255, 255));
    path.moveTo(273.752, 461.584);
    path.cubicTo(QPointF(275.48, 462.376), QPointF(277.928, 463.456), QPointF(279.224, 464.68));
    path.cubicTo(QPointF(280.376, 466.264), QPointF(280.88, 467.776), QPointF(280.736, 469.36));
    path.cubicTo(QPointF(280.52, 470.296), QPointF(279.8, 471.016), QPointF(278.72, 471.448));
    path.cubicTo(QPointF(277.352, 471.808), QPointF(275.768, 471.448), QPointF(274.832, 470.152));
    path.cubicTo(QPointF(274.256, 469.144), QPointF(272.456, 465.472), QPointF(271.952, 464.824));
    path.cubicTo(QPointF(271.232, 463.456), QPointF(271.088, 462.448), QPointF(271.448, 461.584));
    path.cubicTo(QPointF(271.952, 461.152), QPointF(272.744, 461.08), QPointF(273.752, 461.584));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(238.616, 358.552);
    path.cubicTo(QPointF(239.048, 359.2), QPointF(238.976, 359.776), QPointF(238.4, 360.28));
    path.cubicTo(QPointF(237.896, 360.784), QPointF(237.176, 360.712), QPointF(236.24, 360.208));
    path.lineTo(231.632, 356.248);
    path.cubicTo(QPointF(231.056, 355.744), QPointF(230.912, 354.952), QPointF(231.272, 354.088));
    path.cubicTo(QPointF(232.28, 353.44), QPointF(233.144, 353.44), QPointF(233.936, 354.088));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(235.592, 305.992);
    path.cubicTo(QPointF(239.624, 308.224), QPointF(240.848, 313.912), QPointF(238.184, 318.592));
    path.cubicTo(QPointF(235.592, 323.2), QPointF(230.12, 325.144), QPointF(226.016, 322.84));
    path.cubicTo(QPointF(221.984, 320.536), QPointF(220.76, 314.92), QPointF(223.424, 310.24));
    path.cubicTo(QPointF(226.016, 305.56), QPointF(231.488, 303.688), QPointF(235.592, 305.992));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(374.912, 680.536);
    path.cubicTo(QPointF(378.296, 683.128), QPointF(373.256, 687.376), QPointF(371.024, 686.296));
    path.cubicTo(QPointF(369.152, 685.648), QPointF(367.784, 683.488), QPointF(366.92, 682.408));
    path.cubicTo(QPointF(366.128, 681.184), QPointF(366.2, 679.168), QPointF(366.92, 678.448));
    path.cubicTo(QPointF(367.712, 677.44), QPointF(369.728, 677.656), QPointF(371.024, 678.52));
    path.cubicTo(QPointF(372.32, 679.168), QPointF(373.616, 679.888), QPointF(374.912, 680.536));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(297.44, 551.512);
    path.cubicTo(QPointF(338.984, 572.896), QPointF(350, 611.56), QPointF(332.072, 664.192));
    path.cubicTo(QPointF(330.992, 666.64), QPointF(334.16, 668.368), QPointF(335.24, 666.064));
    path.cubicTo(QPointF(354.824, 610.336), QPointF(341.432, 571.312), QPointF(299.024, 548.56));
    path.cubicTo(QPointF(296.864, 547.552), QPointF(295.28, 550.432), QPointF(297.44, 551.512));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(72.008, 569.512);
    path.cubicTo(QPointF(38.312, 627.256), QPointF(38.096, 662.68), QPointF(62.504, 681.328));
    path.cubicTo(QPointF(63.728, 682.264), QPointF(64.448, 680.032), QPointF(63.296, 679.168));
    path.cubicTo(QPointF(36.296, 655.48), QPointF(48.896, 615.52), QPointF(74.168, 570.88));
    path.cubicTo(QPointF(74.888, 569.584), QPointF(72.512, 568.432), QPointF(72.008, 569.512));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(289.376, 586.864);
    path.cubicTo(QPointF(289.232, 589.168), QPointF(288.368, 589.528), QPointF(286.424, 587.368));
    path.cubicTo(QPointF(279.8, 575.848), QPointF(235.088, 551.44), QPointF(213.344, 548.704));
    path.cubicTo(QPointF(209.24, 547.264), QPointF(209.456, 545.392), QPointF(213.488, 544.816));
    path.cubicTo(QPointF(229.184, 544.816), QPointF(241.28, 537.904), QPointF(254.96, 537.904));
    path.cubicTo(QPointF(258.704, 538.048), QPointF(262.304, 539.488), QPointF(264.392, 541.648));
    path.cubicTo(QPointF(269.504, 544.96), QPointF(288.08, 570.592), QPointF(289.376, 586.864));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(180.152, 546.832);
    path.cubicTo(QPointF(180.872, 550.792), QPointF(163.808, 545.68), QPointF(164.744, 556.696));
    path.cubicTo(QPointF(165.032, 559.72), QPointF(160.496, 561.376), QPointF(160.64, 556.696));
    path.cubicTo(QPointF(160.64, 548.272), QPointF(161.072, 548.416), QPointF(152.72, 546.832));
    path.cubicTo(QPointF(151.208, 546.76), QPointF(151.352, 544.528), QPointF(152.72, 544.816));
    path.lineTo(152.72, 544.816);
    path.cubicTo(QPointF(158.696, 546.472), QPointF(166.76, 542.872), QPointF(166.4, 538.84));
    path.cubicTo(QPointF(166.256, 537.472), QPointF(168.56, 537.688), QPointF(168.488, 538.84));
    path.cubicTo(QPointF(167.984, 545.248), QPointF(181.664, 542.152), QPointF(180.152, 546.832));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(193, 193, 191, 255));
    path.moveTo(151.568, 705.376);
    path.cubicTo(QPointF(151.64, 708.328), QPointF(148.76, 707.68), QPointF(148.544, 705.592));
    path.cubicTo(QPointF(140.192, 680.536), QPointF(143.72, 618.832), QPointF(151.856, 598.96));
    path.cubicTo(QPointF(152.432, 596.08), QPointF(156.248, 596.944), QPointF(155.744, 598.96));
    path.cubicTo(QPointF(147.104, 635.464), QPointF(147.248, 673.048), QPointF(151.568, 705.376));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(183, 114, 0, 255));
    path.moveTo(51.704, 684.424);
    path.cubicTo(QPointF(75.68, 707.824), QPointF(91.376, 743.248), QPointF(114.632, 775.288));
    path.cubicTo(QPointF(148.472, 816.04), QPointF(121.472, 858.304), QPointF(66.464, 845.56));
    path.cubicTo(QPointF(38.888, 835.192), QPointF(-0.784, 836.344), QPointF(-32.68, 825.832));
    path.cubicTo(QPointF(-55.072, 820.36), QPointF(-55.864, 809.272), QPointF(-44.416, 787.6));
    path.cubicTo(QPointF(-40.384, 773.776), QPointF(-40.024, 751.312), QPointF(-43.768, 732.592));
    path.cubicTo(QPointF(-45.784, 718.408), QPointF(-39.232, 710.488), QPointF(-24.112, 708.832));
    path.lineTo(-24.112, 708.832);
    path.cubicTo(QPointF(-11.296, 708.688), QPointF(6.56, 713.872), QPointF(16.28, 686.44));
    path.cubicTo(QPointF(23.552, 673.336), QPointF(40.976, 672.976), QPointF(51.704, 684.424));
    path.closeSubpath();
    painter->drawPath(path);
    path = QPainterPath();

    painter->setBrush(QColor(242, 183, 0, 255));
    path.moveTo(24.632, 699.04);
    path.cubicTo(QPointF(23.84, 680.968), QPointF(39.32, 677.296), QPointF(49.688, 688.312));
    path.cubicTo(QPointF(68.192, 710.992), QPointF(85.112, 736.048), QPointF(100.376, 764.992));
    path.cubicTo(QPointF(124.712, 804.16), QPointF(104.624, 842.68), QPointF(67.904, 828.064));
    path.cubicTo(QPointF(49.688, 817.84), QPointF(6.128, 813.304), QPointF(-17.344, 809.128));
    path.cubicTo(QPointF(-33.04, 807.832), QPointF(-35.128, 797.608), QPointF(-29.152, 791.848));
    path.cubicTo(QPointF(-20.944, 782.416), QPointF(-20.08, 759.808), QPointF(-27.856, 740.512));
    path.cubicTo(QPointF(-35.56, 728.56), QPointF(-21.088, 715.384), QPointF(-9.712, 720.856));
    path.cubicTo(QPointF(0.8, 727.048), QPointF(25.64, 713.08), QPointF(24.632, 699.04));
    path.closeSubpath();
    painter->drawPath(path);

    painter->setPen(QPen(QColor(255, 0, 0, alpha), 0.25, Qt::SolidLine, Qt::FlatCap, Qt::BevelJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(br.adjusted(-1, -1, 1, 1));
}


XFormWidget::XFormWidget(QWidget *parent)
    : QWidget(parent), textEditor(new QLineEdit)
{
    setWindowTitle(tr("Affine Transformations"));

    view = new XFormView(this);
    view->setMinimumSize(200, 200);

    QGroupBox *mainGroup = new QGroupBox(this);
    mainGroup->setFixedWidth(180);
    mainGroup->setTitle(tr("Affine Transformations"));

    QGroupBox *rotateGroup = new QGroupBox(mainGroup);
    rotateGroup->setTitle(tr("Rotate"));
    QSlider *rotateSlider = new QSlider(Qt::Horizontal, rotateGroup);
    rotateSlider->setRange(0, 3600);
    rotateSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGroupBox *scaleGroup = new QGroupBox(mainGroup);
    scaleGroup->setTitle(tr("Scale"));
    QSlider *scaleSlider = new QSlider(Qt::Horizontal, scaleGroup);
    scaleSlider->setRange(1, 4000);
    scaleSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGroupBox *shearGroup = new QGroupBox(mainGroup);
    shearGroup->setTitle(tr("Shear"));
    QSlider *shearSlider = new QSlider(Qt::Horizontal, shearGroup);
    shearSlider->setRange(-990, 990);
    shearSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QGroupBox *typeGroup = new QGroupBox(mainGroup);
    typeGroup->setTitle(tr("Type"));
    QRadioButton *vectorType = new QRadioButton(typeGroup);
    QRadioButton *pixmapType = new QRadioButton(typeGroup);
    QRadioButton *textType= new QRadioButton(typeGroup);
    vectorType->setText(tr("Vector Image"));
    pixmapType->setText(tr("Pixmap"));
    textType->setText(tr("Text"));

    QPushButton *resetButton = new QPushButton(mainGroup);
    resetButton->setText(tr("Reset Transform"));

    QPushButton *animateButton = new QPushButton(mainGroup);
    animateButton->setText(tr("Animate"));
    animateButton->setCheckable(true);

    QPushButton *showSourceButton = new QPushButton(mainGroup);
    showSourceButton->setText(tr("Show Source"));
#ifdef QT_OPENGL_SUPPORT
    QPushButton *enableOpenGLButton = new QPushButton(mainGroup);
    enableOpenGLButton->setText(tr("Use OpenGL"));
    enableOpenGLButton->setCheckable(true);
    enableOpenGLButton->setChecked(view->usesOpenGL());
    if (!QGLFormat::hasOpenGL())
        enableOpenGLButton->hide();
#endif
    QPushButton *whatsThisButton = new QPushButton(mainGroup);
    whatsThisButton->setText(tr("What's This?"));
    whatsThisButton->setCheckable(true);

    QHBoxLayout *viewLayout = new QHBoxLayout(this);
    viewLayout->addWidget(view);
    viewLayout->addWidget(mainGroup);

    QVBoxLayout *rotateGroupLayout = new QVBoxLayout(rotateGroup);
    rotateGroupLayout->addWidget(rotateSlider);

    QVBoxLayout *scaleGroupLayout = new QVBoxLayout(scaleGroup);
    scaleGroupLayout->addWidget(scaleSlider);

    QVBoxLayout *shearGroupLayout = new QVBoxLayout(shearGroup);
    shearGroupLayout->addWidget(shearSlider);

    QVBoxLayout *typeGroupLayout = new QVBoxLayout(typeGroup);
    typeGroupLayout->addWidget(vectorType);
    typeGroupLayout->addWidget(pixmapType);
    typeGroupLayout->addWidget(textType);
    typeGroupLayout->addSpacing(4);
    typeGroupLayout->addWidget(textEditor);

    QVBoxLayout *mainGroupLayout = new QVBoxLayout(mainGroup);
    mainGroupLayout->addWidget(rotateGroup);
    mainGroupLayout->addWidget(scaleGroup);
    mainGroupLayout->addWidget(shearGroup);
    mainGroupLayout->addWidget(typeGroup);
    mainGroupLayout->addStretch(1);
    mainGroupLayout->addWidget(resetButton);
    mainGroupLayout->addWidget(animateButton);
    mainGroupLayout->addWidget(showSourceButton);
#ifdef QT_OPENGL_SUPPORT
    mainGroupLayout->addWidget(enableOpenGLButton);
#endif
    mainGroupLayout->addWidget(whatsThisButton);

    connect(rotateSlider, SIGNAL(valueChanged(int)), view, SLOT(changeRotation(int)));
    connect(shearSlider, SIGNAL(valueChanged(int)), view, SLOT(changeShear(int)));
    connect(scaleSlider, SIGNAL(valueChanged(int)), view, SLOT(changeScale(int)));

    connect(vectorType, SIGNAL(clicked()), view, SLOT(setVectorType()));
    connect(pixmapType, SIGNAL(clicked()), view, SLOT(setPixmapType()));
    connect(textType, SIGNAL(clicked()), view, SLOT(setTextType()));
    connect(textType, SIGNAL(toggled(bool)), textEditor, SLOT(setEnabled(bool)));
    connect(textEditor, SIGNAL(textChanged(QString)), view, SLOT(setText(QString)));

    connect(view, SIGNAL(rotationChanged(int)), rotateSlider, SLOT(setValue(int)));
    connect(view, SIGNAL(scaleChanged(int)), scaleSlider, SLOT(setValue(int)));
    connect(view, SIGNAL(shearChanged(int)), shearSlider, SLOT(setValue(int)));

    connect(resetButton, SIGNAL(clicked()), view, SLOT(reset()));
    connect(animateButton, SIGNAL(clicked(bool)), view, SLOT(setAnimation(bool)));
    connect(whatsThisButton, SIGNAL(clicked(bool)), view, SLOT(setDescriptionEnabled(bool)));
    connect(whatsThisButton, SIGNAL(clicked(bool)), view->hoverPoints(), SLOT(setDisabled(bool)));
    connect(view, SIGNAL(descriptionEnabledChanged(bool)), view->hoverPoints(), SLOT(setDisabled(bool)));
    connect(view, SIGNAL(descriptionEnabledChanged(bool)), whatsThisButton, SLOT(setChecked(bool)));
    connect(showSourceButton, SIGNAL(clicked()), view, SLOT(showSource()));
#ifdef QT_OPENGL_SUPPORT
    connect(enableOpenGLButton, SIGNAL(clicked(bool)), view, SLOT(enableOpenGL(bool)));
#endif
    view->loadSourceFile(":res/affine/xform.cpp");
    view->loadDescription(":res/affine/xform.html");

    // defaults
    view->reset();
    vectorType->setChecked(true);
    textEditor->setText("Qt Affine Transformation Example");
    textEditor->setEnabled(false);

    animateButton->animateClick();
}
