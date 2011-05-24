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

#include "qs60style.h"
#include "qs60style_p.h"
#include "qpainter.h"
#include "qstyleoption.h"
#include "qstyle.h"
#include "private/qt_s60_p.h"
#include "private/qpixmap_s60_p.h"
#include "private/qcore_symbian_p.h"
#include "private/qvolatileimage_p.h"
#include "qapplication.h"
#include "qsettings.h"

#include <w32std.h>
#include <AknsConstants.h>
#include <aknconsts.h>
#include <AknsItemID.h>
#include <AknsUtils.h>
#include <AknsDrawUtils.h>
#include <AknsSkinInstance.h>
#include <AknsBasicBackgroundControlContext.h>
#include <avkon.mbg>
#include <AknFontAccess.h>
#include <AknLayoutFont.h>
#include <AknUtils.h>
#include <aknnavi.h>
#include <gulicon.h>
#include <AknBitmapAnimation.h>
#include <centralrepository.h>

#if !defined(QT_NO_STYLE_S60) || defined(QT_PLUGIN)

QT_BEGIN_NAMESPACE

enum TDrawType {
    EDrawIcon,
    EDrawGulIcon,
    EDrawBackground,
    EDrawAnimation,
    ENoDraw
};

const TUid personalisationUID = { 0x101F876F };

enum TSupportRelease {
    ES60_None     = 0x0000, //indicates that the commonstyle should draw the graphics
    ES60_3_1      = 0x0001,
    ES60_3_2      = 0x0002,
    ES60_5_0      = 0x0004,
    ES60_5_1      = 0x0008,
    ES60_5_2      = 0x0010,
    ES60_5_3      = 0x0020,
    ES60_3_X      = ES60_3_1 | ES60_3_2,
    // Releases before Symbian Foundation
    ES60_PreSF    = ES60_3_1 | ES60_3_2 | ES60_5_0,
    // Releases before the S60 5.2
    ES60_Pre52    = ES60_3_1 | ES60_3_2 | ES60_5_0 | ES60_5_1,
    // Releases before S60 5.3
    ES60_Pre53    = ES60_3_1 | ES60_3_2 | ES60_5_0 | ES60_5_1 | ES60_5_2,
    // Add all new releases here
    ES60_All = ES60_3_1 | ES60_3_2 | ES60_5_0 | ES60_5_1 | ES60_5_2 | ES60_5_3
};

typedef struct {
    const TAknsItemID &skinID; // Determines default theme graphics ID.
    TDrawType drawType; // Determines which native drawing routine is used to draw this item.
    int supportInfo;    // Defines the S60 versions that use the default graphics.
    // These two, define new graphics that are used in releases other than partMapEntry.supportInfo defined releases.
    // In general, these are given in numeric form to allow style compilation in earlier 
    // native releases that do not contain the new graphics.
    int newMajorSkinId;
    int newMinorSkinId;
} partMapEntry;

AnimationData::AnimationData(const QS60StyleEnums::SkinParts part, int frames, int interval) : m_id(part),
    m_frames(frames), m_interval(interval), m_mode(QS60StyleEnums::AM_Looping)
{
}

AnimationDataV2::AnimationDataV2(const AnimationData &data) : AnimationData(data.m_id, data.m_frames, data.m_interval),
    m_animation(0), m_currentFrame(0), m_resourceBased(false), m_timerId(0)
{
}
AnimationDataV2::~AnimationDataV2()
{
    delete m_animation;
}

QS60StyleAnimation::QS60StyleAnimation(const QS60StyleEnums::SkinParts part, int frames, int interval)
{
    QT_TRAP_THROWING(m_defaultData = new (ELeave) AnimationData(part, frames, interval));
    QT_TRAP_THROWING(m_currentData = new (ELeave) AnimationDataV2(*m_defaultData));
}

QS60StyleAnimation::~QS60StyleAnimation()
{
    delete m_currentData;
    delete m_defaultData;
}

void QS60StyleAnimation::setAnimationObject(CAknBitmapAnimation* animation)
{
    Q_ASSERT(animation);
    if (m_currentData->m_animation)
        delete m_currentData->m_animation;
    m_currentData->m_animation = animation;
}

void QS60StyleAnimation::resetToDefaults()
{
    delete m_currentData;
    m_currentData = 0;
    QT_TRAP_THROWING(m_currentData = new (ELeave) AnimationDataV2(*m_defaultData));
}

class QS60StyleModeSpecifics
{
public:
    static QPixmap skinnedGraphics(QS60StyleEnums::SkinParts stylepart,
        const QSize &size, QS60StylePrivate::SkinElementFlags flags);
    static QPixmap skinnedGraphics(QS60StylePrivate::SkinFrameElements frameElement, const QSize &size, QS60StylePrivate::SkinElementFlags flags);
    static QPixmap colorSkinnedGraphics(const QS60StyleEnums::SkinParts &stylepart,
        const QSize &size, QPainter *painter, QS60StylePrivate::SkinElementFlags flags);
    static QColor colorValue(const TAknsItemID &colorGroup, int colorIndex);
    static QPixmap fromFbsBitmap(CFbsBitmap *icon, CFbsBitmap *mask, QS60StylePrivate::SkinElementFlags flags, const TSize& targetSize);
    static bool disabledPartGraphic(QS60StyleEnums::SkinParts &part);
    static bool disabledFrameGraphic(QS60StylePrivate::SkinFrameElements &frame);
    static QPixmap generateMissingThemeGraphic(QS60StyleEnums::SkinParts &part, const QSize &size, QS60StylePrivate::SkinElementFlags flags);
    static TAknsItemID partSpecificThemeId(int part);

    static QVariant themeDefinition(QS60StyleEnums::ThemeDefinitions definition, QS60StyleEnums::SkinParts part);

private:
    static QPixmap createSkinnedGraphicsLX(QS60StyleEnums::SkinParts part,
        const QSize &size, QS60StylePrivate::SkinElementFlags flags);
    static QPixmap createSkinnedGraphicsLX(QS60StylePrivate::SkinFrameElements frameElement, const QSize &size, QS60StylePrivate::SkinElementFlags flags);
    static QPixmap colorSkinnedGraphicsLX(const QS60StyleEnums::SkinParts &stylepart,
        const QSize &size, QPainter *painter, QS60StylePrivate::SkinElementFlags flags);
    static void frameIdAndCenterId(QS60StylePrivate::SkinFrameElements frameElement, TAknsItemID &frameId, TAknsItemID &centerId);
    static TRect innerRectFromElement(QS60StylePrivate::SkinFrameElements frameElement, const TRect &outerRect);
    static void fallbackInfo(const QS60StyleEnums::SkinParts &stylePart, TInt &fallbackIndex);
    static bool checkSupport(const int supportedRelease);
    // Array to match the skin ID, fallback graphics and Qt widget graphics.
    static const partMapEntry m_partMap[];
};

