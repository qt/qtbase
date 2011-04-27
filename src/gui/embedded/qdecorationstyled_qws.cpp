/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qapplication.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qdrawutil.h>
#include "qdecorationstyled_qws.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qpaintengine.h"

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_QWS_DECORATION_STYLED) || defined(QT_PLUGIN)

QDecorationStyled::QDecorationStyled()
    : QDecorationDefault()
{
}

QDecorationStyled::~QDecorationStyled()
{
}

int QDecorationStyled::titleBarHeight(const QWidget *widget)
{
    QStyleOptionTitleBar opt;
    opt.subControls = QStyle::SC_TitleBarLabel
                      | QStyle::SC_TitleBarSysMenu
                      | QStyle::SC_TitleBarNormalButton
                      | QStyle::SC_TitleBarContextHelpButton
                      | QStyle::SC_TitleBarMinButton
                      | QStyle::SC_TitleBarMaxButton
                      | QStyle::SC_TitleBarCloseButton;
    opt.titleBarFlags = widget->windowFlags();
    opt.direction = QApplication::layoutDirection();
    opt.text = windowTitleFor(widget);
    opt.icon = widget->windowIcon();
    opt.rect = widget->rect();

    QStyle *style = QApplication::style();
    if (!style)
        return 18;

    return style->pixelMetric(QStyle::PM_TitleBarHeight, &opt, 0);
}

bool QDecorationStyled::paint(QPainter *painter, const QWidget *widget, int decorationRegion,
                            DecorationState state)
{
    if (decorationRegion == None)
        return false;

    bool isActive = (widget == qApp->activeWindow());
    QPalette pal = qApp->palette();
    //ideally, the difference between Active and Inactive should be enough, so we shouldn't need to test this
    if (!isActive) {
        //pal.setCurrentColorGroup(QPalette::Disabled); //Can't do this either, because of palette limitations
        //copied from Q3TitleBar:
	pal.setColor(QPalette::Inactive, QPalette::Highlight,
		     pal.color(QPalette::Inactive, QPalette::Dark));
        pal.setColor(QPalette::Inactive, QPalette::Base,
                      pal.color(QPalette::Inactive, QPalette::Dark));
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText,
                      pal.color(QPalette::Inactive, QPalette::Window));
    }

    Qt::WindowFlags flags = widget->windowFlags();
    bool hasBorder = !widget->isMaximized();
    bool hasTitle = flags & Qt::WindowTitleHint;
    bool hasSysMenu = flags & Qt::WindowSystemMenuHint;
    bool hasContextHelp = flags & Qt::WindowContextHelpButtonHint;
    bool hasMinimize = flags & Qt::WindowMinimizeButtonHint;
    bool hasMaximize = flags & Qt::WindowMaximizeButtonHint;

    bool paintAll = (DecorationRegion(decorationRegion) == All);
    bool handled = false;

    QStyle *style = QApplication::style();

    // In the case of a borderless title bar, the title bar must be expanded one
    // borderWidth to the left, right and up.
    bool noTitleBorder = style->styleHint(QStyle::SH_TitleBar_NoBorder, 0, widget);
    int borderWidth = style->pixelMetric(QStyle::PM_MDIFrameWidth, 0, 0);
    int titleHeight = titleBarHeight(widget) + (noTitleBorder ? borderWidth : 0);
    int titleExtra = noTitleBorder ? borderWidth : 0;

    if ((paintAll || decorationRegion & Borders) && state == Normal && hasBorder) {
        QRegion newClip = painter->clipRegion();
        if (hasTitle) { // reduce flicker
            QRect rect(widget->rect());
            QRect r(rect.left() - titleExtra, rect.top() - titleHeight,
                    rect.width() + 2 * titleExtra, titleHeight);
            newClip -= r;
        }
        if (!newClip.isEmpty()) {
            QRect br = QDecoration::region(widget).boundingRect();
            painter->save();
            painter->setClipRegion(newClip);

            QStyleOptionFrame opt;
            opt.palette = pal;
            opt.rect = br;
            opt.lineWidth = borderWidth;

            if (isActive)
                opt.state |= QStyle::State_Active;
            bool porterDuff = painter->paintEngine()->hasFeature(QPaintEngine::PorterDuff);
            if (porterDuff)
                painter->setCompositionMode(QPainter::CompositionMode_Source);
            painter->fillRect(br, pal.window());
            if (porterDuff)
                painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
            style->drawPrimitive(QStyle::PE_FrameWindow, &opt, painter, 0);
            painter->restore();

            decorationRegion &= (~Borders);
            handled |= true;
        }
    }

    if (hasTitle) {
        painter->save();

        QStyleOptionTitleBar opt;
        opt.subControls = (decorationRegion & Title
                              ? QStyle::SC_TitleBarLabel : QStyle::SubControl(0))
                          | (decorationRegion & Menu
                              ? QStyle::SC_TitleBarSysMenu : QStyle::SubControl(0))
                          | (decorationRegion & Help
                              ? QStyle::SC_TitleBarContextHelpButton : QStyle::SubControl(0))
                          | (decorationRegion & Minimize
                              ? QStyle::SC_TitleBarMinButton : QStyle::SubControl(0))
                          | (decorationRegion & Maximize
                              ? QStyle::SC_TitleBarMaxButton : QStyle::SubControl(0))
                          | (decorationRegion & (Minimize | Maximize)
                              ? QStyle::SC_TitleBarNormalButton : QStyle::SubControl(0))
                          | (decorationRegion & Close
                              ? QStyle::SC_TitleBarCloseButton : QStyle::SubControl(0));
        opt.titleBarFlags = widget->windowFlags();
        opt.titleBarState = widget->windowState();
        if (isActive)
            opt.titleBarState |= QStyle::State_Active;
        opt.text = windowTitleFor(widget);
        opt.icon = widget->windowIcon();
        opt.palette = pal;
        opt.rect = QRect(widget->rect().x() - titleExtra, -titleHeight,
                         widget->rect().width() + 2 * titleExtra, titleHeight);

        if (paintAll) {
            painter->setClipRegion(opt.rect);
        } else {
            const QRect widgetRect = widget->rect();
            QRegion newClip = opt.rect;
            if (!(decorationRegion & Menu) && hasSysMenu)
                newClip -= region(widget, widgetRect, Menu);
            if (!(decorationRegion & Title) && hasTitle)
                newClip -= region(widget, widgetRect, Title);
            if (!(decorationRegion & Help) && hasContextHelp)
                newClip -= region(widget, widgetRect, Help);
            if (!(decorationRegion & Minimize) && hasMinimize)
                newClip -= region(widget, widgetRect, Minimize);
            if (!(decorationRegion & Maximize) && hasMaximize)
                newClip -= region(widget, widgetRect, Maximize);
            if (!(decorationRegion & (Minimize | Maximize)) && (hasMaximize | hasMinimize))
                newClip -= region(widget, widgetRect, Normal);
            if (!(decorationRegion & Close))
                newClip -= region(widget, widgetRect, Close);
            painter->setClipRegion(newClip);
        }

        if (state == Pressed)
            opt.activeSubControls = opt.subControls;

        style->drawComplexControl(QStyle::CC_TitleBar, &opt, painter, 0);
        painter->restore();

        decorationRegion &= ~(Title | Menu | Help | Normalize | Minimize | Maximize | Close);
        handled |= true;
    }

    return handled;
}

