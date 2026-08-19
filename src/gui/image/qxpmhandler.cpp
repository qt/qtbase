// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "private/qxpmhandler_p.h"

#ifndef QT_NO_IMAGEFORMAT_XPM

#include <qbytearraymatcher.h>
#include <qdebug.h>
#include <qimage.h>
#include <qloggingcategory.h>
#include <qmap.h>
#include <qtextstream.h>
#include <qvariant.h>

#include <private/qcolor_p.h>
#include <private/qduplicatetracker_p.h> // for easier std::pmr detection
#include <private/qtools_p.h>
#include <private/qimage_p.h>

#include <algorithm>
#include <array>

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;

static quint64 xpmHash(const QString &str)
{
    unsigned int hashValue = 0;
    for (int i = 0; i < str.size(); ++i) {
        hashValue <<= 8;
        hashValue += (unsigned int)str.at(i).unicode();
    }
    return hashValue;
}
static quint64 xpmHash(char *str)
{
    unsigned int hashValue = 0;
    while (*str != '\0') {
        hashValue <<= 8;
        hashValue += (unsigned int)*str;
        ++str;
    }
    return hashValue;
}

static constexpr struct XPMRGBData {
    QRgb  value;
    const char name[21];
} xpmRgbTbl[] = {
  { qRgb(240,248,255),  "aliceblue" },
  { qRgb(250,235,215),  "antiquewhite" },
  { qRgb(255,239,219),  "antiquewhite1" },
  { qRgb(238,223,204),  "antiquewhite2" },
  { qRgb(205,192,176),  "antiquewhite3" },
  { qRgb(139,131,120),  "antiquewhite4" },
  { qRgb(127,255,212),  "aquamarine" },
  { qRgb(127,255,212),  "aquamarine1" },
  { qRgb(118,238,198),  "aquamarine2" },
  { qRgb(102,205,170),  "aquamarine3" },
  { qRgb( 69,139,116),  "aquamarine4" },
  { qRgb(240,255,255),  "azure" },
  { qRgb(240,255,255),  "azure1" },
  { qRgb(224,238,238),  "azure2" },
  { qRgb(193,205,205),  "azure3" },
  { qRgb(131,139,139),  "azure4" },
  { qRgb(245,245,220),  "beige" },
  { qRgb(255,228,196),  "bisque" },
  { qRgb(255,228,196),  "bisque1" },
  { qRgb(238,213,183),  "bisque2" },
  { qRgb(205,183,158),  "bisque3" },
  { qRgb(139,125,107),  "bisque4" },
  { qRgb(  0,  0,  0),  "black" },
  { qRgb(255,235,205),  "blanchedalmond" },
  { qRgb(  0,  0,255),  "blue" },
  { qRgb(  0,  0,255),  "blue1" },
  { qRgb(  0,  0,238),  "blue2" },
  { qRgb(  0,  0,205),  "blue3" },
  { qRgb(  0,  0,139),  "blue4" },
  { qRgb(138, 43,226),  "blueviolet" },
  { qRgb(165, 42, 42),  "brown" },
  { qRgb(255, 64, 64),  "brown1" },
  { qRgb(238, 59, 59),  "brown2" },
  { qRgb(205, 51, 51),  "brown3" },
  { qRgb(139, 35, 35),  "brown4" },
  { qRgb(222,184,135),  "burlywood" },
  { qRgb(255,211,155),  "burlywood1" },
  { qRgb(238,197,145),  "burlywood2" },
  { qRgb(205,170,125),  "burlywood3" },
  { qRgb(139,115, 85),  "burlywood4" },
  { qRgb( 95,158,160),  "cadetblue" },
  { qRgb(152,245,255),  "cadetblue1" },
  { qRgb(142,229,238),  "cadetblue2" },
  { qRgb(122,197,205),  "cadetblue3" },
  { qRgb( 83,134,139),  "cadetblue4" },
  { qRgb(127,255,  0),  "chartreuse" },
  { qRgb(127,255,  0),  "chartreuse1" },
  { qRgb(118,238,  0),  "chartreuse2" },
  { qRgb(102,205,  0),  "chartreuse3" },
  { qRgb( 69,139,  0),  "chartreuse4" },
  { qRgb(210,105, 30),  "chocolate" },
  { qRgb(255,127, 36),  "chocolate1" },
  { qRgb(238,118, 33),  "chocolate2" },
  { qRgb(205,102, 29),  "chocolate3" },
  { qRgb(139, 69, 19),  "chocolate4" },
  { qRgb(255,127, 80),  "coral" },
  { qRgb(255,114, 86),  "coral1" },
  { qRgb(238,106, 80),  "coral2" },
  { qRgb(205, 91, 69),  "coral3" },
  { qRgb(139, 62, 47),  "coral4" },
  { qRgb(100,149,237),  "cornflowerblue" },
  { qRgb(255,248,220),  "cornsilk" },
  { qRgb(255,248,220),  "cornsilk1" },
  { qRgb(238,232,205),  "cornsilk2" },
  { qRgb(205,200,177),  "cornsilk3" },
  { qRgb(139,136,120),  "cornsilk4" },
  { qRgb(  0,255,255),  "cyan" },
  { qRgb(  0,255,255),  "cyan1" },
  { qRgb(  0,238,238),  "cyan2" },
  { qRgb(  0,205,205),  "cyan3" },
  { qRgb(  0,139,139),  "cyan4" },
  { qRgb(  0,  0,139),  "darkblue" },
  { qRgb(  0,139,139),  "darkcyan" },
  { qRgb(184,134, 11),  "darkgoldenrod" },
  { qRgb(255,185, 15),  "darkgoldenrod1" },
  { qRgb(238,173, 14),  "darkgoldenrod2" },
  { qRgb(205,149, 12),  "darkgoldenrod3" },
  { qRgb(139,101,  8),  "darkgoldenrod4" },
  { qRgb(169,169,169),  "darkgray" },
  { qRgb(  0,100,  0),  "darkgreen" },
  { qRgb(169,169,169),  "darkgrey" },
  { qRgb(189,183,107),  "darkkhaki" },
  { qRgb(139,  0,139),  "darkmagenta" },
  { qRgb( 85,107, 47),  "darkolivegreen" },
  { qRgb(202,255,112),  "darkolivegreen1" },
  { qRgb(188,238,104),  "darkolivegreen2" },
  { qRgb(162,205, 90),  "darkolivegreen3" },
  { qRgb(110,139, 61),  "darkolivegreen4" },
  { qRgb(255,140,  0),  "darkorange" },
  { qRgb(255,127,  0),  "darkorange1" },
  { qRgb(238,118,  0),  "darkorange2" },
  { qRgb(205,102,  0),  "darkorange3" },
  { qRgb(139, 69,  0),  "darkorange4" },
  { qRgb(153, 50,204),  "darkorchid" },
  { qRgb(191, 62,255),  "darkorchid1" },
  { qRgb(178, 58,238),  "darkorchid2" },
  { qRgb(154, 50,205),  "darkorchid3" },
  { qRgb(104, 34,139),  "darkorchid4" },
  { qRgb(139,  0,  0),  "darkred" },
  { qRgb(233,150,122),  "darksalmon" },
  { qRgb(143,188,143),  "darkseagreen" },
  { qRgb(193,255,193),  "darkseagreen1" },
  { qRgb(180,238,180),  "darkseagreen2" },
  { qRgb(155,205,155),  "darkseagreen3" },
  { qRgb(105,139,105),  "darkseagreen4" },
  { qRgb( 72, 61,139),  "darkslateblue" },
  { qRgb( 47, 79, 79),  "darkslategray" },
  { qRgb(151,255,255),  "darkslategray1" },
  { qRgb(141,238,238),  "darkslategray2" },
  { qRgb(121,205,205),  "darkslategray3" },
  { qRgb( 82,139,139),  "darkslategray4" },
  { qRgb( 47, 79, 79),  "darkslategrey" },
  { qRgb(  0,206,209),  "darkturquoise" },
  { qRgb(148,  0,211),  "darkviolet" },
  { qRgb(255, 20,147),  "deeppink" },
  { qRgb(255, 20,147),  "deeppink1" },
  { qRgb(238, 18,137),  "deeppink2" },
  { qRgb(205, 16,118),  "deeppink3" },
  { qRgb(139, 10, 80),  "deeppink4" },
  { qRgb(  0,191,255),  "deepskyblue" },
  { qRgb(  0,191,255),  "deepskyblue1" },
  { qRgb(  0,178,238),  "deepskyblue2" },
  { qRgb(  0,154,205),  "deepskyblue3" },
  { qRgb(  0,104,139),  "deepskyblue4" },
  { qRgb(105,105,105),  "dimgray" },
  { qRgb(105,105,105),  "dimgrey" },
  { qRgb( 30,144,255),  "dodgerblue" },
  { qRgb( 30,144,255),  "dodgerblue1" },
  { qRgb( 28,134,238),  "dodgerblue2" },
  { qRgb( 24,116,205),  "dodgerblue3" },
  { qRgb( 16, 78,139),  "dodgerblue4" },
  { qRgb(178, 34, 34),  "firebrick" },
  { qRgb(255, 48, 48),  "firebrick1" },
  { qRgb(238, 44, 44),  "firebrick2" },
  { qRgb(205, 38, 38),  "firebrick3" },
  { qRgb(139, 26, 26),  "firebrick4" },
  { qRgb(255,250,240),  "floralwhite" },
  { qRgb( 34,139, 34),  "forestgreen" },
  { qRgb(220,220,220),  "gainsboro" },
  { qRgb(248,248,255),  "ghostwhite" },
  { qRgb(255,215,  0),  "gold" },
  { qRgb(255,215,  0),  "gold1" },
  { qRgb(238,201,  0),  "gold2" },
  { qRgb(205,173,  0),  "gold3" },
  { qRgb(139,117,  0),  "gold4" },
  { qRgb(218,165, 32),  "goldenrod" },
  { qRgb(255,193, 37),  "goldenrod1" },
  { qRgb(238,180, 34),  "goldenrod2" },
  { qRgb(205,155, 29),  "goldenrod3" },
  { qRgb(139,105, 20),  "goldenrod4" },
  { qRgb(190,190,190),  "gray" },
  { qRgb(  0,  0,  0),  "gray0" },
  { qRgb(  3,  3,  3),  "gray1" },
  { qRgb( 26, 26, 26),  "gray10" },
  { qRgb(255,255,255),  "gray100" },
  { qRgb( 28, 28, 28),  "gray11" },
  { qRgb( 31, 31, 31),  "gray12" },
  { qRgb( 33, 33, 33),  "gray13" },
  { qRgb( 36, 36, 36),  "gray14" },
  { qRgb( 38, 38, 38),  "gray15" },
  { qRgb( 41, 41, 41),  "gray16" },
  { qRgb( 43, 43, 43),  "gray17" },
  { qRgb( 46, 46, 46),  "gray18" },
  { qRgb( 48, 48, 48),  "gray19" },
  { qRgb(  5,  5,  5),  "gray2" },
  { qRgb( 51, 51, 51),  "gray20" },
  { qRgb( 54, 54, 54),  "gray21" },
  { qRgb( 56, 56, 56),  "gray22" },
  { qRgb( 59, 59, 59),  "gray23" },
  { qRgb( 61, 61, 61),  "gray24" },
  { qRgb( 64, 64, 64),  "gray25" },
  { qRgb( 66, 66, 66),  "gray26" },
  { qRgb( 69, 69, 69),  "gray27" },
  { qRgb( 71, 71, 71),  "gray28" },
  { qRgb( 74, 74, 74),  "gray29" },
  { qRgb(  8,  8,  8),  "gray3" },
  { qRgb( 77, 77, 77),  "gray30" },
  { qRgb( 79, 79, 79),  "gray31" },
  { qRgb( 82, 82, 82),  "gray32" },
  { qRgb( 84, 84, 84),  "gray33" },
  { qRgb( 87, 87, 87),  "gray34" },
  { qRgb( 89, 89, 89),  "gray35" },
  { qRgb( 92, 92, 92),  "gray36" },
  { qRgb( 94, 94, 94),  "gray37" },
  { qRgb( 97, 97, 97),  "gray38" },
  { qRgb( 99, 99, 99),  "gray39" },
  { qRgb( 10, 10, 10),  "gray4" },
  { qRgb(102,102,102),  "gray40" },
  { qRgb(105,105,105),  "gray41" },
  { qRgb(107,107,107),  "gray42" },
  { qRgb(110,110,110),  "gray43" },
  { qRgb(112,112,112),  "gray44" },
  { qRgb(115,115,115),  "gray45" },
  { qRgb(117,117,117),  "gray46" },
  { qRgb(120,120,120),  "gray47" },
  { qRgb(122,122,122),  "gray48" },
  { qRgb(125,125,125),  "gray49" },
  { qRgb( 13, 13, 13),  "gray5" },
  { qRgb(127,127,127),  "gray50" },
  { qRgb(130,130,130),  "gray51" },
  { qRgb(133,133,133),  "gray52" },
  { qRgb(135,135,135),  "gray53" },
  { qRgb(138,138,138),  "gray54" },
  { qRgb(140,140,140),  "gray55" },
  { qRgb(143,143,143),  "gray56" },
  { qRgb(145,145,145),  "gray57" },
  { qRgb(148,148,148),  "gray58" },
  { qRgb(150,150,150),  "gray59" },
  { qRgb( 15, 15, 15),  "gray6" },
  { qRgb(153,153,153),  "gray60" },
  { qRgb(156,156,156),  "gray61" },
  { qRgb(158,158,158),  "gray62" },
  { qRgb(161,161,161),  "gray63" },
  { qRgb(163,163,163),  "gray64" },
  { qRgb(166,166,166),  "gray65" },
  { qRgb(168,168,168),  "gray66" },
  { qRgb(171,171,171),  "gray67" },
  { qRgb(173,173,173),  "gray68" },
  { qRgb(176,176,176),  "gray69" },
  { qRgb( 18, 18, 18),  "gray7" },
  { qRgb(179,179,179),  "gray70" },
  { qRgb(181,181,181),  "gray71" },
  { qRgb(184,184,184),  "gray72" },
  { qRgb(186,186,186),  "gray73" },
  { qRgb(189,189,189),  "gray74" },
  { qRgb(191,191,191),  "gray75" },
  { qRgb(194,194,194),  "gray76" },
  { qRgb(196,196,196),  "gray77" },
  { qRgb(199,199,199),  "gray78" },
  { qRgb(201,201,201),  "gray79" },
  { qRgb( 20, 20, 20),  "gray8" },
  { qRgb(204,204,204),  "gray80" },
  { qRgb(207,207,207),  "gray81" },
  { qRgb(209,209,209),  "gray82" },
  { qRgb(212,212,212),  "gray83" },
  { qRgb(214,214,214),  "gray84" },
  { qRgb(217,217,217),  "gray85" },
  { qRgb(219,219,219),  "gray86" },
  { qRgb(222,222,222),  "gray87" },
  { qRgb(224,224,224),  "gray88" },
  { qRgb(227,227,227),  "gray89" },
  { qRgb( 23, 23, 23),  "gray9" },
  { qRgb(229,229,229),  "gray90" },
  { qRgb(232,232,232),  "gray91" },
  { qRgb(235,235,235),  "gray92" },
  { qRgb(237,237,237),  "gray93" },
  { qRgb(240,240,240),  "gray94" },
  { qRgb(242,242,242),  "gray95" },
  { qRgb(245,245,245),  "gray96" },
  { qRgb(247,247,247),  "gray97" },
  { qRgb(250,250,250),  "gray98" },
  { qRgb(252,252,252),  "gray99" },
  { qRgb(  0,255,  0),  "green" },
  { qRgb(  0,255,  0),  "green1" },
  { qRgb(  0,238,  0),  "green2" },
  { qRgb(  0,205,  0),  "green3" },
  { qRgb(  0,139,  0),  "green4" },
  { qRgb(173,255, 47),  "greenyellow" },
  { qRgb(190,190,190),  "grey" },
  { qRgb(  0,  0,  0),  "grey0" },
  { qRgb(  3,  3,  3),  "grey1" },
  { qRgb( 26, 26, 26),  "grey10" },
  { qRgb(255,255,255),  "grey100" },
  { qRgb( 28, 28, 28),  "grey11" },
  { qRgb( 31, 31, 31),  "grey12" },
  { qRgb( 33, 33, 33),  "grey13" },
  { qRgb( 36, 36, 36),  "grey14" },
  { qRgb( 38, 38, 38),  "grey15" },
  { qRgb( 41, 41, 41),  "grey16" },
  { qRgb( 43, 43, 43),  "grey17" },
  { qRgb( 46, 46, 46),  "grey18" },
  { qRgb( 48, 48, 48),  "grey19" },
  { qRgb(  5,  5,  5),  "grey2" },
  { qRgb( 51, 51, 51),  "grey20" },
  { qRgb( 54, 54, 54),  "grey21" },
  { qRgb( 56, 56, 56),  "grey22" },
  { qRgb( 59, 59, 59),  "grey23" },
  { qRgb( 61, 61, 61),  "grey24" },
  { qRgb( 64, 64, 64),  "grey25" },
  { qRgb( 66, 66, 66),  "grey26" },
  { qRgb( 69, 69, 69),  "grey27" },
  { qRgb( 71, 71, 71),  "grey28" },
  { qRgb( 74, 74, 74),  "grey29" },
  { qRgb(  8,  8,  8),  "grey3" },
  { qRgb( 77, 77, 77),  "grey30" },
  { qRgb( 79, 79, 79),  "grey31" },
  { qRgb( 82, 82, 82),  "grey32" },
  { qRgb( 84, 84, 84),  "grey33" },
  { qRgb( 87, 87, 87),  "grey34" },
  { qRgb( 89, 89, 89),  "grey35" },
  { qRgb( 92, 92, 92),  "grey36" },
  { qRgb( 94, 94, 94),  "grey37" },
  { qRgb( 97, 97, 97),  "grey38" },
  { qRgb( 99, 99, 99),  "grey39" },
  { qRgb( 10, 10, 10),  "grey4" },
  { qRgb(102,102,102),  "grey40" },
  { qRgb(105,105,105),  "grey41" },
  { qRgb(107,107,107),  "grey42" },
  { qRgb(110,110,110),  "grey43" },
  { qRgb(112,112,112),  "grey44" },
  { qRgb(115,115,115),  "grey45" },
  { qRgb(117,117,117),  "grey46" },
  { qRgb(120,120,120),  "grey47" },
  { qRgb(122,122,122),  "grey48" },
  { qRgb(125,125,125),  "grey49" },
  { qRgb( 13, 13, 13),  "grey5" },
  { qRgb(127,127,127),  "grey50" },
  { qRgb(130,130,130),  "grey51" },
  { qRgb(133,133,133),  "grey52" },
  { qRgb(135,135,135),  "grey53" },
  { qRgb(138,138,138),  "grey54" },
  { qRgb(140,140,140),  "grey55" },
  { qRgb(143,143,143),  "grey56" },
  { qRgb(145,145,145),  "grey57" },
  { qRgb(148,148,148),  "grey58" },
  { qRgb(150,150,150),  "grey59" },
  { qRgb( 15, 15, 15),  "grey6" },
  { qRgb(153,153,153),  "grey60" },
  { qRgb(156,156,156),  "grey61" },
  { qRgb(158,158,158),  "grey62" },
  { qRgb(161,161,161),  "grey63" },
  { qRgb(163,163,163),  "grey64" },
  { qRgb(166,166,166),  "grey65" },
  { qRgb(168,168,168),  "grey66" },
  { qRgb(171,171,171),  "grey67" },
  { qRgb(173,173,173),  "grey68" },
  { qRgb(176,176,176),  "grey69" },
  { qRgb( 18, 18, 18),  "grey7" },
  { qRgb(179,179,179),  "grey70" },
  { qRgb(181,181,181),  "grey71" },
  { qRgb(184,184,184),  "grey72" },
  { qRgb(186,186,186),  "grey73" },
  { qRgb(189,189,189),  "grey74" },
  { qRgb(191,191,191),  "grey75" },
  { qRgb(194,194,194),  "grey76" },
  { qRgb(196,196,196),  "grey77" },
  { qRgb(199,199,199),  "grey78" },
  { qRgb(201,201,201),  "grey79" },
  { qRgb( 20, 20, 20),  "grey8" },
  { qRgb(204,204,204),  "grey80" },
  { qRgb(207,207,207),  "grey81" },
  { qRgb(209,209,209),  "grey82" },
  { qRgb(212,212,212),  "grey83" },
  { qRgb(214,214,214),  "grey84" },
  { qRgb(217,217,217),  "grey85" },
  { qRgb(219,219,219),  "grey86" },
  { qRgb(222,222,222),  "grey87" },
  { qRgb(224,224,224),  "grey88" },
  { qRgb(227,227,227),  "grey89" },
  { qRgb( 23, 23, 23),  "grey9" },
  { qRgb(229,229,229),  "grey90" },
  { qRgb(232,232,232),  "grey91" },
  { qRgb(235,235,235),  "grey92" },
  { qRgb(237,237,237),  "grey93" },
  { qRgb(240,240,240),  "grey94" },
  { qRgb(242,242,242),  "grey95" },
  { qRgb(245,245,245),  "grey96" },
  { qRgb(247,247,247),  "grey97" },
  { qRgb(250,250,250),  "grey98" },
  { qRgb(252,252,252),  "grey99" },
  { qRgb(240,255,240),  "honeydew" },
  { qRgb(240,255,240),  "honeydew1" },
  { qRgb(224,238,224),  "honeydew2" },
  { qRgb(193,205,193),  "honeydew3" },
  { qRgb(131,139,131),  "honeydew4" },
  { qRgb(255,105,180),  "hotpink" },
  { qRgb(255,110,180),  "hotpink1" },
  { qRgb(238,106,167),  "hotpink2" },
  { qRgb(205, 96,144),  "hotpink3" },
  { qRgb(139, 58, 98),  "hotpink4" },
  { qRgb(205, 92, 92),  "indianred" },
  { qRgb(255,106,106),  "indianred1" },
  { qRgb(238, 99, 99),  "indianred2" },
  { qRgb(205, 85, 85),  "indianred3" },
  { qRgb(139, 58, 58),  "indianred4" },
  { qRgb(255,255,240),  "ivory" },
  { qRgb(255,255,240),  "ivory1" },
  { qRgb(238,238,224),  "ivory2" },
  { qRgb(205,205,193),  "ivory3" },
  { qRgb(139,139,131),  "ivory4" },
  { qRgb(240,230,140),  "khaki" },
  { qRgb(255,246,143),  "khaki1" },
  { qRgb(238,230,133),  "khaki2" },
  { qRgb(205,198,115),  "khaki3" },
  { qRgb(139,134, 78),  "khaki4" },
  { qRgb(230,230,250),  "lavender" },
  { qRgb(255,240,245),  "lavenderblush" },
  { qRgb(255,240,245),  "lavenderblush1" },
  { qRgb(238,224,229),  "lavenderblush2" },
  { qRgb(205,193,197),  "lavenderblush3" },
  { qRgb(139,131,134),  "lavenderblush4" },
  { qRgb(124,252,  0),  "lawngreen" },
  { qRgb(255,250,205),  "lemonchiffon" },
  { qRgb(255,250,205),  "lemonchiffon1" },
  { qRgb(238,233,191),  "lemonchiffon2" },
  { qRgb(205,201,165),  "lemonchiffon3" },
  { qRgb(139,137,112),  "lemonchiffon4" },
  { qRgb(173,216,230),  "lightblue" },
  { qRgb(191,239,255),  "lightblue1" },
  { qRgb(178,223,238),  "lightblue2" },
  { qRgb(154,192,205),  "lightblue3" },
  { qRgb(104,131,139),  "lightblue4" },
  { qRgb(240,128,128),  "lightcoral" },
  { qRgb(224,255,255),  "lightcyan" },
  { qRgb(224,255,255),  "lightcyan1" },
  { qRgb(209,238,238),  "lightcyan2" },
  { qRgb(180,205,205),  "lightcyan3" },
  { qRgb(122,139,139),  "lightcyan4" },
  { qRgb(238,221,130),  "lightgoldenrod" },
  { qRgb(255,236,139),  "lightgoldenrod1" },
  { qRgb(238,220,130),  "lightgoldenrod2" },
  { qRgb(205,190,112),  "lightgoldenrod3" },
  { qRgb(139,129, 76),  "lightgoldenrod4" },
  { qRgb(250,250,210),  "lightgoldenrodyellow" },
  { qRgb(211,211,211),  "lightgray" },
  { qRgb(144,238,144),  "lightgreen" },
  { qRgb(211,211,211),  "lightgrey" },
  { qRgb(255,182,193),  "lightpink" },
  { qRgb(255,174,185),  "lightpink1" },
  { qRgb(238,162,173),  "lightpink2" },
  { qRgb(205,140,149),  "lightpink3" },
  { qRgb(139, 95,101),  "lightpink4" },
  { qRgb(255,160,122),  "lightsalmon" },
  { qRgb(255,160,122),  "lightsalmon1" },
  { qRgb(238,149,114),  "lightsalmon2" },
  { qRgb(205,129, 98),  "lightsalmon3" },
  { qRgb(139, 87, 66),  "lightsalmon4" },
  { qRgb( 32,178,170),  "lightseagreen" },
  { qRgb(135,206,250),  "lightskyblue" },
  { qRgb(176,226,255),  "lightskyblue1" },
  { qRgb(164,211,238),  "lightskyblue2" },
  { qRgb(141,182,205),  "lightskyblue3" },
  { qRgb( 96,123,139),  "lightskyblue4" },
  { qRgb(132,112,255),  "lightslateblue" },
  { qRgb(119,136,153),  "lightslategray" },
  { qRgb(119,136,153),  "lightslategrey" },
  { qRgb(176,196,222),  "lightsteelblue" },
  { qRgb(202,225,255),  "lightsteelblue1" },
  { qRgb(188,210,238),  "lightsteelblue2" },
  { qRgb(162,181,205),  "lightsteelblue3" },
  { qRgb(110,123,139),  "lightsteelblue4" },
  { qRgb(255,255,224),  "lightyellow" },
  { qRgb(255,255,224),  "lightyellow1" },
  { qRgb(238,238,209),  "lightyellow2" },
  { qRgb(205,205,180),  "lightyellow3" },
  { qRgb(139,139,122),  "lightyellow4" },
  { qRgb( 50,205, 50),  "limegreen" },
  { qRgb(250,240,230),  "linen" },
  { qRgb(255,  0,255),  "magenta" },
  { qRgb(255,  0,255),  "magenta1" },
  { qRgb(238,  0,238),  "magenta2" },
  { qRgb(205,  0,205),  "magenta3" },
  { qRgb(139,  0,139),  "magenta4" },
  { qRgb(176, 48, 96),  "maroon" },
  { qRgb(255, 52,179),  "maroon1" },
  { qRgb(238, 48,167),  "maroon2" },
  { qRgb(205, 41,144),  "maroon3" },
  { qRgb(139, 28, 98),  "maroon4" },
  { qRgb(102,205,170),  "mediumaquamarine" },
  { qRgb(  0,  0,205),  "mediumblue" },
  { qRgb(186, 85,211),  "mediumorchid" },
  { qRgb(224,102,255),  "mediumorchid1" },
  { qRgb(209, 95,238),  "mediumorchid2" },
  { qRgb(180, 82,205),  "mediumorchid3" },
  { qRgb(122, 55,139),  "mediumorchid4" },
  { qRgb(147,112,219),  "mediumpurple" },
  { qRgb(171,130,255),  "mediumpurple1" },
  { qRgb(159,121,238),  "mediumpurple2" },
  { qRgb(137,104,205),  "mediumpurple3" },
  { qRgb( 93, 71,139),  "mediumpurple4" },
  { qRgb( 60,179,113),  "mediumseagreen" },
  { qRgb(123,104,238),  "mediumslateblue" },
  { qRgb(  0,250,154),  "mediumspringgreen" },
  { qRgb( 72,209,204),  "mediumturquoise" },
  { qRgb(199, 21,133),  "mediumvioletred" },
  { qRgb( 25, 25,112),  "midnightblue" },
  { qRgb(245,255,250),  "mintcream" },
  { qRgb(255,228,225),  "mistyrose" },
  { qRgb(255,228,225),  "mistyrose1" },
  { qRgb(238,213,210),  "mistyrose2" },
  { qRgb(205,183,181),  "mistyrose3" },
  { qRgb(139,125,123),  "mistyrose4" },
  { qRgb(255,228,181),  "moccasin" },
  { qRgb(255,222,173),  "navajowhite" },
  { qRgb(255,222,173),  "navajowhite1" },
  { qRgb(238,207,161),  "navajowhite2" },
  { qRgb(205,179,139),  "navajowhite3" },
  { qRgb(139,121, 94),  "navajowhite4" },
  { qRgb(  0,  0,128),  "navy" },
  { qRgb(  0,  0,128),  "navyblue" },
  { qRgb(253,245,230),  "oldlace" },
  { qRgb(107,142, 35),  "olivedrab" },
  { qRgb(192,255, 62),  "olivedrab1" },
  { qRgb(179,238, 58),  "olivedrab2" },
  { qRgb(154,205, 50),  "olivedrab3" },
  { qRgb(105,139, 34),  "olivedrab4" },
  { qRgb(255,165,  0),  "orange" },
  { qRgb(255,165,  0),  "orange1" },
  { qRgb(238,154,  0),  "orange2" },
  { qRgb(205,133,  0),  "orange3" },
  { qRgb(139, 90,  0),  "orange4" },
  { qRgb(255, 69,  0),  "orangered" },
  { qRgb(255, 69,  0),  "orangered1" },
  { qRgb(238, 64,  0),  "orangered2" },
  { qRgb(205, 55,  0),  "orangered3" },
  { qRgb(139, 37,  0),  "orangered4" },
  { qRgb(218,112,214),  "orchid" },
  { qRgb(255,131,250),  "orchid1" },
  { qRgb(238,122,233),  "orchid2" },
  { qRgb(205,105,201),  "orchid3" },
  { qRgb(139, 71,137),  "orchid4" },
  { qRgb(238,232,170),  "palegoldenrod" },
  { qRgb(152,251,152),  "palegreen" },
  { qRgb(154,255,154),  "palegreen1" },
  { qRgb(144,238,144),  "palegreen2" },
  { qRgb(124,205,124),  "palegreen3" },
  { qRgb( 84,139, 84),  "palegreen4" },
  { qRgb(175,238,238),  "paleturquoise" },
  { qRgb(187,255,255),  "paleturquoise1" },
  { qRgb(174,238,238),  "paleturquoise2" },
  { qRgb(150,205,205),  "paleturquoise3" },
  { qRgb(102,139,139),  "paleturquoise4" },
  { qRgb(219,112,147),  "palevioletred" },
  { qRgb(255,130,171),  "palevioletred1" },
  { qRgb(238,121,159),  "palevioletred2" },
  { qRgb(205,104,137),  "palevioletred3" },
  { qRgb(139, 71, 93),  "palevioletred4" },
  { qRgb(255,239,213),  "papayawhip" },
  { qRgb(255,218,185),  "peachpuff" },
  { qRgb(255,218,185),  "peachpuff1" },
  { qRgb(238,203,173),  "peachpuff2" },
  { qRgb(205,175,149),  "peachpuff3" },
  { qRgb(139,119,101),  "peachpuff4" },
  { qRgb(205,133, 63),  "peru" },
  { qRgb(255,192,203),  "pink" },
  { qRgb(255,181,197),  "pink1" },
  { qRgb(238,169,184),  "pink2" },
  { qRgb(205,145,158),  "pink3" },
  { qRgb(139, 99,108),  "pink4" },
  { qRgb(221,160,221),  "plum" },
  { qRgb(255,187,255),  "plum1" },
  { qRgb(238,174,238),  "plum2" },
  { qRgb(205,150,205),  "plum3" },
  { qRgb(139,102,139),  "plum4" },
  { qRgb(176,224,230),  "powderblue" },
  { qRgb(160, 32,240),  "purple" },
  { qRgb(155, 48,255),  "purple1" },
  { qRgb(145, 44,238),  "purple2" },
  { qRgb(125, 38,205),  "purple3" },
  { qRgb( 85, 26,139),  "purple4" },
  { qRgb(255,  0,  0),  "red" },
  { qRgb(255,  0,  0),  "red1" },
  { qRgb(238,  0,  0),  "red2" },
  { qRgb(205,  0,  0),  "red3" },
  { qRgb(139,  0,  0),  "red4" },
  { qRgb(188,143,143),  "rosybrown" },
  { qRgb(255,193,193),  "rosybrown1" },
  { qRgb(238,180,180),  "rosybrown2" },
  { qRgb(205,155,155),  "rosybrown3" },
  { qRgb(139,105,105),  "rosybrown4" },
  { qRgb( 65,105,225),  "royalblue" },
  { qRgb( 72,118,255),  "royalblue1" },
  { qRgb( 67,110,238),  "royalblue2" },
  { qRgb( 58, 95,205),  "royalblue3" },
  { qRgb( 39, 64,139),  "royalblue4" },
  { qRgb(139, 69, 19),  "saddlebrown" },
  { qRgb(250,128,114),  "salmon" },
  { qRgb(255,140,105),  "salmon1" },
  { qRgb(238,130, 98),  "salmon2" },
  { qRgb(205,112, 84),  "salmon3" },
  { qRgb(139, 76, 57),  "salmon4" },
  { qRgb(244,164, 96),  "sandybrown" },
  { qRgb( 46,139, 87),  "seagreen" },
  { qRgb( 84,255,159),  "seagreen1" },
  { qRgb( 78,238,148),  "seagreen2" },
  { qRgb( 67,205,128),  "seagreen3" },
  { qRgb( 46,139, 87),  "seagreen4" },
  { qRgb(255,245,238),  "seashell" },
  { qRgb(255,245,238),  "seashell1" },
  { qRgb(238,229,222),  "seashell2" },
  { qRgb(205,197,191),  "seashell3" },
  { qRgb(139,134,130),  "seashell4" },
  { qRgb(160, 82, 45),  "sienna" },
  { qRgb(255,130, 71),  "sienna1" },
  { qRgb(238,121, 66),  "sienna2" },
  { qRgb(205,104, 57),  "sienna3" },
  { qRgb(139, 71, 38),  "sienna4" },
  { qRgb(135,206,235),  "skyblue" },
  { qRgb(135,206,255),  "skyblue1" },
  { qRgb(126,192,238),  "skyblue2" },
  { qRgb(108,166,205),  "skyblue3" },
  { qRgb( 74,112,139),  "skyblue4" },
  { qRgb(106, 90,205),  "slateblue" },
  { qRgb(131,111,255),  "slateblue1" },
  { qRgb(122,103,238),  "slateblue2" },
  { qRgb(105, 89,205),  "slateblue3" },
  { qRgb( 71, 60,139),  "slateblue4" },
  { qRgb(112,128,144),  "slategray" },
  { qRgb(198,226,255),  "slategray1" },
  { qRgb(185,211,238),  "slategray2" },
  { qRgb(159,182,205),  "slategray3" },
  { qRgb(108,123,139),  "slategray4" },
  { qRgb(112,128,144),  "slategrey" },
  { qRgb(255,250,250),  "snow" },
  { qRgb(255,250,250),  "snow1" },
  { qRgb(238,233,233),  "snow2" },
  { qRgb(205,201,201),  "snow3" },
  { qRgb(139,137,137),  "snow4" },
  { qRgb(  0,255,127),  "springgreen" },
  { qRgb(  0,255,127),  "springgreen1" },
  { qRgb(  0,238,118),  "springgreen2" },
  { qRgb(  0,205,102),  "springgreen3" },
  { qRgb(  0,139, 69),  "springgreen4" },
  { qRgb( 70,130,180),  "steelblue" },
  { qRgb( 99,184,255),  "steelblue1" },
  { qRgb( 92,172,238),  "steelblue2" },
  { qRgb( 79,148,205),  "steelblue3" },
  { qRgb( 54,100,139),  "steelblue4" },
  { qRgb(210,180,140),  "tan" },
  { qRgb(255,165, 79),  "tan1" },
  { qRgb(238,154, 73),  "tan2" },
  { qRgb(205,133, 63),  "tan3" },
  { qRgb(139, 90, 43),  "tan4" },
  { qRgb(216,191,216),  "thistle" },
  { qRgb(255,225,255),  "thistle1" },
  { qRgb(238,210,238),  "thistle2" },
  { qRgb(205,181,205),  "thistle3" },
  { qRgb(139,123,139),  "thistle4" },
  { qRgb(255, 99, 71),  "tomato" },
  { qRgb(255, 99, 71),  "tomato1" },
  { qRgb(238, 92, 66),  "tomato2" },
  { qRgb(205, 79, 57),  "tomato3" },
  { qRgb(139, 54, 38),  "tomato4" },
  { qRgb( 64,224,208),  "turquoise" },
  { qRgb(  0,245,255),  "turquoise1" },
  { qRgb(  0,229,238),  "turquoise2" },
  { qRgb(  0,197,205),  "turquoise3" },
  { qRgb(  0,134,139),  "turquoise4" },
  { qRgb(238,130,238),  "violet" },
  { qRgb(208, 32,144),  "violetred" },
  { qRgb(255, 62,150),  "violetred1" },
  { qRgb(238, 58,140),  "violetred2" },
  { qRgb(205, 50,120),  "violetred3" },
  { qRgb(139, 34, 82),  "violetred4" },
  { qRgb(245,222,179),  "wheat" },
  { qRgb(255,231,186),  "wheat1" },
  { qRgb(238,216,174),  "wheat2" },
  { qRgb(205,186,150),  "wheat3" },
  { qRgb(139,126,102),  "wheat4" },
  { qRgb(255,255,255),  "white" },
  { qRgb(245,245,245),  "whitesmoke" },
  { qRgb(255,255,  0),  "yellow" },
  { qRgb(255,255,  0),  "yellow1" },
  { qRgb(238,238,  0),  "yellow2" },
  { qRgb(205,205,  0),  "yellow3" },
  { qRgb(139,139,  0),  "yellow4" },
  { qRgb(154,205, 50),  "yellowgreen" } };