const partMapEntry QS60StyleModeSpecifics::m_partMap[] = {
    /* SP_QgnGrafBarWaitAnim */            {KAknsIIDQgnGrafBarWaitAnim,       EDrawAnimation,   ES60_All,    -1,-1},
    /* SP_QgnGrafBarFrameCenter */         {KAknsIIDQgnGrafBarFrameCenter,         EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnGrafBarFrameSideL */          {KAknsIIDQgnGrafBarFrameSideL,          EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnGrafBarFrameSideR */          {KAknsIIDQgnGrafBarFrameSideR,          EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnGrafBarProgress */            {KAknsIIDQgnGrafBarProgress,            EDrawIcon,   ES60_All,    -1,-1},
    // No drop area for 3.x non-touch devices
    /* SP_QgnGrafOrgBgGrid */              {KAknsIIDNone,                          EDrawIcon,   ES60_3_X,    EAknsMajorGeneric ,0x1eba}, //KAknsIIDQgnGrafOrgBgGrid
    /* SP_QgnGrafScrollArrowDown */        {KAknsIIDQgnGrafScrollArrowDown,     EDrawGulIcon,   ES60_All,    -1,-1},
    /* SP_QgnGrafScrollArrowLeft */        {KAknsIIDQgnGrafScrollArrowLeft,     EDrawGulIcon,   ES60_All,    -1,-1},
    /* SP_QgnGrafScrollArrowRight */       {KAknsIIDQgnGrafScrollArrowRight,    EDrawGulIcon,   ES60_All,    -1,-1},
    /* SP_QgnGrafScrollArrowUp */          {KAknsIIDQgnGrafScrollArrowUp,       EDrawGulIcon,   ES60_All,    -1,-1},

    // In S60 5.3 there is a new tab graphic
    /* SP_QgnGrafTabActiveL */             {KAknsIIDQgnGrafTabActiveL,             EDrawIcon,   ES60_Pre53,    EAknsMajorSkin, 0x2219}, //KAknsIIDQtgFrTabActiveNormalL
    /* SP_QgnGrafTabActiveM */             {KAknsIIDQgnGrafTabActiveM,             EDrawIcon,   ES60_Pre53,    EAknsMajorSkin, 0x221b}, //KAknsIIDQtgFrTabActiveNormalC
    /* SP_QgnGrafTabActiveR */             {KAknsIIDQgnGrafTabActiveR,             EDrawIcon,   ES60_Pre53,    EAknsMajorSkin, 0x221a}, //KAknsIIDQtgFrTabActiveNormalR
    /* SP_QgnGrafTabPassiveL */            {KAknsIIDQgnGrafTabPassiveL,            EDrawIcon,   ES60_Pre53,    EAknsMajorSkin, 0x2221}, //KAknsIIDQtgFrTabPassiveNormalL
    /* SP_QgnGrafTabPassiveM */            {KAknsIIDQgnGrafTabPassiveM,            EDrawIcon,   ES60_Pre53,    EAknsMajorSkin, 0x2223}, //KAknsIIDQtgFrTabPassiveNormalC
    /* SP_QgnGrafTabPassiveR */            {KAknsIIDQgnGrafTabPassiveR,            EDrawIcon,   ES60_Pre53,    EAknsMajorSkin, 0x2222}, //KAknsIIDQtgFrTabPassiveNormalR

    // In 3.1 there is no slider groove.
    /* SP_QgnGrafNsliderEndLeft */         {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x19cf /* KAknsIIDQgnGrafNsliderEndLeft */},
    /* SP_QgnGrafNsliderEndRight */        {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x19d0 /* KAknsIIDQgnGrafNsliderEndRight */},
    /* SP_QgnGrafNsliderMiddle */          {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x19d2 /* KAknsIIDQgnGrafNsliderMiddle */},
    /* SP_QgnIndiCheckboxOff */            {KAknsIIDQgnIndiCheckboxOff,            EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnIndiCheckboxOn */             {KAknsIIDQgnIndiCheckboxOn,             EDrawIcon,   ES60_All,    -1,-1},

    // Following 5 items (SP_QgnIndiHlColSuper - SP_QgnIndiHlLineStraight) are available starting from S60 release 3.2.
    // In 3.1 CommonStyle drawing is used for these QTreeView elements, since no similar icons in AVKON UI.
    /* SP_QgnIndiHlColSuper */             {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x17d5 /* KAknsIIDQgnIndiHlColSuper */},
    /* SP_QgnIndiHlExpSuper */             {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x17d6 /* KAknsIIDQgnIndiHlExpSuper */},
    /* SP_QgnIndiHlLineBranch */           {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x17d7 /* KAknsIIDQgnIndiHlLineBranch */},
    /* SP_QgnIndiHlLineEnd */              {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x17d8 /* KAknsIIDQgnIndiHlLineEnd */},
    /* SP_QgnIndiHlLineStraight */         {KAknsIIDNone,                          EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x17d9 /* KAknsIIDQgnIndiHlLineStraight */},
    /* SP_QgnIndiMarkedAdd */              {KAknsIIDQgnIndiMarkedAdd,              EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnIndiNaviArrowLeft */          {KAknsIIDQgnIndiNaviArrowLeft,          EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnIndiNaviArrowRight */         {KAknsIIDQgnIndiNaviArrowRight,         EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnIndiRadiobuttOff */           {KAknsIIDQgnIndiRadiobuttOff,           EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnIndiRadiobuttOn */            {KAknsIIDQgnIndiRadiobuttOn,            EDrawIcon,   ES60_All,    -1,-1},

    // In 3.1 there different slider graphic and no pressed state.
    /* SP_QgnGrafNsliderMarker */          {KAknsIIDQgnIndiSliderEdit,             EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x19d1 /* KAknsIIDQgnGrafNsliderMarker */},
    /* SP_QgnGrafNsliderMarkerSelected */  {KAknsIIDQgnIndiSliderEdit,             EDrawIcon,   ES60_3_1,    EAknsMajorGeneric, 0x1a4a /* KAknsIIDQgnGrafNsliderMarkerSelected */},
    /* SP_QgnIndiSubmenu */                {KAknsIIDQgnIndiSubmenu,                EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnNoteErased */                 {KAknsIIDQgnNoteErased,                 EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnNoteError */                  {KAknsIIDQgnNoteError,                  EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnNoteInfo */                   {KAknsIIDQgnNoteInfo,                   EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnNoteOk */                     {KAknsIIDQgnNoteOk,                     EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnNoteQuery */                  {KAknsIIDQgnNoteQuery,                  EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnNoteWarning */                {KAknsIIDQgnNoteWarning,                EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnPropFileSmall */              {KAknsIIDQgnPropFileSmall,              EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnPropFolderCurrent */          {KAknsIIDQgnPropFolderCurrent,          EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnPropFolderSmall */            {KAknsIIDQgnPropFolderSmall,            EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnPropFolderSmallNew */         {KAknsIIDQgnPropFolderSmallNew,         EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QgnPropPhoneMemcLarge */         {KAknsIIDQgnPropPhoneMemcLarge,         EDrawIcon,   ES60_All,    -1,-1},

    // Toolbar graphics is different in 3.1/3.2 vs. 5.0
    /* SP_QgnFrSctrlButtonCornerTl */   {KAknsIIDQsnFrButtonTbCornerTl,         ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2301}, /* KAknsIIDQgnFrSctrlButtonCornerTl*/
    /* SP_QgnFrSctrlButtonCornerTr */   {KAknsIIDQsnFrButtonTbCornerTr,         ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2302},
    /* SP_QgnFrSctrlButtonCornerBl */   {KAknsIIDQsnFrButtonTbCornerBl,         ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2303},
    /* SP_QgnFrSctrlButtonCornerBr */   {KAknsIIDQsnFrButtonTbCornerBr,         ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2304},
    /* SP_QgnFrSctrlButtonSideT */      {KAknsIIDQsnFrButtonTbSideT,            ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2305},
    /* SP_QgnFrSctrlButtonSideB */      {KAknsIIDQsnFrButtonTbSideB,            ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2306},
    /* SP_QgnFrSctrlButtonSideL */      {KAknsIIDQsnFrButtonTbSideL,            ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2307},
    /* SP_QgnFrSctrlButtonSideR */      {KAknsIIDQsnFrButtonTbSideR,            ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2308},
    /* SP_QgnFrSctrlButtonCenter */     {KAknsIIDQsnFrButtonTbCenter,           ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2309}, /*KAknsIIDQgnFrSctrlButtonCenter*/

    // No pressed state for toolbar button in 3.1/3.2.
    /* SP_QgnFrSctrlButtonCornerTlPressed */ {KAknsIIDQsnFrButtonTbCornerTl,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2621},  /*KAknsIIDQgnFrSctrlButtonCornerTlPressed*/
    /* SP_QgnFrSctrlButtonCornerTrPressed */ {KAknsIIDQsnFrButtonTbCornerTr,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2622},
    /* SP_QgnFrSctrlButtonCornerBlPressed */ {KAknsIIDQsnFrButtonTbCornerBl,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2623},
    /* SP_QgnFrSctrlButtonCornerBrPressed */ {KAknsIIDQsnFrButtonTbCornerBr,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2624},
    /* SP_QgnFrSctrlButtonSideTPressed */    {KAknsIIDQsnFrButtonTbSideT,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2625},
    /* SP_QgnFrSctrlButtonSideBPressed */    {KAknsIIDQsnFrButtonTbSideB,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2626},
    /* SP_QgnFrSctrlButtonSideLPressed */    {KAknsIIDQsnFrButtonTbSideL,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2627},
    /* SP_QgnFrSctrlButtonSideRPressed */    {KAknsIIDQsnFrButtonTbSideR,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2628},
    /* SP_QgnFrSctrlButtonCenterPressed */   {KAknsIIDQsnFrButtonTbCenter,      ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2629},

    // 3.1 & 3.2 do not have pressed state for scrollbar, so use normal scrollbar graphics instead.
    /* SP_QsnCpScrollHandleBottomPressed*/ {KAknsIIDQsnCpScrollHandleBottom,    EDrawIcon,   ES60_3_X,    EAknsMajorGeneric, 0x20f8}, /*KAknsIIDQsnCpScrollHandleBottomPressed*/
    /* SP_QsnCpScrollHandleMiddlePressed*/ {KAknsIIDQsnCpScrollHandleMiddle,    EDrawIcon,   ES60_3_X,    EAknsMajorGeneric, 0x20f9}, /*KAknsIIDQsnCpScrollHandleMiddlePressed*/
    /* SP_QsnCpScrollHandleTopPressed*/    {KAknsIIDQsnCpScrollHandleTop,       EDrawIcon,   ES60_3_X,    EAknsMajorGeneric, 0x20fa}, /*KAknsIIDQsnCpScrollHandleTopPressed*/

    /* SP_QsnBgScreen */                {KAknsIIDQsnBgScreen,              EDrawBackground,  ES60_All,    -1,-1},

    /* SP_QsnCpScrollBgBottom */        {KAknsIIDQsnCpScrollBgBottom,           EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QsnCpScrollBgMiddle */        {KAknsIIDQsnCpScrollBgMiddle,           EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QsnCpScrollBgTop */           {KAknsIIDQsnCpScrollBgTop,              EDrawIcon,   ES60_All,    -1,-1},

    /* SP_QsnCpScrollHandleBottom */    {KAknsIIDQsnCpScrollHandleBottom,       EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QsnCpScrollHandleMiddle */    {KAknsIIDQsnCpScrollHandleMiddle,       EDrawIcon,   ES60_All,    -1,-1},
    /* SP_QsnCpScrollHandleTop */       {KAknsIIDQsnCpScrollHandleTop,          EDrawIcon,   ES60_All,    -1,-1},

    /* SP_QsnFrButtonTbCornerTl */      {KAknsIIDQsnFrButtonTbCornerTl,         ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbCornerTr */      {KAknsIIDQsnFrButtonTbCornerTr,         ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbCornerBl */      {KAknsIIDQsnFrButtonTbCornerBl,         ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbCornerBr */      {KAknsIIDQsnFrButtonTbCornerBr,         ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbSideT */         {KAknsIIDQsnFrButtonTbSideT,            ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbSideB */         {KAknsIIDQsnFrButtonTbSideB,            ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbSideL */         {KAknsIIDQsnFrButtonTbSideL,            ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbSideR */         {KAknsIIDQsnFrButtonTbSideR,            ENoDraw,     ES60_All,    -1, -1},
    /* SP_QsnFrButtonTbCenter */        {KAknsIIDQsnFrButtonTbCenter,           EDrawIcon,   ES60_All,    -1, -1},

    /* SP_QsnFrButtonTbCornerTlPressed */{KAknsIIDQsnFrButtonTbCornerTlPressed, ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbCornerTrPressed */{KAknsIIDQsnFrButtonTbCornerTrPressed, ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbCornerBlPressed */{KAknsIIDQsnFrButtonTbCornerBlPressed, ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbCornerBrPressed */{KAknsIIDQsnFrButtonTbCornerBrPressed, ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbSideTPressed */   {KAknsIIDQsnFrButtonTbSideTPressed,    ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbSideBPressed */   {KAknsIIDQsnFrButtonTbSideBPressed,    ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbSideLPressed */   {KAknsIIDQsnFrButtonTbSideLPressed,    ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbSideRPressed */   {KAknsIIDQsnFrButtonTbSideRPressed,    ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrButtonTbCenterPressed */  {KAknsIIDQsnFrButtonTbCenterPressed,   EDrawIcon,   ES60_All,    -1,-1},

    /* SP_QsnFrCaleCornerTl */          {KAknsIIDQsnFrCaleCornerTl,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleCornerTr */          {KAknsIIDQsnFrCaleCornerTr,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleCornerBl */          {KAknsIIDQsnFrCaleCornerBl,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleCornerBr */          {KAknsIIDQsnFrCaleCornerBr,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleSideT */             {KAknsIIDQsnFrCaleSideT,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleSideB */             {KAknsIIDQsnFrCaleSideB,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleSideL */             {KAknsIIDQsnFrCaleSideL,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleSideR */             {KAknsIIDQsnFrCaleSideR,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleCenter */            {KAknsIIDQsnFrCaleCenter,               ENoDraw,     ES60_All,    -1,-1},

    /* SP_QsnFrCaleHeadingCornerTl */   {KAknsIIDQsnFrCaleHeadingCornerTl,      ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingCornerTr */   {KAknsIIDQsnFrCaleHeadingCornerTr,      ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingCornerBl */   {KAknsIIDQsnFrCaleHeadingCornerBl,      ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingCornerBr */   {KAknsIIDQsnFrCaleHeadingCornerBr,      ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingSideT */      {KAknsIIDQsnFrCaleHeadingSideT,         ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingSideB */      {KAknsIIDQsnFrCaleHeadingSideB,         ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingSideL */      {KAknsIIDQsnFrCaleHeadingSideL,         ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingSideR */      {KAknsIIDQsnFrCaleHeadingSideR,         ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrCaleHeadingCenter */     {KAknsIIDQsnFrCaleHeadingCenter,        ENoDraw,     ES60_All,    -1,-1},

    /* SP_QsnFrInputCornerTl */         {KAknsIIDQsnFrInputCornerTl,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputCornerTr */         {KAknsIIDQsnFrInputCornerTr,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputCornerBl */         {KAknsIIDQsnFrInputCornerBl,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputCornerBr */         {KAknsIIDQsnFrInputCornerBr,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputSideT */            {KAknsIIDQsnFrInputSideT,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputSideB */            {KAknsIIDQsnFrInputSideB,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputSideL */            {KAknsIIDQsnFrInputSideL,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputSideR */            {KAknsIIDQsnFrInputSideR,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrInputCenter */           {KAknsIIDQsnFrInputCenter,              ENoDraw,     ES60_All,    -1,-1},

    /* SP_QsnFrListCornerTl */          {KAknsIIDQsnFrListCornerTl,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListCornerTr */          {KAknsIIDQsnFrListCornerTr,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListCornerBl */          {KAknsIIDQsnFrListCornerBl,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListCornerBr */          {KAknsIIDQsnFrListCornerBr,             ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListSideT */             {KAknsIIDQsnFrListSideT,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListSideB */             {KAknsIIDQsnFrListSideB,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListSideL */             {KAknsIIDQsnFrListSideL,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListSideR */             {KAknsIIDQsnFrListSideR,                ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrListCenter */            {KAknsIIDQsnFrListCenter,               ENoDraw,     ES60_All,    -1,-1},

    /* SP_QsnFrPopupCornerTl */         {KAknsIIDQsnFrPopupCornerTl,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupCornerTr */         {KAknsIIDQsnFrPopupCornerTr,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupCornerBl */         {KAknsIIDQsnFrPopupCornerBl,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupCornerBr */         {KAknsIIDQsnFrPopupCornerBr,            ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupSideT */            {KAknsIIDQsnFrPopupSideT,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupSideB */            {KAknsIIDQsnFrPopupSideB,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupSideL */            {KAknsIIDQsnFrPopupSideL,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupSideR */            {KAknsIIDQsnFrPopupSideR,               ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrPopupCenter */           {KAknsIIDQsnFrPopupCenterSubmenu,       ENoDraw,     ES60_All,    -1,-1},

    // ToolTip graphics different in 3.1 vs. 3.2+.
    /* SP_QsnFrPopupPreviewCornerTl */  {KAknsIIDQsnFrPopupCornerTl,            ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c5}, /* KAknsIIDQsnFrPopupPreviewCornerTl */
    /* SP_QsnFrPopupPreviewCornerTr */  {KAknsIIDQsnFrPopupCornerTr,            ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c6},
    /* SP_QsnFrPopupPreviewCornerBl */  {KAknsIIDQsnFrPopupCornerBl,            ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c3},
    /* SP_QsnFrPopupPreviewCornerBr */  {KAknsIIDQsnFrPopupCornerBr,            ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c4},
    /* SP_QsnFrPopupPreviewSideT */     {KAknsIIDQsnFrPopupSideT,               ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19ca},
    /* SP_QsnFrPopupPreviewSideB */     {KAknsIIDQsnFrPopupSideB,               ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c7},
    /* SP_QsnFrPopupPreviewSideL */     {KAknsIIDQsnFrPopupSideL,               ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c8},
    /* SP_QsnFrPopupPreviewSideR */     {KAknsIIDQsnFrPopupSideR,               ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c9},
    /* SP_QsnFrPopupPreviewCenter */    {KAknsIIDQsnFrPopupCenter,              ENoDraw,     ES60_3_1,    EAknsMajorSkin, 0x19c2},

    /* SP_QsnFrSetOptCornerTl */        {KAknsIIDQsnFrSetOptCornerTl,           ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptCornerTr */        {KAknsIIDQsnFrSetOptCornerTr,           ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptCornerBl */        {KAknsIIDQsnFrSetOptCornerBl,           ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptCornerBr */        {KAknsIIDQsnFrSetOptCornerBr,           ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptSideT */           {KAknsIIDQsnFrSetOptSideT,              ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptSideB */           {KAknsIIDQsnFrSetOptSideB,              ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptSideL */           {KAknsIIDQsnFrSetOptSideL,              ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptSideR */           {KAknsIIDQsnFrSetOptSideR,              ENoDraw,     ES60_All,    -1,-1},
    /* SP_QsnFrSetOptCenter */          {KAknsIIDQsnFrSetOptCenter,             ENoDraw,     ES60_All,    -1,-1},

    // No toolbar frame for 5.0+ releases.
    /* SP_QsnFrPopupSubCornerTl */      {KAknsIIDQsnFrPopupSubCornerTl,         ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubCornerTr */      {KAknsIIDQsnFrPopupSubCornerTr,         ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubCornerBl */      {KAknsIIDQsnFrPopupSubCornerBl,         ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubCornerBr */      {KAknsIIDQsnFrPopupSubCornerBr,         ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubSideT */         {KAknsIIDQsnFrPopupSubSideT,            ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubSideB */         {KAknsIIDQsnFrPopupSubSideB,            ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubSideL */         {KAknsIIDQsnFrPopupSubSideL,            ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubSideR */         {KAknsIIDQsnFrPopupSubSideR,            ENoDraw,     ES60_3_X,    -1,-1},
    /* SP_QsnFrPopupSubCenter */        {KAknsIIDQsnFrPopupCenterSubmenu,       ENoDraw,     ES60_3_X,    -1,-1},

    // No inactive button graphics in 3.1/3.2
    /* SP_QsnFrButtonCornerTlInactive */ {KAknsIIDQsnFrButtonTbCornerTl,        ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b1}, /*KAknsIIDQsnFrButtonCornerTlInactive*/
    /* SP_QsnFrButtonCornerTrInactive */ {KAknsIIDQsnFrButtonTbCornerTr,        ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b2},
    /* SP_QsnFrButtonCornerBlInactive */ {KAknsIIDQsnFrButtonTbCornerBl,        ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b3},
    /* SP_QsnFrButtonCornerTrInactive */ {KAknsIIDQsnFrButtonTbCornerBr,        ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b4},
    /* SP_QsnFrButtonSideTInactive */    {KAknsIIDQsnFrButtonTbSideT,           ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b5},
    /* SP_QsnFrButtonSideBInactive */    {KAknsIIDQsnFrButtonTbSideB,           ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b6},
    /* SP_QsnFrButtonSideLInactive */    {KAknsIIDQsnFrButtonTbSideL,           ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b7},
    /* SP_QsnFrButtonSideRInactive */    {KAknsIIDQsnFrButtonTbSideR,           ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x21b8},
    /* SP_QsnFrButtonCenterInactive */   {KAknsIIDQsnFrButtonTbCenter,          EDrawIcon,   ES60_3_X,    EAknsMajorSkin, 0x21b9},

    // No pressed down grid in 3.1/3.2
    /* SP_QsnFrGridCornerTlPressed */    {KAknsIIDQsnFrGridCornerTl,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2681}, /*KAknsIIDQsnFrGridCornerTlPressed*/
    /* SP_QsnFrGridCornerTrPressed */    {KAknsIIDQsnFrGridCornerTr,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2682},
    /* SP_QsnFrGridCornerBlPressed */    {KAknsIIDQsnFrGridCornerBl,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2683},
    /* SP_QsnFrGridCornerBrPressed */    {KAknsIIDQsnFrGridCornerBr,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2684},
    /* SP_QsnFrGridSideTPressed */       {KAknsIIDQsnFrGridSideT,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2685},
    /* SP_QsnFrGridSideBPressed */       {KAknsIIDQsnFrGridSideB,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2686},
    /* SP_QsnFrGridSideLPressed */       {KAknsIIDQsnFrGridSideL,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2687},
    /* SP_QsnFrGridSideRPressed */       {KAknsIIDQsnFrGridSideR,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2688},
    /* SP_QsnFrGridCenterPressed */      {KAknsIIDQsnFrGridCenter,      ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2689},

    // No pressed down list in 3.1/3.2
    /* SP_QsnFrListCornerTlPressed */    {KAknsIIDQsnFrListCornerTl,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x268b}, /*KAknsIIDQsnFrListCornerTlPressed*/
    /* SP_QsnFrListCornerTrPressed */    {KAknsIIDQsnFrListCornerTr,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x268c},
    /* SP_QsnFrListCornerBlPressed */    {KAknsIIDQsnFrListCornerBl,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x268d},
    /* SP_QsnFrListCornerBrPressed */    {KAknsIIDQsnFrListCornerBr,    ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x268e},
    /* SP_QsnFrListSideTPressed */       {KAknsIIDQsnFrListSideT,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x268f},
    /* SP_QsnFrListSideBPressed */       {KAknsIIDQsnFrListSideB,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2690},
    /* SP_QsnFrListSideLPressed */       {KAknsIIDQsnFrListSideL,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2691},
    /* SP_QsnFrListSideRPressed */       {KAknsIIDQsnFrListSideR,       ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2692},
    /* SP_QsnFrListCenterPressed */      {KAknsIIDQsnFrListCenter,      ENoDraw,     ES60_3_X,    EAknsMajorSkin, 0x2693},
};

QPixmap QS60StyleModeSpecifics::skinnedGraphics(
    QS60StyleEnums::SkinParts stylepart, const QSize &size,
    QS60StylePrivate::SkinElementFlags flags)
{
    QPixmap themedImage;
    TRAPD( error, QT_TRYCATCH_LEAVING({
            const QPixmap skinnedImage = createSkinnedGraphicsLX(stylepart, size, flags);
            themedImage = skinnedImage;
    }));
    if (error)
        return themedImage = QPixmap();
    return themedImage;
}

QPixmap QS60StyleModeSpecifics::skinnedGraphics(
    QS60StylePrivate::SkinFrameElements frame, const QSize &size, QS60StylePrivate::SkinElementFlags flags)
{
    QPixmap themedImage;
    TRAPD( error, QT_TRYCATCH_LEAVING({
            const QPixmap skinnedImage = createSkinnedGraphicsLX(frame, size, flags);
            themedImage = skinnedImage;
    }));
    if (error)
        return themedImage = QPixmap();
    return themedImage;
}

QPixmap QS60StyleModeSpecifics::colorSkinnedGraphics(
    const QS60StyleEnums::SkinParts &stylepart, const QSize &size, QPainter *painter,
    QS60StylePrivate::SkinElementFlags flags)
{
    QPixmap colorGraphics;
    TRAPD(error, QT_TRYCATCH_LEAVING(colorGraphics = colorSkinnedGraphicsLX(stylepart, size, painter, flags)));
    return error ? QPixmap() : colorGraphics;
}

void QS60StyleModeSpecifics::fallbackInfo(const QS60StyleEnums::SkinParts &stylePart, TInt &fallbackIndex)
{
    switch(stylePart) {
        case QS60StyleEnums::SP_QgnGrafBarWaitAnim:
            fallbackIndex = EMbmAvkonQgn_graf_bar_wait_1;
            break;
        case QS60StyleEnums::SP_QgnGrafBarFrameCenter:
            fallbackIndex = EMbmAvkonQgn_graf_bar_frame_center;
            break;
        case QS60StyleEnums::SP_QgnGrafBarFrameSideL:
            fallbackIndex = EMbmAvkonQgn_graf_bar_frame_side_l;
            break;
        case QS60StyleEnums::SP_QgnGrafBarFrameSideR:
            fallbackIndex = EMbmAvkonQgn_graf_bar_frame_side_r;
            break;
        case QS60StyleEnums::SP_QgnGrafBarProgress:
            fallbackIndex = EMbmAvkonQgn_graf_bar_progress;
            break;
        case QS60StyleEnums::SP_QgnGrafTabActiveL:
            fallbackIndex = EMbmAvkonQgn_graf_tab_active_l;
            break;
        case QS60StyleEnums::SP_QgnGrafTabActiveM:
            fallbackIndex = EMbmAvkonQgn_graf_tab_active_m;
            break;
        case QS60StyleEnums::SP_QgnGrafTabActiveR:
            fallbackIndex = EMbmAvkonQgn_graf_tab_active_r;
            break;
        case QS60StyleEnums::SP_QgnGrafTabPassiveL:
            fallbackIndex = EMbmAvkonQgn_graf_tab_passive_l;
            break;
        case QS60StyleEnums::SP_QgnGrafTabPassiveM:
            fallbackIndex = EMbmAvkonQgn_graf_tab_passive_m;
            break;
        case QS60StyleEnums::SP_QgnGrafTabPassiveR:
            fallbackIndex = EMbmAvkonQgn_graf_tab_passive_r;
            break;
        case QS60StyleEnums::SP_QgnIndiCheckboxOff:
            fallbackIndex = EMbmAvkonQgn_indi_checkbox_off;
            break;
        case QS60StyleEnums::SP_QgnIndiCheckboxOn:
            fallbackIndex = EMbmAvkonQgn_indi_checkbox_on;
            break;
        case QS60StyleEnums::SP_QgnIndiHlColSuper:
            fallbackIndex = 0x4456; /* EMbmAvkonQgn_indi_hl_col_super */
            break;
        case QS60StyleEnums::SP_QgnIndiHlExpSuper:
            fallbackIndex = 0x4458; /* EMbmAvkonQgn_indi_hl_exp_super */
            break;
        case QS60StyleEnums::SP_QgnIndiHlLineBranch:
            fallbackIndex = 0x445A; /* EMbmAvkonQgn_indi_hl_line_branch */
            break;
        case QS60StyleEnums::SP_QgnIndiHlLineEnd:
            fallbackIndex = 0x445C; /* EMbmAvkonQgn_indi_hl_line_end */
            break;
        case QS60StyleEnums::SP_QgnIndiHlLineStraight:
            fallbackIndex = 0x445E; /* EMbmAvkonQgn_indi_hl_line_straight */
            break;
        case QS60StyleEnums::SP_QgnIndiMarkedAdd:
            fallbackIndex = EMbmAvkonQgn_indi_marked_add;
            break;
        case QS60StyleEnums::SP_QgnIndiNaviArrowLeft:
            fallbackIndex = EMbmAvkonQgn_indi_navi_arrow_left;
            break;
        case QS60StyleEnums::SP_QgnIndiNaviArrowRight:
            fallbackIndex = EMbmAvkonQgn_indi_navi_arrow_right;
            break;
        case QS60StyleEnums::SP_QgnIndiRadiobuttOff:
            fallbackIndex = EMbmAvkonQgn_indi_radiobutt_off;
            break;
        case QS60StyleEnums::SP_QgnIndiRadiobuttOn:
            fallbackIndex = EMbmAvkonQgn_indi_radiobutt_on;
            break;
        case QS60StyleEnums::SP_QgnGrafNsliderMarker:
            fallbackIndex = 17572; /* EMbmAvkonQgn_graf_nslider_marker */
            break;
        case QS60StyleEnums::SP_QgnGrafNsliderMarkerSelected:
            fallbackIndex = 17574; /* EMbmAvkonQgn_graf_nslider_marker_selected */
            break;
        case QS60StyleEnums::SP_QgnIndiSubmenu:
            fallbackIndex = EMbmAvkonQgn_indi_submenu;
            break;
        case QS60StyleEnums::SP_QgnNoteErased:
            fallbackIndex = EMbmAvkonQgn_note_erased;
            break;
        case QS60StyleEnums::SP_QgnNoteError:
            fallbackIndex = EMbmAvkonQgn_note_error;
            break;
        case QS60StyleEnums::SP_QgnNoteInfo:
            fallbackIndex = EMbmAvkonQgn_note_info;
            break;
        case QS60StyleEnums::SP_QgnNoteOk:
            fallbackIndex = EMbmAvkonQgn_note_ok;
            break;
        case QS60StyleEnums::SP_QgnNoteQuery:
            fallbackIndex = EMbmAvkonQgn_note_query;
            break;
        case QS60StyleEnums::SP_QgnNoteWarning:
            fallbackIndex = EMbmAvkonQgn_note_warning;
            break;
        case QS60StyleEnums::SP_QgnPropFileSmall:
            fallbackIndex = EMbmAvkonQgn_prop_file_small;
            break;
        case QS60StyleEnums::SP_QgnPropFolderCurrent:
            fallbackIndex = EMbmAvkonQgn_prop_folder_current;
            break;
        case QS60StyleEnums::SP_QgnPropFolderSmall:
            fallbackIndex = EMbmAvkonQgn_prop_folder_small;
            break;
        case QS60StyleEnums::SP_QgnPropFolderSmallNew:
            fallbackIndex = EMbmAvkonQgn_prop_folder_small_new;
            break;
        case QS60StyleEnums::SP_QgnPropPhoneMemcLarge:
            fallbackIndex = EMbmAvkonQgn_prop_phone_memc_large;
            break;
        default:
            fallbackIndex = -1;
            break;
    }
}

QPixmap QS60StyleModeSpecifics::colorSkinnedGraphicsLX(
    const QS60StyleEnums::SkinParts &stylepart,
    const QSize &size, QPainter *painter, QS60StylePrivate::SkinElementFlags flags)
{
    // this function can throw both exceptions and leaves. There are no cleanup dependencies between Qt and Symbian parts.
    const int stylepartIndex = (int)stylepart;
    const TAknsItemID skinId = m_partMap[stylepartIndex].skinID;

    TInt fallbackGraphicID = -1;
    HBufC* iconFile = HBufC::NewLC( KMaxFileName );
    fallbackInfo(stylepart, fallbackGraphicID);

    TAknsItemID colorGroup = KAknsIIDQsnIconColors;
    TRgb defaultColor = KRgbBlack;
    int colorIndex = -1; //set a bogus value to color index to ensure that painter color is used
                         //to color the icon
    if (painter) {
        QRgb widgetColor = painter->pen().color().rgb();
        defaultColor = TRgb(qRed(widgetColor), qGreen(widgetColor), qBlue(widgetColor));
    }

    const bool rotatedBy90or270 =
        (flags & (QS60StylePrivate::SF_PointEast | QS60StylePrivate::SF_PointWest));
    const TSize targetSize =
        rotatedBy90or270?TSize(size.height(), size.width()):TSize(size.width(), size.height());
    CFbsBitmap *icon = 0;
    CFbsBitmap *iconMask = 0;
    const TInt fallbackGraphicsMaskID =
        fallbackGraphicID == KErrNotFound?KErrNotFound:fallbackGraphicID+1; //masks are auto-generated as next in mif files
    MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();
    AknsUtils::CreateColorIconLC(
        skinInstance,
        skinId,
        colorGroup,
        colorIndex,
        icon,
        iconMask,
        AknIconUtils::AvkonIconFileName(),
        fallbackGraphicID,
        fallbackGraphicsMaskID,
        defaultColor);

    QPixmap result = fromFbsBitmap(icon, iconMask, flags, targetSize);
    CleanupStack::PopAndDestroy(3); //icon, iconMask, iconFile
    return result;
}

QColor QS60StyleModeSpecifics::colorValue(const TAknsItemID &colorGroup, int colorIndex)
{
    TRgb skinnedColor;
    MAknsSkinInstance* skin = AknsUtils::SkinInstance();
    AknsUtils::GetCachedColor(skin, skinnedColor, colorGroup, colorIndex);
    return QColor(skinnedColor.Red(),skinnedColor.Green(),skinnedColor.Blue());
}

struct QAutoFbsBitmapHeapLock
{
    QAutoFbsBitmapHeapLock(CFbsBitmap* aBmp) : mBmp(aBmp) { mBmp->LockHeap(); }
    ~QAutoFbsBitmapHeapLock() { mBmp->UnlockHeap(); }
    CFbsBitmap* mBmp;
};

QPixmap QS60StyleModeSpecifics::fromFbsBitmap(CFbsBitmap *icon, CFbsBitmap *mask, QS60StylePrivate::SkinElementFlags flags, const TSize &targetSize)
{
    Q_ASSERT(icon);

    AknIconUtils::DisableCompression(icon);
    TInt error = AknIconUtils::SetSize(icon, targetSize, EAspectRatioNotPreserved);

    if (mask && !error) {
        AknIconUtils::DisableCompression(mask);
        error = AknIconUtils::SetSize(mask, targetSize, EAspectRatioNotPreserved);
    }
    if (error)
        return QPixmap();

    QPixmap pixmap;
    QScopedPointer<QPixmapData> pd(QPixmapData::create(0, 0, QPixmapData::PixmapType));
    if (mask) {
        // Try the efficient path with less copying and conversion.
        QVolatileImage img(icon, mask);
        pd->fromNativeType(&img, QPixmapData::VolatileImage);
        if (!pd->isNull())
            pixmap = QPixmap(pd.take());
    }
    if (pixmap.isNull()) {
        // Potentially more expensive path.
        pd->fromNativeType(icon, QPixmapData::FbsBitmap);
        pixmap = QPixmap(pd.take());
        if (mask) {
            pixmap.setAlphaChannel(QPixmap::fromSymbianCFbsBitmap(mask));
        }
    }

    if ((flags & QS60StylePrivate::SF_PointEast) ||
        (flags & QS60StylePrivate::SF_PointSouth) ||
        (flags & QS60StylePrivate::SF_PointWest)) {
        QImage iconImage = pixmap.toImage();
        QTransform imageTransform;
        if (flags & QS60StylePrivate::SF_PointEast) {
            imageTransform.rotate(90);
        } else if (flags & QS60StylePrivate::SF_PointSouth) {
            imageTransform.rotate(180);
            iconImage = iconImage.transformed(imageTransform);
        } else if (flags & QS60StylePrivate::SF_PointWest) {
            imageTransform.rotate(270);
        }
        if (imageTransform.isRotating())
            iconImage = iconImage.transformed(imageTransform);

        pixmap = QPixmap::fromImage(iconImage);
    }
    if ((flags & QS60StylePrivate::SF_Mirrored_X_Axis) ||
        (flags & QS60StylePrivate::SF_Mirrored_Y_Axis)) {
        QImage iconImage = pixmap.toImage().mirrored(
            flags & QS60StylePrivate::SF_Mirrored_X_Axis,
            flags & QS60StylePrivate::SF_Mirrored_Y_Axis);
        pixmap = QPixmap::fromImage(iconImage);
    }

    return pixmap;
}

bool QS60StylePrivate::isTouchSupported()
{
    return bool(AknLayoutUtils::PenEnabled());
}

bool QS60StylePrivate::isToolBarBackground()
{
    return (QSysInfo::s60Version() == QSysInfo::SV_S60_3_1 || QSysInfo::s60Version() == QSysInfo::SV_S60_3_2);
}

bool QS60StylePrivate::hasSliderGrooveGraphic()
{
    return QSysInfo::s60Version() != QSysInfo::SV_S60_3_1;
}

bool QS60StylePrivate::isSingleClickUi()
{
    return (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0);
}

void QS60StylePrivate::deleteStoredSettings()
{
    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("QS60Style"));
    settings.remove(QString());
    settings.endGroup();
}

// Since S60Style has 'button' as a graphic, we don't have any native color which to use
// for QPalette::Button. Therefore S60Style needs to guesstimate palette color by calculating
// average rgb values for button pixels.
// Returns Qt::black if there is an issue with the graphics (image is NULL, or no constBits() found).
QColor QS60StylePrivate::colorFromFrameGraphics(SkinFrameElements frame) const
{
#ifndef QT_NO_SETTINGS
    TInt themeID = 0;
    //First we need to fetch active theme ID. We need to store the themeID at the same time
    //as color, so that we can later check if the stored color is still from the same theme.
    //Native side stores active theme UID/Timestamp into central repository.
    int error = 0;
    QT_TRAP_THROWING(
        CRepository *themeRepository = CRepository::NewLC(personalisationUID);
        if (themeRepository) {
            TBuf<32> value; //themeID is currently max of 8 + 1 + 8 characters, but lets have some extra space
            const TUint32 key = 0x00000002; //active theme key in the repository
            error = themeRepository->Get(key, value);
            if (error == KErrNone) {
                TLex lex(value);
                TPtrC numberToken(lex.NextToken());
                if (numberToken.Length())
                    error = TLex(numberToken).Val(themeID);
                else
                    error = KErrArgument;
            }
        }
        CleanupStack::PopAndDestroy(themeRepository);
    );

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("QS60Style"));
    if (themeID != 0) {
        QVariant buttonColor = settings.value(QLatin1String("ButtonColor"));
        if (!buttonColor.isNull()) {
            //there is a stored color value, lets see if the theme ID matches
            if (error == KErrNone) {
                QVariant themeUID = settings.value(QLatin1String("ThemeUID"));
                if (!themeUID.isNull() && themeUID.toInt() == themeID) {
                    QColor storedColor(buttonColor.value<QColor>());
                    if (storedColor.isValid())
                        return storedColor;
                }
            }
            settings.remove(QString()); //if color was invalid, or theme has been changed, just delete all stored settings
        }
    }
#endif

    QColor color = calculatedColor(frame);

#ifndef QT_NO_SETTINGS
    settings.setValue(QLatin1String("ThemeUID"), QVariant(themeID));
    if (frame == SF_ButtonNormal) //other colors are not currently calculated from graphics
        settings.setValue(QLatin1String("ButtonColor"), QVariant(color));
    settings.endGroup();
#endif

    return color;
}

QPoint qt_s60_fill_background_offset(const QWidget *targetWidget)
{
    CCoeControl *control = targetWidget->effectiveWinId();
    TPoint pos(0,0);
    if (control)
        pos = control->PositionRelativeToScreen();
    return QPoint(pos.iX, pos.iY);
}

QPixmap QS60StyleModeSpecifics::createSkinnedGraphicsLX(
    QS60StyleEnums::SkinParts part, const QSize &size,
    QS60StylePrivate::SkinElementFlags flags)
{
    // this function can throw both exceptions and leaves. There are no cleanup dependencies between Qt and Symbian parts.
    if (!size.isValid())
        return QPixmap();

    // Check release support and change part, if necessary.
    const TAknsItemID skinId = partSpecificThemeId((int)part);
    const int stylepartIndex = (int)part;
    const TDrawType drawType = m_partMap[stylepartIndex].drawType;
    Q_ASSERT(drawType != ENoDraw);
    const bool rotatedBy90or270 =
        (flags & (QS60StylePrivate::SF_PointEast | QS60StylePrivate::SF_PointWest));
    const TSize targetSize =
        rotatedBy90or270 ? TSize(size.height(), size.width()) : qt_QSize2TSize(size);

    MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();
    static const TDisplayMode displayMode = S60->supportsPremultipliedAlpha ? Q_SYMBIAN_ECOLOR16MAP : EColor16MA;
    static const TInt drawParam = S60->supportsPremultipliedAlpha ? KAknsDrawParamDefault : KAknsDrawParamRGBOnly;

    QPixmap result;

    switch (drawType) {
        case EDrawGulIcon: {
            CGulIcon* icon = AknsUtils::CreateGulIconL( AknsUtils::SkinInstance(), skinId, EFalse );
            if (icon)
                result = fromFbsBitmap(icon->Bitmap(), icon->Mask(), flags, targetSize);
            delete icon;
            break;
        }
        case EDrawIcon: {
            TInt fallbackGraphicID = -1;
            fallbackInfo(part, fallbackGraphicID);

            CFbsBitmap *icon = 0;
            CFbsBitmap *iconMask = 0;
            const TInt fallbackGraphicsMaskID =
                fallbackGraphicID == KErrNotFound?KErrNotFound:fallbackGraphicID+1; //masks are auto-generated as next in mif files

            AknsUtils::CreateIconL(
                skinInstance,
                skinId,
                icon,
                iconMask,
                AknIconUtils::AvkonIconFileName(),
                fallbackGraphicID ,
                fallbackGraphicsMaskID);

            result = fromFbsBitmap(icon, iconMask, flags, targetSize);
            delete icon;
            delete iconMask;
            break;
        }
        case EDrawBackground: {
    //        QS60WindowSurface::unlockBitmapHeap();
            CFbsBitmap *background = new (ELeave) CFbsBitmap(); //offscreen
            CleanupStack::PushL(background);
            User::LeaveIfError(background->Create(targetSize, displayMode));

            CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(background);
            CleanupStack::PushL(dev);
            CFbsBitGc *gc = NULL;
            User::LeaveIfError(dev->CreateContext(gc));
            CleanupStack::PushL(gc);

            CAknsBasicBackgroundControlContext *bgContext = CAknsBasicBackgroundControlContext::NewL(
                skinId,
                targetSize,
                EFalse);
            CleanupStack::PushL(bgContext);

            const TBool drawn = AknsDrawUtils::DrawBackground(
                skinInstance,
                bgContext,
                NULL,
                *gc,
                TPoint(),
                targetSize,
                drawParam);

            if (drawn)
                result = fromFbsBitmap(background, NULL, flags, targetSize);
            // if drawing fails in skin server, just ignore the background (probably OOM case)

            CleanupStack::PopAndDestroy(4, background); //background, dev, gc, bgContext
    //        QS60WindowSurface::lockBitmapHeap();
            break;
        }
        case EDrawAnimation: {
            CFbsBitmap* animationFrame;
            CFbsBitmap* frameMask;
            CAknBitmapAnimation* aknAnimation = 0;
            TBool constructedFromTheme = ETrue;

            QS60StyleAnimation* animation = QS60StylePrivate::animationDefinition(part); //ownership is not passed
            if (animation) {
                if (!animation->animationObject() && !animation->isResourceBased()) {// no pre-made item exists, create new animation
                    CAknBitmapAnimation* newAnimation = CAknBitmapAnimation::NewL();
                    CleanupStack::PushL(newAnimation);
                    if (newAnimation)
                        constructedFromTheme = newAnimation->ConstructFromSkinL(skinId);
                    if (constructedFromTheme && newAnimation->BitmapAnimData()->FrameArray().Count() > 0) {
                        animation->setResourceBased(false);
                        animation->setAnimationObject(newAnimation); //animation takes ownership
                    }
                    CleanupStack::Pop(newAnimation);
                }
                //fill-in stored information
                aknAnimation = animation->animationObject();
                constructedFromTheme = !animation->isResourceBased();
            }

            const int currentFrame = QS60StylePrivate::currentAnimationFrame(part);
            if (constructedFromTheme && aknAnimation && aknAnimation->BitmapAnimData()->FrameArray().Count() > 0) {
                //Animation was created successfully and contains frames, just fetch current frame
                if(currentFrame >= aknAnimation->BitmapAnimData()->FrameArray().Count())
                    User::Leave(KErrOverflow);
                const CBitmapFrameData* frameData = aknAnimation->BitmapAnimData()->FrameArray().At(currentFrame);
                if (frameData) {
                    animationFrame = frameData->Bitmap();
                    frameMask = frameData->Mask();
                }
            } else {
                //Theme does not contain animation theming, create frames from resource file
                TInt fallbackGraphicID = -1;
                fallbackInfo(part, fallbackGraphicID);
                fallbackGraphicID = fallbackGraphicID + (currentFrame * 2); //skip masks
                TInt fallbackGraphicsMaskID =
                    (fallbackGraphicID == KErrNotFound) ? KErrNotFound : fallbackGraphicID + 1; //masks are auto-generated as next in mif files
                if (fallbackGraphicsMaskID != KErrNotFound)
                    fallbackGraphicsMaskID = fallbackGraphicsMaskID + (currentFrame * 2); //skip actual graphics

                //Then draw animation frame
                AknsUtils::CreateIconL(
                    skinInstance,
                    KAknsIIDDefault, //animation is not themed, lets force fallback graphics
                    animationFrame,
                    frameMask,
                    AknIconUtils::AvkonIconFileName(),
                    fallbackGraphicID ,
                    fallbackGraphicsMaskID);
            }
            result = fromFbsBitmap(animationFrame, frameMask, flags, targetSize);
            if (!constructedFromTheme) {
                delete animationFrame;
                animationFrame = 0;
                delete frameMask;
                frameMask = 0;
            }
            break;
        }
    }
    if (!result)
        result = QPixmap();

    return result;
}

QPixmap QS60StyleModeSpecifics::createSkinnedGraphicsLX(QS60StylePrivate::SkinFrameElements frameElement,
    const QSize &size, QS60StylePrivate::SkinElementFlags flags)
{
    // this function can throw both exceptions and leaves. There are no cleanup dependencies between Qt and Symbian parts.
    if (!size.isValid())
        return QPixmap();

    const bool rotatedBy90or270 =
        (flags & (QS60StylePrivate::SF_PointEast | QS60StylePrivate::SF_PointWest));
    const TSize targetSize =
        rotatedBy90or270 ? TSize(size.height(), size.width()) : qt_QSize2TSize(size);

    MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();
    QPixmap result;

    static const TDisplayMode displayMode = S60->supportsPremultipliedAlpha ? Q_SYMBIAN_ECOLOR16MAP : EColor16MA;
    static const TInt drawParam = S60->supportsPremultipliedAlpha ? KAknsDrawParamDefault : KAknsDrawParamNoClearUnderImage|KAknsDrawParamRGBOnly;

    CFbsBitmap *frame = new (ELeave) CFbsBitmap(); //offscreen
    CleanupStack::PushL(frame);
    User::LeaveIfError(frame->Create(targetSize, displayMode));

    CFbsBitmapDevice* bitmapDev = CFbsBitmapDevice::NewL(frame);
    CleanupStack::PushL(bitmapDev);
    CFbsBitGc* bitmapGc = NULL;
    User::LeaveIfError(bitmapDev->CreateContext(bitmapGc));
    CleanupStack::PushL(bitmapGc);

    frame->LockHeap();
    memset(frame->DataAddress(), 0, frame->SizeInPixels().iWidth * frame->SizeInPixels().iHeight * 4);  // 4: argb bytes
    frame->UnlockHeap();

    const TRect outerRect(TPoint(0, 0), targetSize);
    const TRect innerRect = innerRectFromElement(frameElement, outerRect);

    TAknsItemID frameSkinID, centerSkinID;
    frameSkinID = centerSkinID = partSpecificThemeId(QS60StylePrivate::m_frameElementsData[frameElement].center);
    frameIdAndCenterId(frameElement, frameSkinID, centerSkinID);

    TBool drawn = AknsDrawUtils::DrawFrame(
        skinInstance,
        *bitmapGc,
        outerRect,
        innerRect,
        frameSkinID,
        centerSkinID,
        drawParam );

    if (S60->supportsPremultipliedAlpha) {
        if (drawn) {
            result = fromFbsBitmap(frame, NULL, flags, targetSize);
        } else {
            // Drawing might fail due to OOM (we can do nothing about that),
            // or due to skin item not being available.
            // If the latter occurs, lets try switch to non-release specific items (if available)
            // and re-try the drawing.
            frameSkinID = centerSkinID = m_partMap[(int)QS60StylePrivate::m_frameElementsData[frameElement].center].skinID;
            frameIdAndCenterId(frameElement, frameSkinID, centerSkinID);
            drawn = AknsDrawUtils::DrawFrame( skinInstance,
                                   *bitmapGc, outerRect, innerRect,
                                   frameSkinID, centerSkinID,
                                   drawParam );
            // in case drawing fails, even after using default graphics, ignore the error
            if (drawn)
                result = fromFbsBitmap(frame, NULL, flags, targetSize);
        }
    } else {
        TDisplayMode maskDepth = EGray256;
        // Query the skin item for possible frame graphics mask details.
        if (skinInstance) {
            CAknsMaskedBitmapItemData* skinMaskedBmp = static_cast<CAknsMaskedBitmapItemData*>(
                    skinInstance->GetCachedItemData(frameSkinID,EAknsITMaskedBitmap));
            if (skinMaskedBmp && skinMaskedBmp->Mask())
                maskDepth = skinMaskedBmp->Mask()->DisplayMode();
        }
        if (maskDepth != ENone) {
            CFbsBitmap *frameMask = new (ELeave) CFbsBitmap(); //offscreen
            CleanupStack::PushL(frameMask);
            User::LeaveIfError(frameMask->Create(targetSize, maskDepth));

            CFbsBitmapDevice* maskBitmapDevice = CFbsBitmapDevice::NewL(frameMask);
            CleanupStack::PushL(maskBitmapDevice);
            CFbsBitGc* maskBitGc = NULL;
            User::LeaveIfError(maskBitmapDevice->CreateContext(maskBitGc));
            CleanupStack::PushL(maskBitGc);

            if (drawn) {
                //ensure that the mask is really transparent
                maskBitGc->Activate( maskBitmapDevice );
                maskBitGc->SetPenStyle(CGraphicsContext::ENullPen);
                maskBitGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
                maskBitGc->SetBrushColor(KRgbWhite);
                maskBitGc->Clear();
                maskBitGc->SetBrushStyle(CGraphicsContext::ENullBrush);

                drawn = AknsDrawUtils::DrawFrame(skinInstance,
                                           *maskBitGc, outerRect, innerRect,
                                           frameSkinID, centerSkinID,
                                           KAknsSDMAlphaOnly |KAknsDrawParamNoClearUnderImage);
                if (drawn)
                    result = fromFbsBitmap(frame, frameMask, flags, targetSize);
            }
            CleanupStack::PopAndDestroy(3, frameMask);
        }
    }
    CleanupStack::PopAndDestroy(3, frame); //frame, bitmapDev, bitmapGc
    return result;
}

void QS60StyleModeSpecifics::frameIdAndCenterId(QS60StylePrivate::SkinFrameElements frameElement, TAknsItemID &frameId, TAknsItemID &centerId)
{
// There are some major mix-ups in skin declarations for some frames.
// First, the frames are not declared in sequence.
// Second, the parts use different major than the frame-master.

    switch(frameElement) {
        case QS60StylePrivate::SF_ToolTip:
            if (QSysInfo::s60Version() != QSysInfo::SV_S60_3_1) {
                centerId.Set(EAknsMajorGeneric, 0x19c2);
                frameId.Set(EAknsMajorSkin, 0x5300);
            } else {
                centerId.Set(KAknsIIDQsnFrPopupCenter);
                frameId.iMinor = centerId.iMinor - 9;
            }
            break;
        case QS60StylePrivate::SF_ToolBar:
            if (QSysInfo::s60Version() == QSysInfo::SV_S60_3_1 || 
                QSysInfo::s60Version() == QSysInfo::SV_S60_3_2) {
                centerId.Set(KAknsIIDQsnFrPopupCenterSubmenu);
                frameId.Set(KAknsIIDQsnFrPopupSub);
            }
            break;
        case QS60StylePrivate::SF_PopupBackground:
            centerId.Set(KAknsIIDQsnFrPopupCenterSubmenu);
            frameId.Set(KAknsIIDQsnFrPopupSub);
            break;
        case QS60StylePrivate::SF_PanelBackground:
            // remove center piece for panel graphics, so that only border is drawn
            centerId.Set(KAknsIIDNone);
            frameId.Set(KAknsIIDQsnFrSetOpt);
            break;
        default:
            // center should be correct here
            frameId.iMinor = centerId.iMinor - 9;
            break;
    }
}

TRect QS60StyleModeSpecifics::innerRectFromElement(QS60StylePrivate::SkinFrameElements frameElement, const TRect &outerRect)
{
    TInt widthShrink = QS60StylePrivate::pixelMetric(PM_FrameCornerWidth);
    TInt heightShrink = QS60StylePrivate::pixelMetric(PM_FrameCornerHeight);
    switch(frameElement) {
        case QS60StylePrivate::SF_PanelBackground:
            // panel should have slightly slimmer border to enable thin line of background graphics between closest component
            widthShrink = widthShrink - 2;
            heightShrink = heightShrink - 2;
            break;
        case QS60StylePrivate::SF_ToolTip:
            widthShrink = widthShrink >> 1;
            heightShrink = heightShrink >> 1;
            break;
        case QS60StylePrivate::SF_ListHighlight:
            //In Sym^3 devices highlights are less blocky
            if (QSysInfo::s60Version() > QSysInfo::SV_S60_5_0) {
                widthShrink += 2;
                heightShrink += 2;
            } else {
                widthShrink -= 2;
                heightShrink -= 2;
            }
            break;
        case QS60StylePrivate::SF_PopupBackground:
            widthShrink = widthShrink + 5;
            heightShrink = heightShrink + 5;
            break;
        default:
            break;
    }
    TRect innerRect(outerRect);
    innerRect.Shrink(widthShrink, heightShrink);
    return innerRect;
}

bool QS60StyleModeSpecifics::checkSupport(const int supportedRelease)
{
    const QSysInfo::S60Version currentRelease = QSysInfo::s60Version();
    return ( (currentRelease == QSysInfo::SV_S60_3_1 && supportedRelease & ES60_3_1) ||
             (currentRelease == QSysInfo::SV_S60_3_2 && supportedRelease & ES60_3_2) ||
             (currentRelease == QSysInfo::SV_S60_5_0 && supportedRelease & ES60_5_0) ||
             (currentRelease == QSysInfo::SV_S60_5_1 && supportedRelease & ES60_5_1) ||
             (currentRelease == QSysInfo::SV_S60_5_2 && supportedRelease & ES60_5_2) ||
             (currentRelease == QSysInfo::SV_S60_5_3 && supportedRelease & ES60_5_3) );
}

TAknsItemID QS60StyleModeSpecifics::partSpecificThemeId(int part)
{
    TAknsItemID newSkinId;
    if (!checkSupport(m_partMap[(int)part].supportInfo))
        newSkinId.Set(m_partMap[(int)part].newMajorSkinId, m_partMap[(int)part].newMinorSkinId);
    else
        newSkinId.Set(m_partMap[(int)part].skinID);
    return newSkinId;
}

QFont QS60StylePrivate::s60Font_specific(
    QS60StyleEnums::FontCategories fontCategory,
    int pointSize, bool resolveFontSize)
{
    Q_UNUSED(resolveFontSize);

    TAknFontCategory aknFontCategory = EAknFontCategoryUndefined;
    switch (fontCategory) {
        case QS60StyleEnums::FC_Primary:
            aknFontCategory = EAknFontCategoryPrimary;
            break;
        case QS60StyleEnums::FC_Secondary:
            aknFontCategory = EAknFontCategorySecondary;
            break;
        case QS60StyleEnums::FC_Title:
            aknFontCategory = EAknFontCategoryTitle;
            break;
        case QS60StyleEnums::FC_PrimarySmall:
            aknFontCategory = EAknFontCategoryPrimarySmall;
            break;
        case QS60StyleEnums::FC_Digital:
            aknFontCategory = EAknFontCategoryDigital;
            break;
        case QS60StyleEnums::FC_Undefined:
        default:
            break;
    }

    // Create AVKON font according the given parameters
    CWsScreenDevice* dev = CCoeEnv::Static()->ScreenDevice();
    TAknFontSpecification spec(aknFontCategory, TFontSpec(), NULL);
    if (pointSize > 0) {
        const TInt pixelSize = dev->VerticalTwipsToPixels(pointSize * KTwipsPerPoint);
        spec.SetTextPaneHeight(pixelSize + 4); // TODO: Is 4 a reasonable top+bottom margin?
    }

    QFont result;
    TRAPD( error, QT_TRYCATCH_LEAVING({
        const CAknLayoutFont* aknFont =
            AknFontAccess::CreateLayoutFontFromSpecificationL(*dev, spec);

        result = qt_TFontSpec2QFontL(aknFont->DoFontSpecInTwips());
        if (result.pointSize() != pointSize)
            result.setPointSize(pointSize); // Correct the font size returned by CreateLayoutFontFromSpecificationL()

        delete aknFont;
    }));
    if (error) result = QFont();
    return result;
}

void QS60StylePrivate::setActiveLayout()
{
    const QSize activeScreenSize(screenSize());
    int activeLayoutIndex = -1;
    const short screenHeight = (short)activeScreenSize.height();
    const short screenWidth = (short)activeScreenSize.width();
    for (int i=0; i<m_numberOfLayouts; i++) {
        if (screenHeight==m_layoutHeaders[i].height &&
            screenWidth==m_layoutHeaders[i].width) {
            activeLayoutIndex = i;
            break;
        }
    }

    //not found, lets try with either of dimensions
    if (activeLayoutIndex==-1){
        const QSysInfo::S60Version currentRelease = QSysInfo::s60Version();
        const bool landscape = screenHeight < screenWidth;

        activeLayoutIndex = (currentRelease == QSysInfo::SV_S60_3_1 || currentRelease == QSysInfo::SV_S60_3_2) ? 0 : 2;
        activeLayoutIndex += (!landscape) ? 1 : 0;
    }

    setCurrentLayout(activeLayoutIndex);
}

Q_GLOBAL_STATIC(QList<QS60StyleAnimation *>, m_animations)

QS60StylePrivate::QS60StylePrivate()
{
    //Animation defaults need to be created when style is instantiated
    QS60StyleAnimation* progressBarAnimation = new QS60StyleAnimation(QS60StyleEnums::SP_QgnGrafBarWaitAnim, 7, 100);
    m_animations()->append(progressBarAnimation);
    // No need to set active layout, if dynamic metrics API is available
    setActiveLayout();
}

void QS60StylePrivate::removeAnimations()
{
    //currently only one animation in the list.
    m_animations()->removeFirst();
}

QColor QS60StylePrivate::s60Color(QS60StyleEnums::ColorLists list,
    int index, const QStyleOption *option)
{
    static const TAknsItemID *idMap[] = {
        &KAknsIIDQsnHighlightColors,
        &KAknsIIDQsnIconColors,
        &KAknsIIDQsnLineColors,
        &KAknsIIDQsnOtherColors,
        &KAknsIIDQsnParentColors,
        &KAknsIIDQsnTextColors
    };
    Q_ASSERT((int)list < (int)sizeof(idMap)/sizeof(idMap[0]));
    const QColor color = QS60StyleModeSpecifics::colorValue(*idMap[(int) list], index - 1);
    return option ? QS60StylePrivate::stateColor(color, option) : color;
}

// In some cases, the AVKON UI themegraphic is already in 'disabled state'.
// If so, return true for these parts.
bool QS60StyleModeSpecifics::disabledPartGraphic(QS60StyleEnums::SkinParts &part)
{
    bool disabledGraphic = false;
    switch(part){
        // inactive button graphics are available from 5.0 onwards
        case QS60StyleEnums::SP_QsnFrButtonCornerTlInactive:
        case QS60StyleEnums::SP_QsnFrButtonCornerTrInactive:
        case QS60StyleEnums::SP_QsnFrButtonCornerBlInactive:
        case QS60StyleEnums::SP_QsnFrButtonCornerBrInactive:
        case QS60StyleEnums::SP_QsnFrButtonSideTInactive:
        case QS60StyleEnums::SP_QsnFrButtonSideBInactive:
        case QS60StyleEnums::SP_QsnFrButtonSideLInactive:
        case QS60StyleEnums::SP_QsnFrButtonSideRInactive:
        case QS60StyleEnums::SP_QsnFrButtonCenterInactive:
            if (!(QSysInfo::s60Version()==QSysInfo::SV_S60_3_1 ||
                  QSysInfo::s60Version()==QSysInfo::SV_S60_3_2))
                disabledGraphic = true;
            break;
        default:
            break;
    }
    return disabledGraphic;
}

// In some cases, the AVKON UI themegraphic is already in 'disabled state'.
// If so, return true for these frames.
bool QS60StyleModeSpecifics::disabledFrameGraphic(QS60StylePrivate::SkinFrameElements &frame)
{
    bool disabledGraphic = false;
    switch(frame){
        // inactive button graphics are available from 5.0 onwards
        case QS60StylePrivate::SF_ButtonInactive:
            if (!(QSysInfo::s60Version()==QSysInfo::SV_S60_3_1 ||
                  QSysInfo::s60Version()==QSysInfo::SV_S60_3_2))
                disabledGraphic = true;
            break;
        default:
            break;
    }
    return disabledGraphic;
}

QPixmap QS60StyleModeSpecifics::generateMissingThemeGraphic(QS60StyleEnums::SkinParts &part,
        const QSize &size, QS60StylePrivate::SkinElementFlags flags)
{
    if (!QS60StylePrivate::isTouchSupported())
        return QPixmap();

    QS60StyleEnums::SkinParts updatedPart = part;
    switch(part){
    // AVKON UI has a abnormal handling for scrollbar graphics. It is possible that the root
    // skin does not contain mandatory graphics for scrollbar pressed states. Therefore, AVKON UI
    // creates dynamically these graphics by modifying the normal state scrollbar graphics slightly.
    // S60Style needs to work similarly. Therefore if skingraphics call provides to be a miss
    // (i.e. result is not valid), style needs to draw normal graphics instead and apply some
    // modifications (similar to generatedIconPixmap()) to the result.
    case QS60StyleEnums::SP_QsnCpScrollHandleBottomPressed:
        updatedPart = QS60StyleEnums::SP_QsnCpScrollHandleBottom;
        break;
    case QS60StyleEnums::SP_QsnCpScrollHandleMiddlePressed:
        updatedPart = QS60StyleEnums::SP_QsnCpScrollHandleMiddle;
        break;
    case QS60StyleEnums::SP_QsnCpScrollHandleTopPressed:
        updatedPart = QS60StyleEnums::SP_QsnCpScrollHandleTop;
        break;
    default:
        break;
    }
    if (part==updatedPart) {
        return QPixmap();
    } else {
        QPixmap result = skinnedGraphics(updatedPart, size, flags);
        QStyleOption opt;
        QPalette *themePalette = QS60StylePrivate::themePalette();
        if (themePalette)
            opt.palette = *themePalette;

        // For now, always generate new icon based on "selected". In the future possibly, expand
        // this to consist other possibilities as well.
        result = QApplication::style()->generatedIconPixmap(QIcon::Selected, result, &opt);
        return result;
    }
}

QPixmap QS60StylePrivate::part(QS60StyleEnums::SkinParts part,
    const QSize &size, QPainter *painter, SkinElementFlags flags)
{
    QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);

    QPixmap result = (flags & SF_ColorSkinned)?
          QS60StyleModeSpecifics::colorSkinnedGraphics(part, size, painter, flags)
        : QS60StyleModeSpecifics::skinnedGraphics(part, size, flags);

    lock.relock();

    if (flags & SF_StateDisabled && !QS60StyleModeSpecifics::disabledPartGraphic(part)) {
        QStyleOption opt;
        QPalette *themePalette = QS60StylePrivate::themePalette();
        if (themePalette)
            opt.palette = *themePalette;
        result = QApplication::style()->generatedIconPixmap(QIcon::Disabled, result, &opt);
    }

    if (!result)
        result = QS60StyleModeSpecifics::generateMissingThemeGraphic(part, size, flags);

    return result;
}

QPixmap QS60StylePrivate::frame(SkinFrameElements frame, const QSize &size, SkinElementFlags flags)
{
    QSymbianFbsHeapLock lock(QSymbianFbsHeapLock::Unlock);
    QPixmap result = QS60StyleModeSpecifics::skinnedGraphics(frame, size, flags);
    lock.relock();

    if (flags & SF_StateDisabled && !QS60StyleModeSpecifics::disabledFrameGraphic(frame)) {
        QStyleOption opt;
        QPalette *themePalette = QS60StylePrivate::themePalette();
        if (themePalette)
            opt.palette = *themePalette;
        result = QApplication::style()->generatedIconPixmap(QIcon::Disabled, result, &opt);
    }
    return result;
}

QPixmap QS60StylePrivate::backgroundTexture(bool skipCreation)
{
    bool createNewBackground = false;
    TRect applicationRect = (static_cast<CEikAppUi*>(S60->appUi())->ApplicationRect());
    if (!m_background) {
        createNewBackground = true;
    } else {
        //if background brush does not match screensize, re-create it
        if (m_background->width() != applicationRect.Width() ||
            m_background->height() != applicationRect.Height()) {
            delete m_background;
            m_background = 0;
            createNewBackground = true;
        }
    }

    if (createNewBackground && !skipCreation) {
        QPixmap background = part(QS60StyleEnums::SP_QsnBgScreen,
            QSize(applicationRect.Width(), applicationRect.Height()), 0, SkinElementFlags());
        m_background = new QPixmap(background);

        // Notify all widgets that palette is updated with the actual background texture.
        QPalette pal = QApplication::palette();
        pal.setBrush(QPalette::Window, *m_background);
        QApplication::setPalette(pal);
        setThemePaletteHash(&pal);
        storeThemePalette(&pal);
        foreach (QWidget *widget, QApplication::allWidgets()){
            QEvent e(QEvent::PaletteChange);
            QApplication::sendEvent(widget, &e);
            setThemePalette(widget);
            widget->ensurePolished();
        }
    }
    if (!m_background)
        return QPixmap();
    return *m_background;
}

QSize QS60StylePrivate::screenSize()
{
    return QSize(S60->screenWidthInPixels, S60->screenHeightInPixels);
}

QS60Style::QS60Style()
    : QCommonStyle(*new QS60StylePrivate)
{
}

#ifdef Q_WS_S60
void QS60StylePrivate::handleDynamicLayoutVariantSwitch()
{
    clearCaches(QS60StylePrivate::CC_LayoutChange);
    setBackgroundTexture(qApp);
    setActiveLayout();
    foreach (QWidget *widget, QApplication::allWidgets())
        widget->ensurePolished();
}

void QS60StylePrivate::handleSkinChange()
{
    clearCaches(QS60StylePrivate::CC_ThemeChange);
    setThemePalette(qApp);
    foreach (QWidget *topLevelWidget, QApplication::allWidgets()){
        QEvent e(QEvent::StyleChange);
        QApplication::sendEvent(topLevelWidget, &e);
        setThemePalette(topLevelWidget);
        topLevelWidget->ensurePolished();
    }
#ifndef QT_NO_PROGRESSBAR
    //re-start animation timer
    stopAnimation(QS60StyleEnums::SP_QgnGrafBarWaitAnim); //todo: once we have more animations, we could say "stop all running ones"
    startAnimation(QS60StyleEnums::SP_QgnGrafBarWaitAnim); //and "re-start all previously running ones"
#endif
}

int QS60StylePrivate::currentAnimationFrame(QS60StyleEnums::SkinParts part)
{
    QS60StyleAnimation *animation = animationDefinition(part);
    // todo: looping could be done in QS60Style::timerEvent
    if (animation->frameCount() == animation->currentFrame())
        animation->setCurrentFrame(0);
    return animation->currentFrame();
}

QS60StyleAnimation* QS60StylePrivate::animationDefinition(QS60StyleEnums::SkinParts part)
{
    int i = 0;
    const int animationsCount = m_animations()->isEmpty() ? 0 : m_animations()->count();
    for(; i < animationsCount; i++) {
        if (part == m_animations()->at(i)->animationId())
            break;
    }
    return m_animations()->at(i);
}

void QS60StylePrivate::startAnimation(QS60StyleEnums::SkinParts animationPart)
{
    Q_Q(QS60Style);

    //Query animation data from theme and store values to local struct.
    QVariant themeAnimationDataVariant = QS60StyleModeSpecifics::themeDefinition(
        QS60StyleEnums::TD_AnimationData, animationPart);
    QList<QVariant> themeAnimationData = themeAnimationDataVariant.toList();

    QS60StyleAnimation *animation = QS60StylePrivate::animationDefinition(animationPart);
    if (animation) {
        if (themeAnimationData.at(QS60StyleEnums::AD_Interval).toInt() != 0)
            animation->setInterval(themeAnimationData.at(QS60StyleEnums::AD_Interval).toInt());

        if (themeAnimationData.at(QS60StyleEnums::AD_NumberOfFrames).toInt() != 0)
            animation->setFrameCount(themeAnimationData.at(QS60StyleEnums::AD_NumberOfFrames).toInt());

        //todo: playmode is ignored for now, since it seems to return invalid data on some themes
        //lets use the table values for play mode

        animation->setCurrentFrame(0); //always initialize
        const int timerId = q->startTimer(animation->interval());
        animation->setTimerId(timerId);
    }
}

void QS60StylePrivate::stopAnimation(QS60StyleEnums::SkinParts animationPart)
{
    Q_Q(QS60Style);

    QS60StyleAnimation *animation = QS60StylePrivate::animationDefinition(animationPart);
    if (animation) {
        animation->setCurrentFrame(0);
        if (animation->timerId() != 0) {
            q->killTimer(animation->timerId());
            animation->setTimerId(0);
        }
        animation->resetToDefaults();
    }
}

QVariant QS60StyleModeSpecifics::themeDefinition(
    QS60StyleEnums::ThemeDefinitions definition, QS60StyleEnums::SkinParts part)
{
    MAknsSkinInstance* skinInstance = AknsUtils::SkinInstance();

    Q_ASSERT(skinInstance);

    switch(definition) {
    //Animation definitions
    case QS60StyleEnums::TD_AnimationData:
        {
            CAknsBmpAnimItemData *animationData;
            TAknsItemID animationSkinId = partSpecificThemeId(part);
            QList<QVariant> list;

            TRAPD( error, QT_TRYCATCH_LEAVING(
                    animationData = static_cast<CAknsBmpAnimItemData*>(skinInstance->CreateUncachedItemDataL(
                            animationSkinId, EAknsITBmpAnim))));
            if (error)
                return list;

            if (animationData) {
                list.append((int)animationData->FrameInterval());
                list.append((int)animationData->NumberOfImages());

                QS60StyleEnums::AnimationMode playMode;
                switch(animationData->PlayMode()) {
                    case CBitmapAnimClientData::EPlay:
                        playMode = QS60StyleEnums::AM_PlayOnce;
                        break;
                    case CBitmapAnimClientData::ECycle:
                        playMode = QS60StyleEnums::AM_Looping;
                        break;
                    case CBitmapAnimClientData::EBounce:
                        playMode = QS60StyleEnums::AM_Bounce;
                        break;
                    default:
                        break;
                }
                list.append(QVariant((int)playMode));
                delete animationData;
            } else {
                list.append(0);
                list.append(0);
            }
            return list;
        }
        break;
    default:
        break;
    }
    return QVariant();
}

#endif // Q_WS_S60

QT_END_NAMESPACE

#endif // QT_NO_STYLE_S60 || QT_PLUGIN
