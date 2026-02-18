// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMACSTYLE_P_H
#define QMACSTYLE_P_H

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

#include <QtGui/qtguiglobal.h>

#if defined(QT_WIDGETS_LIB) && defined(QT_QUICK_LIB)
#  error "Cannot use QtGui Mac style with both Widgets and Quick"
#endif

#if defined(QT_WIDGETS_LIB)
#  define OPTIONAL_WIDGET_ARGUMENT , const QWidget *w = nullptr
#  define FORWARD_OPTIONAL_WIDGET_ARGUMENT , w
# else
#  define OPTIONAL_WIDGET_ARGUMENT
#  define FORWARD_OPTIONAL_WIDGET_ARGUMENT
#endif

#include <AppKit/NSApplication.h>

QT_BEGIN_NAMESPACE

class QPainter;

/*
    Helper class to ensure that the Mac style in Widgets and Quick
    draw their NSViews and NSCells with the correct appearance,
    as the native controls use NSAppearance.currentDrawingAppearance
    instead of NSApp.effectiveAppearance when drawing.

    Due to the duplicated class hierarchies between Widgets and Quick
    for the styles, with the Quick styles missing the QWidget pointer
    in the function parameters, we have to opt for an awkward macro
    to solve this.
*/
template <typename Style, typename StyleOption, typename StyleOptionComplex>
class QMacApperanceStyle : public Style
{
public:
    void drawPrimitive(typename Style::PrimitiveElement pe, const StyleOption *opt, QPainter *p
                       OPTIONAL_WIDGET_ARGUMENT) const override
    {
        [NSApp.effectiveAppearance performAsCurrentDrawingAppearance:^{
            Style::drawPrimitive(pe, opt, p
                FORWARD_OPTIONAL_WIDGET_ARGUMENT);
        }];
    }

    void drawControl(typename Style::ControlElement element, const StyleOption *opt, QPainter *p
                     OPTIONAL_WIDGET_ARGUMENT) const override
    {
        [NSApp.effectiveAppearance performAsCurrentDrawingAppearance:^{
            Style::drawControl(element, opt, p
                FORWARD_OPTIONAL_WIDGET_ARGUMENT);
        }];
    }

    void drawComplexControl(typename Style::ComplexControl cc, const StyleOptionComplex *opt, QPainter *p
                            OPTIONAL_WIDGET_ARGUMENT) const override
    {
        [NSApp.effectiveAppearance performAsCurrentDrawingAppearance:^{
            Style::drawComplexControl(cc, opt, p
                FORWARD_OPTIONAL_WIDGET_ARGUMENT);
        }];
    }
};

QT_END_NAMESPACE

#endif // QMACSTYLE_P_H
