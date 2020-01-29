/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "paths.h"

QPainterPath Paths::rect()
{
    QPainterPath path;

    path.moveTo(45.885571, 62.857143);
    path.lineTo(154.11442, 62.857143);
    path.cubicTo(162.1236, 62.857143,
                 168.57142, 70.260744,
                 168.57142, 79.457144);
    path.lineTo(168.57142, 123.4);
    path.cubicTo(168.57142, 132.5964,
                 162.1236,  140,
                 154.11442, 140);
    path.lineTo(45.885571, 140);
    path.cubicTo(37.876394, 140,
                 31.428572, 132.5964,
                 31.428572, 123.4);
    path.lineTo(31.428572, 79.457144);
    path.cubicTo(31.428572,70.260744,
                 37.876394,62.857143,
                 45.885571,62.857143);
    path.closeSubpath();
    return path;
}

QPainterPath Paths::rect6()
{
    QPainterPath path;

    path.moveTo(45.885571, 62.857143);
    path.lineTo(154.11442, 62.857143);
    path.cubicTo(162.1236, 62.857143,
                 168.57142, 70.260744,
                 168.57142, 79.457144);
    path.lineTo(168.57142, 123.4);
    path.cubicTo(168.57142, 132.5964,
                 162.1236,  140,
                 154.11442, 140);
    path.lineTo(45.885571, 140);
    path.closeSubpath();
    return path;
}


QPainterPath Paths::heart()
{
    QPainterPath path;
    path.moveTo(263.41570, 235.14588);
    path.cubicTo(197.17570,235.14588,
                 143.41575,288.90587,
                 143.41575,355.14588);
    path.cubicTo(143.41575, 489.90139,
                 279.34890, 525.23318,
                 371.97820, 658.45392);
    path.cubicTo(459.55244,526.05056,
                 600.54070,485.59932,
                 600.54070,355.14588);
    path.cubicTo(600.54070,288.90588, 546.78080,235.14587, 480.54070,235.14588);
    path.cubicTo(432.49280,235.14588, 391.13910,263.51631, 371.97820,304.33338);
    path.cubicTo(352.81740,263.51630, 311.46370,235.14587, 263.41570,235.14588);
    path.closeSubpath();
    return path;
}


QPainterPath Paths::body()
{
    QPainterPath path;
    path.moveTo(62.500000,15.531250);
    path.cubicTo(48.633197,15.531250, 37.374999,26.789445, 37.375000,40.656250);
    path.cubicTo(37.375000,54.523053, 48.633195,65.781252, 62.500000,65.781250);
    path.cubicTo(76.366803,65.781250, 87.624998,54.523052, 87.625000,40.656250);
    path.cubicTo(87.625000,26.789447, 76.366802,15.531250, 62.500000,15.531250);
    path.closeSubpath();
    path.moveTo(54.437500,65.812500);
    path.cubicTo(35.184750,65.812499, 19.687500,81.341002, 19.687500,100.59375);
    path.lineTo(19.687500,155.68750);
    path.cubicTo(19.687501,167.50351, 25.539122,177.88308, 34.500000,184.15625);
    path.lineTo(34.500000,254.03125);
    path.cubicTo(34.499999,257.03306, 46.990615,259.43748, 62.500000,259.43750);
    path.cubicTo(78.009381,259.43751, 90.468750,257.03307, 90.468750,254.03125);
    path.lineTo(90.468750,184.15625);
    path.cubicTo(99.429633,177.88307, 105.28125,167.50352, 105.28125,155.68750);
    path.lineTo(105.28125,100.59375);
    path.cubicTo(105.28125,81.341000, 89.784000,65.812500, 70.531250,65.812500);
    path.lineTo(54.437500,65.812500);
    path.closeSubpath();

    return path;
}


QPainterPath Paths::mailbox()
{
    QPainterPath path;
    path.moveTo(355.22951,136.82424);
    path.lineTo(332.03629,112.56585);
    path.lineTo(325.71086,57.501867);
    path.cubicTo(325.71086,57.501867, 410.12308,19.428758, 427.45202,29.094560);
    path.cubicTo(444.78096,38.760366, 443.62570,54.289660, 443.62570,54.289660);
    path.lineTo(443.62570,100.11509);
    path.lineTo(355.22951,136.82424);
    path.closeSubpath();

    return path;
}


