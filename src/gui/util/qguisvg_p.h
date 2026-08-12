// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QGUISVG_P_H
#define QGUISVG_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/qpainterpath.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QGuiSvg {

enum LengthType {
    LT_PERCENT,
    LT_PX,
    LT_PC,
    LT_PT,
    LT_MM,
    LT_CM,
    LT_IN,
    LT_OTHER
};

Q_GUI_EXPORT bool isDigit(ushort ch);
Q_GUI_EXPORT qreal toDouble(QStringView *str);
Q_GUI_EXPORT qreal toDouble(QStringView str, bool *ok = NULL);
Q_GUI_EXPORT qreal parseLength(QStringView str, LengthType *type, bool *ok = NULL);
Q_GUI_EXPORT qreal convertToPixels(qreal len, bool , LengthType type);
Q_GUI_EXPORT std::optional<qreal> parseAngle(QStringView str);
Q_GUI_EXPORT void parseNumbersArray(QStringView *str, QVarLengthArray<qreal, 8> &points,
                                    const char *pattern = nullptr);
Q_GUI_EXPORT std::optional<QPainterPath> parsePath(QStringView dataStr, bool limitLength = true);
Q_GUI_EXPORT void pathArc(QPainterPath &path, qreal rx, qreal ry, qreal x_axis_rotation,
                          int large_arc_flag, int sweep_flag, qreal x, qreal y, qreal curx,
                          qreal cury);

}

QT_END_NAMESPACE

#endif // QGUISVG_P_H