inline bool operator<(const char *name, const XPMRGBData &data)
{ return qstrcmp(name, data.name) < 0; }
inline bool operator<(const XPMRGBData &data, const char *name)
{ return qstrcmp(data.name, name) < 0; }

static inline std::optional<QRgb> qt_get_named_xpm_rgb(const char *name_no_space)
{
    const auto end = std::end(xpmRgbTbl);
    const auto it = std::lower_bound(std::begin(xpmRgbTbl), end, name_no_space);
    if (it != end && !(name_no_space < *it))
        return it->value;
    return {};
}

/*****************************************************************************
  Misc. utility functions
 *****************************************************************************/
static QString fbname(const QString &fileName) // get file basename (sort of)
{
    QString s = fileName;
    if (!s.isEmpty()) {
        int i = qMax(s.lastIndexOf(u'/'), s.lastIndexOf(u'\\'));
        if (i < 0)
            i = 0;
        auto checkChar = [](QChar ch) -> bool {
            uchar uc = ch.unicode();
            return isAsciiLetterOrNumber(uc) || uc == '_';
        };
        int start = -1;
        for (; i < s.size(); ++i) {
            if (checkChar(s.at(i))) {
                start = i;
            } else if (start > 0)
                break;
        }
        if (start < 0)
            s.clear();
        else
            s = s.mid(start, i - start);
    }
    if (s.isEmpty())
        s = QString::fromLatin1("dummy");
    return s;
}

