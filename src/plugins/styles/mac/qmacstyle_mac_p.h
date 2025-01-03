// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMACSTYLE_MAC_P_H
#define QMACSTYLE_MAC_P_H

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
#include <QtWidgets/qcommonstyle.h>

QT_BEGIN_NAMESPACE

class QPalette;

class QPushButton;
class QStyleOptionButton;
class QMacStylePrivate;
class QMacStyle : public QCommonStyle
{
    Q_OBJECT
public:
    QMacStyle();
    virtual ~QMacStyle();

    void polish(QWidget *w);
    void unpolish(QWidget *w);

    void polish(QApplication*);
    void unpolish(QApplication*);

    void polish(QPalette &pal);

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = nullptr) const;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = nullptr) const;
    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget = nullptr) const;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = nullptr) const;
    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                               const QPoint &pt, const QWidget *w = nullptr) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                         const QWidget *w = nullptr) const;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *w = nullptr) const;

    int pixelMetric(PixelMetric pm, const QStyleOption *opt = 0, const QWidget *widget = nullptr) const;

    QPalette standardPalette() const;

    virtual int styleHint(StyleHint sh, const QStyleOption *opt = 0, const QWidget *w = nullptr,
                          QStyleHintReturn *shret = nullptr) const;

    QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *opt,
                           const QWidget *widget = nullptr) const;

    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                const QStyleOption *opt) const;

    virtual void drawItemText(QPainter *p, const QRect &r, int flags, const QPalette &pal,
                              bool enabled, const QString &text, QPalette::ColorRole textRole  = QPalette::NoRole) const;

    bool event(QEvent *e);

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *opt = nullptr,
                       const QWidget *widget = nullptr) const;
    int layoutSpacing(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2,
                      Qt::Orientation orientation, const QStyleOption *option = nullptr,
                      const QWidget *widget = nullptr) const;

private:
    Q_DISABLE_COPY_MOVE(QMacStyle)
    Q_DECLARE_PRIVATE(QMacStyle)
};

QT_END_NAMESPACE

#endif // QMACSTYLE_MAC_P_H
