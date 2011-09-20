/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qs60style_p.h"

#include "qapplication.h"
#include "qpainter.h"
#include "qstyleoption.h"
#include "qevent.h"
#include "qpixmapcache.h"

#include "qcalendarwidget.h"
#include "qdial.h"
#include "qdialog.h"
#include "qmessagebox.h"
#include "qgroupbox.h"
#include "qheaderview.h"
#include "qlist.h"
#include "qlistwidget.h"
#include "qlistview.h"
#include "qmenu.h"
#include "qmenubar.h"
#include "qpushbutton.h"
#include "qscrollarea.h"
#include "qscrollbar.h"
#include "qtabbar.h"
#include "qtableview.h"
#include "qtextedit.h"
#include "qtoolbar.h"
#include "qtoolbutton.h"
#include "qfocusframe.h"
#include "qformlayout.h"
#include "qradiobutton.h"
#include "qcheckbox.h"
#include "qdesktopwidget.h"
#include "qprogressbar.h"
#include "qlabel.h"

#include "private/qtoolbarextension_p.h"
#include "private/qcombobox_p.h"
#include "private/qwidget_p.h"
#include "private/qapplication_p.h"
#include "private/qfont_p.h"

#if !defined(QT_NO_STYLE_S60) || defined(QT_PLUGIN)

QT_BEGIN_NAMESPACE

const QS60StylePrivate::SkinElementFlags QS60StylePrivate::KDefaultSkinElementFlags =
    SkinElementFlags(SF_PointNorth | SF_StateEnabled);

static const qreal goldenRatio = 1.618;

const layoutHeader QS60StylePrivate::m_layoutHeaders[] = {
// *** generated layout data ***
{240,320,1,19,"QVGA Landscape"},
{320,240,1,19,"QVGA Portrait"},
{360,640,1,19,"NHD Landscape"},
{640,360,1,19,"NHD Portrait"},
{352,800,1,12,"E90 Landscape"},
{480,640,1,19,"VGA Landscape"}
// *** End of generated data ***
};
const int QS60StylePrivate::m_numberOfLayouts =
    (int)sizeof(QS60StylePrivate::m_layoutHeaders)/sizeof(QS60StylePrivate::m_layoutHeaders[0]);

const short QS60StylePrivate::data[][MAX_PIXELMETRICS] = {
// *** generated pixel metrics ***
{5,0,-909,0,0,2,0,2,-1,7,12,22,15,15,7,198,-909,-909,-909,20,13,2,0,0,21,7,18,30,3,3,1,-909,-909,0,1,0,0,12,20,15,15,18,18,1,115,18,0,-909,-909,-909,-909,0,0,16,2,-909,0,0,-909,16,-909,-909,-909,-909,32,18,55,24,55,4,4,4,9,13,-909,5,51,11,5,0,3,3,6,8,3,3,-909,2,-909,-909,-909,-909,5,5,3,1,106},
{5,0,-909,0,0,1,0,2,-1,8,14,22,15,15,7,164,-909,-909,-909,19,15,2,0,0,21,8,27,28,4,4,1,-909,-909,0,7,6,0,13,23,17,17,21,21,7,115,21,0,-909,-909,-909,-909,0,0,15,1,-909,0,0,-909,15,-909,-909,-909,-909,32,21,65,27,65,3,3,5,10,15,-909,5,58,13,5,0,4,4,7,9,4,4,-909,2,-909,-909,-909,-909,6,6,3,1,106},
{7,0,-909,0,0,2,0,5,-1,25,69,46,37,37,9,258,-909,-909,-909,23,19,11,0,0,32,25,72,44,5,5,2,-909,-909,0,7,21,0,17,29,22,22,27,27,7,173,29,0,-909,-909,-909,-909,0,0,25,2,-909,0,0,-909,25,-909,-909,-909,-909,87,27,77,35,77,13,3,6,8,19,-909,7,74,19,7,0,5,5,8,12,5,5,-909,3,-909,-909,-909,-909,7,7,3,1,135},
{7,0,-909,0,0,2,0,5,-1,25,68,46,37,37,9,258,-909,-909,-909,31,19,13,0,0,32,25,60,52,5,5,2,-909,-909,0,7,32,0,17,29,22,22,27,27,7,173,29,0,-909,-909,-909,-909,0,0,26,2,-909,0,0,-909,26,-909,-909,-909,-909,87,27,96,35,96,12,3,6,8,19,-909,7,74,22,7,0,5,5,8,12,5,5,-909,3,-909,-909,-909,-909,7,7,3,1,135},
{7,0,-909,0,0,2,0,2,-1,10,20,27,18,18,9,301,-909,-909,-909,29,18,5,0,0,35,7,32,30,5,5,2,-909,-909,0,2,8,0,16,28,21,21,26,26,2,170,26,0,-909,-909,-909,-909,0,0,21,6,-909,0,0,-909,-909,-909,-909,-909,-909,54,26,265,34,265,5,5,6,3,18,-909,7,72,19,7,0,5,6,8,11,6,5,-909,2,-909,-909,-909,-909,5,5,3,1,106},
{9,0,-909,0,0,2,0,5,-1,34,99,76,51,51,25,352,-909,-909,-909,29,25,7,0,0,43,34,42,76,7,7,2,-909,-909,0,9,14,0,23,39,30,30,37,37,9,391,40,0,-909,-909,-909,-909,0,0,29,2,-909,0,0,-909,29,-909,-909,-909,-909,115,37,96,48,96,19,19,9,1,25,-909,9,101,24,9,0,7,7,7,16,7,7,-909,3,-909,-909,-909,-909,9,9,3,1,184}
// *** End of generated data ***
};

const short *QS60StylePrivate::m_pmPointer = QS60StylePrivate::data[0];

// theme background texture
QPixmap *QS60StylePrivate::m_background = 0;
QPixmap *QS60StylePrivate::m_placeHolderTexture = 0;

// theme palette
QPalette *QS60StylePrivate::m_themePalette = 0;

qint64 QS60StylePrivate::m_webPaletteKey = 0;

QPointer<QWidget> QS60StylePrivate::m_pressedWidget = 0;

const struct QS60StylePrivate::frameElementCenter QS60StylePrivate::m_frameElementsData[] = {
    {SE_ButtonNormal,           QS60StyleEnums::SP_QsnFrButtonTbCenter},
    {SE_ButtonPressed,          QS60StyleEnums::SP_QsnFrButtonTbCenterPressed},
    {SE_FrameLineEdit,          QS60StyleEnums::SP_QsnFrInputCenter},
    {SE_ListHighlight,          QS60StyleEnums::SP_QsnFrListCenter},
    {SE_PopupBackground,        QS60StyleEnums::SP_QsnFrPopupCenter},
    {SE_SettingsList,           QS60StyleEnums::SP_QsnFrSetOptCenter},
    {SE_TableItem,              QS60StyleEnums::SP_QsnFrCaleCenter},
    {SE_TableHeaderItem,        QS60StyleEnums::SP_QsnFrCaleHeadingCenter},
    {SE_ToolTip,                QS60StyleEnums::SP_QsnFrPopupPreviewCenter},
    {SE_ToolBar,                QS60StyleEnums::SP_QsnFrPopupSubCenter},
    {SE_ToolBarButton,          QS60StyleEnums::SP_QgnFrSctrlButtonCenter},
    {SE_ToolBarButtonPressed,   QS60StyleEnums::SP_QgnFrSctrlButtonCenterPressed},
    {SE_PanelBackground,        QS60StyleEnums::SP_QsnFrSetOptCenter},
    {SE_ButtonInactive,         QS60StyleEnums::SP_QsnFrButtonCenterInactive},
    {SE_Editor,                 QS60StyleEnums::SP_QsnFrInputCenter},
    {SE_TableItemPressed,       QS60StyleEnums::SP_QsnFrGridCenterPressed},
    {SE_ListItemPressed,        QS60StyleEnums::SP_QsnFrListCenterPressed},
};

static const int frameElementsCount =
    int(sizeof(QS60StylePrivate::m_frameElementsData)/sizeof(QS60StylePrivate::m_frameElementsData[0]));

const int KNotFound = -909;
const double KTabFontMul = 0.72;

QS60StylePrivate::~QS60StylePrivate()
{
    clearCaches(); //deletes also background image
    if (m_placeHolderTexture) {
        delete m_placeHolderTexture;
        m_placeHolderTexture = 0;
    }
    deleteThemePalette();
#ifdef Q_WS_S60
    removeAnimations();
#endif
}