// Skip until ", read until the next ", return the rest in *buf
// Returns false on error, true on success

static bool read_xpm_string(QByteArray &buf, QIODevice *d, const char * const *source, int &index,
                            QByteArray &state)
{
    if (source) {
        buf = source[index++];
        return true;
    }

    buf = "";
    bool gotQuote = false;
    int offset = 0;
    forever {
        if (offset == state.size() || state.isEmpty()) {
            char buf[2048];
            qint64 bytesRead = d->read(buf, sizeof(buf));
            if (bytesRead <= 0)
                return false;
            state = QByteArray(buf, int(bytesRead));
            offset = 0;
        }

        if (!gotQuote) {
            if (state.at(offset++) == '"')
                gotQuote = true;
        } else {
            char c = state.at(offset++);
            if (c == '"')
                break;
            buf += c;
        }
    }
    state.remove(0, offset);
    return true;
}

// Tests if the given prefix can be the start of an XPM color specification

static bool is_xpm_color_spec_prefix(const QByteArray& prefix)
{
    return prefix == "c" ||
           prefix == "g" ||
           prefix == "g4" ||
           prefix == "m" ||
           prefix == "s";
}

// Reads XPM header.

static bool read_xpm_header(
    QIODevice *device, const char * const * source, int& index, QByteArray &state,
    int *cpp, int *ncols, int *w, int *h)
{
    QByteArray buf(200, 0);

    if (!read_xpm_string(buf, device, source, index, state))
        return false;

#ifdef Q_CC_MSVC
        if (sscanf_s(buf, "%d %d %d %d", w, h, ncols, cpp) < 4)
#else
    if (sscanf(buf, "%d %d %d %d", w, h, ncols, cpp) < 4)
#endif
        return false;                                        // < 4 numbers parsed

    if (*w <= 0 || *w > 32767 || *h <= 0 || *h > 32767 || *ncols <= 0 || *ncols > (64 * 64 * 64 * 64) || *cpp <= 0 || *cpp > 15)
        return false;                                        // failed sanity check

    return true;
}

