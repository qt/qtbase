/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QPIXMAPFILTER_H
#define QPIXMAPFILTER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtCore/qnamespace.h>
#include <QtGui/qpixmap.h>
#include <QtWidgets/qgraphicseffect.h>

QT_REQUIRE_CONFIG(graphicseffect);

QT_BEGIN_NAMESPACE

class QPainter;
class QPlatformPixmap;

class QPixmapFilterPrivate;

class Q_WIDGETS_EXPORT QPixmapFilter : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapFilter)
public:
    virtual ~QPixmapFilter() = 0;

    enum FilterType {
        ConvolutionFilter,
        ColorizeFilter,
        DropShadowFilter,
        BlurFilter,

        UserFilter = 1024
    };

    FilterType type() const;

    virtual QRectF boundingRectFor(const QRectF &rect) const;

    virtual void draw(QPainter *painter, const QPointF &p, const QPixmap &src, const QRectF &srcRect = QRectF()) const = 0;

protected:
    QPixmapFilter(QPixmapFilterPrivate &d, FilterType type, QObject *parent);
    QPixmapFilter(FilterType type, QObject *parent);
};

class QPixmapConvolutionFilterPrivate;

class Q_WIDGETS_EXPORT QPixmapConvolutionFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapConvolutionFilter)

public:
    QPixmapConvolutionFilter(QObject *parent = nullptr);
    ~QPixmapConvolutionFilter();

    void setConvolutionKernel(const qreal *matrix, int rows, int columns);

    QRectF boundingRectFor(const QRectF &rect) const override;
    void draw(QPainter *painter, const QPointF &dest, const QPixmap &src, const QRectF &srcRect = QRectF()) const override;

private:
    friend class QGLPixmapConvolutionFilter;
    friend class QVGPixmapConvolutionFilter;
    const qreal *convolutionKernel() const;
    int rows() const;
    int columns() const;
};

class QPixmapBlurFilterPrivate;

class Q_WIDGETS_EXPORT QPixmapBlurFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapBlurFilter)

public:
    QPixmapBlurFilter(QObject *parent = nullptr);
    ~QPixmapBlurFilter();

    void setRadius(qreal radius);
    void setBlurHints(QGraphicsBlurEffect::BlurHints hints);

    qreal radius() const;
    QGraphicsBlurEffect::BlurHints blurHints() const;

    QRectF boundingRectFor(const QRectF &rect) const override;
    void draw(QPainter *painter, const QPointF &dest, const QPixmap &src, const QRectF &srcRect = QRectF()) const override;

private:
    friend class QGLPixmapBlurFilter;
};

class QPixmapColorizeFilterPrivate;

class Q_WIDGETS_EXPORT QPixmapColorizeFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapColorizeFilter)

public:
    QPixmapColorizeFilter(QObject *parent = nullptr);
    ~QPixmapColorizeFilter();

    void setColor(const QColor& color);
    QColor color() const;

    void setStrength(qreal strength);
    qreal strength() const;

    void draw(QPainter *painter, const QPointF &dest, const QPixmap &src, const QRectF &srcRect = QRectF()) const override;
};

class QPixmapDropShadowFilterPrivate;

class Q_WIDGETS_EXPORT QPixmapDropShadowFilter : public QPixmapFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPixmapDropShadowFilter)

public:
    QPixmapDropShadowFilter(QObject *parent = nullptr);
    ~QPixmapDropShadowFilter();

    QRectF boundingRectFor(const QRectF &rect) const override;
    void draw(QPainter *p, const QPointF &pos, const QPixmap &px, const QRectF &src = QRectF()) const override;

    qreal blurRadius() const;
    void setBlurRadius(qreal radius);

    QColor color() const;
    void setColor(const QColor &color);

    QPointF offset() const;
    void setOffset(const QPointF &offset);
    inline void setOffset(qreal dx, qreal dy) { setOffset(QPointF(dx, dy)); }
};

QT_END_NAMESPACE

#endif // QPIXMAPFILTER_H