void QS60StylePrivate::drawSkinElement(SkinElements element, QPainter *painter,
    const QRect &rect, SkinElementFlags flags)
{
    switch (element) {
    case SE_ButtonNormal:
        drawFrame(SF_ButtonNormal, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ButtonPressed:
        drawFrame(SF_ButtonPressed, painter, rect, flags | SF_PointNorth);
        break;
    case SE_FrameLineEdit:
        drawFrame(SF_FrameLineEdit, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ProgressBarGrooveHorizontal:
        drawRow(QS60StyleEnums::SP_QgnGrafBarFrameSideL, QS60StyleEnums::SP_QgnGrafBarFrameCenter,
            QS60StyleEnums::SP_QgnGrafBarFrameSideR, Qt::Horizontal, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ProgressBarGrooveVertical:
        drawRow(QS60StyleEnums::SP_QgnGrafBarFrameSideL, QS60StyleEnums::SP_QgnGrafBarFrameCenter,
            QS60StyleEnums::SP_QgnGrafBarFrameSideR, Qt::Vertical, painter, rect, flags | SF_PointEast);
        break;
    case SE_ProgressBarIndicatorHorizontal:
        drawPart(QS60StyleEnums::SP_QgnGrafBarProgress, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ProgressBarIndicatorVertical:
        drawPart(QS60StyleEnums::SP_QgnGrafBarProgress, painter, rect, flags | SF_PointWest);
        break;
    case SE_ScrollBarGrooveHorizontal:
        drawRow(QS60StyleEnums::SP_QsnCpScrollBgBottom, QS60StyleEnums::SP_QsnCpScrollBgMiddle,
            QS60StyleEnums::SP_QsnCpScrollBgTop, Qt::Horizontal, painter, rect, flags | SF_PointEast);
        break;
    case SE_ScrollBarGrooveVertical:
        drawRow(QS60StyleEnums::SP_QsnCpScrollBgTop, QS60StyleEnums::SP_QsnCpScrollBgMiddle,
            QS60StyleEnums::SP_QsnCpScrollBgBottom, Qt::Vertical, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ScrollBarHandleHorizontal:
        drawRow(QS60StyleEnums::SP_QsnCpScrollHandleBottom, QS60StyleEnums::SP_QsnCpScrollHandleMiddle,
            QS60StyleEnums::SP_QsnCpScrollHandleTop, Qt::Horizontal, painter, rect, flags | SF_PointEast);
        break;
    case SE_ScrollBarHandleVertical:
        drawRow(QS60StyleEnums::SP_QsnCpScrollHandleTop, QS60StyleEnums::SP_QsnCpScrollHandleMiddle,
            QS60StyleEnums::SP_QsnCpScrollHandleBottom, Qt::Vertical, painter, rect, flags | SF_PointNorth);
        break;
    case SE_SliderHandleHorizontal:
        drawPart(QS60StyleEnums::SP_QgnGrafNsliderMarker, painter, rect, flags | SF_PointNorth);
        break;
    case SE_SliderHandleVertical:
        drawPart(QS60StyleEnums::SP_QgnGrafNsliderMarker, painter, rect, flags | SF_PointEast);
        break;
    case SE_SliderHandleSelectedHorizontal:
        drawPart(QS60StyleEnums::SP_QgnGrafNsliderMarkerSelected, painter, rect, flags | SF_PointNorth);
        break;
    case SE_SliderHandleSelectedVertical:
        drawPart(QS60StyleEnums::SP_QgnGrafNsliderMarkerSelected, painter, rect, flags | SF_PointEast);
        break;
    case SE_SliderGrooveVertical:
        drawRow(QS60StyleEnums::SP_QgnGrafNsliderEndLeft, QS60StyleEnums::SP_QgnGrafNsliderMiddle,
                QS60StyleEnums::SP_QgnGrafNsliderEndRight, Qt::Vertical, painter, rect, flags | SF_PointEast);
        break;
    case SE_SliderGrooveHorizontal:
        drawRow(QS60StyleEnums::SP_QgnGrafNsliderEndLeft, QS60StyleEnums::SP_QgnGrafNsliderMiddle,
                QS60StyleEnums::SP_QgnGrafNsliderEndRight, Qt::Horizontal, painter, rect, flags | SF_PointNorth);
        break;
    case SE_TabBarTabEastActive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabActiveL, QS60StyleEnums::SP_QgnGrafTabActiveM,
            QS60StyleEnums::SP_QgnGrafTabActiveR, Qt::Vertical, painter, rect, flags | SF_PointEast);
        break;
    case SE_TabBarTabEastInactive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabPassiveL, QS60StyleEnums::SP_QgnGrafTabPassiveM,
            QS60StyleEnums::SP_QgnGrafTabPassiveR, Qt::Vertical, painter, rect, flags | SF_PointEast);
        break;
    case SE_TabBarTabNorthActive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabActiveL, QS60StyleEnums::SP_QgnGrafTabActiveM,
            QS60StyleEnums::SP_QgnGrafTabActiveR, Qt::Horizontal, painter, rect, flags | SF_PointNorth);
        break;
    case SE_TabBarTabNorthInactive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabPassiveL, QS60StyleEnums::SP_QgnGrafTabPassiveM,
            QS60StyleEnums::SP_QgnGrafTabPassiveR, Qt::Horizontal, painter, rect, flags | SF_PointNorth);
        break;
    case SE_TabBarTabSouthActive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabActiveR, QS60StyleEnums::SP_QgnGrafTabActiveM,
            QS60StyleEnums::SP_QgnGrafTabActiveL, Qt::Horizontal, painter, rect, flags | SF_PointSouth);
        break;
    case SE_TabBarTabSouthInactive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabPassiveR, QS60StyleEnums::SP_QgnGrafTabPassiveM,
            QS60StyleEnums::SP_QgnGrafTabPassiveL, Qt::Horizontal, painter, rect, flags | SF_PointSouth);
        break;
    case SE_TabBarTabWestActive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabActiveR, QS60StyleEnums::SP_QgnGrafTabActiveM,
            QS60StyleEnums::SP_QgnGrafTabActiveL, Qt::Vertical, painter, rect, flags | SF_PointWest);
        break;
    case SE_TabBarTabWestInactive:
        drawRow(QS60StyleEnums::SP_QgnGrafTabPassiveR, QS60StyleEnums::SP_QgnGrafTabPassiveM,
            QS60StyleEnums::SP_QgnGrafTabPassiveL, Qt::Vertical, painter, rect, flags | SF_PointWest);
        break;
    case SE_ListHighlight:
        drawFrame(SF_ListHighlight, painter, rect, flags | SF_PointNorth);
        break;
    case SE_PopupBackground:
        drawFrame(SF_PopupBackground, painter, rect, flags | SF_PointNorth);
        break;
    case SE_SettingsList:
        drawFrame(SF_SettingsList, painter, rect, flags | SF_PointNorth);
        break;
    case SE_TableItem:
        drawFrame(SF_TableItem, painter, rect, flags | SF_PointNorth);
        break;
    case SE_TableHeaderItem:
        drawFrame(SF_TableHeaderItem, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ToolTip:
        drawFrame(SF_ToolTip, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ToolBar:
        drawFrame(SF_ToolBar, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ToolBarButton:
        drawFrame(SF_ToolBarButton, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ToolBarButtonPressed:
        drawFrame(SF_ToolBarButtonPressed, painter, rect, flags | SF_PointNorth);
        break;
    case SE_PanelBackground:
        drawFrame(SF_PanelBackground, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ScrollBarHandlePressedHorizontal:
        drawRow(QS60StyleEnums::SP_QsnCpScrollHandleBottomPressed, QS60StyleEnums::SP_QsnCpScrollHandleMiddlePressed,
            QS60StyleEnums::SP_QsnCpScrollHandleTopPressed, Qt::Horizontal, painter, rect, flags | SF_PointEast);
        break;
    case SE_ScrollBarHandlePressedVertical:
        drawRow(QS60StyleEnums::SP_QsnCpScrollHandleTopPressed, QS60StyleEnums::SP_QsnCpScrollHandleMiddlePressed,
            QS60StyleEnums::SP_QsnCpScrollHandleBottomPressed, Qt::Vertical, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ButtonInactive:
        drawFrame(SF_ButtonInactive, painter, rect, flags | SF_PointNorth);
        break;
    case SE_Editor:
        drawFrame(SF_FrameLineEdit, painter, rect, flags | SF_PointNorth);
        break;
    case SE_DropArea:
        drawPart(QS60StyleEnums::SP_QgnGrafOrgBgGrid, painter, rect, flags | SF_PointNorth);
        break;
    case SE_TableItemPressed:
        drawFrame(SF_TableItemPressed, painter, rect, flags | SF_PointNorth);
        break;
    case SE_ListItemPressed:
        drawFrame(SF_ListItemPressed, painter, rect, flags | SF_PointNorth);
        break;
    default:
        break;
    }
}

void QS60StylePrivate::drawSkinPart(QS60StyleEnums::SkinParts part,
    QPainter *painter, const QRect &rect, SkinElementFlags flags)
{
    drawPart(part, painter, rect, flags);
}

short QS60StylePrivate::pixelMetric(int metric)
{
    //If it is a custom value, need to strip away the base to map to internal
    //pixel metric value table
    if (metric & QStyle::PM_CustomBase) {
        metric -= QStyle::PM_CustomBase;
        metric += MAX_NON_CUSTOM_PIXELMETRICS - 1;
    }

    Q_ASSERT(metric < MAX_PIXELMETRICS);
    const short returnValue = m_pmPointer[metric];
    return returnValue;
}

QColor QS60StylePrivate::stateColor(const QColor &color, const QStyleOption *option)
{
    QColor retColor (color);
    if (option && !(option->state & QStyle::State_Enabled)) {
        QColor hsvColor = retColor.toHsv();
        int colorSat = hsvColor.saturation();
        int colorVal = hsvColor.value();
        colorSat = (colorSat != 0) ? (colorSat >> 1) : 128;
        colorVal = (colorVal != 0) ? (colorVal >> 1) : 128;
        hsvColor.setHsv(hsvColor.hue(), colorSat, colorVal);
        retColor = hsvColor.toRgb();
    }
    return retColor;
}

QColor QS60StylePrivate::lighterColor(const QColor &baseColor)
{
    QColor result(baseColor);
    bool modifyColor = false;
    if (result.saturation() == 0) {
        result.setHsv(result.hue(), 128, result.value());
        modifyColor = true;
    }
    if (result.value() == 0) {
        result.setHsv(result.hue(), result.saturation(), 128);
        modifyColor = true;
    }
    if (modifyColor)
        result = result.lighter(175);
    else
        result = result.lighter(225);
    return result;
}

bool QS60StylePrivate::drawsOwnThemeBackground(const QWidget *widget)
{
    return (widget ? (widget->windowType() == Qt::Dialog) : false);
}

QFont QS60StylePrivate::s60Font(
    QS60StyleEnums::FontCategories fontCategory,
    int pointSize, bool resolveFontSize) const
{
    QFont result;
    int actualPointSize = pointSize;
    if (actualPointSize <= 0) {
        const QFont appFont = QApplication::font();
        actualPointSize = appFont.pointSize();
        if (actualPointSize <= 0)
            actualPointSize = appFont.pixelSize() * 72 / qt_defaultDpiY();
    }
    Q_ASSERT(actualPointSize > 0);
    const QPair<QS60StyleEnums::FontCategories, int> key(fontCategory, actualPointSize);
    if (!m_mappedFontsCache.contains(key)) {
        result = s60Font_specific(fontCategory, actualPointSize, resolveFontSize);
        m_mappedFontsCache.insert(key, result);
    } else {
        result = m_mappedFontsCache.value(key);
        if (result.pointSize() != actualPointSize)
            result.setPointSize(actualPointSize);
    }
    return result;
}

void QS60StylePrivate::clearCaches(CacheClearReason reason)
{
    switch(reason){
    case CC_LayoutChange:
        // when layout changes, the colors remain in cache, but graphics and fonts can change
        m_mappedFontsCache.clear();
        QPixmapCache::clear();
        break;
    case CC_ThemeChange:
        QPixmapCache::clear();
#ifdef Q_WS_S60
        deleteStoredSettings();
#endif
        deleteBackground();
        break;
    case CC_UndefinedChange:
    default:
        m_mappedFontsCache.clear();
        QPixmapCache::clear();
        deleteBackground();
        break;
    }
}

QColor QS60StylePrivate::calculatedColor(SkinFrameElements frame) const
{
    const int frameCornerWidth = pixelMetric(PM_FrameCornerWidth);
    const int frameCornerHeight = pixelMetric(PM_FrameCornerHeight);
    Q_ASSERT(2 * frameCornerWidth < 32);
    Q_ASSERT(2 * frameCornerHeight < 32);

    const QImage frameImage = QS60StylePrivate::frame(frame, QSize(32, 32)).toImage();
    Q_ASSERT(frameImage.bytesPerLine() > 0);
    if (frameImage.isNull())
        return Qt::black;

    const QRgb *pixelRgb = (const QRgb*)frameImage.constBits();
    const int pixels = frameImage.byteCount() / sizeof(QRgb);

    int estimatedRed = 0;
    int estimatedGreen = 0;
    int estimatedBlue = 0;

    int skips = 0;
    int estimations = 0;

    const int topBorderLastPixel = frameCornerHeight * frameImage.width() - 1;
    const int bottomBorderFirstPixel = frameImage.width() * frameImage.height() - topBorderLastPixel;
    const int rightBorderFirstPixel = frameImage.width() - frameCornerWidth;
    const int leftBorderLastPixel = frameCornerWidth;

    while ((skips + estimations) < pixels) {
        if ((skips + estimations) > topBorderLastPixel &&
            (skips + estimations) < bottomBorderFirstPixel) {
            for (int rowIndex = 0; rowIndex < frameImage.width(); rowIndex++) {
                if (rowIndex > leftBorderLastPixel &&
                    rowIndex < rightBorderFirstPixel) {
                    estimatedRed += qRed(*pixelRgb);
                    estimatedGreen += qGreen(*pixelRgb);
                    estimatedBlue += qBlue(*pixelRgb);
                }
                pixelRgb++;
                estimations++;
            }
        } else {
            pixelRgb++;
            skips++;
        }
    }
    QColor frameColor(estimatedRed/estimations, estimatedGreen/estimations, estimatedBlue/estimations);
    return !estimations ? Qt::black : frameColor;
}

void QS60StylePrivate::setThemePalette(QApplication *app) const
{
    Q_UNUSED(app)
    QPalette widgetPalette = QPalette(Qt::white);
    setThemePalette(&widgetPalette);
}

QPalette* QS60StylePrivate::themePalette()
{
    return m_themePalette;
}

bool QS60StylePrivate::equalToThemePalette(QColor color, QPalette::ColorRole role)
{
    if (!m_themePalette)
        return false;
    if (color == m_themePalette->color(role))
        return true;
    return false;
}

bool QS60StylePrivate::equalToThemePalette(qint64 cacheKey, QPalette::ColorRole role)
{
    if (!m_themePalette)
        return false;
    if (cacheKey == m_themePalette->brush(role).texture().cacheKey())
        return true;
    return false;
}

void QS60StylePrivate::setBackgroundTexture(QApplication *app) const
{
    Q_UNUSED(app)
    QPalette applicationPalette = QApplication::palette();
    // The initial QPalette::Window is just a placeHolder QPixmap to save RAM
    // if the actual texture is not needed. The real texture is created just before
    // painting it in qt_s60_fill_background().
    applicationPalette.setBrush(QPalette::Window, placeHolderTexture());
    setThemePalette(&applicationPalette);
}

void QS60StylePrivate::deleteBackground()
{
    if (m_background) {
        delete m_background;
        m_background = 0;
    }
}

void QS60StylePrivate::setCurrentLayout(int index)
{
    m_pmPointer = data[index];
}

void QS60StylePrivate::drawPart(QS60StyleEnums::SkinParts skinPart,
    QPainter *painter, const QRect &rect, SkinElementFlags flags)
{
    static const bool doCache =
#if defined(Q_WS_S60)
        // Freezes on 3.1. Anyways, caching is only really needed on touch UI
        !(QSysInfo::s60Version() == QSysInfo::SV_S60_3_1 || QSysInfo::s60Version() == QSysInfo::SV_S60_3_2);
#else
        true;
#endif

    const QPixmap skinPartPixMap((doCache ? cachedPart : part)(skinPart, rect.size(), painter, flags));
    if (!skinPartPixMap.isNull())
        painter->drawPixmap(rect.topLeft(), skinPartPixMap);
}

void QS60StylePrivate::drawFrame(SkinFrameElements frameElement, QPainter *painter, const QRect &rect, SkinElementFlags flags)
{
    static const bool doCache =
#if defined(Q_WS_S60)
        // Freezes on 3.1. Anyways, caching is only really needed on touch UI
        !(QSysInfo::s60Version() == QSysInfo::SV_S60_3_1 || QSysInfo::s60Version() == QSysInfo::SV_S60_3_2);
#else
        true;
#endif
    const QPixmap frameElementPixMap((doCache ? cachedFrame : frame)(frameElement, rect.size(), flags));
    if (!frameElementPixMap.isNull())
        painter->drawPixmap(rect.topLeft(), frameElementPixMap);
}

void QS60StylePrivate::drawRow(QS60StyleEnums::SkinParts start,
    QS60StyleEnums::SkinParts middle, QS60StyleEnums::SkinParts end,
    Qt::Orientation orientation, QPainter *painter, const QRect &rect,
    SkinElementFlags flags)
{
    QSize startEndSize(partSize(start, flags));
    startEndSize.scale(rect.size(), Qt::KeepAspectRatio);

    QRect startRect = QRect(rect.topLeft(), startEndSize);
    QRect middleRect = rect;
    QRect endRect;

    if (orientation == Qt::Horizontal) {
        startRect.setHeight(rect.height());
        startRect.setWidth(qMin((rect.width() >> 1) - 1, startRect.width()));
        endRect = startRect.translated(rect.width() - startRect.width(), 0);
        middleRect.adjust(startRect.width(), 0, -startRect.width(), 0);
        if (startRect.bottomRight().x() > endRect.topLeft().x()) {
            const int overlap = (startRect.bottomRight().x() -  endRect.topLeft().x()) >> 1;
            startRect.setWidth(startRect.width() - overlap);
            endRect.adjust(overlap, 0, 0, 0);
        }
    } else {
        startRect.setWidth(rect.width());
        startRect.setHeight(qMin((rect.height() >> 1) - 1, startRect.height()));
        endRect = startRect.translated(0, rect.height() - startRect.height());
        middleRect.adjust(0, startRect.height(), 0, -startRect.height());
        if (startRect.topRight().y() > endRect.bottomLeft().y()) {
            const int overlap = (startRect.topRight().y() - endRect.bottomLeft().y()) >> 1;
            startRect.setHeight(startRect.height() - overlap);
            endRect.adjust(0, overlap, 0, 0);
        }
    }

#if 0
    painter->save();
    painter->setOpacity(.3);
    painter->fillRect(startRect, Qt::red);
    painter->fillRect(middleRect, Qt::green);
    painter->fillRect(endRect, Qt::blue);
    painter->restore();
#else
    drawPart(start, painter, startRect, flags);
    if (middleRect.isValid())
        drawPart(middle, painter, middleRect, flags);
    drawPart(end, painter, endRect, flags);
#endif
}

QPixmap QS60StylePrivate::cachedPart(QS60StyleEnums::SkinParts part,
    const QSize &size, QPainter *painter, SkinElementFlags flags)
{
    QPixmap result;
    const int animationFrame = (flags & SF_Animation) ? currentAnimationFrame(part) : 0;

    const QString cacheKey =
        QString::fromLatin1("S60Style: SkinParts=%1 QSize=%2|%3 SkinPartFlags=%4 AnimationFrame=%5")
            .arg((int)part).arg(size.width()).arg(size.height()).arg((int)flags).arg(animationFrame);
    if (!QPixmapCache::find(cacheKey, result)) {
        result = QS60StylePrivate::part(part, size, painter, flags);
        QPixmapCache::insert(cacheKey, result);
    }
    return result;
}

QPixmap QS60StylePrivate::cachedFrame(SkinFrameElements frame, const QSize &size, SkinElementFlags flags)
{
    QPixmap result;
    const QString cacheKey =
        QString::fromLatin1("S60Style: SkinFrameElements=%1 QSize=%2|%3 SkinElementFlags=%4")
            .arg((int)frame).arg(size.width()).arg(size.height()).arg((int)flags);
    if (!QPixmapCache::find(cacheKey, result)) {
        result = QS60StylePrivate::frame(frame, size, flags);
        QPixmapCache::insert(cacheKey, result);
    }
    return result;
}

void QS60StylePrivate::setFont(QWidget *widget) const
{
    QS60StyleEnums::FontCategories fontCategory = QS60StyleEnums::FC_Undefined;
    if (!widget)
        return;
    if (qobject_cast<QPushButton *>(widget)){
        fontCategory = QS60StyleEnums::FC_Primary;
    } else if (qobject_cast<QToolButton *>(widget)){
        fontCategory = QS60StyleEnums::FC_Primary;
    } else if (qobject_cast<QHeaderView *>(widget)){
        fontCategory = QS60StyleEnums::FC_Secondary;
    } else if (qobject_cast<QGroupBox *>(widget)){
        fontCategory = QS60StyleEnums::FC_Title;
    } else if (qobject_cast<QMessageBox *>(widget)){
        fontCategory = QS60StyleEnums::FC_Primary;
    } else if (qobject_cast<QMenu *>(widget)){
        fontCategory = QS60StyleEnums::FC_Primary;
    } else if (qobject_cast<QCalendarWidget *>(widget)){
        fontCategory = QS60StyleEnums::FC_Secondary;
    }
    if (fontCategory != QS60StyleEnums::FC_Undefined) {
        const bool resolveFontSize = widget->testAttribute(Qt::WA_SetFont)
            && (widget->font().resolve() & QFont::SizeResolved);
        const QFont suggestedFont =
            s60Font(fontCategory, widget->font().pointSizeF(), resolveFontSize);
        widget->setFont(suggestedFont);
    }
}

void QS60StylePrivate::setThemePalette(QWidget *widget)
{
    if(!widget)
        return;

    //header view and its viewport need to be set 100% transparent button color, since drawing code will
    //draw transparent theme graphics to table column and row headers.
    if (qobject_cast<QHeaderView *>(widget)){
        QPalette widgetPalette = QApplication::palette(widget);
        widgetPalette.setColor(QPalette::Active, QPalette::ButtonText,
            s60Color(QS60StyleEnums::CL_QsnTextColors, 23, 0));
        QHeaderView* header = qobject_cast<QHeaderView *>(widget);
        widgetPalette.setColor(QPalette::Button, Qt::transparent );
        if (header->viewport())
            header->viewport()->setPalette(widgetPalette);
        QApplication::setPalette(widgetPalette, "QHeaderView");
    } else if (qobject_cast<QLabel *>(widget)) {
        if (widget->window() && widget->window()->windowType() == Qt::Dialog) {
            QPalette widgetPalette = widget->palette();
            widgetPalette.setColor(QPalette::WindowText,
                s60Color(QS60StyleEnums::CL_QsnTextColors, 19, 0));
            widget->setPalette(widgetPalette);
        }
    }
}

void QS60StylePrivate::setThemePalette(QPalette *palette) const
{
    if (!palette)
        return;

    // basic colors
    palette->setColor(QPalette::WindowText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 6, 0));
    palette->setColor(QPalette::ButtonText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 20, 0));
    palette->setColor(QPalette::Text,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 6, 0));
    palette->setColor(QPalette::ToolTipText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 55, 0));
    palette->setColor(QPalette::BrightText, palette->color(QPalette::WindowText).lighter());
    palette->setColor(QPalette::HighlightedText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 24, 0));
    palette->setColor(QPalette::Link,
        s60Color(QS60StyleEnums::CL_QsnHighlightColors, 3, 0));
    palette->setColor(QPalette::LinkVisited, palette->color(QPalette::Link).darker());
    palette->setColor(QPalette::Highlight,
        s60Color(QS60StyleEnums::CL_QsnHighlightColors, 2, 0));
    // The initial QPalette::Window is just a placeHolder QPixmap to save RAM
    // if the actual texture is not needed. The real texture is created just before
    // painting it in qt_s60_fill_background().
    palette->setBrush(QPalette::Window, placeHolderTexture());
    // set as transparent so that styled full screen theme background is visible
    palette->setBrush(QPalette::Base, Qt::transparent);
    // set button color based on pixel colors
#ifndef Q_WS_S60
    //For emulated style, just calculate the color every time
    const QColor buttonColor = calculatedColor(SF_ButtonNormal);
#else
    const QColor buttonColor = colorFromFrameGraphics(SF_ButtonNormal);
#endif
    palette->setColor(QPalette::Button, buttonColor);
    palette->setColor(QPalette::Light, palette->color(QPalette::Button).lighter());
    palette->setColor(QPalette::Dark, palette->color(QPalette::Button).darker());
    palette->setColor(QPalette::Midlight, palette->color(QPalette::Button).lighter(125));
    palette->setColor(QPalette::Mid, palette->color(QPalette::Button).darker(150));
    palette->setColor(QPalette::Shadow, Qt::black);
    QColor alternateBase = palette->light().color();
    alternateBase.setAlphaF(0.8);
    palette->setColor(QPalette::AlternateBase, alternateBase);

    QApplication::setPalette(*palette); //calling QApplication::setPalette clears palette hash
    setThemePaletteHash(palette);
    storeThemePalette(palette);
}

void QS60StylePrivate::deleteThemePalette()
{
    if (m_themePalette) {
        delete m_themePalette;
        m_themePalette = 0;
    }
}

void QS60StylePrivate::storeThemePalette(QPalette *palette)
{
    deleteThemePalette();
    //store specified palette for latter use.
    m_themePalette = new QPalette(*palette);
}

// set widget specific palettes
void QS60StylePrivate::setThemePaletteHash(QPalette *palette)
{
    if (!palette)
        return;

    //store the original palette
    QPalette widgetPalette = *palette;
    const QColor mainAreaTextColor =
        s60Color(QS60StyleEnums::CL_QsnTextColors, 6, 0);

    widgetPalette.setColor(QPalette::WindowText,
        s60Color(QS60StyleEnums::CL_QsnLineColors, 8, 0));
    QApplication::setPalette(widgetPalette, "QSlider");
    // return to original palette after each widget
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::Active, QPalette::ButtonText, mainAreaTextColor);
    widgetPalette.setColor(QPalette::Inactive, QPalette::ButtonText, mainAreaTextColor);
    const QStyleOption opt;
    widgetPalette.setColor(QPalette::Disabled, QPalette::ButtonText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 6, &opt));
    QApplication::setPalette(widgetPalette, "QPushButton");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::Active, QPalette::ButtonText, mainAreaTextColor);
    widgetPalette.setColor(QPalette::Inactive, QPalette::ButtonText, mainAreaTextColor);
    QApplication::setPalette(widgetPalette, "QToolButton");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::Active, QPalette::ButtonText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 23, 0));
    QApplication::setPalette(widgetPalette, "QHeaderView");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::ButtonText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 8, 0));
    QApplication::setPalette(widgetPalette, "QMenuBar");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::Text,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 22, 0));
    widgetPalette.setColor(QPalette::HighlightedText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 11, 0));
    QApplication::setPalette(widgetPalette, "QMenu");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::WindowText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 4, 0));
    widgetPalette.setColor(QPalette::HighlightedText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 3, 0));
    QApplication::setPalette(widgetPalette, "QTabBar");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::HighlightedText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 10, 0));
    QApplication::setPalette(widgetPalette, "QListView");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::Text,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 22, 0));
    widgetPalette.setColor(QPalette::HighlightedText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 11, 0));
    QApplication::setPalette(widgetPalette, "QTableView");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::Text,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 27, 0));
    widgetPalette.setColor(QPalette::HighlightedText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 24, 0));
    QApplication::setPalette(widgetPalette, "QLineEdit");
    QApplication::setPalette(widgetPalette, "QTextEdit");
    QApplication::setPalette(widgetPalette, "QComboBox");
    QApplication::setPalette(widgetPalette, "QSpinBox");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::WindowText, s60Color(QS60StyleEnums::CL_QsnTextColors, 7, 0));
    widgetPalette.setColor(QPalette::HighlightedText,
        s60Color(QS60StyleEnums::CL_QsnTextColors, 11, 0));
    QApplication::setPalette(widgetPalette, "QRadioButton");
    QApplication::setPalette(widgetPalette, "QCheckBox");
    widgetPalette = *palette;

    widgetPalette.setColor(QPalette::WindowText, mainAreaTextColor);
    widgetPalette.setColor(QPalette::Button, QApplication::palette().color(QPalette::Button));
    widgetPalette.setColor(QPalette::Dark, mainAreaTextColor.darker());
    widgetPalette.setColor(QPalette::Light, mainAreaTextColor.lighter());
    QApplication::setPalette(widgetPalette, "QDial");
    widgetPalette = *palette;

    widgetPalette.setBrush(QPalette::Window, QBrush());
    QApplication::setPalette(widgetPalette, "QScrollArea");
    widgetPalette = *palette;

    //Webpages should not use S60 theme colors as they are designed to work
    //with themeBackground and do not generally mesh well with web page backgrounds.
    QPalette webPalette = *palette;
    webPalette.setColor(QPalette::WindowText, Qt::black);
    webPalette.setColor(QPalette::Text, Qt::black);
    webPalette.setBrush(QPalette::Base, Qt::white);

    QApplication::setPalette(webPalette, "QWebView");
    QApplication::setPalette(webPalette, "QGraphicsWebView");

    m_webPaletteKey = webPalette.cacheKey();
}