// Reads XPM body (color information & pixels).

static bool read_xpm_body(
    QIODevice *device, const char * const * source, int& index, QByteArray& state,
    int cpp, int ncols, int w, int h, QImage& image)
{
    QByteArray buf(200, 0);
    int i;

    if (cpp < 0 || cpp > 15)
        return false;

    // For > 256 colors, we delay creation of the image until
    // after we have read the color specifications, so that we can
    // create it in correct format (Format_RGB32 vs Format_ARGB32,
    // depending on absence or presence of "c none", respectively)
    if (ncols <= 256) {
        if (!QImageIOHandler::allocateImage(QSize(w, h), QImage::Format_Indexed8, &image))
            return false;
        image.setColorCount(ncols);
    }

    QMap<quint64, int> colorMap;
    int currentColor;
    bool hasTransparency = false;

    for(currentColor=0; currentColor < ncols; ++currentColor) {
        if (!read_xpm_string(buf, device, source, index, state)) {
            qCWarning(lcImageIo, "XPM color specification missing");
            return false;
        }
        QByteArray index;
        index = buf.left(cpp);
        buf = buf.mid(cpp).simplified().trimmed().toLower();
        QList<QByteArray> tokens = buf.split(' ');
        i = tokens.indexOf('c');
        if (i < 0)
            i = tokens.indexOf('g');
        if (i < 0)
            i = tokens.indexOf("g4");
        if (i < 0)
            i = tokens.indexOf('m');
        if (i < 0) {
            qCWarning(lcImageIo, "XPM color specification is missing: %s", buf.constData());
            return false;        // no c/g/g4/m specification at all
        }
        QByteArray color;
        while ((++i < tokens.size()) && !is_xpm_color_spec_prefix(tokens.at(i))) {
            color.append(tokens.at(i));
        }
        if (color.isEmpty()) {
            qCWarning(lcImageIo, "XPM color value is missing from specification: %s", buf.constData());
            return false;        // no color value
        }
        buf = color;
        if (buf == "none") {
            hasTransparency = true;
            int transparentColor = currentColor;
            if (ncols <= 256) {
                image.setColor(transparentColor, 0);
                colorMap.insert(xpmHash(QLatin1StringView(index.constData())), transparentColor);
            } else {
                colorMap.insert(xpmHash(QLatin1StringView(index.constData())), 0);
            }
        } else {
            QRgb c_rgb = 0;
            if (((buf.size()-1) % 3) && (buf[0] == '#')) {
                buf.truncate(((buf.size()-1) / 4 * 3) + 1); // remove alpha channel left by imagemagick
            }
            if (buf[0] == '#') {
                c_rgb = qt_get_hex_rgb(buf).value_or(0);
            } else {
                c_rgb = qt_get_named_xpm_rgb(buf).value_or(0);
            }
            if (ncols <= 256) {
                image.setColor(currentColor, 0xff000000 | c_rgb);
                colorMap.insert(xpmHash(QLatin1StringView(index.constData())), currentColor);
            } else {
                colorMap.insert(xpmHash(QLatin1StringView(index.constData())), 0xff000000 | c_rgb);
            }
        }
    }

    if (ncols > 256) {
        // Now we can create 32-bit image of appropriate format
        QImage::Format format = hasTransparency ?
                                QImage::Format_ARGB32 : QImage::Format_RGB32;
        if (!QImageIOHandler::allocateImage(QSize(w, h), format, &image))
            return false;
    }

    // Read pixels
    for(int y=0; y<h; y++) {
        if (!read_xpm_string(buf, device, source, index, state)) {
            qCWarning(lcImageIo, "XPM pixels missing on image line %d", y);
            return false;
        }
        if (image.depth() == 8) {
            uchar *p = image.scanLine(y);
            uchar *d = (uchar *)buf.data();
            uchar *end = d + buf.size();
            int x;
            if (cpp == 1) {
                char b[2];
                b[1] = '\0';
                for (x=0; x<w && d<end; x++) {
                    b[0] = *d++;
                    *p++ = (uchar)colorMap[xpmHash(b)];
                }
            } else {
                char b[16];
                b[cpp] = '\0';
                for (x = 0; x < w && d + cpp <= end; x++) {
                    memcpy(b, (char *)d, cpp);
                    *p++ = (uchar)colorMap[xpmHash(b)];
                    d += cpp;
                }
            }
            // avoid uninitialized memory for malformed xpms
            if (x < w) {
                qCWarning(lcImageIo, "XPM pixels missing on image line %d (possibly a C++ trigraph).", y);
                memset(p, 0, w - x);
            }
        } else {
            QRgb *p = (QRgb*)image.scanLine(y);
            uchar *d = (uchar *)buf.data();
            uchar *end = d + buf.size();
            int x;
            char b[16];
            b[cpp] = '\0';
            for (x = 0; x < w && d + cpp <= end; x++) {
                memcpy(b, (char *)d, cpp);
                *p++ = (QRgb)colorMap[xpmHash(b)];
                d += cpp;
            }
            // avoid uninitialized memory for malformed xpms
            if (x < w) {
                qCWarning(lcImageIo, "XPM pixels missing on image line %d (possibly a C++ trigraph).", y);
                memset(p, 0, (w - x)*4);
            }
        }
    }

    if (device) {
        // Rewind unused characters, and skip to the end of the XPM struct.
        for (int i = state.size() - 1; i >= 0; --i)
            device->ungetChar(state[i]);
        char c;
        while (device->getChar(&c) && c != ';') {}
        while (device->getChar(&c) && c != '\n') {}
    }
    return true;
}