QPainterPath Paths::deer()
{
    QPainterPath path;

    path.moveTo(39.88,31.658);
    path.cubicTo(35.632,31.658, 31.398,31.004, 27.871,32.82);
    path.cubicTo(25.015,34.29, 19.608,34.158, 16.297,34.158);
    path.cubicTo(14.722,34.158, 17.755,37.718, 17.709,38.922);
    path.cubicTo(17.578,42.396, 24.612,43.15, 26.755,44.058);
    path.cubicTo(30.062,45.46, 28.682,47.701, 28.963,50.574);
    path.cubicTo(29.715,58.243, 26.887,63.745, 24.182,70.589);
    path.cubicTo(23.365,72.657, 21.772,75.56, 21.972,77.866);
    path.cubicTo(22.333,82.029, 15.803,77.207, 13.894,76.535);
    path.cubicTo(10.977,75.508, 5.507,74.071, 2.424,75.331);
    path.cubicTo(-1.532,76.947, 0.076,80.491, 2.169,82.806);
    path.cubicTo(6.17,87.234, 2.703,90.713, 3.895,95.363);
    path.cubicTo(4.321,97.026, 11.682,104.683, 12.858,103.668);
    path.cubicTo(16.706,100.347, 11.464,98.692, 10.105,96.164);
    path.cubicTo(9.487,95.015, 8.616,83.742, 8.866,83.759);
    path.cubicTo(10.018,83.837, 12.591,85.867, 13.671,86.392);
    path.cubicTo(16.889,87.954, 20.066,89.63, 22.963,91.741);
    path.cubicTo(29.156,94.47, 35.543,96.965, 42.102,98.676);
    path.cubicTo(51.085,101.02, 59.407,102.003, 68.009,106.005);
    path.cubicTo(72.92,108.289, 72.05,113.282, 75.744,117.004);
    path.cubicTo(79.422,120.709, 84.733,123.053, 88.978,126.053);
    path.cubicTo(92.402,128.473, 95.422,132.308, 97.334,135.998);
    path.cubicTo(99.551,140.279, 99.071,146.004, 99.838,150.674);
    path.cubicTo(100.369,153.91, 104.378,156.321, 106.302,158.859);
    path.cubicTo(110.471,164.355, 109.86,155.112, 108.163,154.412);
    path.cubicTo(104.97,153.094, 103.991,146.625, 103.812,143.439);
    path.cubicTo(103.525,138.336, 105.568,134.331, 101.918,130.346);
    path.cubicTo(95.104,122.907, 89.488,114.182, 94.711,103.742);
    path.cubicTo(96.889,99.388, 91.191,95.497, 96.94,94.368);
    path.cubicTo(99.551,93.856, 102.49,94.367, 104.326,92.034);
    path.cubicTo(106.639,89.095, 105.063,85.343, 102.943,82.798);
    path.cubicTo(102.686,82.417, 102.359,82.121, 101.962,81.909);
    path.cubicTo(102.331,81.909, 101.923,86.98, 100.981,87.628);
    path.cubicTo(98.868,89.082, 95.569,91.586, 92.88,91.672);
    path.cubicTo(90.569,91.745, 86.738,89.184, 85.212,87.658);
    path.cubicTo(84.092,86.538, 80.176,86.157, 78.598,85.83);
    path.cubicTo(74.737,85.031, 71.741,84.326, 68.012,82.806);
    path.cubicTo(63.318,80.893, 58.687,78.672, 54.555,75.71);
    path.cubicTo(44.573,68.555, 42.755,56.146, 44.022,44.495);
    path.cubicTo(44.295,41.987, 43.169,38.057, 44.617,35.915);
    path.cubicTo(44.961,35.406, 46.52,35.553, 47.119,35.024);
    path.cubicTo(47.882,34.35, 49.574,31.822, 49.878,30.792);
    path.cubicTo(51.126,26.569, 44.36,32.002, 45.336,31.938);
    path.cubicTo(43.861,32.036, 47.011,22.934, 47.191,22.574);
    path.cubicTo(47.555,21.846, 52.489,13.123, 49.511,13.222);
    path.cubicTo(47.643,13.284, 48.563,18.667, 46.354,18.227);
    path.cubicTo(43.964,17.751, 40.522,11.396, 41.566,9.011);
    path.cubicTo(43.4,4.819, 39.743,3.905, 39.214,7.564);
    path.cubicTo(39.112,8.269, 40.893,13.438, 38.159,12.665);
    path.cubicTo(35.335,11.866, 35.748,-0.125, 34.38,-8.0352391e-15);
    path.cubicTo(31.991,0.219, 34.074,10.836, 33.361,12.176);
    path.cubicTo(33.144,12.584, 29.68,8.66, 29.459,7.718);
    path.cubicTo(28.48,3.558, 28.031,5.106, 26.87,7.752);
    path.cubicTo(25.333,11.254, 37.159,17.423, 39.292,18.663);
    path.cubicTo(40.993,19.651, 42.39,20.504, 42.973,22.48);
    path.cubicTo(43.482,24.205, 44.098,26.568, 42.926,28.191);
    path.cubicTo(42.092,29.346, 39.88,29.982, 39.88,31.658);
    return path;
}


