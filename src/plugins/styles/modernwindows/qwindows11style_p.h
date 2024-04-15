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
class QWindows11Style;

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
    Q_DISABLE_COPY_MOVE(QWindows11Style)
    Q_DECLARE_PRIVATE(QWindows11Style)
    friend class QStyleFactory;

    bool highContrastTheme = false;
    int colorSchemeIndex = 0;
    const QFont assetFont = QFont("Segoe Fluent Icons"); //Font to load icons from
};

class QWindows11StylePrivate : public QWindowsVistaStylePrivate {
    Q_DECLARE_PUBLIC(QWindows11Style)
    QPalette defaultPalette;
};

QT_END_NAMESPACE

#endif // QWINDOWS11STYLE_P_H