QRegion QDecorationStyled::region(const QWidget *widget, const QRect &rect, int decorationRegion)
{
    QStyle *style = QApplication::style();

    // In the case of a borderless title bar, the title bar must be expanded one
    // borderWidth to the left, right and up.
    bool noTitleBorder = style->styleHint(QStyle::SH_TitleBar_NoBorder, 0, widget);
    int borderWidth = style->pixelMetric(QStyle::PM_MDIFrameWidth, 0, 0);
    int titleHeight = titleBarHeight(widget) + (noTitleBorder ? borderWidth : 0);
    int titleExtra = noTitleBorder ? borderWidth : 0;

    QRect inside = QRect(rect.x() - titleExtra, rect.top() - titleHeight,
                         rect.width() + 2 * titleExtra, titleHeight);

    Qt::WindowFlags flags = widget->windowFlags();
    bool hasSysMenu = flags & Qt::WindowSystemMenuHint;
    bool hasContextHelp = flags & Qt::WindowContextHelpButtonHint;
    bool hasMinimize = flags & Qt::WindowMinimizeButtonHint;
    bool hasMaximize = flags & Qt::WindowMaximizeButtonHint;

    QStyleOptionTitleBar opt;
    opt.subControls = QStyle::SC_TitleBarLabel
                      | QStyle::SC_TitleBarSysMenu
                      | QStyle::SC_TitleBarNormalButton
                      | QStyle::SC_TitleBarMinButton
                      | QStyle::SC_TitleBarMaxButton
                      | QStyle::SC_TitleBarCloseButton;
    opt.titleBarFlags = widget->windowFlags();
    opt.direction = QApplication::layoutDirection();
    opt.text = windowTitleFor(widget);
    opt.icon = widget->windowIcon();
    opt.rect = inside;

    QRegion region;
    switch (decorationRegion) {
    case Title:
        region = style->subControlRect(QStyle::CC_TitleBar, &opt,
                                       QStyle::SC_TitleBarLabel, 0);
        break;
    case Menu:
        if (hasSysMenu)
            region = style->subControlRect(QStyle::CC_TitleBar, &opt,
                                           QStyle::SC_TitleBarSysMenu, 0);
        break;
    case Help:
        if (hasContextHelp)
            region = style->subControlRect(QStyle::CC_TitleBar, &opt,
                                           QStyle::SC_TitleBarContextHelpButton,
                                           0);
        break;
    case Normalize:
        if (hasMaximize | hasMinimize)
            region = style->subControlRect(QStyle::CC_TitleBar, &opt,
                                           QStyle::SC_TitleBarNormalButton,
                                           0);
        break;
    case Minimize:
        if (hasMinimize)
            region = style->subControlRect(QStyle::CC_TitleBar, &opt,
                                           QStyle::SC_TitleBarMinButton,
                                           0);
        break;
    case Maximize:
        if (hasMaximize)
            region = style->subControlRect(QStyle::CC_TitleBar, &opt,
                                           QStyle::SC_TitleBarMaxButton,
                                           0);
        break;
    case Close:
        region = style->subControlRect(QStyle::CC_TitleBar, &opt,
                                       QStyle::SC_TitleBarCloseButton, 0);
        break;

    default:
        region = QDecorationDefault::region(widget, rect, decorationRegion);
    }

    opt.rect = QRect(rect.x() - titleExtra, rect.top() - titleHeight,
                         rect.width() + 2 * titleExtra,
                         rect.height() + titleHeight + titleExtra);

    QStyleHintReturnMask mask;
    style->styleHint(QStyle::SH_WindowFrame_Mask, &opt, 0, &mask);

    return (mask.region.isEmpty() ? region : (region & mask.region));
}

#endif // QT_NO_QWS_DECORATION_STYLED

QT_END_NAMESPACE