QSize QS60StylePrivate::partSize(QS60StyleEnums::SkinParts part, SkinElementFlags flags)
{
    QSize result(20, 20);
    switch (part)
        {
        case QS60StyleEnums::SP_QgnGrafBarProgress:
            result.setWidth(pixelMetric(QStyle::PM_ProgressBarChunkWidth));
            break;
        case QS60StyleEnums::SP_QgnGrafTabActiveM:
        case QS60StyleEnums::SP_QgnGrafTabPassiveM:
        case QS60StyleEnums::SP_QgnGrafTabActiveR:
        case QS60StyleEnums::SP_QgnGrafTabPassiveR:
        case QS60StyleEnums::SP_QgnGrafTabPassiveL:
        case QS60StyleEnums::SP_QgnGrafTabActiveL:
            //Returned QSize for tabs must not be square, but narrow rectangle with width:height
            //ratio of 1:2 for horizontal tab bars (and 2:1 for vertical ones).
            result.setWidth(result.height() >> 1);
            break;

        case QS60StyleEnums::SP_QgnGrafNsliderEndLeft:
        case QS60StyleEnums::SP_QgnGrafNsliderEndRight:
        case QS60StyleEnums::SP_QgnGrafNsliderMiddle:
            break;

        case QS60StyleEnums::SP_QgnGrafNsliderMarker:
        case QS60StyleEnums::SP_QgnGrafNsliderMarkerSelected:
            result.scale(pixelMetric(QStyle::PM_SliderLength),
                pixelMetric(QStyle::PM_SliderControlThickness), Qt::IgnoreAspectRatio);
            break;

        case QS60StyleEnums::SP_QgnGrafBarFrameSideL:
        case QS60StyleEnums::SP_QgnGrafBarFrameSideR:
            result.setWidth(pixelMetric(PM_FrameCornerWidth));
            break;

        case QS60StyleEnums::SP_QsnCpScrollHandleTopPressed:
        case QS60StyleEnums::SP_QsnCpScrollBgBottom:
        case QS60StyleEnums::SP_QsnCpScrollBgTop:
        case QS60StyleEnums::SP_QsnCpScrollHandleBottom:
        case QS60StyleEnums::SP_QsnCpScrollHandleTop:
        case QS60StyleEnums::SP_QsnCpScrollHandleBottomPressed:
            result.setHeight(pixelMetric(QStyle::PM_ScrollBarExtent));
            result.setWidth(pixelMetric(QStyle::PM_ScrollBarExtent));
            break;
        case QS60StyleEnums::SP_QsnCpScrollHandleMiddlePressed:
        case QS60StyleEnums::SP_QsnCpScrollBgMiddle:
        case QS60StyleEnums::SP_QsnCpScrollHandleMiddle:
            result.setHeight(pixelMetric(QStyle::PM_ScrollBarExtent));
            result.setWidth(pixelMetric(QStyle::PM_ScrollBarSliderMin));
            break;
        default:
            // Generic frame part size gathering.
            for (int i = 0; i < frameElementsCount; ++i)
            {
                switch (m_frameElementsData[i].center - part) {
                    case 8: /* CornerTl */
                    case 7: /* CornerTr */
                    case 6: /* CornerBl */
                    case 5: /* CornerBr */
                        result.setWidth(pixelMetric(PM_FrameCornerWidth));
                        // Falltrough intended...
                    case 4: /* SideT */
                    case 3: /* SideB */
                        result.setHeight(pixelMetric(PM_FrameCornerHeight));
                        break;
                    case 2: /* SideL */
                    case 1: /* SideR */
                        result.setWidth(pixelMetric(PM_FrameCornerWidth));
                        break;
                    case 0: /* center */
                    default:
                        break;
                }
            }
            break;
    }
    if (flags & (SF_PointEast | SF_PointWest)) {
        const int temp = result.width();
        result.setWidth(result.height());
        result.setHeight(temp);
    }
    return result;
}

bool QS60StylePrivate::canDrawThemeBackground(const QBrush &backgroundBrush, const QWidget *widget)
{
    // Always return true for web pages.
    if (widget && m_webPaletteKey == QApplication::palette(widget).cacheKey())
        return true;
    //If brush is not changed from style's default values, draw theme graphics.
    return (backgroundBrush.color() == Qt::transparent ||
            backgroundBrush.style() == Qt::NoBrush) ? true : false;
}

bool QS60StylePrivate::isWidgetPressed(const QWidget *widget)
{
    return (widget && widget == m_pressedWidget);
}

// Generates 1*1 white pixmap as a placeholder for real texture.
// The actual theme texture is drawn in qt_s60_fill_background().
QPixmap QS60StylePrivate::placeHolderTexture()
{
    if (!m_placeHolderTexture) {
        m_placeHolderTexture = new QPixmap(1,1);
        m_placeHolderTexture->fill(Qt::green);
    }
    return *m_placeHolderTexture;
}

/*!
  \class QS60Style
  \brief The QS60Style class provides a look and feel suitable for applications on S60.
  \since 4.6
  \ingroup appearance
  \inmodule QtWidgets

  \sa QMacStyle, QWindowsStyle, QWindowsXPStyle, QWindowsVistaStyle, QPlastiqueStyle, QCleanlooksStyle, QMotifStyle
*/


/*!
    Destroys the style.
*/
QS60Style::~QS60Style()
{
}

/*!
  \reimp
*/
void QS60Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    const QS60StylePrivate::SkinElementFlags flags = (option->state & State_Enabled) ?  QS60StylePrivate::SF_StateEnabled : QS60StylePrivate::SF_StateDisabled;
    SubControls sub = option->subControls;

    switch (control) {
#ifndef QT_NO_SCROLLBAR
    case CC_ScrollBar:
        if (const QStyleOptionSlider *optionSlider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = optionSlider->orientation == Qt::Horizontal;

            const QRect scrollBarSlider = subControlRect(control, optionSlider, SC_ScrollBarSlider, widget);
            const QRect grooveRect = subControlRect(control, optionSlider, SC_ScrollBarGroove, widget);

            const QS60StylePrivate::SkinElements grooveElement =
                horizontal ? QS60StylePrivate::SE_ScrollBarGrooveHorizontal : QS60StylePrivate::SE_ScrollBarGrooveVertical;
            QS60StylePrivate::drawSkinElement(grooveElement, painter, grooveRect, flags);

            const SubControls subControls = optionSlider->subControls;

            // select correct slider (horizontal/vertical/pressed)
            const bool sliderPressed = ((optionSlider->state & State_Sunken) && (subControls & SC_ScrollBarSlider));
            const QS60StylePrivate::SkinElements handleElement =
                horizontal ?
                    ( sliderPressed ?
                        QS60StylePrivate::SE_ScrollBarHandlePressedHorizontal :
                        QS60StylePrivate::SE_ScrollBarHandleHorizontal ) :
                    ( sliderPressed ?
                        QS60StylePrivate::SE_ScrollBarHandlePressedVertical :
                        QS60StylePrivate::SE_ScrollBarHandleVertical);
            QS60StylePrivate::drawSkinElement(handleElement, painter, scrollBarSlider, flags);
        }
        break;
#endif // QT_NO_SCROLLBAR
#ifndef QT_NO_SLIDER
    case CC_Slider:
        if (const QStyleOptionSlider *optionSlider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {

            const QRect sliderGroove = subControlRect(control, optionSlider, SC_SliderGroove, widget);
            const bool horizontal = optionSlider->orientation == Qt::Horizontal;

            //Highlight
/*            if (optionSlider->state & State_HasFocus)
                drawPrimitive(PE_FrameFocusRect, optionSlider, painter, widget);*/

            //Groove graphics
            if (QS60StylePrivate::hasSliderGrooveGraphic()) {
                const QS60StylePrivate::SkinElements grooveElement = horizontal ?
                    QS60StylePrivate::SE_SliderGrooveHorizontal :
                    QS60StylePrivate::SE_SliderGrooveVertical;
                QS60StylePrivate::drawSkinElement(grooveElement, painter, sliderGroove, flags);
            } else {
                const QPoint sliderGrooveCenter = sliderGroove.center();
                const bool horizontal = optionSlider->orientation == Qt::Horizontal;
                painter->save();
                if (widget)
                    painter->setPen(widget->palette().windowText().color());
                if (horizontal)
                    painter->drawLine(0, sliderGrooveCenter.y(), sliderGroove.right(), sliderGrooveCenter.y());
                else
                    painter->drawLine(sliderGrooveCenter.x(), 0, sliderGrooveCenter.x(), sliderGroove.bottom());
                painter->restore();
            }

            //Handle graphics
            const QRect sliderHandle = subControlRect(control, optionSlider, SC_SliderHandle, widget);
            QS60StylePrivate::SkinElements handleElement;
            if (optionSlider->state & State_Sunken)
                handleElement =
                        horizontal ? QS60StylePrivate::SE_SliderHandleSelectedHorizontal : QS60StylePrivate::SE_SliderHandleSelectedVertical;
            else
                handleElement =
                    horizontal ? QS60StylePrivate::SE_SliderHandleHorizontal : QS60StylePrivate::SE_SliderHandleVertical;
            QS60StylePrivate::drawSkinElement(handleElement, painter, sliderHandle, flags);
        }
        break;
#endif // QT_NO_SLIDER
#ifndef QT_NO_COMBOBOX
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            const QRect cmbxEditField = subControlRect(CC_ComboBox, option, SC_ComboBoxEditField, widget);
            const QRect cmbxFrame = subControlRect(CC_ComboBox, option, SC_ComboBoxFrame, widget);
            const bool direction = cmb->direction == Qt::LeftToRight;

            // Button frame
            QStyleOptionFrame  buttonOption;
            buttonOption.QStyleOption::operator=(*cmb);
            const int maxButtonSide = cmbxFrame.width() - cmbxEditField.width();
            const int newTop = cmbxEditField.center().y() - maxButtonSide / 2;
            const int topLeftPoint = direction ?
                (cmbxEditField.right() + 1) : (cmbxEditField.left() + 1 - maxButtonSide);
            const QRect buttonRect(topLeftPoint, newTop, maxButtonSide, maxButtonSide);
            buttonOption.rect = buttonRect;
            buttonOption.state = cmb->state;
            drawPrimitive(PE_PanelButtonCommand, &buttonOption, painter, widget);

            // draw label background - label itself is drawn separately
            const QS60StylePrivate::SkinElements skinElement = QS60StylePrivate::SE_FrameLineEdit;
            QS60StylePrivate::drawSkinElement(skinElement, painter, cmbxEditField, flags);

            // Draw the combobox arrow
            if (sub & SC_ComboBoxArrow) {
                // Make rect slightly smaller
                buttonOption.rect.adjust(1, 1, -1, -1);
                painter->save();
                painter->setPen(option->palette.buttonText().color());
                drawPrimitive(PE_IndicatorSpinDown, &buttonOption, painter, widget);
                painter->restore();
            }
        }
        break;
#endif // QT_NO_COMBOBOX
#ifndef QT_NO_TOOLBUTTON
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolBtn = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            State bflags = toolBtn->state & ~State_Sunken;

            if (bflags & State_AutoRaise) {
                if (!(bflags & State_MouseOver) || !(bflags & State_Enabled)) {
                    bflags &= ~State_Raised;
                }
            }
            State mflags = bflags;
            if (toolBtn->state & State_Sunken) {
                bflags |= State_Sunken;
                mflags |= State_Sunken;
            }

            const QRect button(subControlRect(control, toolBtn, SC_ToolButton, widget));
            QRect menuRect = QRect();
            if (toolBtn->subControls & SC_ToolButtonMenu)
                menuRect = subControlRect(control, toolBtn, SC_ToolButtonMenu, widget);

            if (toolBtn->subControls & SC_ToolButton) {
                QStyleOption tool(0);
                tool.palette = toolBtn->palette;

                if (bflags & (State_Sunken | State_On | State_Raised | State_Enabled)) {
                    tool.rect = button.unite(menuRect);
                    tool.state = bflags;
                    drawPrimitive(PE_PanelButtonTool, &tool, painter, widget);
                }
                if (toolBtn->subControls & SC_ToolButtonMenu) {
                    tool.rect = menuRect;
                    tool.state = mflags;
                    drawPrimitive(PE_IndicatorArrowDown, &tool, painter, widget);
                }
            }
            QStyleOptionToolButton toolButton = *toolBtn;
            if (toolBtn->features & QStyleOptionToolButton::Arrow) {
                PrimitiveElement pe;
                switch (toolBtn->arrowType) {
                    case Qt::LeftArrow:
                        pe = PE_IndicatorArrowLeft;
                        break;
                    case Qt::RightArrow:
                        pe = PE_IndicatorArrowRight;
                        break;
                    case Qt::UpArrow:
                        pe = PE_IndicatorArrowUp;
                        break;
                    case Qt::DownArrow:
                        pe = PE_IndicatorArrowDown;
                        break;
                    default:
                        break; }
                toolButton.rect = button;
                drawPrimitive(pe, &toolButton, painter, widget);
            }

            if (toolBtn->text.length() > 0 ||
                !toolBtn->icon.isNull()) {
                const int frameWidth = pixelMetric(PM_DefaultFrameWidth, option, widget);
                toolButton.rect = button.adjusted(frameWidth, frameWidth, -frameWidth, -frameWidth);
                drawControl(CE_ToolButtonLabel, &toolButton, painter, widget);
                }
            }
        break;
#endif //QT_NO_TOOLBUTTON
#ifndef QT_NO_SPINBOX
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QStyleOptionSpinBox copy = *spinBox;
            PrimitiveElement pe;

            if (spinBox->subControls & SC_SpinBoxUp) {
                copy.subControls = SC_SpinBoxUp;
                QPalette spinBoxPal = spinBox->palette;
                if (!(spinBox->stepEnabled & QAbstractSpinBox::StepUpEnabled)) {
                    spinBoxPal.setCurrentColorGroup(QPalette::Disabled);
                    copy.state &= ~State_Enabled;
                    copy.palette = spinBoxPal;
                }

                if (spinBox->activeSubControls == SC_SpinBoxUp && (spinBox->state & State_Sunken)) {
                    copy.state |= State_On;
                    copy.state |= State_Sunken;
                } else {
                    copy.state |= State_Raised;
                    copy.state &= ~State_Sunken;
                }
                pe = (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) ?
                      PE_IndicatorSpinPlus :
                      PE_IndicatorSpinUp;

                copy.rect = subControlRect(CC_SpinBox, spinBox, SC_SpinBoxUp, widget);
                drawPrimitive(PE_PanelButtonBevel, &copy, painter, widget);
                copy.rect.adjust(1, 1, -1, -1);
                drawPrimitive(pe, &copy, painter, widget);
            }

            if (spinBox->subControls & SC_SpinBoxDown) {
                copy.subControls = SC_SpinBoxDown;
                copy.state = spinBox->state;
                QPalette spinBoxPal = spinBox->palette;
                if (!(spinBox->stepEnabled & QAbstractSpinBox::StepDownEnabled)) {
                    spinBoxPal.setCurrentColorGroup(QPalette::Disabled);
                    copy.state &= ~State_Enabled;
                    copy.palette = spinBoxPal;
                }

                if (spinBox->activeSubControls == SC_SpinBoxDown && (spinBox->state & State_Sunken)) {
                    copy.state |= State_On;
                    copy.state |= State_Sunken;
                } else {
                    copy.state |= State_Raised;
                    copy.state &= ~State_Sunken;
                }
                pe = (spinBox->buttonSymbols == QAbstractSpinBox::PlusMinus) ?
                      PE_IndicatorSpinMinus :
                      PE_IndicatorSpinDown;

                copy.rect = subControlRect(CC_SpinBox, spinBox, SC_SpinBoxDown, widget);
                drawPrimitive(PE_PanelButtonBevel, &copy, painter, widget);
                copy.rect.adjust(1, 1, -1, -1);
                drawPrimitive(pe, &copy, painter, widget);
            }
        }
        break;
#endif //QT_NO_SPINBOX
#ifndef QT_NO_GROUPBOX
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            // Draw frame
            const QRect textRect = subControlRect(CC_GroupBox, option, SC_GroupBoxLabel, widget);
            const QRect checkBoxRect = subControlRect(CC_GroupBox, option, SC_GroupBoxCheckBox, widget);
            if (groupBox->subControls & SC_GroupBoxFrame) {
                QStyleOptionFrameV2 frame;
                frame.QStyleOption::operator=(*groupBox);
                frame.features = groupBox->features;
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);
                drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
            }

            // Draw title
            if ((groupBox->subControls & SC_GroupBoxLabel) && !groupBox->text.isEmpty()) {
                const QColor textColor = groupBox->textColor;
                painter->save();

                if (textColor.isValid())
                    painter->setPen(textColor);
                int alignment = int(groupBox->textAlignment);
                if (!styleHint(SH_UnderlineShortcut, option, widget))
                    alignment |= Qt::TextHideMnemonic;

                drawItemText(painter, textRect,  Qt::TextShowMnemonic | Qt::AlignHCenter | Qt::AlignVCenter | alignment,
                             groupBox->palette, groupBox->state & State_Enabled, groupBox->text,
                             textColor.isValid() ? QPalette::NoRole : QPalette::WindowText);
                painter->restore();
            }

            // Draw checkbox
            if (groupBox->subControls & SC_GroupBoxCheckBox) {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*groupBox);
                box.rect = checkBoxRect;
                drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
            }
        }
        break;
#endif //QT_NO_GROUPBOX
    default:
        QCommonStyle::drawComplexControl(control, option, painter, widget);
    }
}

/*!
  \reimp
*/
void QS60Style::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    Q_D(const QS60Style);
    const QS60StylePrivate::SkinElementFlags flags = (option->state & State_Enabled) ?  QS60StylePrivate::SF_StateEnabled : QS60StylePrivate::SF_StateDisabled;
    switch (element) {
        case CE_CheckBox:
        case CE_RadioButton:
            if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                bool isRadio = (element == CE_RadioButton);
                QStyleOptionButton subopt = *btn;

                // Highlight needs to be drawn first, as it goes "underneath" the text and indicator.
                if (btn->state & State_HasFocus) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*btn);
                    fropt.rect = subElementRect(isRadio ? SE_RadioButtonFocusRect
                                                        : SE_CheckBoxFocusRect, btn, widget);
                    drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);

                    subopt.palette.setColor(QPalette::Active, QPalette::WindowText,
                        subopt.palette.highlightedText().color());
                }

                subopt.rect = subElementRect(isRadio ? SE_RadioButtonIndicator
                                                     : SE_CheckBoxIndicator, btn, widget);
                drawPrimitive(isRadio ? PE_IndicatorRadioButton : PE_IndicatorCheckBox,
                              &subopt, painter, widget);
                subopt.rect = subElementRect(isRadio ? SE_RadioButtonContents
                                                     : SE_CheckBoxContents, btn, widget);

                drawControl(isRadio ? CE_RadioButtonLabel : CE_CheckBoxLabel, &subopt, painter, widget);
            }
            break;

    case CE_PushButton:
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {

            drawControl(CE_PushButtonBevel, btn, painter, widget);
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);

            drawControl(CE_PushButtonLabel, &subopt, painter, widget);
        }
        break;
    case CE_PushButtonBevel:
        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const bool isDisabled = !(option->state & State_Enabled);
            const bool isFlat = button->features & QStyleOptionButton::Flat;
            QS60StyleEnums::SkinParts skinPart;
            QS60StylePrivate::SkinElements skinElement;
            if (!isDisabled) {
                const bool isPressed = (option->state & State_Sunken) ||
                                       (option->state & State_On);
                if (isFlat) {
                    skinPart =
                        isPressed ? QS60StyleEnums::SP_QsnFrButtonTbCenterPressed : QS60StyleEnums::SP_QsnFrButtonTbCenter;
                } else {
                    skinElement =
                        isPressed ? QS60StylePrivate::SE_ButtonPressed : QS60StylePrivate::SE_ButtonNormal;
                }
            } else {
                if (isFlat)
                    skinPart =QS60StyleEnums::SP_QsnFrButtonCenterInactive;
                else
                    skinElement = QS60StylePrivate::SE_ButtonInactive;
            }
            if (isFlat)
                QS60StylePrivate::drawSkinPart(skinPart, painter, option->rect, flags);
            else
                QS60StylePrivate::drawSkinElement(skinElement, painter, option->rect, flags);
            }
        break;
#ifndef QT_NO_TOOLBUTTON
    case CE_ToolButtonLabel:
        if (const QStyleOptionToolButton *toolBtn = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QStyleOptionToolButton optionToolButton = *toolBtn;

            if (!optionToolButton.icon.isNull() && (optionToolButton.state & State_Sunken)
                    && (optionToolButton.state & State_Enabled)) {

                    const QIcon::State state = optionToolButton.state & State_On ? QIcon::On : QIcon::Off;
                    const QPixmap pm(optionToolButton.icon.pixmap(optionToolButton.rect.size().boundedTo(optionToolButton.iconSize),
                            QIcon::Normal, state));
                    optionToolButton.icon = generatedIconPixmap(QIcon::Selected, pm, &optionToolButton);
            }

            QCommonStyle::drawControl(element, &optionToolButton, painter, widget);
        }
        break;
