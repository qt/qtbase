/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef THEME_H
#define THEME_H

#include <QPen>
#include <QPainter>

#include "gvbwidget.h"
#include "listitem.h"

class Theme : public QObject
{
    Q_OBJECT

public:
    enum Themes
    {
        Blue = 0,
        Lime = 1,
    };

    enum Fonts
    {
        ContactName,
        ContactNumber,
        ContactEmail,
        TitleBar,
        StatusBar,
        MenuItem,
    };

    virtual ~Theme();

    static Theme* p();

    void setTheme(const QString theme);
    void setTheme(const Themes theme);

    Themes theme() const { return m_currentTheme; }
    QString currentThemeName() { return m_availableThemes.at(m_currentTheme); }
    QStringList themes() const { return m_availableThemes; }
    int themesCount() const { return m_availableThemes.count(); }

    QPixmap pixmap(const QString filename = "", QSize size = QSize(0,0));
    QFont font(Fonts type) const { return m_fonts[type]; }
    QString pixmapPath() const { return m_pixmapPath; }

    QBrush listItemBackgroundBrushEven() const { return m_listItemBackgroundBrushEven; }
    QBrush listItemBackgroundBrushOdd() const { return m_listItemBackgroundBrushOdd; }
    qreal listItemBackgroundOpacityEven() const { return m_listItemBackgroundOpacityEven; }
    qreal listItemBackgroundOpacityOdd() const { return m_listItemBackgroundOpacityOdd; }

    QPen listItemBorderPen() const { return m_listItemBorderPen; }
    QSize listItemRounding() const { return m_listItemRounding; }

    bool isIconOpacityEffectEnabled(const ListItem::IconItemPos iconPos) const { return m_iconOpacityEffectEnabled[iconPos]; }

    qreal iconRotation(const ListItem::IconItemPos iconPos) const { return m_iconRotation[iconPos]; }
    bool isIconSmoothTransformationEnabled(const ListItem::IconItemPos iconPos) const { return m_iconSmoothTransformation[iconPos]; }

signals:
    void themeChanged();

private:
    Theme(QObject *parent = 0);

    void setBlueTheme();
    void setLimeTheme();

private:
    Q_DISABLE_COPY(Theme)

    Themes m_currentTheme;
    QStringList m_availableThemes;
    QHash<Fonts, QFont> m_fonts;
    QString m_pixmapPath;

    QBrush m_listItemBackgroundBrushEven;
    qreal m_listItemBackgroundOpacityEven;
    QBrush m_listItemBackgroundBrushOdd;
    qreal m_listItemBackgroundOpacityOdd;

    QPen m_listItemBorderPen;
    QSize m_listItemRounding;

    QHash<ListItem::IconItemPos, bool> m_iconOpacityEffectEnabled;
    QHash<ListItem::IconItemPos, qreal> m_iconRotation;
    QHash<ListItem::IconItemPos, bool> m_iconSmoothTransformation;
};

#endif // THEME_H
