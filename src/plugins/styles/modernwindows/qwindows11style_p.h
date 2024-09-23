// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWS11STYLE_P_H
#define QWINDOWS11STYLE_P_H

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
#include <qwindowsvistastyle_p_p.h>

QT_BEGIN_NAMESPACE

class QWindows11StylePrivate;

enum WINUI3Color {
    subtleHighlightColor,             //Subtle highlight based on alpha used for hovered elements
    subtlePressedColor,               //Subtle highlight based on alpha used for pressed elements
    frameColorLight,                  //Color of frame around flyouts and controls except for Checkbox and Radiobutton
    frameColorStrong,                 //Color of frame around Checkbox and Radiobuttons
    controlStrongFill,                //Color of controls with strong filling such as the right side of a slider
    controlStrokeSecondary,
    controlStrokePrimary,
    controlFillTertiary,              //Color of filled sunken controls
    controlFillSecondary,             //Color of filled hovered controls
    menuPanelFill,                    //Color of menu panel
    textOnAccentPrimary,              //Color of text on controls filled in accent color
    textOnAccentSecondary,            //Color of text of sunken controls in accent color
    controlTextSecondary,             //Color of text of sunken controls
    controlStrokeOnAccentSecondary,   //Color of frame around Buttons in accent color
    controlFillSolid,                 //Color for solid fill
    surfaceStroke,                    //Color of MDI window frames
    controlAccentDisabled,
    textAccentDisabled,
    focusFrameInnerStroke,
    focusFrameOuterStroke
};

class QWindows11Style : public QWindowsVistaStyle
{
    Q_OBJECT
public:
    QWindows11Style();
    ~QWindows11Style() override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                  QPainter *painter, const QWidget *widget) const override;
    QRect subElementRect(QStyle::SubElement element, const QStyleOption *option,
                   const QWidget *widget = nullptr) const override;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                         SubControl subControl, const QWidget *widget) const override;
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override;
    int styleHint(StyleHint hint, const QStyleOption *opt = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;
    void polish(QWidget* widget) override;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size, const QWidget *widget) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;
    void polish(QPalette &pal) override;
    void unpolish(QWidget *widget) override;
protected:
    QWindows11Style(QWindows11StylePrivate &dd);

private:
    static inline QBrush buttonFillBrush(const QStyleOption *option);
    QColor buttonLabelColor(const QStyleOption *option) const;
    void drawLineEditFrame(QPainter *p, const QRectF &rect, const QStyleOption *o, bool isEditable = true) const;
    inline QColor winUI3Color(enum WINUI3Color col) const;

private:
    Q_DISABLE_COPY_MOVE(QWindows11Style)
    Q_DECLARE_PRIVATE(QWindows11Style)
    friend class QStyleFactory;

    bool highContrastTheme = false;
    int colorSchemeIndex = 0;
    const QFont assetFont = QFont("Segoe Fluent Icons"); //Font to load icons from
};

class QWindows11StylePrivate : public QWindowsVistaStylePrivate {
    Q_DECLARE_PUBLIC(QWindows11Style)
};

QT_END_NAMESPACE

#endif // QWINDOWS11STYLE_P_H