//
// INTERNAL
//
// Reads an .xpm from either the QImageIO or from the QString *.
// One of the two HAS to be 0, the other one is used.
//

bool qt_read_xpm_image_or_array(QIODevice *device, const char * const * source, QImage &image)
{
    if (!source)
        return true;

    QByteArray buf(200, 0);
    QByteArray state;

    int cpp, ncols, w, h, index = 0;

    if (device) {
        // "/* XPM */"
        int readBytes;
        if ((readBytes = device->readLine(buf.data(), buf.size())) < 0)
            return false;

        static constexpr auto matcher = qMakeStaticByteArrayMatcher("/* XPM");

        if (matcher.indexIn(buf) != 0) {
            while (readBytes > 0) {
                device->ungetChar(buf.at(readBytes - 1));
                --readBytes;
            }
            return false;
        }// bad magic
    }

    if (!read_xpm_header(device, source, index, state, &cpp, &ncols, &w, &h))
        return false;

    return read_xpm_body(device, source, index, state, cpp, ncols, w, h, image);
}

namespace {
template <size_t N>
struct CharBuffer : std::array<char, N>
{
    CharBuffer() {} // avoid value-initializing the whole array
};
}

static const char* xpm_color_name(int cpp, int index, CharBuffer<5> && returnable = {})
{
    static const char code[] = ".#abcdefghijklmnopqrstuvwxyzABCD"
                               "EFGHIJKLMNOPQRSTUVWXYZ0123456789";
    // cpp is limited to 4 and index is limited to 64^cpp
    if (cpp > 1) {
        if (cpp > 2) {
            if (cpp > 3) {
                returnable[4] = '\0';
                returnable[3] = code[index % 64];
                index /= 64;
            } else
                returnable[3] = '\0';
            returnable[2] = code[index % 64];
            index /= 64;
        } else
            returnable[2] = '\0';
        // the following 4 lines are a joke!
        if (index == 0)
            index = 64*44+21;
        else if (index == 64*44+21)
            index = 0;
        returnable[1] = code[index % 64];
        index /= 64;
    } else
        returnable[1] = '\0';
    returnable[0] = code[index];

    return returnable.data();
}


