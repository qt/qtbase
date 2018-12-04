/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "language.h"

#include <QtCore/qtextstream.h>

namespace language {

QTextStream &operator<<(QTextStream &str, const qtConfig &c)
{
    str << "QT_CONFIG(" << c.parameter() << ')';
    return str;
}

QTextStream &operator<<(QTextStream &str, const openQtConfig &c)
{
    str << "#if " << qtConfig(c.parameter())  << '\n';
    return str;
}

QTextStream &operator<<(QTextStream &str, const closeQtConfig &c)
{
    str << "#endif // " << qtConfig(c.parameter()) << '\n';
    return str;
}

struct EnumLookup
{
    int value;
    const char *valueString;
};

template <int N>
const char *lookupEnum(const EnumLookup(&array)[N], int value, int defaultIndex = 0)
{
    for (int i = 0; i < N; ++i) {
        if (value == array[i].value)
            return array[i].valueString;
    }
    const char *defaultValue = array[defaultIndex].valueString;
    qWarning("uic: Warning: Invalid enumeration value %d, defaulting to %s",
             value, defaultValue);
    return defaultValue;
}

const char *toolbarArea(int v)
{
    static const EnumLookup toolBarAreas[] =
    {
        {0,   "NoToolBarArea"},
        {0x1, "LeftToolBarArea"},
        {0x2, "RightToolBarArea"},
        {0x4, "TopToolBarArea"},
        {0x8, "BottomToolBarArea"},
        {0xf, "AllToolBarAreas"}
    };
    return lookupEnum(toolBarAreas, v);
}

const char *sizePolicy(int v)
{
    static const EnumLookup sizePolicies[] =
    {
        {0,   "Fixed"},
        {0x1, "Minimum"},
        {0x4, "Maximum"},
        {0x5, "Preferred"},
        {0x3, "MinimumExpanding"},
        {0x7, "Expanding"},
        {0xD, "Ignored"}
    };
    return lookupEnum(sizePolicies, v, 3);
}

const char *dockWidgetArea(int v)
{
    static const EnumLookup dockWidgetAreas[] =
    {
        {0,   "NoDockWidgetArea"},
        {0x1, "LeftDockWidgetArea"},
        {0x2, "RightDockWidgetArea"},
        {0x4, "TopDockWidgetArea"},
        {0x8, "BottomDockWidgetArea"},
        {0xf, "AllDockWidgetAreas"}
    };
    return lookupEnum(dockWidgetAreas, v);
}

const char *paletteColorRole(int v)
{
    static const EnumLookup colorRoles[] =
    {
        {0, "WindowText"},
        {1, "Button"},
        {2, "Light"},
        {3, "Midlight"},
        {4, "Dark"},
        {5, "Mid"},
        {6, "Text"},
        {7, "BrightText"},
        {8, "ButtonText"},
        {9, "Base"},
        {10, "Window"},
        {11, "Shadow"},
        {12, "Highlight"},
        {13, "HighlightedText"},
        {14, "Link"},
        {15, "LinkVisited"},
        {16, "AlternateBase"},
        {17, "NoRole"},
        {18, "ToolTipBase"},
        {19, "ToolTipText"},
        {20, "PlaceholderText"},
    };
    return lookupEnum(colorRoles, v);
}

} // namespace language
