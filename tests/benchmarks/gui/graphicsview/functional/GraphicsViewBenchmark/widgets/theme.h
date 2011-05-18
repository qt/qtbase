/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
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