#endif //QT_NO_TOOLBUTTON
#ifndef QT_NO_COMBOBOX
    case CE_ComboBoxLabel:
        if (const QStyleOptionComboBox *comboBox = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            QStyleOption optionComboBox = *comboBox;
            optionComboBox.palette.setColor(QPalette::Active, QPalette::WindowText,
                optionComboBox.palette.text().color() );
            optionComboBox.palette.setColor(QPalette::Inactive, QPalette::WindowText,
                optionComboBox.palette.text().color() );
            QRect editRect = subControlRect(CC_ComboBox, comboBox, SC_ComboBoxEditField, widget);
            const int frameW = proxy()->pixelMetric(PM_DefaultFrameWidth, option, widget);

            if (!comboBox->currentIcon.isNull()) {
                const QIcon::Mode mode = comboBox->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
                const QPixmap pixmap = comboBox->currentIcon.pixmap(comboBox->iconSize, mode);
                QRect iconRect(editRect);
                iconRect.setWidth(comboBox->iconSize.width() + frameW);
                iconRect = alignedRect(comboBox->direction,
                                       Qt::AlignLeft | Qt::AlignVCenter,
                                       iconRect.size(), editRect);
                if (comboBox->editable)
                    painter->fillRect(iconRect, optionComboBox.palette.brush(QPalette::Base));
                drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

                if (comboBox->direction == Qt::RightToLeft)
                    editRect.setRight(editRect.right() - frameW - comboBox->iconSize.width());
                else
                    editRect.setLeft(comboBox->iconSize.width() + frameW);
            }
            if (!comboBox->currentText.isEmpty() && !comboBox->editable) {
                const Qt::TextElideMode elideMode = (comboBox->direction == Qt::LeftToRight) ? Qt::ElideRight : Qt::ElideLeft;
                const QString text = comboBox->fontMetrics.elidedText(comboBox->currentText, elideMode, editRect.width());

                QCommonStyle::drawItemText(painter,
                            editRect.adjusted(QS60StylePrivate::pixelMetric(PM_FrameCornerWidth), 0, -1, 0),
                            visualAlignment(comboBox->direction, Qt::AlignLeft | Qt::AlignVCenter),
                            comboBox->palette, comboBox->state & State_Enabled, text);
            }
        }
        break;
#endif //QT_NO_COMBOBOX
#ifndef QT_NO_ITEMVIEWS
    case CE_ItemViewItem:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option)) {
            QStyleOptionViewItemV4 voptAdj = *vopt;
            painter->save();

            painter->setClipRect(voptAdj.rect);
            const bool isSelected = (vopt->state & State_Selected);
            const bool hasFocus = (vopt->state & State_HasFocus);

            bool isScrollBarVisible = false;
            int scrollBarWidth = 0;
            QList<QScrollBar *> scrollBars = widget->findChildren<QScrollBar *>();
            for (int i = 0; i < scrollBars.size(); ++i) {
                QScrollBar *scrollBar = scrollBars.at(i);
                if (scrollBar && scrollBar->orientation() == Qt::Vertical) {
                    isScrollBarVisible = scrollBar->isVisible();
                    scrollBarWidth = scrollBar->size().width();
                    break;
                }
            }

            int rightValue = widget ? widget->contentsRect().right() : voptAdj.rect.right();

            if (isScrollBarVisible)
                rightValue -= scrollBarWidth;

            if (voptAdj.rect.right() > rightValue)
                voptAdj.rect.setRight(rightValue);

            const QRect iconRect = subElementRect(SE_ItemViewItemDecoration, &voptAdj, widget);
            QRect textRect = subElementRect(SE_ItemViewItemText, &voptAdj, widget);
            const QAbstractItemView *itemView = qobject_cast<const QAbstractItemView *>(widget);

            const bool singleSelection = itemView &&
                ((itemView->selectionMode() == QAbstractItemView::SingleSelection ||
                 itemView->selectionMode() == QAbstractItemView::NoSelection));
            const bool selectItems = itemView && (itemView->selectionBehavior() == QAbstractItemView::SelectItems);

            // draw themed background for itemview unless background brush has been defined.
            if (vopt->backgroundBrush == Qt::NoBrush) {
                if (itemView) {
                    //With single item selection, use highlight focus as selection indicator.
                    if (singleSelection && isSelected){
                        voptAdj.state = voptAdj.state | State_HasFocus;
                        if (!hasFocus && selectItems) {
                            painter->save();
                            painter->setOpacity(0.5);
                        }
                    }
                    drawPrimitive(PE_PanelItemViewItem, &voptAdj, painter, widget);
                    if (singleSelection && isSelected && !hasFocus && selectItems)
                        painter->restore();
                }
            } else { QCommonStyle::drawPrimitive(PE_PanelItemViewItem, &voptAdj, painter, widget);}

             // draw the icon
             const QIcon::Mode mode = (voptAdj.state & State_Enabled) ? QIcon::Normal : QIcon::Disabled;
             const QIcon::State state = (voptAdj.state & State_Open) ? QIcon::On : QIcon::Off;
             voptAdj.icon.paint(painter, iconRect, voptAdj.decorationAlignment, mode, state);

             // Draw selection check mark or checkbox
             if (itemView && (!singleSelection || (vopt->features & QStyleOptionViewItemV2::HasCheckIndicator))) {
                 const QRect selectionRect = subElementRect(SE_ItemViewItemCheckIndicator, &voptAdj, widget);

                 QStyleOptionViewItemV4 checkMarkOption(voptAdj);
                 if (selectionRect.isValid())
                     checkMarkOption.rect = selectionRect;
                 // Draw selection mark.
                 if (isSelected && selectItems) {
                     proxy()->drawPrimitive(PE_IndicatorViewItemCheck, &checkMarkOption, painter, widget);
                     // @todo: this should happen in the rect retrievel i.e. subElementRect()
                     if (textRect.right() > selectionRect.left())
                         textRect.setRight(selectionRect.left());
                 } else if (voptAdj.features & QStyleOptionViewItemV2::HasCheckIndicator) {
                     checkMarkOption.state = checkMarkOption.state & ~State_HasFocus;

                     switch (vopt->checkState) {
                     case Qt::Unchecked:
                         checkMarkOption.state |= State_Off;
                         break;
                     case Qt::PartiallyChecked:
                         checkMarkOption.state |= State_NoChange;
                         break;
                     case Qt::Checked:
                         checkMarkOption.state |= State_On;
                         break;
                     }
                     drawPrimitive(PE_IndicatorViewItemCheck, &checkMarkOption, painter, widget);
                 }
             }

             // draw the text
            if (!voptAdj.text.isEmpty()) {
                if (hasFocus)
                    painter->setPen(voptAdj.palette.highlightedText().color());
                else
                    painter->setPen(voptAdj.palette.text().color());
                d->viewItemDrawText(painter, &voptAdj, textRect);
            }
            painter->restore();
        }
        break;
#endif // QT_NO_ITEMVIEWS
#ifndef QT_NO_TABBAR
    case CE_TabBarTabShape:
        if (const QStyleOptionTabV3 *optionTab = qstyleoption_cast<const QStyleOptionTabV3 *>(option)) {
            QStyleOptionTabV3 optionTabAdj = *optionTab;
            const bool isSelected = optionTab->state & State_Selected;
            const bool directionMirrored = (optionTab->direction == Qt::RightToLeft);
            QS60StylePrivate::SkinElements skinElement;
            switch (optionTab->shape) {
                case QTabBar::TriangularEast:
                case QTabBar::RoundedEast:
                    skinElement = isSelected ? QS60StylePrivate::SE_TabBarTabEastActive:
                        QS60StylePrivate::SE_TabBarTabEastInactive;
                    break;
                case QTabBar::TriangularSouth:
                case QTabBar::RoundedSouth:
                    skinElement = isSelected ? QS60StylePrivate::SE_TabBarTabSouthActive:
                        QS60StylePrivate::SE_TabBarTabSouthInactive;
                    break;
                case QTabBar::TriangularWest:
                case QTabBar::RoundedWest:
                    skinElement = isSelected ? QS60StylePrivate::SE_TabBarTabWestActive:
                        QS60StylePrivate::SE_TabBarTabWestInactive;
                    break;
                case QTabBar::TriangularNorth:
                case QTabBar::RoundedNorth:
                default:
                    skinElement = isSelected ? QS60StylePrivate::SE_TabBarTabNorthActive:
                        QS60StylePrivate::SE_TabBarTabNorthInactive;
                    break;
            }
            if (skinElement == QS60StylePrivate::SE_TabBarTabEastInactive ||
                    skinElement == QS60StylePrivate::SE_TabBarTabNorthInactive ||
                    skinElement == QS60StylePrivate::SE_TabBarTabSouthInactive ||
                    skinElement == QS60StylePrivate::SE_TabBarTabWestInactive ||
                    skinElement == QS60StylePrivate::SE_TabBarTabEastActive ||
                    skinElement == QS60StylePrivate::SE_TabBarTabNorthActive ||
                    skinElement == QS60StylePrivate::SE_TabBarTabSouthActive ||
                    skinElement==QS60StylePrivate::SE_TabBarTabWestActive) {
                const int borderThickness =
                    QS60StylePrivate::pixelMetric(PM_DefaultFrameWidth);
                int tabOverlap = pixelMetric(PM_TabBarTabOverlap);
                if (tabOverlap > borderThickness)
                    tabOverlap -= borderThickness;

                const bool usesScrollButtons = 
                    (widget) ? (qobject_cast<const QTabBar*>(widget))->usesScrollButtons() : false;
                const int roomForScrollButton = 
                    usesScrollButtons ? QS60StylePrivate::pixelMetric(PM_TabBarScrollButtonWidth) : 0;

                // adjust for overlapping tabs and scrollbuttons, if necessary
                if (skinElement == QS60StylePrivate::SE_TabBarTabEastInactive ||
                        skinElement == QS60StylePrivate::SE_TabBarTabEastActive ||
                        skinElement == QS60StylePrivate::SE_TabBarTabWestInactive ||
                        skinElement == QS60StylePrivate::SE_TabBarTabWestActive){
                    if (optionTabAdj.position == QStyleOptionTabV3::Beginning)
                        optionTabAdj.rect.adjust(0, roomForScrollButton, 0, tabOverlap);
                    else if (optionTabAdj.position == QStyleOptionTabV3::End)
                        optionTabAdj.rect.adjust(0, 0, 0, tabOverlap);
                    else
                        optionTabAdj.rect.adjust(0, 0, 0, tabOverlap);
                } else {
                    if (directionMirrored) {
                        if (optionTabAdj.position == QStyleOptionTabV3::Beginning)
                            optionTabAdj.rect.adjust(-tabOverlap, 0, -roomForScrollButton, 0);
                        else
                            optionTabAdj.rect.adjust(-tabOverlap, 0, 0, 0);
                    } else {
                        if (optionTabAdj.position == QStyleOptionTabV3::Beginning)
                            optionTabAdj.rect.adjust(roomForScrollButton, 0, tabOverlap, 0);
                        else
                            optionTabAdj.rect.adjust(0, 0, tabOverlap, 0);
                    }
                }
            }
            QS60StylePrivate::drawSkinElement(skinElement, painter, optionTabAdj.rect, flags);
        }
        break;
    case CE_TabBarTabLabel:
        if (const QStyleOptionTabV3 *tab = qstyleoption_cast<const QStyleOptionTabV3 *>(option)) {
            QStyleOptionTabV3 optionTab = *tab;
            QRect tr = optionTab.rect;
            const bool directionMirrored = (optionTab.direction == Qt::RightToLeft);
            const int borderThickness =
                QS60StylePrivate::pixelMetric(PM_DefaultFrameWidth);
            int tabOverlap = pixelMetric(PM_TabBarTabOverlap);
            if (tabOverlap > borderThickness)
                tabOverlap -= borderThickness;
            const bool usesScrollButtons = 
                (widget) ? (qobject_cast<const QTabBar*>(widget))->usesScrollButtons() : false;
            const int roomForScrollButton = 
                usesScrollButtons ? QS60StylePrivate::pixelMetric(PM_TabBarScrollButtonWidth) : 0;

            switch (tab->shape) {
                case QTabBar::TriangularWest:
                case QTabBar::RoundedWest:
                case QTabBar::TriangularEast:
                case QTabBar::RoundedEast:
                    tr.adjust(0, 0, 0, tabOverlap);
                    break;
                case QTabBar::TriangularSouth:
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularNorth:
                case QTabBar::RoundedNorth:
                default:
                    if (directionMirrored)
                        tr.adjust(-tabOverlap, 0, 0, 0);
                    else
                        tr.adjust(0, 0, tabOverlap, 0);
                    break;
            }
            painter->save();
            QFont f = painter->font();
            f.setPointSizeF(f.pointSizeF() * KTabFontMul);
            painter->setFont(f);

            const bool selected = optionTab.state & State_Selected;
            if (selected)
                optionTab.palette.setColor(QPalette::Active, QPalette::WindowText,
                    optionTab.palette.highlightedText().color());

            const bool verticalTabs = optionTab.shape == QTabBar::RoundedEast
                                || optionTab.shape == QTabBar::RoundedWest
                                || optionTab.shape == QTabBar::TriangularEast
                                || optionTab.shape == QTabBar::TriangularWest;

            //make room for scrollbuttons
            if (!verticalTabs) {
                if ((tab->position == QStyleOptionTabV3::Beginning && !directionMirrored))
                    tr.adjust(roomForScrollButton, 0, 0, 0);
                else if ((tab->position == QStyleOptionTabV3::Beginning && directionMirrored))
                    tr.adjust(0, 0, -roomForScrollButton, 0);
            } else {
                if (tab->position == QStyleOptionTabV3::Beginning)
                    tr.adjust(0, roomForScrollButton, 0, 0);
            }

            if (verticalTabs) {
                painter->save();
                int newX, newY, newRotation;
                if (optionTab.shape == QTabBar::RoundedEast || optionTab.shape == QTabBar::TriangularEast) {
                    newX = tr.width();
                    newY = tr.y();
                    newRotation = 90;
                } else {
                    newX = 0;
                    newY = tr.y() + tr.height();
                    newRotation = -90;
                }
                tr.setRect(0, 0, tr.height(), tr.width());
                QTransform m;
                m.translate(newX, newY);
                m.rotate(newRotation);
                painter->setTransform(m, true);
            }
            tr.adjust(0, 0, pixelMetric(PM_TabBarTabShiftHorizontal, tab, widget),
                            pixelMetric(PM_TabBarTabShiftVertical, tab, widget));

            if (selected) {
                tr.setBottom(tr.bottom() - pixelMetric(PM_TabBarTabShiftVertical, tab, widget));
                tr.setRight(tr.right() - pixelMetric(PM_TabBarTabShiftHorizontal, tab, widget));
            }

            int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
            if (!styleHint(SH_UnderlineShortcut, &optionTab, widget))
                alignment |= Qt::TextHideMnemonic;
            if (!optionTab.icon.isNull()) {
                QSize iconSize = optionTab.iconSize;
                if (!iconSize.isValid()) {
                    const int iconExtent = pixelMetric(PM_TabBarIconSize);
                    iconSize = QSize(iconExtent, iconExtent);
                }
                QPixmap tabIcon = optionTab.icon.pixmap(iconSize,
                    (optionTab.state & State_Enabled) ? QIcon::Normal : QIcon::Disabled);
                if (tab->text.isEmpty())
                    painter->drawPixmap(tr.center().x() - (tabIcon.height() >> 1),
                                        tr.center().y() - (tabIcon.height() >> 1),
                                        tabIcon);
                else
                    painter->drawPixmap(tr.left() + tabOverlap,
                                        tr.center().y() - (tabIcon.height() >> 1),
                                        tabIcon);
                tr.setLeft(tr.left() + iconSize.width() + 4); //todo: magic four
            }

            QCommonStyle::drawItemText(painter, tr, alignment, optionTab.palette, tab->state & State_Enabled, tab->text, QPalette::WindowText);
            if (verticalTabs)
                painter->restore();

            painter->restore();
        }
        break;
#endif // QT_NO_TABBAR
#ifndef QT_NO_PROGRESSBAR
    case CE_ProgressBarContents:
        if (const QStyleOptionProgressBarV2 *optionProgressBar = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
            QRect progressRect = optionProgressBar->rect;

            if (optionProgressBar->minimum == optionProgressBar->maximum && optionProgressBar->minimum == 0) {
                // busy indicator
                const QS60StylePrivate::SkinElementFlag orientationFlag = optionProgressBar->orientation == Qt::Horizontal ?
                    QS60StylePrivate::SF_PointNorth : QS60StylePrivate::SF_PointWest;

                QS60StylePrivate::drawSkinPart(QS60StyleEnums::SP_QgnGrafBarWaitAnim,
                        painter, progressRect, flags | orientationFlag | QS60StylePrivate::SF_Animation );
            } else {
                const qreal progressFactor = (optionProgressBar->minimum == optionProgressBar->maximum) ? 1.0
                    : (qreal)optionProgressBar->progress / optionProgressBar->maximum;
                const int frameWidth = pixelMetric(PM_DefaultFrameWidth, option, widget);
                if (optionProgressBar->orientation == Qt::Horizontal) {
                    progressRect.setWidth(int(progressRect.width() * progressFactor));
                    if(optionProgressBar->direction == Qt::RightToLeft)
                        progressRect.translate(optionProgressBar->rect.width() - progressRect.width(), 0);
                    progressRect.adjust(frameWidth, 0, -frameWidth, 0);
                } else {
                    progressRect.adjust(0, frameWidth, 0, -frameWidth);
                    progressRect.setTop(progressRect.bottom() - int(progressRect.height() * progressFactor));
                }

                const QS60StylePrivate::SkinElements skinElement = optionProgressBar->orientation == Qt::Horizontal ?
                    QS60StylePrivate::SE_ProgressBarIndicatorHorizontal : QS60StylePrivate::SE_ProgressBarIndicatorVertical;
                QS60StylePrivate::drawSkinElement(skinElement, painter, progressRect, flags);
            }
        }
        break;
    case CE_ProgressBarGroove:
        if (const QStyleOptionProgressBarV2 *optionProgressBar = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
            const QS60StylePrivate::SkinElements skinElement = optionProgressBar->orientation == Qt::Horizontal ?
                QS60StylePrivate::SE_ProgressBarGrooveHorizontal : QS60StylePrivate::SE_ProgressBarGrooveVertical;
            QS60StylePrivate::drawSkinElement(skinElement, painter, option->rect, flags);
        }
        break;
    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBarV2 *progressbar = qstyleoption_cast<const QStyleOptionProgressBarV2 *>(option)) {
            QStyleOptionProgressBarV2 optionProgressBar = *progressbar;
            QCommonStyle::drawItemText(painter, progressbar->rect, flags | Qt::AlignCenter | Qt::TextSingleLine, optionProgressBar.palette,
                progressbar->state & State_Enabled, progressbar->text, QPalette::WindowText);
        }
        break;
#endif // QT_NO_PROGRESSBAR
#ifndef QT_NO_MENU
    case CE_MenuItem:
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            QStyleOptionMenuItem optionMenuItem = *menuItem;

            bool drawSubMenuIndicator = false;
            bool drawSeparator = false;
            switch(menuItem->menuItemType) {
                case QStyleOptionMenuItem::Separator:
                    drawSeparator = true;
                    break;
                case QStyleOptionMenuItem::Scroller:
                    return; // no scrollers in S60 menus
                case QStyleOptionMenuItem::SubMenu:
                    drawSubMenuIndicator = true;
                    break;
                default:
                    break;
            }
            if (drawSeparator) {
                painter->save();
                painter->setPen(QS60StylePrivate::s60Color(QS60StyleEnums::CL_QsnLineColors, 10, 0));
                painter->drawLine(optionMenuItem.rect.topLeft(), optionMenuItem.rect.bottomRight());
                painter->restore();
                return;
            }
            const bool enabled = optionMenuItem.state & State_Enabled;
            const bool checkable = optionMenuItem.checkType != QStyleOptionMenuItem::NotCheckable;
            bool ignoreCheckMark = false;