QPainterPath Paths::fire()
{
    QPainterPath path;

    path.moveTo(362.83759,116.70426);
    path.cubicTo(342.56574,131.59686, 300.71403,161.23127, 311.38454,218.12635);
    path.cubicTo(322.05506,275.02144, 358.53432,301.66527, 328.90674,328.73285);
    path.cubicTo(299.27916,355.80044, 251.48877,339.59410, 255.46042,288.61972);
    path.cubicTo(258.22374,253.15368, 278.34141,205.10942, 278.34141,205.10942);
    path.cubicTo(278.34141,205.10942, 234.02455,233.13427, 219.68939,254.01270);
    path.cubicTo(205.35424,274.89113, 189.71452,330.07842, 208.58356,373.33974);
    path.cubicTo(227.45261,416.60109, 316.46286,456.33444, 351.12048,514.32780);
    path.cubicTo(374.10258,552.78425, 355.05815,613.59741, 310.80422,636.59310);
    path.cubicTo(256.63287,664.74219, 299.16588,580.49238, 285.22551,523.86186);
    path.cubicTo(273.46790,476.09839, 265.70022,445.12001, 188.03132,432.51681);
    path.cubicTo(233.72591,465.34901, 242.16068,495.04075, 241.45928,524.11772);
    path.cubicTo(240.78648,552.00862, 214.39595,634.57293, 177.39967,596.79021);
    path.cubicTo(140.72642,559.33737, 214.27071,512.68654, 170.92945,471.62081);
    path.cubicTo(174.73284,501.40284, 145.30515,514.98828, 131.55318,544.54392);
    path.cubicTo(118.22673,573.18509, 123.55251,610.30651, 139.07596,645.41379);
    path.cubicTo(181.14122,740.38745, 266.95518,726.23964, 208.75321,797.88229);
    path.cubicTo(164.01134,852.95649, 162.90150,907.45084, 205.60384,970.81121);
    path.cubicTo(240.06795,1021.9479, 371.11663,1060.7652, 432.20697,960.93460);
    path.cubicTo(501.87852,820.00694, 357.14883,780.33174, 386.29974,732.84721);
    path.cubicTo(405.70205,701.24238, 472.56601,668.86516, 501.09199,644.21233);
    path.cubicTo(564.18184,587.55421, 561.84437,497.32621, 522.74229,471.25817);
    path.cubicTo(530.19030,501.05022, 514.99952,542.79339, 483.67099,551.29691);
    path.cubicTo(423.41173,567.65308, 458.18351,411.79373, 564.02075,393.61925);
    path.cubicTo(530.91135,366.44998, 501.31413,367.33484, 454.91711,379.11707);
    path.cubicTo(397.61736,393.57908, 407.64322,315.40944, 494.34643,262.67861);
    path.cubicTo(549.19500,229.32101, 499.11573,147.63302, 491.66772,136.46100);
    path.cubicTo(485.38713,213.93294, 435.43515,233.35601, 409.98053,235.72292);
    path.cubicTo(375.27049,238.95043, 377.84554,214.33812, 396.75003,178.92950);
    path.cubicTo(416.21172,142.47722, 448.15395,89.429942, 376.51366,44.060977);
    path.cubicTo(388.13560,71.270572, 395.93673,94.012962, 362.83759,116.70426);
    path.closeSubpath();
    return path;
}


