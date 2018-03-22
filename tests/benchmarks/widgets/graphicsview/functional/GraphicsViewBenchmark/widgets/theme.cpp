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

#include <QDebug>
#include <QPainter>
#include <QPixmapCache>
#include <QSvgRenderer>

#include "theme.h"

Q_DECLARE_METATYPE(Theme::Themes*)

Theme::Theme(QObject *parent)
  : QObject(parent)
  , m_currentTheme()
  , m_availableThemes()
  , m_fonts()
  , m_pixmapPath()
  , m_listItemBackgroundBrushEven()
  , m_listItemBackgroundOpacityEven()
  , m_listItemBackgroundBrushOdd()
  , m_listItemBackgroundOpacityOdd()
  , m_listItemBorderPen(QPen())
  , m_listItemRounding()
  , m_iconOpacityEffectEnabled()
  , m_iconRotation()
  , m_iconSmoothTransformation()
{
    m_availableThemes << "Blue" << "Lime";

    // Set blue theme as a default theme without emiting themeChanged() signal
    setBlueTheme();
}

Theme::~Theme()
{
}

Theme* Theme::p()
{
    static Theme t;
    return &t;
}

void Theme::setTheme(const QString theme)
{
    if (theme.compare("blue", Qt::CaseInsensitive) == 0)
    {
        setTheme(Theme::Blue);
    }
    else if (theme.compare("lime", Qt::CaseInsensitive) == 0)
    {
        setTheme(Theme::Lime);
    }
    else
    {
        qDebug() << "Unknown theme";
    }
}

void Theme::setTheme(const Themes theme)
{
    if (m_currentTheme == theme)
        return;

    switch (theme)
    {
        case Theme::Blue:
            setBlueTheme();
            emit themeChanged();
            break;

        case Theme::Lime:
            setLimeTheme();
            emit themeChanged();
            break;
    }
}

void Theme::setBlueTheme()
{
    m_currentTheme = Theme::Blue;

    m_fonts[ContactName].setFamily("Arial");
    m_fonts[ContactName].setPixelSize(16);
    m_fonts[ContactName].setStyle(QFont::StyleNormal);
    m_fonts[ContactName].setWeight(QFont::Normal);

    m_fonts[ContactNumber].setFamily("Arial");
    m_fonts[ContactNumber].setPixelSize(14);
    m_fonts[ContactNumber].setStyle(QFont::StyleNormal);

    m_fonts[ContactEmail].setFamily("Arial");
    m_fonts[ContactEmail].setPixelSize(14);
    m_fonts[ContactEmail].setStyle(QFont::StyleNormal);

    m_fonts[TitleBar].setFamily("Arial");
    m_fonts[TitleBar].setPixelSize(36);
    m_fonts[TitleBar].setStyle(QFont::StyleNormal);

    m_fonts[StatusBar].setFamily("Arial");
    m_fonts[StatusBar].setPixelSize(16);
    m_fonts[StatusBar].setStyle(QFont::StyleNormal);

    m_fonts[MenuItem].setFamily("Arial");
    m_fonts[MenuItem].setPixelSize(14);
    m_fonts[MenuItem].setStyle(QFont::StyleNormal);

    m_pixmapPath = ":/themes/blue/";

    m_listItemBackgroundBrushEven = QBrush(Qt::NoBrush);
    m_listItemBackgroundOpacityEven = 1.0;
    m_listItemBackgroundBrushOdd = QBrush(QColor(44,214,250), Qt::SolidPattern);
    m_listItemBackgroundOpacityOdd = 1.0;

    m_listItemBorderPen = QPen(Qt::NoPen);
    m_listItemRounding = QSize(0.0, 0.0);

    m_iconOpacityEffectEnabled[ListItem::LeftIcon] = false;
    m_iconOpacityEffectEnabled[ListItem::RightIcon] = false;

    m_iconRotation[ListItem::LeftIcon] =  0.0;
    m_iconRotation[ListItem::RightIcon] = 0.0;

    m_iconSmoothTransformation[ListItem::LeftIcon] = false;
    m_iconSmoothTransformation[ListItem::RightIcon] = false;
}

void Theme::setLimeTheme()
{
    m_currentTheme = Theme::Lime;

    m_fonts[ContactName].setFamily("Arial");
    m_fonts[ContactName].setPixelSize(16);
    m_fonts[ContactName].setStyle(QFont::StyleItalic);
    m_fonts[ContactName].setWeight(QFont::Bold);

    m_fonts[ContactNumber].setFamily("Arial");
    m_fonts[ContactNumber].setPixelSize(14);
    m_fonts[ContactNumber].setStyle(QFont::StyleItalic);

    m_fonts[ContactEmail].setFamily("Arial");
    m_fonts[ContactEmail].setPixelSize(14);
    m_fonts[ContactEmail].setStyle(QFont::StyleItalic);

    m_fonts[TitleBar].setFamily("Arial");
    m_fonts[TitleBar].setPixelSize(36);
    m_fonts[TitleBar].setStyle(QFont::StyleItalic);

    m_fonts[StatusBar].setFamily("Arial");
    m_fonts[StatusBar].setPixelSize(16);
    m_fonts[StatusBar].setStyle(QFont::StyleItalic);

    m_fonts[MenuItem].setFamily("Arial");
    m_fonts[MenuItem].setPixelSize(14);
    m_fonts[MenuItem].setStyle(QFont::StyleItalic);

    m_pixmapPath = ":/themes/lime/";

    m_listItemBackgroundBrushEven = QBrush(QPixmap(":/avatars/avatar_014.png"));
    m_listItemBackgroundOpacityEven = 0.05;

    m_listItemBackgroundBrushOdd = QBrush(QPixmap(":/avatars/avatar_012.png"));
    m_listItemBackgroundOpacityOdd = 0.15;

    m_listItemBorderPen = QPen(QColor(0,0,0,55), 3, Qt::SolidLine);
    m_listItemRounding = QSize(12.0, 12.0);

    m_iconOpacityEffectEnabled[ListItem::LeftIcon] = true;
    m_iconOpacityEffectEnabled[ListItem::RightIcon] = false;

    m_iconRotation[ListItem::LeftIcon] = -4.0;
    m_iconRotation[ListItem::RightIcon] = 0.0;

    m_iconSmoothTransformation[ListItem::LeftIcon] = true;
    m_iconSmoothTransformation[ListItem::RightIcon] = false;
}

QPixmap Theme::pixmap(const QString filename, QSize size)
{
    if (filename.endsWith(".svg", Qt::CaseInsensitive))
    {
        QSvgRenderer doc(m_pixmapPath+filename);
        if (size == QSize(0,0))
            size = doc.defaultSize();
        QPixmap pix(size.width(),size.height());
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setViewport(0, 0, size.width(), size.height());
        doc.render(&painter);
        return pix;
    }
    else
    {
        QPixmap pix(m_pixmapPath+filename);
        return pix.scaled(size);
    }
}