#ifndef QT_NO_COMBOBOX
            if (qobject_cast<const QComboBox*>(widget))
                ignoreCheckMark = true; //ignore the checkmarks provided by the QComboMenuDelegate
#endif

            uint text_flags = Qt::AlignLeading | Qt::TextShowMnemonic | Qt::TextDontClip
                            | Qt::TextSingleLine | Qt::AlignVCenter;
            if (!styleHint(SH_UnderlineShortcut, menuItem, widget))
                text_flags |= Qt::TextHideMnemonic;

            QRect iconRect = subElementRect(SE_ItemViewItemDecoration, &optionMenuItem, widget);
            QRect textRect = subElementRect(SE_ItemViewItemText, &optionMenuItem, widget);

            QStyleOptionMenuItem optionCheckBox;

            //Regardless of checkbox visibility, make room for it, this mirrors native implementation,
            //where text and icon placement is static regardless of content of menu item.
            optionCheckBox.QStyleOptionMenuItem::operator=(*menuItem);
            optionCheckBox.rect.setWidth(pixelMetric(PM_IndicatorWidth));
            optionCheckBox.rect.setHeight(pixelMetric(PM_IndicatorHeight));

            const int vSpacing = QS60StylePrivate::pixelMetric(PM_LayoutVerticalSpacing);
            //The vertical spacing is doubled; it needs one spacing to separate checkbox from
            //highlight and then it needs one to separate it whatever is shown after it (text/icon/both).
            const int moveByX = optionCheckBox.rect.width() + 2 * vSpacing;
            optionCheckBox.rect.moveCenter(QPoint(
                    optionCheckBox.rect.center().x() + moveByX >> 1, 
                    menuItem->rect.center().y()));

            if (optionMenuItem.direction != Qt::LeftToRight)
                optionCheckBox.rect.translate(textRect.width() + iconRect.width(), 0);


            const bool selected = (option->state & State_Selected) && (option->state & State_Enabled);
            if (selected) {
                const int spacing = ignoreCheckMark ? (vSpacing + QS60StylePrivate::pixelMetric(PM_DefaultFrameWidth)) : 0;
                const int start = optionMenuItem.rect.left() + spacing;
                const int end = optionMenuItem.rect.right() - spacing;
                //-1 adjustment to avoid highlight being on top of possible separator item
                const QRect highlightRect = QRect(
                        QPoint(start, option->rect.top()), 
                        QPoint(end, option->rect.bottom() - 1));
                QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_ListHighlight, painter, highlightRect, flags);
            }
            
            if (checkable && !ignoreCheckMark)
                drawPrimitive(PE_IndicatorMenuCheckMark, &optionCheckBox, painter, widget);

            //draw icon and/or checkState
            QPixmap pix = menuItem->icon.pixmap(pixelMetric(PM_SmallIconSize),
                enabled ? QIcon::Normal : QIcon::Disabled);
            const bool itemWithIcon = !pix.isNull();
            if (itemWithIcon) {
                drawItemPixmap(painter, iconRect, text_flags, pix);
                if (optionMenuItem.direction == Qt::LeftToRight)
                    textRect.translate(vSpacing, 0);
                else
                    textRect.translate(-vSpacing, 0);
                textRect.setWidth(textRect.width() - vSpacing);
            }

            //draw indicators
            if (drawSubMenuIndicator) {
                QStyleOptionMenuItem arrowOptions;
                arrowOptions.QStyleOption::operator=(*menuItem);
                const int indicatorWidth = (pixelMetric(PM_ListViewIconSize, option, widget) >> 1) +
                    pixelMetric(PM_LayoutVerticalSpacing, option, widget);
                if (optionMenuItem.direction == Qt::LeftToRight)
                    arrowOptions.rect.setLeft(textRect.right());
                arrowOptions.rect.setWidth(indicatorWidth);
                //by default sub menu indicator in S60 points to east,so here icon
                // direction is set to north (and south when in RightToLeft)
                const QS60StylePrivate::SkinElementFlag arrowDirection = (arrowOptions.direction == Qt::LeftToRight) ?
                    QS60StylePrivate::SF_PointNorth : QS60StylePrivate::SF_PointSouth;
                painter->save();
                painter->setPen(option->palette.windowText().color());
                QS60StylePrivate::drawSkinPart(QS60StyleEnums::SP_QgnIndiSubmenu, painter, arrowOptions.rect,
                    (flags | QS60StylePrivate::SF_ColorSkinned | arrowDirection));
                painter->restore();
            }

            //draw text
            if (!enabled){
                //In s60, if something becomes disabled, it is removed from menu, so no native look-alike available.
                optionMenuItem.palette.setColor(QPalette::Disabled, QPalette::Text, QS60StylePrivate::lighterColor(
                        optionMenuItem.palette.color(QPalette::Disabled, QPalette::Text)));
                painter->save();
                painter->setOpacity(0.5);
            }
            if (selected)
                optionMenuItem.palette.setColor(
                    QPalette::Active, QPalette::Text, optionMenuItem.palette.highlightedText().color());

            QCommonStyle::drawItemText(painter, textRect, text_flags,
                    optionMenuItem.palette, enabled,
                    optionMenuItem.text, QPalette::Text);

            //In Sym^3, native menu items have "lines" between them
            if (QS60StylePrivate::isSingleClickUi()) {
                int diff = widget->geometry().bottom() - optionMenuItem.rect.bottom();
                if (const QComboBox *cb = qobject_cast<const QComboBox*>(widget))
                    diff = cb->view()->geometry().bottom() - optionMenuItem.rect.bottom();

                // Skip drawing the horizontal line for the last menu item.
                if (diff > optionMenuItem.rect.height()) {
                    const QColor lineColorAlpha = QS60StylePrivate::s60Color(QS60StyleEnums::CL_QsnLineColors, 15, 0);
                    //native platform sets each color byte to same value for "line 16" which just defines alpha for
                    //menuitem lines; lets use first byte "red".
                    QColor lineColor = optionMenuItem.palette.text().color();
                    if (lineColorAlpha.isValid())
                        lineColor.setAlpha(lineColorAlpha.red());
                    painter->save();
                    painter->setPen(lineColor);
                    const int horizontalMargin = 2 * QS60StylePrivate::pixelMetric(PM_FrameCornerWidth) - QS60StylePrivate::pixelMetric(PM_DefaultFrameWidth);
                    const int lineStartX = optionMenuItem.rect.left() + horizontalMargin;
                    const int lineEndX = optionMenuItem.rect.right() - horizontalMargin;
                    painter->drawLine(QPoint(lineStartX, optionMenuItem.rect.bottom()), QPoint(lineEndX, optionMenuItem.rect.bottom()));
                    painter->restore();
                }
            }
            if (!enabled)
                painter->restore();
        }
        break;
    case CE_MenuEmptyArea:
        break;
#endif //QT_NO_MENU

#ifndef QT_NO_MENUBAR
    case CE_MenuBarEmptyArea:
        break;
#endif //QT_NO_MENUBAR

    case CE_HeaderSection:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            painter->save();
            QPen linePen = QPen(QS60StylePrivate::s60Color(QS60StyleEnums::CL_QsnLineColors, 1, header));
            const int penWidth = (header->orientation == Qt::Horizontal) ?
                linePen.width() + QS60StylePrivate::pixelMetric(PM_BoldLineWidth)
                : linePen.width() + QS60StylePrivate::pixelMetric(PM_ThinLineWidth);
            linePen.setWidth(penWidth);
            painter->setPen(linePen);
            if (header->orientation == Qt::Horizontal){
                painter->drawLine(header->rect.bottomLeft(), header->rect.bottomRight());
            } else {
                if ( header->direction == Qt::LeftToRight ) {
                    painter->drawLine(header->rect.topRight(), header->rect.bottomRight());
                } else {
                    painter->drawLine(header->rect.topLeft(), header->rect.bottomLeft());
                }
            }
            painter->restore();

            //Draw corner button as normal pushButton.
            if (qobject_cast<const QAbstractButton *>(widget)) {
                //Make cornerButton slightly smaller so that it is not on top of table border graphic.
                QStyleOptionHeader subopt = *header;
                const int borderTweak =
                    QS60StylePrivate::pixelMetric(PM_FrameCornerWidth) >> 1;
                if (subopt.direction == Qt::LeftToRight)
                    subopt.rect.adjust(borderTweak, borderTweak, 0, -borderTweak);
                else
                    subopt.rect.adjust(0, borderTweak, -borderTweak, -borderTweak);
                drawPrimitive(PE_PanelButtonBevel, &subopt, painter, widget);
            } else if ((header->palette.brush(QPalette::Button) != Qt::transparent)) {
                //Draw non-themed background. Background for theme is drawn in CE_ShapedFrame
                //to get continuous theme graphic across all the header cells.
                qDrawShadePanel(painter, header->rect, header->palette,
                                header->state & (State_Sunken | State_On), penWidth,
                                &header->palette.brush(QPalette::Button));
            }
        }
        break;
    case CE_HeaderEmptyArea: // no need to draw this
        break;
    case CE_Header:
        if ( const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            drawControl(CE_HeaderSection, header, painter, widget);
            QStyleOptionHeader subopt = *header;
            subopt.rect = subElementRect(SE_HeaderLabel, header, widget);
            if (subopt.rect.isValid())
                drawControl(CE_HeaderLabel, &subopt, painter, widget);
            if (header->sortIndicator != QStyleOptionHeader::None) {
                subopt.rect = subElementRect(SE_HeaderArrow, option, widget);
                drawPrimitive(PE_IndicatorHeaderArrow, &subopt, painter, widget);
            }
        }
        break;
#ifndef QT_NO_TOOLBAR
    case CE_ToolBar:
        if (const QStyleOptionToolBar *toolBar = qstyleoption_cast<const QStyleOptionToolBar *>(option)) {
            const QToolBar *tbWidget = qobject_cast<const QToolBar *>(widget);

            //toolbar within a toolbar, skip
            if (!tbWidget || (widget && qobject_cast<QToolBar *>(widget->parentWidget())))
                break;

            // Normally in S60 5.0+ there is no background for toolbar, but in some cases with versatile QToolBar,
            // it looks a bit strange. So, lets fillRect with Button.
            if (!QS60StylePrivate::isToolBarBackground()) {
                QList<QAction *> actions = tbWidget->actions();
                bool justToolButtonsInToolBar = true;
                for (int i = 0; i < actions.size(); ++i) {
                    QWidget *childWidget = tbWidget->widgetForAction(actions.at(i));
                    const QToolButton *button = qobject_cast<const QToolButton *>(childWidget);
                    if (!button){
                        justToolButtonsInToolBar = false;
                    }
                }

                // Draw frame background
                // for vertical toolbars with text only and
                // for toolbars with extension buttons and
                // for toolbars with widgets in them.
                if (!justToolButtonsInToolBar ||
                        (tbWidget &&
                         (tbWidget->orientation() == Qt::Vertical) &&
                         (tbWidget->toolButtonStyle() == Qt::ToolButtonTextOnly))) {
                        painter->save();
                        if (widget)
                            painter->setBrush(widget->palette().button());
                        painter->setOpacity(0.3);
                        painter->fillRect(toolBar->rect, painter->brush());
                        painter->restore();
                }
            } else {
                QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_ToolBar, painter, toolBar->rect, flags);
            }
        }
        break;
#endif //QT_NO_TOOLBAR
    case CE_ShapedFrame:
        if (const QTextEdit *textEdit = qobject_cast<const QTextEdit *>(widget)) {
            const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(option);
            if (frame && QS60StylePrivate::canDrawThemeBackground(frame->palette.base(), widget))
                QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_Editor, painter, option->rect, flags);
            else
                QCommonStyle::drawControl(element, option, painter, widget);
        } else if (qobject_cast<const QTableView *>(widget)) {
            QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_TableItem, painter, option->rect, flags);
        } else if (const QHeaderView *header = qobject_cast<const QHeaderView *>(widget)) {
            //QS60style draws header background here instead of in each headersection, to get
            //continuous graphic from section to section.
            QS60StylePrivate::SkinElementFlags adjustableFlags = flags;
            QRect headerRect = option->rect;
            if (header->orientation() != Qt::Horizontal) {
                //todo: update to horizontal table graphic
                adjustableFlags = (adjustableFlags | QS60StylePrivate::SF_PointWest);
            } else {
                const int frameWidth = QS60StylePrivate::pixelMetric(PM_DefaultFrameWidth);
                if (option->direction == Qt::LeftToRight)
                    headerRect.adjust(-2 * frameWidth, 0, 0, 0);
                else
                    headerRect.adjust(0, 0, 2 * frameWidth, 0);
            }
            if (option->palette.brush(QPalette::Button).color() == Qt::transparent)
                QS60StylePrivate::drawSkinElement(
                        QS60StylePrivate::SE_TableHeaderItem, painter, headerRect, adjustableFlags);

        } else if (qobject_cast<const QFrame *>(widget)) {
            QCommonStyle::drawControl(element, option, painter, widget);
        }
        break;
    case CE_MenuScroller:
        break;
    case CE_FocusFrame: {
#ifdef QT_KEYPAD_NAVIGATION
            bool editFocus = false;
            if (const QFocusFrame *focusFrame = qobject_cast<const QFocusFrame*>(widget)) {
                if (focusFrame->widget() && focusFrame->widget()->hasEditFocus())
                    editFocus = true;
            }
            const qreal opacity = editFocus ? 1 : 0.75; // Trial and error factors. Feel free to improve.
#else
            const qreal opacity = 0.85;
#endif
            // We need to reduce the focus frame size if LayoutSpacing is smaller than FocusFrameMargin
            // Otherwise, we would overlay adjacent widgets.
            const int frameHeightReduction =
                    qMin(0, pixelMetric(PM_LayoutVerticalSpacing)
                            - pixelMetric(PM_FocusFrameVMargin));
            const int frameWidthReduction =
                    qMin(0, pixelMetric(PM_LayoutHorizontalSpacing)
                            - pixelMetric(PM_FocusFrameHMargin));
            const int rounding =
                    qMin(pixelMetric(PM_FocusFrameVMargin),
                            pixelMetric(PM_LayoutVerticalSpacing));
            const QRect frameRect =
                    option->rect.adjusted(-frameWidthReduction, -frameHeightReduction,
                            frameWidthReduction, frameHeightReduction);
            QPainterPath framePath;
            framePath.addRoundedRect(frameRect, rounding, rounding);

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setOpacity(opacity);
            painter->fillPath(framePath, option->palette.color(QPalette::Text));
            painter->restore();
        }
        break;
    case CE_Splitter:
        if (option->state & State_Sunken && option->state & State_Enabled && QS60StylePrivate::themePalette()) {
            painter->save();
            painter->setOpacity(0.5);
            painter->setBrush(QS60StylePrivate::themePalette()->light());
            painter->setRenderHint(QPainter::Antialiasing);
            const qreal roundRectRadius = 4 * goldenRatio;
            painter->drawRoundedRect(option->rect, roundRectRadius, roundRectRadius);
            painter->restore();
        }
        break;
    default:
        QCommonStyle::drawControl(element, option, painter, widget);
    }
}

/*!
  \reimp
*/
void QS60Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    const QS60StylePrivate::SkinElementFlags flags = (option->state & State_Enabled) ?  QS60StylePrivate::SF_StateEnabled : QS60StylePrivate::SF_StateDisabled;
    bool commonStyleDraws = false;

    switch (element) {
        case PE_FrameFocusRect: {
            //Draw themed highlight to radiobuttons and checkboxes.
            //For other widgets skip, unless palette has been modified. In that case, draw with commonstyle.
            if (QS60StylePrivate::equalToThemePalette(option->palette.highlight().color(), QPalette::Highlight)) {
                if ((qstyleoption_cast<const QStyleOptionFocusRect *>(option) &&
                    (qobject_cast<const QRadioButton *>(widget) || qobject_cast<const QCheckBox *>(widget))))
                        QS60StylePrivate::drawSkinElement(
                            QS60StylePrivate::isWidgetPressed(widget) ? 
                                QS60StylePrivate::SE_ListItemPressed : 
                                QS60StylePrivate::SE_ListHighlight, painter, option->rect, flags);
            } else {
                commonStyleDraws = true;
            }
        }
        break;
#ifndef QT_NO_LINEEDIT
    case PE_PanelLineEdit:
        if (const QStyleOptionFrame *lineEdit = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
#ifndef QT_NO_COMBOBOX
            if (widget && qobject_cast<const QComboBox *>(widget->parentWidget()))
                break;
#endif
            if (QS60StylePrivate::canDrawThemeBackground(option->palette.base(), widget))
                QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_FrameLineEdit, painter, option->rect, flags);
            else
                commonStyleDraws = true;
        }
    break;
#endif // QT_NO_LINEEDIT
    case PE_IndicatorCheckBox: {
            // Draw checkbox indicator as color skinned graphics.
            const QS60StyleEnums::SkinParts skinPart = (option->state & State_On) ?
                QS60StyleEnums::SP_QgnIndiCheckboxOn : QS60StyleEnums::SP_QgnIndiCheckboxOff;
            painter->save();

            if (QS60StylePrivate::equalToThemePalette(option->palette.windowText().color(), QPalette::WindowText))
                painter->setPen(option->palette.windowText().color());

            QS60StylePrivate::drawSkinPart(skinPart, painter, option->rect, flags | QS60StylePrivate::SF_ColorSkinned );
            painter->restore();
        }
        break;
    case PE_IndicatorViewItemCheck:
#ifndef QT_NO_ITEMVIEWS
        if (const QAbstractItemView *itemView = (qobject_cast<const QAbstractItemView *>(widget))) {
            if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option)) {
                const bool checkBoxVisible = vopt->features & QStyleOptionViewItemV2::HasCheckIndicator;
                const bool singleSelection = itemView->selectionMode() ==
                    QAbstractItemView::SingleSelection || itemView->selectionMode() == QAbstractItemView::NoSelection;
                // draw either checkbox at the beginning
                if (checkBoxVisible && singleSelection) {
                    drawPrimitive(PE_IndicatorCheckBox, option, painter, widget);
                // ... or normal "tick" selection at the end.
                } else if (option->state & State_Selected) {
                    QRect tickRect = option->rect;
                    const int frameBorderWidth = QS60StylePrivate::pixelMetric(PM_FrameCornerWidth);
                    // adjust tickmark rect to exclude frame border
                    tickRect.adjust(0, -frameBorderWidth, 0, -frameBorderWidth);
                    QS60StyleEnums::SkinParts skinPart = QS60StyleEnums::SP_QgnIndiMarkedAdd;
                    QS60StylePrivate::drawSkinPart(skinPart, painter, tickRect,
                        (flags | QS60StylePrivate::SF_ColorSkinned));
                }
            }
        }