QPainterPath Paths::lips()
{
    QPainterPath path;

    path.moveTo(177.02257,176.65905);
    path.cubicTo(154.11895,176.65905, 136.56711,174.32266, 110.41800,155.61729);
    path.cubicTo(83.894106,136.64382, 70.456540,123.78263, 44.264608,101.00195);
    path.cubicTo(36.985036,94.670475, 11.607987,76.421189, 0.62503194,72.562763);
    path.cubicTo(22.778258,60.937514, 46.738237,46.430325, 55.325084,40.325054);
    path.cubicTo(79.128700,23.400628, 99.203004,0.53294656, 116.15033,0.61582047);
    path.cubicTo(129.59137,0.68308215, 144.54744,18.524567, 177.02257,18.524567);
    path.cubicTo(210.04060,18.524567, 224.45379,0.68308215, 237.89483,0.61582047);
    path.cubicTo(254.84216,0.53294656, 274.91646,23.400628, 298.72008,40.325054);
    path.cubicTo(307.30692,46.430325, 331.26690,60.937514, 353.42013,72.562763);
    path.cubicTo(342.43717,76.421189, 317.06013,94.670475, 309.78055,101.00195);
    path.cubicTo(283.58862,123.78263, 270.15105,136.64382, 243.62716,155.61729);
    path.cubicTo(217.47805,174.32266, 199.38332,176.65905, 177.02257,176.65905);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::bezier1()
{
    QPainterPath path;
    path.moveTo(50, 50);
    path.cubicTo(100, 100,
                 520, 90,
                 400,400);
    return path;
}

QPainterPath Paths::bezier2()
{
    QPainterPath path;
    path.moveTo(200,200);
    path.cubicTo(200,125, 500,100, 500,500);

    return path;
}

QPainterPath Paths::random1()
{
    QPainterPath path;

    path.moveTo(65.714286,91.428571);
    path.lineTo(217.14286, 102.85714);
    path.cubicTo(219.04762, 106.66666,
                    220.95238, 110.47619,
                    222.85714,114.28571);
    path.cubicTo(231.2679,  131.10723,
                    214.72525, 138.24185,
                    211.42857,151.42857);
    path.cubicTo(207.25902, 168.10676,
                    213.24674, 175.8441,
                    217.14286,191.42857);
    path.cubicTo(221.088, 207.20915,
                    201.21538,205.71429,
                    188.57143,205.71429);
    path.cubicTo(170.18303, 205.71429,
                    161.42918, 197.50045,
                    145.71429,185.71429);
    path.cubicTo(113.93441, 161.87938,
                    132.73699, 182.37652,
                    137.14286, 200);
    path.cubicTo(140.37884, 212.94392,
                    128.50252, 217.16009,
                    117.14286, 220);
    path.cubicTo(98.323209, 224.70491,
                    91.206108, 205.41767,
                    82.857143, 194.28571);
    path.cubicTo(77.307286, 186.8859,
                    84.541768, 158.97578,
                    85.714286, 154.28571);
    path.cubicTo(87.843677, 145.76815,
                    67.066253, 132.78054,
                    60       , 125.71429);
    path.cubicTo(54.074503, 119.78879,
                    64.646395,  95.700137,
                    65.714286,  91.428571);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::random2()
{
    QPainterPath path;

    path.moveTo(314.28571,160);
    path.cubicTo(434.28571,125.71429,
                 505.71429,200,
                 505.71429,200);
    path.lineTo(454.28571, 305.71429);
    path.lineTo(337.14286, 302.85714);
    path.cubicTo(337.14286, 302.85714,
                 308.57143, 340,
                 337.14286, 302.85714);
    path.cubicTo(365.71429, 265.71429,
                 200,       420,
                 300,       291.42857);
    path.cubicTo(400,       162.85714,
                 254.28571, 240,
                 254.28571, 240);
    path.cubicTo(254.28571,240,
                 240,71.428571,
                 288.57143,134.28571);
    path.cubicTo(337.14286,197.14286,
                 314.28571,162.85714,
                 314.28571,160);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::bezier3()
{
    QPainterPath path;
    path.moveTo(295, 217);
    path.cubicTo(364,  57,
                 377,  34,
                 456, 222);
    return path;
}

QPainterPath Paths::bezier4()
{
    QPainterPath path;
    path.moveTo(200, 125);
    path.cubicTo(200, 125,
                 623, 126,
                 623, 126);
    return path;
}

QPainterPath Paths::heart2()
{
    QPainterPath path;
    path.moveTo(263.41570, 235.14588);
    path.cubicTo(197.17570,235.14588,
                 143.41575,288.90587,
                 143.41575,355.14588);
    path.cubicTo(143.41575, 489.90139,
                 279.34890, 525.23318,
                 371.97820, 658.45392);
    return path;
}

QPainterPath Paths::rect2()
{
    QPainterPath path;

    path.addRect(80, 80, 100, 100);

    return path;
}


QPainterPath Paths::rect3()
{
    QPainterPath path;

    path.addRect(100, 40, 100, 100);

    return path;
}


QPainterPath Paths::rect4()
{
    QPainterPath path;

    path.addRect(100, 0, 200, 200);

    path.addRect(120, 20, 80, 80);

    return path;
}

QPainterPath Paths::simpleCurve()
{
    QPainterPath path;
    path.moveTo(74, 160);
    path.cubicTo( 74, 160,
                 274, 406,
                 425, 166);
    path.cubicTo(577, -73,
                  77, 160,
                  74, 160);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::simpleCurve2()
{
    QPainterPath path;
    path.moveTo(54, 140);
    path.cubicTo( 54, 140,
                 254, 386,
                 405, 146);
    path.cubicTo(557, -93,
                  57, 140,
                  54, 140);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::frame1()
{
    QPainterPath path;
    path.moveTo(190.71429, 40.933613);
    path.lineTo(683.57141, 40.933613);
    path.cubicTo(697.42141, 40.933613,
                 708.57141, 52.083613,
                 708.57141, 65.933613);
    path.lineTo(708.57141, 375.93361);
    path.cubicTo(708.57141, 389.78361,
                 697.42141, 400.93361,
                 683.57141, 400.93361);
    path.lineTo(190.71429, 400.93361);
    path.cubicTo(176.86429, 400.93361,
                 165.71429, 389.78361,
                 165.71429,375.93361);
    path.lineTo(165.71429, 65.933613);
    path.cubicTo(165.71429,52.083613,
                 176.86429,40.933613,
                 190.71429,40.933613);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::frame2()
{
    QPainterPath path;
    path.moveTo(55.114286, 103.79076);
    path.lineTo(187.74288, 103.79076);
    path.cubicTo(192.95048, 103.79076,
                 197.14288, 107.88102,
                 197.14288, 112.96176);
    path.lineTo(197.14288, 131.76261);
    path.cubicTo(197.14288, 136.84335,
                 192.95048, 140.93361,
                 187.74288, 140.93361);
    path.lineTo(55.114286, 140.93361);
    path.cubicTo(49.906687, 140.93361,
                 45.714287, 136.84335,
                 45.714287, 131.76261);
    path.lineTo(45.714287, 112.96176);
    path.cubicTo(45.714287, 107.88102,
                 49.906687, 103.79076,
                 55.114286, 103.79076);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::frame3()
{
    QPainterPath path;
    path.moveTo(200,80.933609);
    path.lineTo(682.85715,80.933609);
    path.lineTo(682.85715,446.6479);
    path.lineTo(200,446.6479);
    path.lineTo(200,80.933609);
    path.closeSubpath();
    return path;
}

QPainterPath Paths::frame4()
{
    QPainterPath path;

    path.moveTo(88.571434,206.64789);
    path.lineTo(231.42858,206.64789);
    path.lineTo(231.42858,246.64789);
    path.lineTo(88.571434,246.64789);
    path.lineTo(88.571434,206.64789);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::simpleCurve3()
{
    QPainterPath path;

    path.moveTo(0, 0);
    path.cubicTo(400,0,
                 0,400,
                 0,0);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::rect5()
{
    QPainterPath path;

    path.addRect(0, 0, 200, 200);

    return path;
}

QPainterPath Paths::triangle1()
{
    QPainterPath path;

    path.moveTo(0, 0);
    path.lineTo(60, 0);
    path.lineTo(60, 60);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::triangle2()
{
    QPainterPath path;

    path.moveTo(0, 120);
    path.lineTo(60, 120);
    path.lineTo(60, 60);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::node()
{
    QRectF m_rect;
    m_rect.setWidth(150);
    m_rect.setHeight(100);

    QPainterPath shape;
    shape.addRoundedRect(m_rect, 25, Qt::RelativeSize);

    const int conWidth = 10;
    const int xOffset  = 7;

    QRectF rect(xOffset,
                conWidth + 20,
                conWidth, conWidth);
    shape.addEllipse(rect);
    //shape.addRect(rect);

    rect = QRectF(m_rect.right() - conWidth - xOffset,
                  conWidth + 20,
                  conWidth, conWidth);
    shape.addEllipse(rect);
    //shape.addRect(rect);
    return shape;
}

QPainterPath Paths::interRect()
{
    QPainterPath path;
    path.addRect(132, 42, 1, 1);
    return path;
}

QPainterPath Paths::bezierFlower()
{
    QPainterPath path;
    path.moveTo(0, 0);
    path.cubicTo(0, 50, -25, 75, -50, 100);
    path.closeSubpath();
    path.moveTo(0, 0);
    path.cubicTo(0, 50, 25, 75, 50, 100);
    path.closeSubpath();

    path.moveTo(0, 0);
    path.cubicTo(0, -50, -25, -75, -50, -100);
    path.closeSubpath();
    path.moveTo(0, 0);
    path.cubicTo(0, -50, 25, -75, 50, -100);
    path.closeSubpath();

    path.moveTo(0, 0);
    path.cubicTo(-50, 0, -75, -25, -100, -50);
    path.closeSubpath();
    path.moveTo(0, 0);
    path.cubicTo(-50, 0, -75, 25, -100, 50);
    path.closeSubpath();

    path.moveTo(0, 0);
    path.cubicTo(50, 0, 75, -25, 100, -50);
    path.closeSubpath();
    path.moveTo(0, 0);
    path.cubicTo(50, 0, 75, 25, 100, 50);
    path.closeSubpath();

    return path;
}

QPainterPath Paths::clover()
{
    QPainterPath path;
    path.moveTo(50, 50);
    path.lineTo(100, 25);
    path.lineTo(100, 75);
    path.lineTo(0, 25);
    path.lineTo(0, 75);
    path.lineTo(50, 50);
    path.lineTo(75, 0);
    path.lineTo(25, 0);
    path.lineTo(75, 100);
    path.lineTo(25, 100);
    path.lineTo(50, 50);
    return path;
}

QPainterPath Paths::ellipses()
{
    QPainterPath path;
    path.addEllipse(0, 0, 100, 100);
    path.addEllipse(0, 20, 100, 60);
    path.addEllipse(0, 40, 100, 20);
    return path;
}

QPainterPath Paths::windingFill()
{
    QPainterPath path;
    path.addRect(0, 0, 100, 100);
    path.addRect(50, 25, 100, 50);
    path.setFillRule(Qt::WindingFill);
    return path;
}

QPainterPath Paths::oddEvenFill()
{
    QPainterPath path;
    path.addRect(0, 0, 100, 100);
    path.moveTo(50, 25);
    path.lineTo(50, 75);
    path.lineTo(150, 75);
    path.lineTo(150, 25);
    path.lineTo(50, 25);
    path.setFillRule(Qt::OddEvenFill);
    return path;
}

QPainterPath Paths::squareWithHole()
{
    QPainterPath path;
    path.addRect(0, 0, 100, 100);
    path.addRect(30, 30, 40, 40);
    return path;
}

QPainterPath Paths::circleWithHole()
{
        QPainterPath path;
        path.addEllipse(0, 0, 100, 100);
        path.addEllipse(30, 30, 40, 40);
        return path;
}

QPainterPath Paths::bezierQuadrant()
{
    QPainterPath path;
    int d = 1;
    for (int i = 25; i <= 85; i += 10) {
        path.moveTo(50, 100);
        path.cubicTo(50, i, 50 + i* d / 2, 0, 50 + 50 * d, 0);
        path.lineTo(50 + 50 * d, 100);
        path.closeSubpath();
    }

    return path;
}
