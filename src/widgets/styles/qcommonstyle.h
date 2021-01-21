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

#ifndef QCOMMONSTYLE_H
#define QCOMMONSTYLE_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qstyle.h>

QT_BEGIN_NAMESPACE

class QCommonStylePrivate;

class Q_WIDGETS_EXPORT QCommonStyle: public QStyle
{
    Q_OBJECT

public:
    QCommonStyle();
    ~QCommonStyle();

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = nullptr) const override;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = nullptr) const override;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                     const QPoint &pt, const QWidget *w = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                         const QWidget *w = nullptr) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = nullptr) const override;

    int pixelMetric(PixelMetric m, const QStyleOption *opt = nullptr, const QWidget *widget = nullptr) const override;

    int styleHint(StyleHint sh, const QStyleOption *opt = nullptr, const QWidget *w = nullptr,
                  QStyleHintReturn *shret = nullptr) const override;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *opt = nullptr,
                       const QWidget *widget = nullptr) const override;
    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *opt = nullptr,
                           const QWidget *widget = nullptr) const override;

    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *opt) const override;
    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                      Qt::Orientation orientation, const QStyleOption *option = nullptr,
                      const QWidget *widget = nullptr) const override;

    void polish(QPalette &) override;
    void polish(QApplication *app) override;
    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;
    void unpolish(QApplication *application) override;

protected:
    QCommonStyle(QCommonStylePrivate &dd);

private:
    Q_DECLARE_PRIVATE(QCommonStyle)
    Q_DISABLE_COPY(QCommonStyle)
#if QT_CONFIG(animation)
    Q_PRIVATE_SLOT(d_func(), void _q_removeAnimation())
#endif
};

QT_END_NAMESPACE

#endif // QCOMMONSTYLE_H