#endif //QT_NO_ITEMVIEWS
        break;
    case PE_IndicatorRadioButton: {
            QRect buttonRect = option->rect;
            //there is empty (a. 33%) space in svg graphics for radiobutton
            const qreal reduceWidth = (qreal)buttonRect.width() / 3.0;
            const qreal rectWidth = (qreal)option->rect.width() != 0 ? option->rect.width() : 1.0;
            // Try to occupy the full area
            const qreal scaler = 1 + (reduceWidth/rectWidth);
            buttonRect.setWidth((int)((buttonRect.width()-reduceWidth) * scaler));
            buttonRect.setHeight((int)(buttonRect.height() * scaler));
            // move the rect up for half of the new height-gain
            const int newY = (buttonRect.bottomRight().y() - option->rect.bottomRight().y()) >> 1 ;
            buttonRect.adjust(0, -newY, -1, -newY);

            painter->save();
            const QColor themeColor = QS60StylePrivate::s60Color(QS60StyleEnums::CL_QsnTextColors, 6, option);
            const QColor buttonTextColor = option->palette.buttonText().color();
            if (themeColor != buttonTextColor)
                painter->setPen(buttonTextColor);
            else
                painter->setPen(themeColor);

            // Draw radiobutton indicator as color skinned graphics.
            QS60StyleEnums::SkinParts skinPart = (option->state & State_On) ?
                QS60StyleEnums::SP_QgnIndiRadiobuttOn : QS60StyleEnums::SP_QgnIndiRadiobuttOff;
            QS60StylePrivate::drawSkinPart(skinPart, painter, buttonRect,
                (flags | QS60StylePrivate::SF_ColorSkinned));
            painter->restore();
        }
        break;
    case PE_PanelButtonCommand:
    case PE_PanelButtonTool:
    case PE_PanelButtonBevel:
    case PE_FrameButtonBevel:
        if (QS60StylePrivate::canDrawThemeBackground(option->palette.base(), widget)) {
            const bool isPressed = (option->state & State_Sunken) || (option->state & State_On);
            QS60StylePrivate::SkinElements skinElement;
            if (element == PE_PanelButtonTool)
                skinElement = isPressed ? QS60StylePrivate::SE_ToolBarButtonPressed : QS60StylePrivate::SE_ToolBarButton;
            else
                skinElement = isPressed ? QS60StylePrivate::SE_ButtonPressed : QS60StylePrivate::SE_ButtonNormal;
            QS60StylePrivate::drawSkinElement(skinElement, painter, option->rect, flags);
        } else {
            commonStyleDraws = true;
        }
        break;
#ifndef QT_NO_TOOLBUTTON
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowUp: {
        QS60StyleEnums::SkinParts skinPart;
        if (element==PE_IndicatorArrowDown)
            skinPart = QS60StyleEnums::SP_QgnGrafScrollArrowDown;
        else if (element==PE_IndicatorArrowLeft)
            skinPart = QS60StyleEnums::SP_QgnGrafScrollArrowLeft;
        else if (element==PE_IndicatorArrowRight)
            skinPart = QS60StyleEnums::SP_QgnGrafScrollArrowRight;
        else if (element==PE_IndicatorArrowUp)
            skinPart = QS60StyleEnums::SP_QgnGrafScrollArrowUp;

        QS60StylePrivate::drawSkinPart(skinPart, painter, option->rect, flags);
        }
        break;
#endif //QT_NO_TOOLBUTTON
#ifndef QT_NO_SPINBOX
    case PE_IndicatorSpinDown:
    case PE_IndicatorSpinUp:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            if (QS60StylePrivate::canDrawThemeBackground(spinBox->palette.base(), widget)) {
                QStyleOptionSpinBox optionSpinBox = *spinBox;
                const QS60StyleEnums::SkinParts part = (element == PE_IndicatorSpinUp) ?
                    QS60StyleEnums::SP_QgnGrafScrollArrowUp :
                    QS60StyleEnums::SP_QgnGrafScrollArrowDown;
                const int iconMargin = QS60StylePrivate::pixelMetric(PM_FrameCornerWidth) >> 1;
                optionSpinBox.rect.translate(0, (element == PE_IndicatorSpinDown) ? iconMargin : -iconMargin );
                QS60StylePrivate::drawSkinPart(part, painter, optionSpinBox.rect, flags);
            } else {
                commonStyleDraws = true;
            }
        }
#endif //QT_NO_SPINBOX
#ifndef QT_NO_COMBOBOX
        if (const QStyleOptionFrame *cmb = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            if (QS60StylePrivate::canDrawThemeBackground( option->palette.base(), widget)) {
                // We want to draw down arrow here for comboboxes as well.
                QStyleOptionFrame optionsComboBox = *cmb;
                const QS60StyleEnums::SkinParts part = QS60StyleEnums::SP_QgnGrafScrollArrowDown;
                const int iconMargin = QS60StylePrivate::pixelMetric(PM_FrameCornerWidth) >> 1;
                optionsComboBox.rect.translate(0, (element == PE_IndicatorSpinDown) ? iconMargin : -iconMargin );
                QS60StylePrivate::drawSkinPart(part, painter, optionsComboBox.rect, flags);
            } else {
                commonStyleDraws = true;
            }
        }
#endif //QT_NO_COMBOBOX
        break;
    case PE_IndicatorSpinMinus:
    case PE_IndicatorSpinPlus:
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            QStyleOptionSpinBox optionSpinBox = *spinBox;
            QCommonStyle::drawPrimitive(element, &optionSpinBox, painter, widget);
        }
#ifndef QT_NO_COMBOBOX
        else if (const QStyleOptionFrame *cmb = qstyleoption_cast<const QStyleOptionFrame *>(option)) {
            // We want to draw down arrow here for comboboxes as well.
            QStyleOptionFrame comboBox = *cmb;
            const int frameWidth = QS60StylePrivate::pixelMetric(PM_DefaultFrameWidth);
            comboBox.rect.adjust(0, frameWidth, 0, -frameWidth);
            QCommonStyle::drawPrimitive(element, &comboBox, painter, widget);
        }
#endif //QT_NO_COMBOBOX
        break;
    case PE_Widget:
        if (QS60StylePrivate::drawsOwnThemeBackground(widget)
#ifndef QT_NO_COMBOBOX
            || qobject_cast<const QComboBoxListView *>(widget)
#endif //QT_NO_COMBOBOX
#ifndef QT_NO_MENU
            || qobject_cast<const QMenu *> (widget)
#endif //QT_NO_MENU
            ) {
            //Need extra check since dialogs have their own theme background
            if (QS60StylePrivate::canDrawThemeBackground(option->palette.base(), widget)
                && QS60StylePrivate::equalToThemePalette(option->palette.window().texture().cacheKey(), QPalette::Window)) {
                    const bool comboMenu = qobject_cast<const QComboBoxListView *>(widget);
                    // Add margin area to the background, to avoid background being cut for first and last item.
                    const int verticalMenuAdjustment = comboMenu ? QS60StylePrivate::pixelMetric(PM_MenuVMargin) : 0;
                    const QRect adjustedMenuRect = option->rect.adjusted(0, -verticalMenuAdjustment, 0, verticalMenuAdjustment);
                    QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_PopupBackground, painter, adjustedMenuRect, flags);
            } else {
                commonStyleDraws = true;
            }
        }
        break;
    case PE_FrameWindow:
    case PE_FrameTabWidget:
        if (const QStyleOptionTabWidgetFrame *tabFrame = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            QStyleOptionTabWidgetFrame optionTabFrame = *tabFrame;
            QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_PanelBackground, painter, optionTabFrame.rect, flags);
        }
        break;
    case PE_IndicatorHeaderArrow:
        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            if (header->sortIndicator & QStyleOptionHeader::SortUp)
                drawPrimitive(PE_IndicatorArrowUp, header, painter, widget);
            else if (header->sortIndicator & QStyleOptionHeader::SortDown)
                drawPrimitive(PE_IndicatorArrowDown, header, painter, widget);
        } // QStyleOptionHeader::None is not drawn => not needed
        break;
#ifndef QT_NO_GROUPBOX
    case PE_FrameGroupBox:
        if (const QStyleOptionFrameV2 *frame = qstyleoption_cast<const QStyleOptionFrameV2 *>(option))
            QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_SettingsList, painter, frame->rect, flags);
        break;
#endif //QT_NO_GROUPBOX

    // Qt3 primitives are not supported
    case PE_Q3CheckListController:
    case PE_Q3CheckListExclusiveIndicator:
    case PE_Q3CheckListIndicator:
    case PE_Q3DockWindowSeparator:
    case PE_Q3Separator:
        Q_ASSERT(false);
        break;
    case PE_Frame:
        break;
#ifndef QT_NO_ITEMVIEWS
    case PE_PanelItemViewItem:
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option)) {
            const bool isSelected = (vopt->state & State_Selected);
            const bool hasFocus = (vopt->state & State_HasFocus);
            const bool isPressed = QS60StylePrivate::isWidgetPressed(widget);

            if (QS60StylePrivate::equalToThemePalette(option->palette.highlight().color(), QPalette::Highlight)) {
                QRect highlightRect = vopt->rect.adjusted(1,1,-1,-1);
                const QAbstractItemView *itemView = qobject_cast<const QAbstractItemView *>(widget);
                QAbstractItemView::SelectionBehavior selectionBehavior =
                    itemView ? itemView->selectionBehavior() : QAbstractItemView::SelectItems;
                // Set the draw area for highlights (focus, select rect or pressed rect)
                if (hasFocus || isPressed) {
                    if (selectionBehavior != QAbstractItemView::SelectItems) {
                        // set highlight rect so that it is continuous from cell to cell, yet sligthly
                        // smaller than cell rect
                        int xBeginning = 0, yBeginning = 0, xEnd = 0, yEnd = 0;
                        if (selectionBehavior == QAbstractItemView::SelectRows) {
                            yBeginning = 1; yEnd = -1;
                            if (vopt->viewItemPosition == QStyleOptionViewItemV4::Beginning)
                                xBeginning = 1;
                            else if (vopt->viewItemPosition == QStyleOptionViewItemV4::End)
                                xEnd = -1;
                        } else if (selectionBehavior == QAbstractItemView::SelectColumns) {
                            xBeginning = 1; xEnd = -1;
                            if (vopt->viewItemPosition == QStyleOptionViewItemV4::Beginning)
                                yBeginning = 1;
                            else if (vopt->viewItemPosition == QStyleOptionViewItemV4::End)
                                yEnd = -1;
                        }
                        highlightRect = option->rect.adjusted(xBeginning, yBeginning, xEnd, yEnd);
                    }
                }
                bool tableView = false;
                if (itemView && qobject_cast<const QTableView *>(widget))
                    tableView = true;

                QS60StylePrivate::SkinElements element;
                bool themeGraphicDefined = false;
                QRect elementRect = option->rect;

                //draw item is drawn as pressed, if it already has focus.
                if (isPressed && hasFocus) {
                    themeGraphicDefined = true;
                    element = tableView ? QS60StylePrivate::SE_TableItemPressed : QS60StylePrivate::SE_ListItemPressed;
                } else if (hasFocus || (isSelected && selectionBehavior != QAbstractItemView::SelectItems)) {
                    element = QS60StylePrivate::SE_ListHighlight;
                    elementRect = highlightRect;
                    themeGraphicDefined = true;
                }
                if (themeGraphicDefined)
                    QS60StylePrivate::drawSkinElement(element, painter, elementRect, flags);
            } else {
                QCommonStyle::drawPrimitive(element, option, painter, widget);
            }
        }
        break;
#endif //QT_NO_ITEMVIEWS

    case PE_IndicatorMenuCheckMark:
        if (const QStyleOptionMenuItem *checkBox = qstyleoption_cast<const QStyleOptionMenuItem *>(option)){
            QStyleOptionMenuItem optionCheckBox = *checkBox;
            if (optionCheckBox.checked)
                optionCheckBox.state = (optionCheckBox.state | State_On);
            drawPrimitive(PE_IndicatorCheckBox, &optionCheckBox, painter, widget);
        }
        break;
#ifndef QT_NO_TOOLBAR
    case PE_IndicatorToolBarHandle:
        // no toolbar handles in S60/AVKON UI
    case PE_IndicatorToolBarSeparator:
        // no separators in S60/AVKON UI
        break;
#endif //QT_NO_TOOLBAR

    case PE_PanelMenuBar:
    case PE_FrameMenu:
        break; //disable frame in menu

    case PE_IndicatorBranch:
#if defined(Q_WS_S60)
        // 3.1 AVKON UI does not have tree view component, use common style for drawing there
        if (QSysInfo::s60Version() == QSysInfo::SV_S60_3_1) {
#else
        if (true) {
#endif
            QCommonStyle::drawPrimitive(element, option, painter, widget);
        } else {
            if (const QStyleOptionViewItemV2 *vopt = qstyleoption_cast<const QStyleOptionViewItemV2 *>(option)) {
                const bool rightLine = option->state & State_Item;
                const bool downLine = option->state & State_Sibling;
                const bool upLine = option->state & (State_Open | State_Children | State_Item | State_Sibling);
                QS60StylePrivate::SkinElementFlags adjustedFlags = flags;

                QS60StyleEnums::SkinParts skinPart;
                bool drawSkinPart = false;
                if (rightLine && downLine && upLine) {
                    skinPart = QS60StyleEnums::SP_QgnIndiHlLineBranch;
                    drawSkinPart = true;
                } else if (rightLine && upLine) {
                    skinPart = QS60StyleEnums::SP_QgnIndiHlLineEnd;
                    drawSkinPart = true;
                } else if (upLine && downLine) {
                    skinPart = QS60StyleEnums::SP_QgnIndiHlLineStraight;
                    drawSkinPart = true;
                }

                if (option->direction == Qt::RightToLeft)
                    adjustedFlags |= QS60StylePrivate::SF_Mirrored_X_Axis;

                if (drawSkinPart)
                    QS60StylePrivate::drawSkinPart(skinPart, painter, option->rect, adjustedFlags);

                if (option->state & State_Children) {
                    QS60StyleEnums::SkinParts skinPart =
                            (option->state & State_Open) ? QS60StyleEnums::SP_QgnIndiHlColSuper : QS60StyleEnums::SP_QgnIndiHlExpSuper;
                    const QRect selectionRect = subElementRect(SE_ItemViewItemCheckIndicator, vopt, widget);
                    const int minDimension = qMin(option->rect.width(), option->rect.height());
                    const int magicTweak = (option->direction == Qt::RightToLeft) ? -3 : 3; //@todo: magic
                    //The branch indicator icon in S60 is supposed to be superimposed on top of branch lines.
                    QRect iconRect(QPoint(option->rect.left() + magicTweak, selectionRect.top() + 1), QSize(minDimension, minDimension));
                    if (!QS60StylePrivate::isTouchSupported())
                        iconRect.translate(0, -4); //@todo: magic
                    QS60StylePrivate::drawSkinPart(skinPart, painter, iconRect, adjustedFlags);
                }
            }
        }
        break;
    case PE_PanelItemViewRow: // ### Qt 5: remove
#ifndef QT_NO_ITEMVIEWS
        if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(option)) {
            if (!QS60StylePrivate::equalToThemePalette(vopt->palette.base().texture().cacheKey(), QPalette::Base)) {
                //QPalette::Base has been changed, let commonstyle draw the item
                commonStyleDraws = true;
            } else {
                QPalette::ColorGroup cg = vopt->state & State_Enabled ? QPalette::Normal : QPalette::Disabled;
                if (cg == QPalette::Normal && !(vopt->state & State_Active))
                    cg = QPalette::Inactive;
                if (vopt->features & QStyleOptionViewItemV2::Alternate)
                    painter->fillRect(vopt->rect, vopt->palette.brush(cg, QPalette::AlternateBase));
                //apart from alternate base, no background for list item is drawn for S60Style
            }
        }
#endif
        break;
    case PE_PanelScrollAreaCorner:
        break;
    case PE_IndicatorItemViewItemDrop:
        if (QS60StylePrivate::isTouchSupported())
            QS60StylePrivate::drawSkinElement(QS60StylePrivate::SE_DropArea, painter, option->rect, flags);
        else
            commonStyleDraws = true;
        break;
        // todo: items are below with #ifdefs "just in case". in final version, remove all non-required cases
    case PE_FrameLineEdit:
    case PE_IndicatorDockWidgetResizeHandle:
    case PE_PanelTipLabel:

#ifndef QT_NO_TABBAR
    case PE_IndicatorTabTear: // No tab tear in S60
#endif // QT_NO_TABBAR
    case PE_FrameDefaultButton:
#ifndef QT_NO_DOCKWIDGET
    case PE_FrameDockWidget:
#endif //QT_NO_DOCKWIDGET
#ifndef QT_NO_PROGRESSBAR
    case PE_IndicatorProgressChunk:
#endif //QT_NO_PROGRESSBAR
#ifndef QT_NO_TOOLBAR
    case PE_PanelToolBar:
#endif //QT_NO_TOOLBAR
#ifndef QT_NO_COLUMNVIEW
    case PE_IndicatorColumnViewArrow:
#endif //QT_NO_COLUMNVIEW
    case PE_FrameTabBarBase: // since tabs are in S60 always in navipane, let's use common style for tab base in Qt.
    default:
        commonStyleDraws = true;
    }
    if (commonStyleDraws) {
        QCommonStyle::drawPrimitive(element, option, painter, widget);
    }
}

/*! \reimp */
int QS60Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    int metricValue = QS60StylePrivate::pixelMetric(metric);
    if (metricValue == KNotFound)
        metricValue = QCommonStyle::pixelMetric(metric, option, widget);

    // Menu scrollers should be set to zero height for combobox popups
    if (metric == PM_MenuScrollerHeight && !qobject_cast<const QMenu *>(widget))
        metricValue = 0;

    //if layout direction is mirrored, switch left and right border margins
    if (option && option->direction == Qt::RightToLeft) {
        if (metric == PM_LayoutLeftMargin)
            metricValue = QS60StylePrivate::pixelMetric(PM_LayoutRightMargin);
        else if (metric == PM_LayoutRightMargin)
            metricValue = QS60StylePrivate::pixelMetric(PM_LayoutLeftMargin);
    }

    if (widget && (metric == PM_LayoutTopMargin || metric == PM_LayoutLeftMargin || metric == PM_LayoutRightMargin))
        if (widget->windowType() == Qt::Dialog)
            //double the layout margins (except bottom) for dialogs, it is very close to real value
            //without having to define custom pixel metric
            metricValue *= 2;

#if defined(Q_WS_S60)
    if (metric == PM_TabBarTabOverlap && (QSysInfo::s60Version() > QSysInfo::SV_S60_5_2))
        metricValue = 0;
#endif

    return metricValue;
}