// write XPM image data
static bool write_xpm_image(const QImage &sourceImage, QIODevice *device, const QString &fileName)
{
    if (!device->isWritable())
        return false;

    QImage image;
    if (sourceImage.format() != QImage::Format_RGB32 && sourceImage.format() != QImage::Format_ARGB32 && sourceImage.format() != QImage::Format_ARGB32_Premultiplied)
        image = sourceImage.convertToFormat(QImage::Format_RGB32);
    else
        image = sourceImage;

#ifdef __cpp_lib_memory_resource
    char buffer[1024];
    std::pmr::monotonic_buffer_resource res{&buffer, sizeof buffer};
    std::pmr::map<QRgb, int> colorMap(&res);
#else
    std::map<QRgb, int> colorMap;
#endif

    const int w = image.width();
    const int h = image.height();
    int ncolors = 0;

    // build color table
    for (int y = 0; y < h; ++y) {
        const QRgb *yp = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            const auto [it, inserted] = colorMap.try_emplace(yp[x], ncolors);
            if (inserted)
                ++ncolors;
        }
    }

    // number of 64-bit characters per pixel needed to encode all colors
    int cpp = 1;
    for (int k = 64; ncolors > k; k *= 64) {
        ++cpp;
        // limit to 4 characters per pixel
        // 64^4 colors is enough for a 4096x4096 image
         if (cpp > 4) {
             qCWarning(lcImageIo, "Qt does not support writing XPM images with more than "
                       "64^4 colors (requested: %d colors).", ncolors);
             return false;
         }
    }

    // write header
    QTextStream s(device);
    s << "/* XPM */" << Qt::endl
      << "static char *" << fbname(fileName) << "[]={" << Qt::endl
      << '\"' << w << ' ' << h << ' ' << ncolors << ' ' << cpp << '\"';

    // write palette
    for (const auto &[color, index] : colorMap) {
        const QString line = image.format() != QImage::Format_RGB32 && !qAlpha(color)
            ? QString::asprintf("\"%s c None\"", xpm_color_name(cpp, index))
            : QString::asprintf("\"%s c #%02x%02x%02x\"", xpm_color_name(cpp, index),
                                qRed(color), qGreen(color), qBlue(color));
        s << ',' << Qt::endl << line;
    }

    // write pixels, limit to 4 characters per pixel
    for (int y = 0; y < h; ++y) {
        s << ',' << Qt::endl << '\"';
        const QRgb *yp = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < w; ++x)
            s << xpm_color_name(cpp, colorMap[yp[x]]);
        s << '\"';
    }
    s << "};" << Qt::endl;
    return static_cast<bool>(s);
}

