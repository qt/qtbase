// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser


#include "qguisvg_p.h"
#include <cmath>
#include <QtCore/qpoint.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/private/qmath_p.h>
#include <array>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

namespace {

using namespace Qt::StringLiterals;

static void pathArcSegment(QPainterPath &path,
                           qreal xc, qreal yc,
                           qreal th0, qreal th1,
                           qreal rx, qreal ry, qreal xAxisRotation)
{
    qreal sinTh, cosTh;
    qreal a00, a01, a10, a11;
    qreal x1, y1, x2, y2, x3, y3;
    qreal t;
    qreal thHalf;

    sinTh = qSin(xAxisRotation * (Q_PI / 180.0));
    cosTh = qCos(xAxisRotation * (Q_PI / 180.0));

    a00 =  cosTh * rx;
    a01 = -sinTh * ry;
    a10 =  sinTh * rx;
    a11 =  cosTh * ry;

    thHalf = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * qSin(thHalf * 0.5) * qSin(thHalf * 0.5) / qSin(thHalf);
    x1 = xc + qCos(th0) - t * qSin(th0);
    y1 = yc + qSin(th0) + t * qCos(th0);
    x3 = xc + qCos(th1);
    y3 = yc + qSin(th1);
    x2 = x3 + t * qSin(th1);
    y2 = y3 - t * qCos(th1);

    path.cubicTo(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1,
                 a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                 a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
}

}
namespace QGuiSvg {

// '0' is 0x30 and '9' is 0x39
bool isDigit(ushort ch)
{
    static quint16 magic = 0x3ff;
    return ((ch >> 4) == 3) && (magic >> (ch & 15));
}

qreal toDouble(QStringView *str)
{
    const int maxLen = 255;//technically doubles can go til 308+ but whatever
    char temp[maxLen+1];
    int pos = 0;

    if (str->startsWith(QLatin1Char('-'))) {
        temp[pos++] = '-';
        str->slice(1);
    } else if (str->startsWith(QLatin1Char('+'))) {
        str->slice(1);
    }
    while (!str->isEmpty() && isDigit(str->first().unicode()) && pos < maxLen) {
        temp[pos++] = str->first().toLatin1();
        str->slice(1);
    }
    if (str->startsWith(QLatin1Char('.')) && pos < maxLen) {
        temp[pos++] = '.';
        str->slice(1);
    }
    while (!str->isEmpty() && isDigit(str->first().unicode()) && pos < maxLen) {
        temp[pos++] = str->first().toLatin1();
        str->slice(1);
    }
    bool exponent = false;
    if ((str->startsWith(QLatin1Char('e')) || str->startsWith(QLatin1Char('E'))) && pos < maxLen) {
        exponent = true;
        temp[pos++] = 'e';
        str->slice(1);
        if ((str->startsWith(QLatin1Char('-')) || str->startsWith(QLatin1Char('+')))
            && pos < maxLen) {
            temp[pos++] = str->first().toLatin1();
            str->slice(1);
        }
        while (!str->isEmpty() && isDigit(str->first().unicode()) && pos < maxLen) {
            temp[pos++] = str->first().toLatin1();
            str->slice(1);
        }
    }

    temp[pos] = '\0';

    qreal val;
    if (!exponent && pos < 10) {
        int ival = 0;
        const char *t = temp;
        bool neg = false;
        if (*t == '-') {
            neg = true;
            ++t;
        }
        while (*t && *t != '.') {
            ival *= 10;
            ival += (*t) - '0';
            ++t;
        }
        if (*t == '.') {
            ++t;
            int div = 1;
            while (*t) {
                ival *= 10;
                ival += (*t) - '0';
                div *= 10;
                ++t;
            }
            val = ((qreal)ival)/((qreal)div);
        } else {
            val = ival;
        }
        if (neg)
            val = -val;
    } else {
        val = QByteArray::fromRawData(temp, pos).toDouble();
        // Do not tolerate values too wild to be represented normally by floats
        if (qFpClassify(float(val)) != FP_NORMAL)
            val = 0;
    }
    return val;

}

qreal toDouble(QStringView str, bool *ok)
{
    const qreal res{ toDouble(&str) };
    if (ok)
        *ok = str.isEmpty();
    return res;
}

qreal parseLength(QStringView str, LengthType *type, bool *ok, bool tiny12FeaturesOnly)
{
    QStringView numStr = QGuiSvg::trimmed(str, tiny12FeaturesOnly);

    if (numStr.isEmpty()) {
        if (ok)
            *ok = false;
        *type = LengthType::LT_OTHER;
        return false;
    }
    if (numStr.endsWith(QLatin1Char('%'))) {
        numStr.chop(1);
        *type = LengthType::LT_PERCENT;
    } else if (numStr.endsWith(QLatin1String("px"))) {
        numStr.chop(2);
        *type = LengthType::LT_PX;
    } else if (numStr.endsWith(QLatin1String("pc"))) {
        numStr.chop(2);
        *type = LengthType::LT_PC;
    } else if (numStr.endsWith(QLatin1String("pt"))) {
        numStr.chop(2);
        *type = LengthType::LT_PT;
    } else if (numStr.endsWith(QLatin1String("mm"))) {
        numStr.chop(2);
        *type = LengthType::LT_MM;
    } else if (numStr.endsWith(QLatin1String("cm"))) {
        numStr.chop(2);
        *type = LengthType::LT_CM;
    } else if (numStr.endsWith(QLatin1String("in"))) {
        numStr.chop(2);
        *type = LengthType::LT_IN;
    } else {
        // default coordinate system
        *type = LengthType::LT_PX;
    }
    qreal len = toDouble(numStr, ok);
    return len;
}

// this should really be called convertToDefaultCoordinateSystem
// and convert when type != QSvgHandler::defaultCoordinateSystem
qreal convertToPixels(qreal len, bool , LengthType type)
{

    switch (type) {
    case LengthType::LT_PERCENT:
        break;
    case LengthType::LT_PX:
        break;
    case LengthType::LT_PC:
        break;
    case LengthType::LT_PT:
        return len * 1.25;
        break;
    case LengthType::LT_MM:
        return len * 3.543307;
        break;
    case LengthType::LT_CM:
        return len * 35.43307;
        break;
    case LengthType::LT_IN:
        return len * 90;
        break;
    case LengthType::LT_OTHER:
        break;
    default:
        break;
    }
    return len;
}

// Parses the angle from a string and convert it to degrees.
// In CSS, units are not optional so if a non-zero value is defined,
// without a unit it should be treated as invalid.
std::optional<qreal> parseAngle(QStringView str, bool tiny12FeaturesOnly)
{
    QStringView numStr = QGuiSvg::trimmed(str, tiny12FeaturesOnly);

    if (numStr.isEmpty())
        return std::nullopt;

    qreal unitFactor = 1.0;
    if (numStr.endsWith(QLatin1String("deg"))) {
        numStr.chop(3);
        unitFactor = 1.0;
    } else if (numStr.endsWith(QLatin1String("grad"))) {
        numStr.chop(4);
        // deg = grad * 0.9;
        unitFactor = 0.9;
    } else if (numStr.endsWith(QLatin1String("rad"))) {
        numStr.chop(3);
        unitFactor = 180.0 / Q_PI;
    } else if (numStr.endsWith(QLatin1String("turn"))) {
        numStr.chop(4);
        // one circle = one turn
        unitFactor = 360.0;
    } else {
        return std::nullopt;
    }

    bool ok;
    qreal angle = numStr.toDouble(&ok);
    if (!ok)
        return std::nullopt;

    return angle * unitFactor;
}

void parseNumbersArray(QStringView *str, QVarLengthArray<qreal, 8> &points, const char *pattern)
{
    const size_t patternLen = qstrlen(pattern);
    while (!str->isEmpty() && str->first().isSpace())
        str->slice(1);
    while ((!str->isEmpty() && QGuiSvg::isDigit(str->first().unicode()))
           || str->startsWith(QLatin1Char('-')) || str->startsWith(QLatin1Char('+'))
           || str->startsWith(QLatin1Char('.'))) {

        if (patternLen && pattern[points.size() % patternLen] == 'f') {
            // flag expected, may only be 0 or 1
            if (!str->startsWith(QLatin1Char('0')) && !str->startsWith(QLatin1Char('1')))
                return;
            points.append(str->startsWith(QLatin1Char('0')) ? 0.0 : 1.0);
            str->slice(1);
        } else {
            points.append(QGuiSvg::toDouble(str));
        }

        while (!str->isEmpty() && str->first().isSpace())
            str->slice(1);
        if (str->startsWith(QLatin1Char(',')))
            str->slice(1);

        //eat the rest of space
        while (!str->isEmpty() && str->first().isSpace())
            str->slice(1);
    }
}

std::optional<QPainterPath> parsePath(QStringView dataStr, bool limitLength)
{
    if (dataStr.isEmpty())
        return std::nullopt;

    // Max QPainterPath element count when limitLength is set to true.
    constexpr int maxElementCount = 0x7fff;
    constexpr QLatin1StringView cmds = "mzlhvcsqta"_L1;
    constexpr std::array cmdsParamCount = {2, 0, 2, 1, 1, 6, 4, 4, 2, 7};
    static_assert(cmds.length() == cmdsParamCount.size());

    QPointF startPoint;
    QPointF currPoint;
    QPointF ctrlPoint;
    char lastMode = 0;

    QPainterPath path;
    while (!dataStr.isEmpty()) {
        while (dataStr.first().isSpace() && dataStr.length() > 1)
            dataStr.slice(1);

        QChar cmd = dataStr.first().toLower();
        if (!cmds.contains(cmd))
            break;

        const bool isRelative = dataStr.first().isLower();
        dataStr.slice(1);
        const char *pattern = nullptr;
        if (cmd == QLatin1Char('a'))
            pattern = "rrrffrr";
        QVarLengthArray<qreal, 8> arg;
        parseNumbersArray(&dataStr, arg, pattern);
        const qreal *num = arg.constData();
        int count = arg.size();

        qsizetype countPos = cmds.indexOf(cmd);
        int paramCount = cmdsParamCount[countPos];

        do {
            if (count < paramCount)
                return path;

            QPointF offset = isRelative ? currPoint : QPointF {0., 0.};
            switch (cmd.unicode()) {
            case 'm': {
                currPoint = startPoint = QPointF {num[0], num[1]} + offset;
                path.moveTo(startPoint);

                // As per 1.2  spec 8.3.2 The "moveto" commands
                // If a 'moveto' is followed by multiple pairs of coordinates without explicit commands,
                // the subsequent pairs shall be treated as implicit 'lineto' commands.
                cmd = QLatin1Char('l');
            }
            break;
            case 'z': {
                currPoint = startPoint;
                path.closeSubpath();
            }
            break;
            case 'l': {
                currPoint = QPointF {num[0], num[1]} + offset;
                path.lineTo(currPoint);
            }
            break;
            case 'h': {
                currPoint.setX(num[0] + offset.x());
                path.lineTo(currPoint);
            }
            break;
            case 'v': {
                currPoint.setY(num[0] + offset.y());
                path.lineTo(currPoint);
            }
            break;
            case 'c': {
                QPointF c1 = QPointF {num[0], num[1]} + offset;
                QPointF c2 = QPointF {num[2], num[3]} + offset;
                QPointF e = QPointF {num[4], num[5]} + offset;
                path.cubicTo(c1, c2, e);
                ctrlPoint = c2;
                currPoint = e;
                break;
            }
            case 's': {
                QPointF c1;
                if (lastMode == 'c' || lastMode == 's')
                    c1 = 2 * currPoint - ctrlPoint;
                else
                    c1 = currPoint;
                QPointF c2 = QPointF {num[0], num[1]} + offset;
                QPointF e = QPointF {num[2], num[3]} + offset;

                path.cubicTo(c1, c2, e);
                ctrlPoint = c2;
                currPoint = e;
                break;
            }
            case 'q': {
                QPointF c = QPointF {num[0], num[1]} + offset;
                QPointF e = QPointF {num[2], num[3]} + offset;
                path.quadTo(c, e);
                ctrlPoint = c;
                currPoint = e;
                break;
            }
            case 't': {
                QPointF e = QPointF {num[0], num[1]} + offset;
                QPointF c;
                if (lastMode == 'q' || lastMode == 't')
                    c = 2 * currPoint - ctrlPoint;
                else
                    c = currPoint;
                path.quadTo(c, e);
                ctrlPoint = c;
                currPoint = e;
                break;
            }
            case 'a': {
                qreal rx = num[0];
                qreal ry = num[1];
                qreal xAxisRotation = num[2];
                qreal largeArcFlag  = num[3];
                qreal sweepFlag = num[4];
                qreal ex = num[5] + offset.x();
                qreal ey = num[6] + offset.y();
                qreal curx = currPoint.x();
                qreal cury = currPoint.y();
                pathArc(path, rx, ry, xAxisRotation, int(largeArcFlag),
                        int(sweepFlag), ex, ey, curx, cury);

                currPoint = QPointF {ex, ey};
            }
            break;
            default:
                Q_ASSERT_X(false, "parsePath", "command table out of sync with switch");
                return std::nullopt;
            }

            lastMode = cmd.toLatin1();
            if (limitLength && path.elementCount() > maxElementCount)
                return std::nullopt;

            // the z/Z command should have no arguments. Otherwise,
            // the remaining part of the path is invalid.
            if (cmd == QLatin1Char('z') && count != 0)
                return path;

            count -= paramCount;
            num += paramCount;
        } while (count > 0);
    }

    return path;
}

// the arc handling code underneath is from XSVG (BSD license)
/*
 * Copyright  2002 USC/Information Sciences Institute
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Information Sciences Institute not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission.  Information Sciences Institute
 * makes no representations about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * INFORMATION SCIENCES INSTITUTE DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL INFORMATION SCIENCES
 * INSTITUTE BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
void pathArc(QPainterPath &path, qreal rx, qreal ry,
             qreal x_axis_rotation, int large_arc_flag,
             int sweep_flag, qreal x, qreal y,
             qreal curx, qreal cury)
{
    // Check if the start point is equal to the end point.
    if (QPointF(curx, cury) == QPointF(x, y))
        return;

    const qreal Pr1 = rx * rx;
    const qreal Pr2 = ry * ry;

    // Avoid nans and division by zero.
    if (qFuzzyIsNull(Pr1) || qFuzzyIsNull(Pr2)) {
        // https://www.w3.org/TR/SVG/paths.html#ArcOutOfRangeParameters says:
        // "If either rx or ry is 0, then this arc is treated as a straight line
        // segment (a "lineto") joining the endpoints."
        path.lineTo(x, y);
        return;
    }

    qreal sin_th, cos_th;
    qreal a00, a01, a10, a11;
    qreal x0, y0, x1, y1, xc, yc;
    qreal d, sfactor, sfactor_sq;
    qreal th0, th1, th_arc;
    int i, n_segs;
    qreal dx, dy, dx1, dy1, Px, Py, check;

    rx = qAbs(rx);
    ry = qAbs(ry);

    sin_th = qSin(x_axis_rotation * (Q_PI / 180.0));
    cos_th = qCos(x_axis_rotation * (Q_PI / 180.0));

    dx = (curx - x) / 2.0;
    dy = (cury - y) / 2.0;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        rx = rx * qSqrt(check);
        ry = ry * qSqrt(check);
    }

    a00 =  cos_th / rx;
    a01 =  sin_th / rx;
    a10 = -sin_th / ry;
    a11 =  cos_th / ry;
    x0 = a00 * curx + a01 * cury;
    y0 = a10 * curx + a11 * cury;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    if (!d)
        return;
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = qSqrt(sfactor_sq);
    if (sweep_flag == large_arc_flag) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = qAtan2(y0 - yc, x0 - xc);
    th1 = qAtan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
        th_arc += 2 * Q_PI;
    else if (th_arc > 0 && !sweep_flag)
        th_arc -= 2 * Q_PI;

    n_segs = qCeil(qAbs(th_arc / (Q_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++) {
        pathArcSegment(path, xc, yc,
                       th0 + i * th_arc / n_segs,
                       th0 + (i + 1) * th_arc / n_segs,
                       rx, ry, x_axis_rotation);
    }
}

namespace {
namespace svgChars {
constexpr auto spaces = " \\t\\n\\r\\f"_L1;
constexpr auto spacesTiny = " \\t\\n\\r"_L1;
} // namespace svgChars
} // namespace

static const QRegularExpression &reNoSpace(bool tiny12FeaturesOnly)
{
    if (tiny12FeaturesOnly) {
        static QRegularExpression reTiny("[^"_L1 + svgChars::spacesTiny + "]"_L1);
        return reTiny;
    } else {
        static QRegularExpression re("[^"_L1 + svgChars::spaces + "]"_L1);
        return re;
    }
}

QStringView trimmed(QStringView sv, bool tiny12FeaturesOnly)
{
    const QRegularExpression &re = reNoSpace(tiny12FeaturesOnly);
    const qsizetype first = sv.indexOf(re);
    if (first == -1) // implies last
        return sv.first(0);
    else
        return sv.sliced(first, sv.lastIndexOf(re) - first + 1);
}
}

QT_END_NAMESPACE
