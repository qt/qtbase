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

#ifndef QPROXYSTYLE_H
#define QPROXYSTYLE_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/QCommonStyle>

QT_BEGIN_NAMESPACE


#if !defined(QT_NO_STYLE_PROXY)

class QProxyStylePrivate;
class Q_WIDGETS_EXPORT QProxyStyle : public QCommonStyle
{
    Q_OBJECT

public:
    QProxyStyle(QStyle *style = nullptr);
    QProxyStyle(const QString &key);
    ~QProxyStyle();

    QStyle *baseStyle() const;
    void setBaseStyle(QStyle *style);

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled,
                      const QString &text, QPalette::ColorRole textRole = QPalette::NoRole) const override;
    virtual void drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const override;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const override;

    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const override;
    QRect itemTextRect(const QFontMetrics &fm, const QRect &r, int flags, bool enabled, const QString &text) const override;
    QRect itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const override;

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option, const QPoint &pos, const QWidget *widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                      Qt::Orientation orientation, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt, const QWidget *widget = nullptr) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const override;
    QPalette standardPalette() const override;

    void polish(QWidget *widget) override;
    void polish(QPalette &pal) override;
    void polish(QApplication *app) override;

    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *app) override;

protected:
    bool event(QEvent *e) override;

private:
    Q_DISABLE_COPY(QProxyStyle)
    Q_DECLARE_PRIVATE(QProxyStyle)
};

#endif // QT_NO_STYLE_PROXY

QT_END_NAMESPACE

#endif // QPROXYSTYLE_H
