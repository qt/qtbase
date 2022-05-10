// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSXPSTYLE_P_H
#define QWINDOWSXPSTYLE_P_H

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
#include <QtWidgets/private/qwindowsstyle_p.h>

QT_BEGIN_NAMESPACE

class QWindowsXPStylePrivate;
class QWindowsXPStyle : public QWindowsStyle
{
    Q_OBJECT
public:
    ~QWindowsXPStyle() override;

    void polish(QWidget*) override;
    void polish(QPalette&) override;
    void unpolish(QWidget*) override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *option, QPainter *p,
                       const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *p,
                     const QWidget *wwidget = nullptr) const override;
    QRect subElementRect(SubElement r, const QStyleOption *option,
                         const QWidget *widget = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *option, SubControl sc,
                         const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option, QPainter *p,
                            const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *option, const QSize &contentsSize,
                           const QWidget *widget = nullptr) const override;
    int pixelMetric(PixelMetric pm, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;

    QPalette standardPalette() const override;
    QPixmap standardPixmap(StandardPixmap standardIcon, const QStyleOption *option,
                           const QWidget *widget = nullptr) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;

protected:
    QWindowsXPStyle(QWindowsXPStylePrivate &dd);

private:
    Q_DISABLE_COPY_MOVE(QWindowsXPStyle)
    Q_DECLARE_PRIVATE(QWindowsXPStyle)
    friend class QStyleFactory;
};

QT_END_NAMESPACE

#endif // QWINDOWSXPSTYLE_P_H