/*! \reimp */
QSize QS60Style::sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                  const QSize &csz, const QWidget *widget) const
{
    QSize sz(csz);
    switch (ct) {
        case CT_ToolButton:
            sz = QCommonStyle::sizeFromContents( ct, opt, csz, widget);
            //FIXME properly - style should calculate the location of border frame-part
            sz += QSize(2 * pixelMetric(PM_ButtonMargin), 2 * pixelMetric(PM_ButtonMargin));
            if (const QStyleOptionToolButton *toolBtn = qstyleoption_cast<const QStyleOptionToolButton *>(opt))
                if (toolBtn->subControls & SC_ToolButtonMenu)
                    sz += QSize(pixelMetric(PM_MenuButtonIndicator), 0);

            //Make toolbuttons in toolbar stretch the whole screen area
            if (widget && qobject_cast<const QToolBar *>(widget->parentWidget())) {
                const QToolBar *tb = qobject_cast<const QToolBar *>(widget->parentWidget());
                const bool parentCanGrowHorizontally = !(tb->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed ||
                        tb->sizePolicy().horizontalPolicy() == QSizePolicy::Maximum) && tb->orientation() == Qt::Horizontal;

                if (parentCanGrowHorizontally) {
                    int buttons = 0;
                    //Make the auto-stretch to happen only for horizontal orientation
                    if (tb && tb->orientation() == Qt::Horizontal) {
                        QList<QAction*> actionList =  tb->actions();
                        for (int i = 0; i < actionList.count(); i++) {
                            buttons++;
                        }
                    }

                    if (widget->parentWidget() && buttons > 0) {
                        QWidget *w = const_cast<QWidget *>(widget);
                        int toolBarMaxWidth = 0;
                        int totalMargin = 0;
                        while (w) {
                            //honor fixed width parents
                            if (w->maximumWidth() == w->minimumWidth())
                                toolBarMaxWidth = qMax(toolBarMaxWidth, w->maximumWidth());
                            if (w->layout() && w->windowType() == Qt::Widget) {
                                totalMargin += w->layout()->contentsMargins().left() +
                                               w->layout()->contentsMargins().right();
                            }
                            w = w->parentWidget();
                        }
                        totalMargin += 2 * pixelMetric(QStyle::PM_ToolBarFrameWidth);

                        if (toolBarMaxWidth == 0)
                            toolBarMaxWidth =
                                QApplication::desktop()->availableGeometry(widget->parentWidget()).width();
                        //Reduce the margins, toolbar frame, item spacing and internal margin from available area
                        toolBarMaxWidth -= totalMargin;

                        //ensure that buttons are side-by-side and not on top of each other
                        const int toolButtonWidth = (toolBarMaxWidth / buttons)
                                - pixelMetric(QStyle::PM_ToolBarItemSpacing)
                                - pixelMetric(QStyle::PM_ToolBarItemMargin)
                        //toolbar frame needs to be reduced again, since QToolBarLayout adds it for each toolbar action
                                - 2 * pixelMetric(QStyle::PM_ToolBarFrameWidth) - 1;
                        sz.setWidth(qMax(toolButtonWidth, sz.width()));
                    }
                }
            }
            break;
        case CT_PushButton:
            sz = QCommonStyle::sizeFromContents( ct, opt, csz, widget);
            //FIXME properly - style should calculate the location of border frame-part
            if (const QAbstractButton *buttonWidget = (qobject_cast<const QAbstractButton *>(widget)))  {
                if (buttonWidget->isCheckable())
                    sz += QSize(pixelMetric(PM_IndicatorWidth) + pixelMetric(PM_CheckBoxLabelSpacing), 0);
                const int iconHeight = (!buttonWidget->icon().isNull()) ? buttonWidget->iconSize().height() : 0;
                const int textHeight = (buttonWidget->text().length() > 0) ?
                    buttonWidget->fontMetrics().size(Qt::TextSingleLine, buttonWidget->text()).height() : opt->fontMetrics.height();
                const int decoratorHeight = (buttonWidget->isCheckable()) ? pixelMetric(PM_IndicatorHeight) : 0;

                const int contentHeight =
                        qMax(qMax(iconHeight, decoratorHeight) + pixelMetric(PM_ButtonMargin),
                             textHeight + 2*pixelMetric(PM_ButtonMargin));
                sz.setHeight(qMax(sz.height(), contentHeight));
                sz += QSize(2 * pixelMetric(PM_ButtonMargin), 0);
            }
            break;
        case CT_LineEdit:
            if (const QStyleOptionFrame *f = qstyleoption_cast<const QStyleOptionFrame *>(opt))
                sz += QSize(2 * f->lineWidth, 4 * f->lineWidth);
            break;
        case CT_TabBarTab: {
                sz = QCommonStyle::sizeFromContents(ct, opt, csz, widget);
                // Adjust beginning tabbar item size, if scrollbuttons are used. This is to ensure that the
                // tabbar item content fits, since scrollbuttons are making beginning tabbar item smaller.
                int scrollButtonSize = 0;
                if (const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget))
                    scrollButtonSize = tabBar->usesScrollButtons() ? pixelMetric(PM_TabBarScrollButtonWidth) : 0;
                if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt)) {
                    const bool verticalTabs = tab->shape == QTabBar::RoundedEast
                            || tab->shape == QTabBar::RoundedWest
                            || tab->shape == QTabBar::TriangularEast
                            || tab->shape == QTabBar::TriangularWest;
                    if (tab->position == QStyleOptionTab::Beginning)
                        sz += QSize(verticalTabs ? 0 : scrollButtonSize, !verticalTabs ? 0 : scrollButtonSize);
                }
            }
            break;
        case CT_MenuItem:
        case CT_ItemViewItem:
            if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
                if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                    sz = QSize(menuItem->rect.width() - 2 * pixelMetric(PM_MenuHMargin) - 2 * QS60StylePrivate::pixelMetric(PM_FrameCornerWidth), 1);
                    break;
                }
            }
            sz = QCommonStyle::sizeFromContents( ct, opt, csz, widget);
            if (QS60StylePrivate::isTouchSupported()) {
                //Make itemview easier to use in touch devices
                sz.setHeight(sz.height() + 2 * pixelMetric(PM_FocusFrameVMargin));
                //QCommonStyle does not adjust height with horizontal margin, it only adjusts width
                if (ct == CT_MenuItem)
                    sz.setHeight(sz.height() - 8); //QCommonstyle adds 8 to height that this style handles through PM values
            }
            break;
#ifndef QT_NO_COMBOBOX
        case CT_ComboBox: {
                // Fixing Ui design issues with too wide QComboBoxes and greedy SizeHints
                // Make sure, that the combobox stays within the screen.
                const QSize desktopContentSize = QApplication::desktop()->availableGeometry().size()
                        - QSize(pixelMetric(PM_LayoutLeftMargin) + pixelMetric(PM_LayoutRightMargin), 0);
                sz = QCommonStyle::sizeFromContents(ct, opt, csz, widget).
                        boundedTo(desktopContentSize);
            }
            break;
#endif
        default:
            sz = QCommonStyle::sizeFromContents( ct, opt, csz, widget);
            break;
    }
    if (!sz.isValid())
        sz = QCommonStyle::sizeFromContents(ct, opt, csz, widget);
    return sz;
}

/*! \reimp */
int QS60Style::styleHint(StyleHint sh, const QStyleOption *opt, const QWidget *widget,
                            QStyleHintReturn *hret) const
{
    int retValue = 0;
    switch (sh) {
        case SH_RequestSoftwareInputPanel:
            if (QS60StylePrivate::isSingleClickUi())
                retValue = RSIP_OnMouseClick;
            else
                retValue = RSIP_OnMouseClickAndAlreadyFocused;
            break;
        case SH_ComboBox_Popup:
            retValue = true;
            break;
        case SH_Table_GridLineColor:
            retValue = int(QS60StylePrivate::s60Color(QS60StyleEnums::CL_QsnLineColors, 2, 0).rgba());
            break;
        case SH_GroupBox_TextLabelColor:
            retValue = int(QS60StylePrivate::s60Color(QS60StyleEnums::CL_QsnTextColors, 6, 0).rgba());
            break;
        case SH_ScrollBar_ScrollWhenPointerLeavesControl:
            retValue = true;
            break;
        case SH_Slider_SnapToValue:
            retValue = true;
            break;
        case SH_Slider_StopMouseOverSlider:
            retValue = true;
            break;
        case SH_LineEdit_PasswordCharacter:
            retValue = '*';
            break;
        case SH_ComboBox_PopupFrameStyle:
            retValue = QFrame::NoFrame | QFrame::Plain;
            break;
        case SH_Dial_BackgroundRole:
            retValue = QPalette::Base;
            break;
        case SH_ItemView_ActivateItemOnSingleClick: {
            if (QS60StylePrivate::isSingleClickUi())
                retValue = true;
            else if (opt && opt->state & QStyle::State_Selected)
                retValue = true;
            break;
        }
        case SH_ProgressDialog_TextLabelAlignment:
            retValue = (QApplication::layoutDirection() == Qt::LeftToRight) ?
                Qt::AlignLeft :
                Qt::AlignRight;
            break;
        case SH_Menu_SubMenuPopupDelay:
            retValue = 300;
            break;
        case SH_Menu_Scrollable:
            retValue = true;
            break;
        case SH_Menu_SelectionWrap:
            retValue = true;
            break;
        case SH_Menu_MouseTracking:
            retValue = true;
            break;
        case SH_ItemView_ShowDecorationSelected:
            retValue = true;
            break;
        case SH_ToolBar_Movable:
            retValue = false;
            break;
        case SH_BlinkCursorWhenTextSelected:
            retValue = true;
            break;
        case SH_UnderlineShortcut:
            retValue = 0;
            break;
        case SH_FormLayoutWrapPolicy:
            retValue = QFormLayout::WrapLongRows;
            break;
        case SH_ScrollBar_ContextMenu:
            retValue = false;
            break;
        default:
            retValue = QCommonStyle::styleHint(sh, opt, widget, hret);
            break;
    }
    return retValue;
}

/*! \reimp */
QRect QS60Style::subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl scontrol, const QWidget *widget) const
{
    QRect ret;
    switch (control) {
#ifndef QT_NO_SCROLLBAR
    // This implementation of subControlRect(CC_ScrollBar..) basically just removes the SC_ScrollBarSubLine and SC_ScrollBarAddLine
    case CC_ScrollBar:
        if (const QStyleOptionSlider *scrollbarOption = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const QRect scrollBarRect = scrollbarOption->rect;
            const bool isHorizontal = scrollbarOption->orientation == Qt::Horizontal;
            const int maxlen = isHorizontal ? scrollBarRect.width() : scrollBarRect.height();
            int sliderlen;

            // calculate slider length
            if (scrollbarOption->maximum != scrollbarOption->minimum) {
                const uint range = scrollbarOption->maximum - scrollbarOption->minimum;
                sliderlen = (qint64(scrollbarOption->pageStep) * maxlen) / (range + scrollbarOption->pageStep);

                const int slidermin = pixelMetric(PM_ScrollBarSliderMin, scrollbarOption, widget);
                if (sliderlen < slidermin || range > (INT_MAX >> 1))
                    sliderlen = slidermin;
                if (sliderlen > maxlen)
                    sliderlen = maxlen;
            } else {
                sliderlen = maxlen;
            }

            const int sliderstart = sliderPositionFromValue(scrollbarOption->minimum,
                                                            scrollbarOption->maximum,
                                                            scrollbarOption->sliderPosition,
                                                            maxlen - sliderlen,
                                                            scrollbarOption->upsideDown);

            switch (scontrol) {
                case SC_ScrollBarSubPage:            // between top/left button and slider
                    if (isHorizontal)
                        ret.setRect(0, 0, sliderstart, scrollBarRect.height());
                    else
                        ret.setRect(0, 0, scrollBarRect.width(), sliderstart);
                    break;
                case SC_ScrollBarAddPage: {         // between bottom/right button and slider
                    const int addPageLength = sliderstart + sliderlen;
                    if (isHorizontal)
                        ret = scrollBarRect.adjusted(addPageLength, 0, 0, 0);
                    else
                        ret = scrollBarRect.adjusted(0, addPageLength, 0, 0);
                    }
                    break;
                case SC_ScrollBarGroove:
                    ret = scrollBarRect;
                    break;
                case SC_ScrollBarSlider:
                    if (scrollbarOption->orientation == Qt::Horizontal)
                        ret.setRect(sliderstart, 0, sliderlen, scrollBarRect.height());
                    else
                        ret.setRect(0, sliderstart, scrollBarRect.width(), sliderlen);
                    break;
                case SC_ScrollBarSubLine:            // top/left button
                case SC_ScrollBarAddLine:            // bottom/right button
                default:
                    break;
            }
            ret = visualRect(scrollbarOption->direction, scrollBarRect, ret);
        }
        break;
#endif // QT_NO_SCROLLBAR
    case CC_SpinBox:
        if (const QStyleOptionSpinBox *spinbox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            const int frameThickness = spinbox->frame ? pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;
            const int buttonMargin = spinbox->frame ? 2 : 0;
            const int buttonContentWidth = QS60StylePrivate::pixelMetric(PM_ButtonIconSize) + 2 * buttonMargin;
            // Spinbox buttons should be no larger than one fourth of total width.
            // Thus, side-by-side buttons would take half of the total width.
            const int maxSize = qMax(spinbox->rect.width() / 4, buttonContentWidth);
            QSize buttonSize;
            buttonSize.setHeight(qMin(maxSize, qMax(8, spinbox->rect.height() - frameThickness)));
            //width should at least be equal to height
            buttonSize.setWidth(qMax(buttonSize.height(), buttonContentWidth));
            buttonSize = buttonSize.expandedTo(QApplication::globalStrut());

            // Normally spinbuttons should be side-by-side, but if spinbox grows very big
            // and spinbuttons reach their maximum size, they can be deployed one top of the other.
            const bool sideBySide = (buttonSize.height() * 2 < spinbox->rect.height()) ? false : true;
            const int y = frameThickness + spinbox->rect.y() +  
                          (spinbox->rect.height() - (sideBySide ? 1 : 2) * buttonSize.height()) / 2;
            const int x = spinbox->rect.x() + 
                          spinbox->rect.width() - frameThickness - (sideBySide ? 2 : 1) * buttonSize.width();

            switch (scontrol) {
                case SC_SpinBoxUp:
                    if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                        return QRect();
                    ret = QRect(x, y, buttonSize.width(), buttonSize.height());
                    break;
                case SC_SpinBoxDown:
                    if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                        return QRect();
                    ret = QRect(x + (sideBySide ? buttonSize.width() : 0), 
                                y + (sideBySide ? 0 : buttonSize.height()), 
                                buttonSize.width(), buttonSize.height());
                    break;
                case SC_SpinBoxEditField:
                    if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                        ret = QRect(
                                frameThickness,
                                frameThickness,
                                spinbox->rect.width() - 2 * frameThickness,
                                spinbox->rect.height() - 2 * frameThickness);
                    else
                        ret = QRect(
                                frameThickness,
                                frameThickness,
                                x - frameThickness,
                                spinbox->rect.height() - 2 * frameThickness);
                    break;
                case SC_SpinBoxFrame:
                    ret = spinbox->rect;
                    break;
                default:
                    break;
            }
        ret = visualRect(spinbox->direction, spinbox->rect, ret);
        }
        break;
    case CC_ComboBox:
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            ret = cmb->rect;
            const int width = cmb->rect.width();
            const int height = cmb->rect.height();
            const int buttonMargin = cmb->frame ? 2 : 0;
            // lets use spinbox frame here as well, as no combobox specific value available.
            const int frameThickness = cmb->frame ? pixelMetric(PM_SpinBoxFrameWidth, cmb, widget) : 0;
            const int buttonMinSize = QS60StylePrivate::pixelMetric(PM_ButtonIconSize) + 2 * buttonMargin;
            QSize buttonSize;
            //allow button to grow to one fourth of the frame height, if the frame is really tall
            buttonSize.setHeight(qMin(height, qMax(width / 4, buttonMinSize)));
            buttonSize.setWidth(buttonSize.height());
            buttonSize = buttonSize.expandedTo(QApplication::globalStrut());
            switch (scontrol) {
                case SC_ComboBoxArrow: {
                    const int xposMod = cmb->rect.x() + width - buttonMargin - buttonSize.width();
                    const int ypos = cmb->rect.y();
                    ret.setRect(xposMod, ypos + buttonMargin, buttonSize.width(), height - 2 * buttonMargin);
                    }
                    break;
                case SC_ComboBoxEditField: {
                    ret = QRect(0, 0, cmb->rect.x() + width - buttonSize.width(), height);
                    }
                break;
                case SC_ComboBoxListBoxPopup: {
                    ret = QApplication::desktop()->availableGeometry();
                    }
                break;
            default:
                break;
            }
            ret = visualRect(cmb->direction, cmb->rect, ret);
        }
        break;
    case CC_GroupBox:
        if (const QStyleOptionGroupBox *groupBox = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            ret = QCommonStyle::subControlRect(control, option, scontrol, widget);
            switch (scontrol) {
                case SC_GroupBoxCheckBox: //fallthrough
                case SC_GroupBoxLabel: {
                    //slightly indent text and boxes, so that dialog border does not mess with them.
                    const int horizontalSpacing =
                        QS60StylePrivate::pixelMetric(PM_LayoutHorizontalSpacing);
                    ret.adjust(2, horizontalSpacing - 3, 0, 0);
                    }
                    break;
                case SC_GroupBoxFrame: {
                    const QRect textBox = subControlRect(control, option, SC_GroupBoxLabel, widget);
                    const int tbHeight = textBox.height();
                    ret.translate(0, -ret.y());
                    // include title to within the groupBox frame
                    ret.setHeight(ret.height() + tbHeight);
                    if (widget && ret.bottom() > widget->rect().bottom())
                        ret.setBottom(widget->rect().bottom());
                    }
                    break;
                default:
                    break;
            }
        }
        break;
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolButton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const int indicatorRect = pixelMetric(PM_MenuButtonIndicator) + 2 * pixelMetric(PM_ButtonMargin);
            const int border = pixelMetric(PM_ButtonMargin) + pixelMetric(PM_DefaultFrameWidth);
            ret = toolButton->rect;
            const bool popup = (toolButton->features &
                    (QStyleOptionToolButton::MenuButtonPopup | QStyleOptionToolButton::PopupDelay))
                    == QStyleOptionToolButton::MenuButtonPopup;
            switch (scontrol) {
            case SC_ToolButton:
                if (popup)
                    ret.adjust(0, 0, -indicatorRect, 0);
                break;
            case SC_ToolButtonMenu:
                if (popup)
                    ret.adjust(ret.width() - indicatorRect, border, -pixelMetric(PM_ButtonMargin), -border);
                break;
            default:
                break;
            }
            ret = visualRect(toolButton->direction, toolButton->rect, ret);
        }
        break;
    default:
        ret = QCommonStyle::subControlRect(control, option, scontrol, widget);
    }
    return ret;
}

