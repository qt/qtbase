/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
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

#include <QtGui>
#include <QSvgRenderer>
#include <QGraphicsEffect>

#include "iconitem.h"

IconItem::IconItem(const QString &filename, QGraphicsItem *parent)
  : GvbWidget(parent)
  , m_filename(filename)
  , m_rotation(0.0)
  , m_opacityEffect(0)
  , m_smoothTransformation(false)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setContentsMargins(0,0,0,0);
    setPreferredSize(58,58);
}

IconItem::~IconItem()
{
}

void IconItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    reload();

    const QPointF c = boundingRect().center();
    painter->translate(c.x(), c.y());
    painter->rotate(m_rotation);
    painter->translate(-c.x(), -c.y());

    if (m_smoothTransformation)
        painter->setRenderHints(QPainter::SmoothPixmapTransform);

    painter->drawPixmap(0,0, m_pixmap);
}

QSizeF IconItem::sizeHint(Qt::SizeHint which,
    const QSizeF &constraint) const
{
    switch (which)
    {
    case Qt::MinimumSize:
    case Qt::PreferredSize:
    case Qt::MaximumSize:
        return m_pixmap.size();

    default:
        return GvbWidget::sizeHint(which, constraint);
    }
}

void IconItem::reload()
{
    const QSize iconSize = size().toSize();
    if (iconSize.width() == 0 || iconSize.height()  == 0)
        return;

    const QString key = m_filename+QString::number(iconSize.width())+QString::number(iconSize.height());
    if (QPixmapCache::find(key, m_pixmap))
        return;

    if (m_filename.endsWith(".svg", Qt::CaseInsensitive))
    {
        m_pixmap = QPixmap(iconSize);
        m_pixmap.fill(Qt::transparent);
        QSvgRenderer doc(m_filename);
        QPainter painter(&m_pixmap);
        painter.setViewport(0, 0, iconSize.width(), iconSize.height());
        doc.render(&painter);
    }
    else
    {
        m_pixmap = QPixmap(m_filename).scaled(iconSize);
    }

    QPixmapCache::insert(key, m_pixmap);
    updateGeometry();
}

QString IconItem::fileName() const
{
    return m_filename;
}

void IconItem::setFileName(const QString &filename)
{
    if( m_filename != filename) {
        m_filename = filename;
        reload();
    }
}

void IconItem::setOpacityEffectEnabled(const bool enable)
{
    if (!m_opacityEffect)
    {
        QRadialGradient gradient(0.5, 0.5, 1.0);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setColorAt(0.0, QColor(0,0,0, 255));
        gradient.setColorAt(0.46, QColor(0,0,0, 255));
        gradient.setColorAt(0.62, QColor(0,0,0, 0));

        m_opacityEffect = new QGraphicsOpacityEffect;
        m_opacityEffect->setOpacityMask(gradient);
        m_opacityEffect->setOpacity(1.0);
        this->setGraphicsEffect(m_opacityEffect);
    }
    m_opacityEffect->setEnabled(enable);
}

bool IconItem::isOpacityEffectEnabled() const
{
    if (m_opacityEffect)
        return m_opacityEffect->isEnabled();

    return false;
}
