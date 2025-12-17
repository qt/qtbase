// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

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
    frameColorStrong,                 //Color of frame around Checkbox and Radiobuttons (normal and hover)
    frameColorStrongDisabled,         //Color of frame around Checkbox and Radiobuttons (pressed and disabled)
    controlStrongFill,                //Color of controls with strong filling such as the right side of a slider
    controlStrokeSecondary,
    controlStrokePrimary,
    menuPanelFill,                    //Color of menu panel
    controlStrokeOnAccentSecondary,   //Color of frame around Buttons in accent color
    controlFillSolid,                 //Color for solid fill
    surfaceStroke,                    //Color of MDI window frames
    focusFrameInnerStroke,
    focusFrameOuterStroke,
    fillControlDefault,               // button default color (alpha)
    fillControlSecondary,             // button hover color (alpha)
    fillControlTertiary,              // button pressed color (alpha)
    fillControlDisabled,              // button disabled color (alpha)
    fillControlInputActive,           // input active
    fillControlAltSecondary,          // checkbox/RadioButton default color (alpha)
    fillControlAltTertiary,           // checkbox/RadioButton hover color (alpha)
    fillControlAltQuarternary,        // checkbox/RadioButton pressed color (alpha)
    fillControlAltDisabled,           // checkbox/RadioButton disabled color (alpha)
    fillAccentDefault,                // button default color (alpha)
    fillAccentSecondary,              // button hover color (alpha)
    fillAccentTertiary,               // button pressed color (alpha)
    fillAccentDisabled,               // button disabled color (alpha)
    fillMicaAltDefault,               // tabbar button default
    fillMicaAltTransparent,           // tabbar button (not selected, not hovered)
    fillMicaAltSecondary,             // tabbar button (not selected, hovered)
    textPrimary,                      // text of default/hovered control
    textSecondary,                    // text of pressed control
    textDisabled,                     // text of disabled control
    textOnAccentPrimary,              // text of default/hovered control on accent color
    textOnAccentSecondary,            // text of pressed control on accent color
    textOnAccentDisabled,             // text of disabled control on accent color
    dividerStrokeDefault,             // divider color (alpha)
};

class QWindows11Style : public QWindowsVistaStyle
{
    Q_OBJECT
public:
    enum class Icon : ushort {
        AcceptMedium = 0xF78C,
        Dash12 = 0xE629,
        CheckMark = 0xE73E,
        CaretLeftSolid8 = 0xEDD9,
        CaretRightSolid8 = 0xEDDA,
        CaretUpSolid8 = 0xEDDB,
        CaretDownSolid8 = 0xEDDC,
        ChevronDown = 0xE70D,
        ChevronUp = 0xE70E,
        ChevronUpMed = 0xE971,
        ChevronDownMed = 0xE972,
        ChevronLeftMed = 0xE973,
        ChevronRightMed = 0xE974,
        ChevronUpSmall = 0xE96D,
        ChevronDownSmall = 0xE96E,
        ChromeMinimize = 0xE921,
        ChromeMaximize = 0xE922,
        ChromeRestore = 0xE923,
        ChromeClose = 0xE8BB,
        More = 0xE712,
        Help = 0xE897,
        Clear = 0xE894,
    };

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
    QPixmap standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option = nullptr,
                           const QWidget *widget = nullptr) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;
    void polish(QApplication *app) override;
    void unpolish(QApplication *app) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    QWindows11Style(QWindows11StylePrivate &dd);

private:
    void dwmSetWindowCornerPreference(const QWidget *widget, bool bSet) const;
    QColor calculateAccentColor(const QStyleOption *option) const;
    QPen borderPenControlAlt(const QStyleOption *option) const;
    enum class ControlType
    {
        Control,
        ControlAlt
    };
    QBrush controlFillBrush(const QStyleOption *option, ControlType controlType) const;
    QBrush inputFillBrush(const QStyleOption *option, const QWidget *widget) const;
    // ControlType::ControlAlt can be mapped to QPalette directly
    QColor controlTextColor(const QStyleOption *option, bool ignoreIsChecked = false) const;
    void drawLineEditFrame(QPainter *p, const QRectF &rect, const QStyleOption *o, bool isEditable = true) const;
    inline QColor winUI3Color(enum WINUI3Color col) const;
    static inline QString fluentIcon(Icon i) { return QChar(ushort(i)); }

private:
    Q_DISABLE_COPY_MOVE(QWindows11Style)
    Q_DECLARE_PRIVATE(QWindows11Style)
    friend class QStyleFactory;

    bool highContrastTheme = false;
    int colorSchemeIndex = 0;

    mutable QVarLengthFlatMap<int, int, 8> m_fontPoint2ChevronDownMedWidth;
};

class QWindows11StylePrivate : public QWindowsVistaStylePrivate {
    Q_DECLARE_PUBLIC(QWindows11Style)

    QWindows11StylePrivate();

protected:
    QVarLengthFlatMap<QWindows11Style::Icon, QIcon, 16> m_standardIcons;
    bool nativeRoundedTopLevelWindows;
};

QT_END_NAMESPACE

#endif // QWINDOWS11STYLE_P_H