/*!
  \reimp
*/
QRect QS60Style::subElementRect(SubElement element, const QStyleOption *opt, const QWidget *widget) const
{
    QRect ret;
    switch (element) {
        case SE_RadioButtonFocusRect:
            ret = opt->rect;
            break;
        case SE_LineEditContents: {
                // in S60 the input text box doesn't start from line Edit's TL, but
                // a bit indented (8 pixels).
                const int KLineEditDefaultIndention = 8;
                ret = visualRect(
                    opt->direction, opt->rect, opt->rect.adjusted(KLineEditDefaultIndention, 0, 0, 0));
            }
            break;
        case SE_TabBarTearIndicator:
            ret = QRect(0, 0, 0, 0);
            break;
        case SE_TabWidgetTabBar:
            if (const QStyleOptionTabWidgetFrame *optionTab = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
                ret = QCommonStyle::subElementRect(element, opt, widget);

                if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(opt)) {
                    const int borderThickness =
                        QS60StylePrivate::pixelMetric(PM_DefaultFrameWidth);
                    int tabOverlap = pixelMetric(PM_TabBarTabOverlap);
                    if (tabOverlap > borderThickness)
                        tabOverlap -= borderThickness;
                    const QTabWidget *tab = qobject_cast<const QTabWidget *>(widget);
                    int gain = (tab) ? tabOverlap * tab->count() : 0;
                    switch (twf->shape) {
                        case QTabBar::RoundedNorth:
                        case QTabBar::TriangularNorth:
                        case QTabBar::RoundedSouth:
                        case QTabBar::TriangularSouth: {
                            if (widget) {
                                // make sure that gain does not set the rect outside of widget boundaries
                                if (twf->direction == Qt::RightToLeft) {
                                    if ((ret.left() - gain) < widget->rect().left())
                                        gain = widget->rect().left() - ret.left();
                                    ret.adjust(-gain, 0, 0, 0);
                                } else {
                                    if ((ret.right() + gain) > widget->rect().right())
                                        gain = widget->rect().right() - ret.right();
                                    ret.adjust(0, 0, gain, 0);
                                }
                            }
                            break;
                        }
                        default: {
                            if (widget) {
                                if ((ret.bottom() + gain) > widget->rect().bottom())
                                    gain = widget->rect().bottom() - ret.bottom();
                                ret.adjust(0, 0, 0, gain);
                            }
                            break;
                        }
                    }
                }
            }
            break;
        case SE_ItemViewItemText:
        case SE_ItemViewItemDecoration:
            if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
                const QAbstractItemView *listItem = qobject_cast<const QAbstractItemView *>(widget);
                const bool multiSelection = !listItem ? false :
                    listItem->selectionMode() == QAbstractItemView::MultiSelection ||
                    listItem->selectionMode() == QAbstractItemView::ExtendedSelection ||
                    listItem->selectionMode() == QAbstractItemView::ContiguousSelection;
                ret = QCommonStyle::subElementRect(element, opt, widget);
                // If both multiselect & check-state, then remove checkbox and move
                // text and decoration towards the beginning
                if (listItem &&
                    multiSelection &&
                    (vopt->features & QStyleOptionViewItemV2::HasCheckIndicator)) {
                    const int verticalSpacing =
                        QS60StylePrivate::pixelMetric(PM_LayoutVerticalSpacing);
                    //const int horizontalSpacing = QS60StylePrivate::pixelMetric(PM_LayoutHorizontalSpacing);
                    const int checkBoxRectWidth = subElementRect(SE_ItemViewItemCheckIndicator, opt, widget).width();
                    ret.adjust(-checkBoxRectWidth - verticalSpacing, 0, -checkBoxRectWidth - verticalSpacing, 0);
                }
            } else if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
                const bool checkable = menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
                const int indicatorWidth = checkable ?
                    pixelMetric(PM_ListViewIconSize, opt, widget) :
                    pixelMetric(PM_SmallIconSize, opt, widget);
                ret = menuItem->rect;

                QRect checkBoxRect = checkable ? menuItem->rect : QRect();
                if (checkable) {
                    checkBoxRect.setWidth(pixelMetric(PM_IndicatorWidth));
                    checkBoxRect.setHeight(pixelMetric(PM_IndicatorHeight));
                }

                const int vSpacing = QS60StylePrivate::pixelMetric(PM_LayoutVerticalSpacing);
                //The vertical spacing is doubled; it needs one spacing to separate checkbox from
                //highlight and then it needs one to separate it whatever is shown after it (text/icon/both).
                const int moveByX = checkBoxRect.width() + 2 * vSpacing;

                if (element == SE_ItemViewItemDecoration) {
                    if (menuItem->icon.isNull()) {
                        ret = QRect();
                    } else {
                        if (menuItem->direction == Qt::RightToLeft)
                            ret.translate(ret.width() - indicatorWidth - moveByX, 0);
                        else
                            ret.translate(moveByX, 0);
                        ret.setWidth(indicatorWidth);
                    }
                } else {
                    if (!menuItem->icon.isNull()) {
                        if (menuItem->direction == Qt::LeftToRight)
                            ret.adjust(indicatorWidth, 0, 0, 0);
                        else
                            ret.adjust(0, 0, -indicatorWidth, 0);
                    }
                    if (menuItem->direction == Qt::LeftToRight)
                        ret.adjust(moveByX, 0, 0, 0);
                    else
                        ret.adjust(0, 0, -moveByX, 0);

                    // Make room for submenu indicator
                    if (menuItem->menuItemType == QStyleOptionMenuItem::SubMenu){
                        // submenu indicator is very small, so lets halve the rect
                        if (menuItem->direction == Qt::LeftToRight)
                            ret.adjust(0, 0, -(indicatorWidth >> 1), 0);
                        else
                            ret.adjust((indicatorWidth >> 1), 0, 0, 0);
                    }
                }
            }
            break;
        case SE_ItemViewItemCheckIndicator:
            if (const QStyleOptionViewItemV4 *vopt = qstyleoption_cast<const QStyleOptionViewItemV4 *>(opt)) {
                const QAbstractItemView *listItem = qobject_cast<const QAbstractItemView *>(widget);

                const bool singleSelection = listItem &&
                    (listItem->selectionMode() == QAbstractItemView::SingleSelection ||
                     listItem->selectionMode() == QAbstractItemView::NoSelection);
                const bool checkBoxOnly = (vopt->features & QStyleOptionViewItemV2::HasCheckIndicator) &&
                    listItem &&
                    singleSelection && vopt->text.isEmpty() && vopt->icon.isNull();

                // Selection check mark rect.
                const int indicatorWidth = QS60StylePrivate::pixelMetric(PM_IndicatorWidth);
                const int indicatorHeight = QS60StylePrivate::pixelMetric(PM_IndicatorHeight);
                const int spacing = QS60StylePrivate::pixelMetric(PM_CheckBoxLabelSpacing);

                const int itemHeight = opt->rect.height();
                int heightOffset = 0;
                if (indicatorHeight < itemHeight)
                    heightOffset = ((itemHeight - indicatorHeight) >> 1);
                if (checkBoxOnly) {
                    // Move rect and make it slightly smaller, so that
                    // a) highlight border does not cross the rect
                    // b) in s60 list checkbox is smaller than normal checkbox
                    //todo; magic three
                    ret.setRect(opt->rect.left() + 3, opt->rect.top() + heightOffset,
                        indicatorWidth - 3, indicatorHeight - 3);
                } else {
                    ret.setRect(opt->rect.right() - indicatorWidth - spacing, opt->rect.top() + heightOffset,
                        indicatorWidth, indicatorHeight);
                }
            } else  {
                ret = QCommonStyle::subElementRect(element, opt, widget);
            }
            break;
        case SE_HeaderLabel:
            ret = QCommonStyle::subElementRect(element, opt, widget);
            if (qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
                // Subtract area needed for line
                if (opt->state & State_Horizontal)
                    ret.setHeight(ret.height() - QS60StylePrivate::pixelMetric(PM_BoldLineWidth));
                else
                    ret.setWidth(ret.width() - QS60StylePrivate::pixelMetric(PM_ThinLineWidth));
                }
            ret = visualRect(opt->direction, opt->rect, ret);
            break;
        case SE_RadioButtonIndicator: {
                const int height = pixelMetric(PM_ExclusiveIndicatorHeight, opt, widget);
                ret.setRect(opt->rect.x(), opt->rect.y() + ((opt->rect.height() - height) >> 1),
                        pixelMetric(PM_ExclusiveIndicatorWidth, opt, widget), height);
                ret.translate(2, 0); //move indicator slightly to avoid highlight crossing over it
                ret = visualRect(opt->direction, opt->rect, ret);
            }
            break;
        case SE_CheckBoxIndicator: {
                const int height = pixelMetric(PM_IndicatorHeight, opt, widget);
                ret.setRect(opt->rect.x(), opt->rect.y() + ((opt->rect.height() - height) >> 1),
                          pixelMetric(PM_IndicatorWidth, opt, widget), height);
                ret.translate(2, 0); //move indicator slightly to avoid highlight crossing over it
                ret = visualRect(opt->direction, opt->rect, ret);
            }
            break;
        case SE_CheckBoxFocusRect:
            ret = opt->rect;
            break;
        case SE_ProgressBarLabel:
        case SE_ProgressBarContents:
        case SE_ProgressBarGroove:
            ret = opt->rect;
            break;
        default:
            ret = QCommonStyle::subElementRect(element, opt, widget);
    }
    return ret;
}

/*!
  \reimp
 */
void QS60Style::polish(QWidget *widget)
{
    Q_D(const QS60Style);
    QCommonStyle::polish(widget);

    if (!widget)
        return;

    //Currently we only support animations in QProgressBar.
#ifndef QT_NO_PROGRESSBAR
    if (qobject_cast<QProgressBar *>(widget))
        widget->installEventFilter(this);
#endif

    if (false
#ifndef QT_NO_SCROLLBAR
        || qobject_cast<QScrollBar *>(widget)
#endif
        ) {
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

    if (QS60StylePrivate::drawsOwnThemeBackground(widget)) {
        widget->setAttribute(Qt::WA_StyledBackground);
    } else if (false
#ifndef QT_NO_MENU
        || qobject_cast<const QMenu *> (widget)
#endif // QT_NO_MENU
    ) {
        widget->setAttribute(Qt::WA_StyledBackground);
    } else if (false
#ifndef QT_NO_COMBOBOX
        || qobject_cast<const QComboBoxListView *>(widget)
#endif //QT_NO_COMBOBOX
        ) {
        widget->setAttribute(Qt::WA_StyledBackground);
    }
    d->setThemePalette(widget);
    d->setFont(widget);
}

/*!
  \reimp
 */
void QS60Style::unpolish(QWidget *widget)
{
    Q_D(QS60Style);

    if (false
    #ifndef QT_NO_SCROLLBAR
        || qobject_cast<QScrollBar *>(widget)
    #endif
        )
        widget->setAttribute(Qt::WA_OpaquePaintEvent);

    if (QS60StylePrivate::drawsOwnThemeBackground(widget)) {
        widget->setAttribute(Qt::WA_StyledBackground, false);
    } else if (false
#ifndef QT_NO_MENU
        || qobject_cast<const QMenu *> (widget)
#endif // QT_NO_MENU
        ) {
        widget->setAttribute(Qt::WA_StyledBackground, false);
    } else if (false
#ifndef QT_NO_COMBOBOX
        || qobject_cast<const QComboBoxListView *>(widget)
#endif //QT_NO_COMBOBOX
        ) {
        widget->setAttribute(Qt::WA_StyledBackground, false);
    }

    if (widget)
        widget->setPalette(QPalette());

#if defined(Q_WS_S60) && !defined(QT_NO_PROGRESSBAR)
    if (QProgressBar *bar = qobject_cast<QProgressBar *>(widget)) {
        widget->removeEventFilter(this);
        d->m_bars.removeAll(bar);
    }
#else
    Q_UNUSED(d)
#endif
    QCommonStyle::unpolish(widget);
}

/*!
  \reimp
 */
void QS60Style::polish(QApplication *application)
{
    Q_D(QS60Style);
    QCommonStyle::polish(qApp);
    d->m_originalPalette = application->palette();
    d->setThemePalette(application);
    if (QS60StylePrivate::isTouchSupported())
        qApp->installEventFilter(this);
}

/*!
  \reimp
 */
void QS60Style::unpolish(QApplication *application)
{
    Q_UNUSED(application)

    Q_D(QS60Style);
    QCommonStyle::unpolish(qApp);
    const QPalette newPalette = QApplication::style()->standardPalette();
    QApplication::setPalette(newPalette);
    QApplicationPrivate::setSystemPalette(d->m_originalPalette);
    if (QS60StylePrivate::isTouchSupported())
        qApp->removeEventFilter(this);
}

/*!
  \reimp
 */
bool QS60Style::event(QEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    Q_D(QS60Style);
    const QEvent::Type eventType = e->type();
    if ((eventType == QEvent::FocusIn ||
         eventType == QEvent::FocusOut ||
         eventType == QEvent::EnterEditFocus ||
         eventType == QEvent::LeaveEditFocus) &&
        QS60StylePrivate::isTouchSupported())
            return false;
#endif

    switch (e->type()) {
    case QEvent::Timer: {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        timerEvent(te);
        }
        break;
#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::FocusIn:
        if (QWidget *focusWidget = QApplication::focusWidget()) {

            // Menus and combobox popups do not draw focus frame around them
            if (qobject_cast<QComboBoxListView *>(focusWidget) ||
                qobject_cast<QMenu *>(focusWidget))
                    break;

            if (!d->m_focusFrame)
                d->m_focusFrame = new QFocusFrame(focusWidget);
            d->m_focusFrame->setWidget(focusWidget);
        } else if (d->m_focusFrame) {
            d->m_focusFrame->setWidget(0);
        }
        break;
    case QEvent::FocusOut:
        if (d->m_focusFrame)
            d->m_focusFrame->setWidget(0);
        break;
    case QEvent::EnterEditFocus:
    case QEvent::LeaveEditFocus:
        if (d->m_focusFrame)
            d->m_focusFrame->update();
        break;
#endif
    default:
        break;
    }
    return false;
}

/*!
    \internal
 */
QIcon QS60Style::standardIconImplementation(StandardPixmap standardIcon,
    const QStyleOption *option, const QWidget *widget) const
{
    QS60StyleEnums::SkinParts part;
    qreal iconHeightMultiplier = 1.0;
    qreal iconWidthMultiplier = 1.0;
    QS60StylePrivate::SkinElementFlags adjustedFlags;
    if (option)
        adjustedFlags = (option->state & State_Enabled || option->state == 0) ?
            QS60StylePrivate::SF_StateEnabled :
            QS60StylePrivate::SF_StateDisabled;

    switch(standardIcon) {
        case SP_MessageBoxWarning:
            // By default, S60 messagebox icons have 4:3 ratio. Value is from S60 LAF documentation.
            iconHeightMultiplier = 1.33;
            part = QS60StyleEnums::SP_QgnNoteWarning;
            break;
        case SP_MessageBoxInformation:
            iconHeightMultiplier = 1.33;
            part = QS60StyleEnums::SP_QgnNoteInfo;
            break;
        case SP_MessageBoxCritical:
            iconHeightMultiplier = 1.33;
            part = QS60StyleEnums::SP_QgnNoteError;
            break;
        case SP_MessageBoxQuestion:
            iconHeightMultiplier = 1.33;
            part = QS60StyleEnums::SP_QgnNoteQuery;
            break;
        case SP_ArrowRight:
            part = QS60StyleEnums::SP_QgnIndiNaviArrowRight;
            break;
        case SP_ArrowLeft:
            part = QS60StyleEnums::SP_QgnIndiNaviArrowLeft;
            break;
        case SP_ArrowUp:
            part = QS60StyleEnums::SP_QgnIndiNaviArrowLeft;
            adjustedFlags |= QS60StylePrivate::SF_PointEast;
            break;
        case SP_ArrowDown:
            part = QS60StyleEnums::SP_QgnIndiNaviArrowLeft;
            adjustedFlags |= QS60StylePrivate::SF_PointWest;
            break;
        case SP_ArrowBack:
            if (QApplication::layoutDirection() == Qt::RightToLeft)
                return QS60Style::standardIcon(SP_ArrowRight, option, widget);
            return QS60Style::standardIcon(SP_ArrowLeft, option, widget);
        case SP_ArrowForward:
            if (QApplication::layoutDirection() == Qt::RightToLeft)
                return QS60Style::standardIcon(SP_ArrowLeft, option, widget);
            return QS60Style::standardIcon(SP_ArrowRight, option, widget);
        case SP_ComputerIcon:
            part = QS60StyleEnums::SP_QgnPropPhoneMemcLarge;
            break;
        case SP_DirClosedIcon:
            part = QS60StyleEnums::SP_QgnPropFolderSmall;
            break;
        case SP_DirOpenIcon:
            part = QS60StyleEnums::SP_QgnPropFolderCurrent;
            break;
        case SP_DirIcon:
            part = QS60StyleEnums::SP_QgnPropFolderSmall;
            break;
        case SP_FileDialogNewFolder:
            part = QS60StyleEnums::SP_QgnPropFolderSmallNew;
            break;
        case SP_FileIcon:
            part = QS60StyleEnums::SP_QgnPropFileSmall;
            break;
        case SP_TrashIcon:
            part = QS60StyleEnums::SP_QgnNoteErased;
            break;
        case SP_ToolBarHorizontalExtensionButton:
            part = QS60StyleEnums::SP_QgnIndiSubmenu;
            if (QApplication::layoutDirection() == Qt::RightToLeft)
                adjustedFlags |= QS60StylePrivate::SF_PointSouth;
            break;
        case SP_ToolBarVerticalExtensionButton:
            adjustedFlags |= QS60StylePrivate::SF_PointEast;
            part = QS60StyleEnums::SP_QgnIndiSubmenu;
            break;
        default:
            return QCommonStyle::standardIconImplementation(standardIcon, option, widget);
    }
    const QS60StylePrivate::SkinElementFlags flags = adjustedFlags;
    const int iconDimension = QS60StylePrivate::pixelMetric(PM_ToolBarIconSize);
    const QRect iconSize = (!option) ? 
        QRect(0, 0, iconDimension * iconWidthMultiplier, iconDimension * iconHeightMultiplier) : option->rect;
    const QPixmap cachedPixMap(QS60StylePrivate::cachedPart(part, iconSize.size(), 0, flags));
    return cachedPixMap.isNull() ?
        QCommonStyle::standardIconImplementation(standardIcon, option, widget) : QIcon(cachedPixMap);
}

/*!
    \internal
    Animate indeterminate progress bars only when visible
*/
bool QS60Style::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QS60Style);
    switch(event->type()) {
        case QEvent::MouseButtonPress: {
            QWidget *w = QApplication::widgetAt(QCursor::pos());
            if (w) {
                QWidget *focusW = w->focusProxy();
                if (qobject_cast<QAbstractItemView *>(focusW) ||
                    qobject_cast<QRadioButton *>(focusW) ||
                    qobject_cast<QCheckBox *>(focusW))
                    d->m_pressedWidget = focusW;
                else if (qobject_cast<QAbstractItemView *>(w)||
                        qobject_cast<QRadioButton *>(w) ||
                        qobject_cast<QCheckBox *>(w))
                    d->m_pressedWidget = w;

                if (d->m_pressedWidget)
                    d->m_pressedWidget->update();
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            if (d->m_pressedWidget) {
                d->m_pressedWidget->update();
                d->m_pressedWidget = 0;
            }
            break;
        }
        default:
            break;
    }

#ifdef Q_WS_S60
#ifndef QT_NO_PROGRESSBAR
    switch(event->type()) {
    case QEvent::StyleChange:
    case QEvent::Show:
        if (QProgressBar *bar = qobject_cast<QProgressBar *>(object)) {
            if (!d->m_bars.contains(bar))
                d->m_bars << bar;
            if (d->m_bars.size() == 1) //only start with first animated progressbar
                d->startAnimation(QS60StyleEnums::SP_QgnGrafBarWaitAnim);
        }
        break;
    case QEvent::Destroy:
    case QEvent::Hide:
        if (QProgressBar *bar = reinterpret_cast<QProgressBar *>(object)) {
            d->stopAnimation(QS60StyleEnums::SP_QgnGrafBarWaitAnim);
            d->m_bars.removeAll(bar);
        }
        break;
    default:
        break;
    }
#endif // QT_NO_PROGRESSBAR
#endif // Q_WS_S60
    return QCommonStyle::eventFilter(object, event);
}

/*!
    \internal
    Handle the timer \a event.
*/
void QS60Style::timerEvent(QTimerEvent *event)
{
#ifdef Q_WS_S60
#ifndef QT_NO_PROGRESSBAR
    Q_D(QS60Style);

    QS60StyleAnimation *progressBarAnimation =
        QS60StylePrivate::animationDefinition(QS60StyleEnums::SP_QgnGrafBarWaitAnim);

    if (event->timerId() == progressBarAnimation->timerId()) {

        Q_ASSERT(progressBarAnimation->interval() > 0);

        if (progressBarAnimation->currentFrame() == progressBarAnimation->frameCount() )
            if (progressBarAnimation->playMode() == QS60StyleEnums::AM_Looping)
                progressBarAnimation->setCurrentFrame(0);
            else
                d->stopAnimation(progressBarAnimation->animationId());

        foreach (QProgressBar *bar, d->m_bars) {
            if ((bar->minimum() == 0 && bar->maximum() == 0))
                bar->update();
        }
        progressBarAnimation->setCurrentFrame(progressBarAnimation->currentFrame() + 1);
    }
#endif // QT_NO_PROGRESSBAR
#endif // Q_WS_S60
    event->ignore();
}

extern QPoint qt_s60_fill_background_offset(const QWidget *targetWidget);

bool qt_s60_fill_background(QPainter *painter, const QRegion &rgn, const QBrush &brush)
{
    // Check if the widget's palette matches placeholder or actual background texture.
    // When accessing backgroundTexture, use parameter value 'true' to avoid creating
    // the texture, if it is not already created.

    const QPixmap placeHolder(QS60StylePrivate::placeHolderTexture());
    const QPixmap bg(QS60StylePrivate::backgroundTexture(true));
    if (placeHolder.cacheKey() != brush.texture().cacheKey()
        && bg.cacheKey() != brush.texture().cacheKey())
        return false;

    const QPixmap backgroundTexture(QS60StylePrivate::backgroundTexture());

    const QPaintDevice *target = painter->device();
    if (target->devType() == QInternal::Widget) {
        const QWidget *widget = static_cast<const QWidget *>(target);
        if (!widget->testAttribute(Qt::WA_TranslucentBackground)) {
            const QVector<QRect> &rects = rgn.rects();
            for (int i = 0; i < rects.size(); ++i) {
                const QRect rect(rects.at(i));
                painter->drawPixmap(rect.topLeft(), backgroundTexture,
                                    rect.translated(qt_s60_fill_background_offset(widget)));
            }
        }
    }
    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_STYLE_S60 || QT_PLUGIN