QXpmHandler::QXpmHandler()
    : state(Ready), index(0)
{
}

bool QXpmHandler::readHeader()
{
    state = Error;
    if (!read_xpm_header(device(), nullptr, index, buffer, &cpp, &ncols, &width, &height))
        return false;
    state = ReadHeader;
    return true;
}

bool QXpmHandler::readImage(QImage *image)
{
    if (state == Error)
        return false;

    if (state == Ready && !readHeader()) {
        state = Error;
        return false;
    }

    if (!read_xpm_body(device(), nullptr, index, buffer, cpp, ncols, width, height, *image)) {
        state = Error;
        return false;
    }

    state = Ready;
    return true;
}

bool QXpmHandler::canRead() const
{
    if (state == Ready && !canRead(device()))
        return false;

    if (state != Error) {
        setFormat("xpm");
        return true;
    }

    return false;
}

bool QXpmHandler::canRead(QIODevice *device)
{
    if (!device) {
        qCWarning(lcImageIo, "QXpmHandler::canRead() called with no device");
        return false;
    }

    char head[6];
    if (device->peek(head, sizeof(head)) != sizeof(head))
        return false;

    return qstrncmp(head, "/* XPM", 6) == 0;
}

bool QXpmHandler::read(QImage *image)
{
    if (!canRead())
        return false;
    return readImage(image);
}

bool QXpmHandler::write(const QImage &image)
{
    return write_xpm_image(image, device(), fileName);
}

bool QXpmHandler::supportsOption(ImageOption option) const
{
    return option == Name
        || option == Size
        || option == ImageFormat;
}

QVariant QXpmHandler::option(ImageOption option) const
{
    if (option == Name) {
        return fileName;
    } else if (option == Size) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QXpmHandler*>(this)->readHeader())
            return QVariant();
        return QSize(width, height);
    } else if (option == ImageFormat) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QXpmHandler*>(this)->readHeader())
            return QVariant();
        // If we have more than 256 colors in the table, we need to
        // figure out, if it contains transparency. That means reading
        // the whole color table, which is too much work work pre-checking
        // the image format
        if (ncols <= 256)
            return QImage::Format_Indexed8;
        else
            return QImage::Format_Invalid;
    }

    return QVariant();
}

void QXpmHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == Name)
        fileName = value.toString();
}

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_XPM
