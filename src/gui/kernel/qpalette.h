/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPALETTE_H
#define QPALETTE_H

#include <QtGui/qwindowdefs.h>
#include <QtGui/qcolor.h>
#include <QtGui/qbrush.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QPalettePrivate;
class QVariant;

class Q_GUI_EXPORT QPalette
{
    Q_GADGET
    Q_ENUMS(ColorGroup ColorRole)
public:
    QPalette();
    QPalette(const QColor &button);
    QPalette(Qt::GlobalColor button);
    QPalette(const QColor &button, const QColor &window);
    QPalette(const QBrush &windowText, const QBrush &button, const QBrush &light,
             const QBrush &dark, const QBrush &mid, const QBrush &text,
             const QBrush &bright_text, const QBrush &base, const QBrush &window);
    QPalette(const QColor &windowText, const QColor &window, const QColor &light,
             const QColor &dark, const QColor &mid, const QColor &text, const QColor &base);
    QPalette(const QPalette &palette);
    ~QPalette();
    QPalette &operator=(const QPalette &palette);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QPalette &operator=(QPalette &&other)
    {
        resolve_mask = other.resolve_mask;
        current_group = other.current_group;
        qSwap(d, other.d); return *this;
    }
#endif
    operator QVariant() const;

    // Do not change the order, the serialization format depends on it
    enum ColorGroup { Active, Disabled, Inactive, NColorGroups, Current, All, Normal = Active };
    enum ColorRole { WindowText, Button, Light, Midlight, Dark, Mid,
                     Text, BrightText, ButtonText, Base, Window, Shadow,
                     Highlight, HighlightedText,
                     Link, LinkVisited, // ### Qt 5: remove
                     AlternateBase,
                     NoRole, // ### Qt 5: value should be 0 or -1
                     ToolTipBase, ToolTipText,
                     NColorRoles = ToolTipText + 1,
                     Foreground = WindowText, Background = Window // ### Qt 5: remove
                   };

    inline ColorGroup currentColorGroup() const { return static_cast<ColorGroup>(current_group); }
    inline void setCurrentColorGroup(ColorGroup cg) { current_group = cg; }

    inline const QColor &color(ColorGroup cg, ColorRole cr) const
    { return brush(cg, cr).color(); }
    const QBrush &brush(ColorGroup cg, ColorRole cr) const;
    inline void setColor(ColorGroup cg, ColorRole cr, const QColor &color);
    inline void setColor(ColorRole cr, const QColor &color);
    inline void setBrush(ColorRole cr, const QBrush &brush);
    bool isBrushSet(ColorGroup cg, ColorRole cr) const;
    void setBrush(ColorGroup cg, ColorRole cr, const QBrush &brush);
    void setColorGroup(ColorGroup cr, const QBrush &windowText, const QBrush &button,
                       const QBrush &light, const QBrush &dark, const QBrush &mid,
                       const QBrush &text, const QBrush &bright_text, const QBrush &base,
                       const QBrush &window);
    bool isEqual(ColorGroup cr1, ColorGroup cr2) const;

    inline const QColor &color(ColorRole cr) const { return color(Current, cr); }
    inline const QBrush &brush(ColorRole cr) const { return brush(Current, cr); }
    inline const QBrush &foreground() const { return brush(WindowText); }
    inline const QBrush &windowText() const { return brush(WindowText); }
    inline const QBrush &button() const { return brush(Button); }
    inline const QBrush &light() const { return brush(Light); }
    inline const QBrush &dark() const { return brush(Dark); }
    inline const QBrush &mid() const { return brush(Mid); }
    inline const QBrush &text() const { return brush(Text); }
    inline const QBrush &base() const { return brush(Base); }
    inline const QBrush &alternateBase() const { return brush(AlternateBase); }
    inline const QBrush &toolTipBase() const { return brush(ToolTipBase); }
    inline const QBrush &toolTipText() const { return brush(ToolTipText); }
    inline const QBrush &background() const { return brush(Window); }
    inline const QBrush &window() const { return brush(Window); }
    inline const QBrush &midlight() const { return brush(Midlight); }
    inline const QBrush &brightText() const { return brush(BrightText); }
    inline const QBrush &buttonText() const { return brush(ButtonText); }
    inline const QBrush &shadow() const { return brush(Shadow); }
    inline const QBrush &highlight() const { return brush(Highlight); }
    inline const QBrush &highlightedText() const { return brush(HighlightedText); }
    inline const QBrush &link() const { return brush(Link); }
    inline const QBrush &linkVisited() const { return brush(LinkVisited); }

    bool operator==(const QPalette &p) const;
    inline bool operator!=(const QPalette &p) const { return !(operator==(p)); }
    bool isCopyOf(const QPalette &p) const;

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED inline int serialNumber() const { return cacheKey() >> 32; }
#endif
    qint64 cacheKey() const;

    QPalette resolve(const QPalette &) const;
    inline uint resolve() const { return resolve_mask; }
    inline void resolve(uint mask) { resolve_mask = mask; }

private:
    void setColorGroup(ColorGroup cr, const QBrush &windowText, const QBrush &button,
                       const QBrush &light, const QBrush &dark, const QBrush &mid,
                       const QBrush &text, const QBrush &bright_text,
                       const QBrush &base, const QBrush &alternate_base,
                       const QBrush &window, const QBrush &midlight,
                       const QBrush &button_text, const QBrush &shadow,
                       const QBrush &highlight, const QBrush &highlighted_text,
                       const QBrush &link, const QBrush &link_visited);
    void setColorGroup(ColorGroup cr, const QBrush &windowText, const QBrush &button,
                       const QBrush &light, const QBrush &dark, const QBrush &mid,
                       const QBrush &text, const QBrush &bright_text,
                       const QBrush &base, const QBrush &alternate_base,
                       const QBrush &window, const QBrush &midlight,
                       const QBrush &button_text, const QBrush &shadow,
                       const QBrush &highlight, const QBrush &highlighted_text,
                       const QBrush &link, const QBrush &link_visited,
                       const QBrush &toolTipBase, const QBrush &toolTipText);
    void init();
    void detach();

    QPalettePrivate *d;
    uint current_group : 4;
    uint resolve_mask : 28;
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &s, const QPalette &p);
};

inline void QPalette::setColor(ColorGroup acg, ColorRole acr,
                               const QColor &acolor)
{ setBrush(acg, acr, QBrush(acolor)); }
inline void QPalette::setColor(ColorRole acr, const QColor &acolor)
{ setColor(All, acr, acolor); }
inline void QPalette::setBrush(ColorRole acr, const QBrush &abrush)
{ setBrush(All, acr, abrush); }

/*****************************************************************************
  QPalette stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &ds, const QPalette &p);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &ds, QPalette &p);
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QPalette &);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPALETTE_H
