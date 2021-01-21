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

#ifndef QRUBBERBAND_H
#define QRUBBERBAND_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>

QT_REQUIRE_CONFIG(rubberband);

QT_BEGIN_NAMESPACE

class QRubberBandPrivate;
class QStyleOptionRubberBand;

class Q_WIDGETS_EXPORT QRubberBand : public QWidget
{
    Q_OBJECT

public:
    enum Shape { Line, Rectangle };
    explicit QRubberBand(Shape, QWidget * = nullptr);
    ~QRubberBand();

    Shape shape() const;

    void setGeometry(const QRect &r);

    inline void setGeometry(int x, int y, int w, int h);
    inline void move(int x, int y);
    inline void move(const QPoint &p)
    { move(p.x(), p.y()); }
    inline void resize(int w, int h)
    { setGeometry(geometry().x(), geometry().y(), w, h); }
    inline void resize(const QSize &s)
    { resize(s.width(), s.height()); }

protected:
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *) override;
    void changeEvent(QEvent *) override;
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void initStyleOption(QStyleOptionRubberBand *option) const;

private:
    Q_DECLARE_PRIVATE(QRubberBand)
};

inline void QRubberBand::setGeometry(int ax, int ay, int aw, int ah)
{ setGeometry(QRect(ax, ay, aw, ah)); }
inline void QRubberBand::move(int ax, int ay)
{ setGeometry(ax, ay, width(), height()); }

QT_END_NAMESPACE

#endif // QRUBBERBAND_H
